#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <system_error>
#include "BuferedFileStream.h"


namespace
{
    [[noreturn]] void ThrowLastWindowsError(const char* message)
    {
        const DWORD errorCode = ::GetLastError();

        throw std::system_error(static_cast<int>(errorCode), std::system_category(), message);
    }


    std::wstring Utf8ToWide(const std::string& text)
    {
        if (text.empty())
        {
            return {};
        }

        const int requiredSize = ::MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            nullptr,
            0);

        if (requiredSize == 0)
        {
            ThrowLastWindowsError("Cannot convert UTF-8 filename to UTF-16");
        }

        std::wstring result(static_cast<size_t>(requiredSize), L'\0');

        const int convertedSize = ::MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            result.data(),
            requiredSize);

        if (convertedSize == 0)
        {
            ThrowLastWindowsError("Cannot convert UTF-8 filename to UTF-16");
        }

        return result;
    }
}


MappedFileStream::MappedFileStream(const uint64_t bufferSize) : m_bufsize(bufferSize)
{
    if (m_bufsize == 0)
    {
        throw std::invalid_argument("MappedFileStream buffer size cannot be zero");
    }

    SYSTEM_INFO systemInfo{};
    ::GetSystemInfo(&systemInfo);

    m_dwSysGran = static_cast<uint64_t>(systemInfo.dwAllocationGranularity);

    if (m_dwSysGran == 0)
    {
        throw std::runtime_error("Invalid system allocation granularity");
    }
}


MappedFileStream::MappedFileStream(const std::string& filename, const uint64_t bufferSize)
    : MappedFileStream(bufferSize)
{
    Open(filename);
}


MappedFileStream::~MappedFileStream()
{
    Close();
}


bool MappedFileStream::IsOpen()
{
    return m_hFile != nullptr && m_hFile != INVALID_HANDLE_VALUE;
}


bool MappedFileStream::Eof()
{
    if (!IsOpen())
    {
        return true;
    }

    return m_position >= m_dwFileSize;
}


void MappedFileStream::Open(const std::string& filename)
{
    Close();

    const std::wstring wideFilename = Utf8ToWide(filename);

    HANDLE newFile = ::CreateFileW(
        wideFilename.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL |
        FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr);

    if (newFile == INVALID_HANDLE_VALUE)
    {
        ThrowLastWindowsError("Cannot open file");
    }

    LARGE_INTEGER fileSize{};

    if (!::GetFileSizeEx(newFile, &fileSize))
    {
        const DWORD errorCode = ::GetLastError();

        ::CloseHandle(newFile);

        ::SetLastError(errorCode);
        ThrowLastWindowsError("Cannot get file size");
    }

    if (fileSize.QuadPart < 0)
    {
        ::CloseHandle(newFile);

        throw std::runtime_error("File has an invalid negative size");
    }

    const uint64_t newFileSize = static_cast<uint64_t>(fileSize.QuadPart);

    HANDLE newMapping = nullptr;

    // CreateFileMapping для файла нулевого размера завершается
    // с ошибкой ERROR_FILE_INVALID, поэтому пустой файл
    // оставляем открытым, но mapping object не создаём.
    if (newFileSize != 0)
    {
        newMapping = ::CreateFileMappingW(
            newFile,
            nullptr,
            PAGE_READONLY,
            0,
            0,
            nullptr);

        if (newMapping == nullptr)
        {
            const DWORD errorCode = ::GetLastError();

            ::CloseHandle(newFile);

            ::SetLastError(errorCode);
            ThrowLastWindowsError("Cannot create file mapping");
        }
    }

    m_hFile = newFile;
    m_hMapFile = newMapping;
    m_dwFileSize = newFileSize;
    m_position = 0;

    // Не обязательно отображать первый блок прямо здесь.
    // Он будет отображён при первом Read/ReadByte.
}


void MappedFileStream::Close()
{
    unmapCurrentView();

    if (m_hMapFile != nullptr)
    {
        ::CloseHandle(m_hMapFile);
        m_hMapFile = nullptr;
    }

    if (m_hFile != nullptr && m_hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_hFile);
    }

    m_hFile = nullptr;

    m_dwFileSize = 0;
    m_dwMapViewSize = 0;
    m_dwFileMapStart = 0;
    m_dwViewDelta = 0;
    m_position = 0;

    m_pbegin = nullptr;
    m_pcurr = nullptr;
    m_pend = nullptr;
}

void MappedFileStream::unmapCurrentView() noexcept
{
    if (m_lpMapAddress != nullptr)
    {
        ::UnmapViewOfFile(m_lpMapAddress);
        m_lpMapAddress = nullptr;
    }

    m_pbegin = nullptr;
    m_pcurr = nullptr;
    m_pend = nullptr;

    m_dwMapViewSize = 0;
    m_dwFileMapStart = 0;
    m_dwViewDelta = 0;
}


void MappedFileStream::fetchmore()
{
    unmapCurrentView();

    if (!IsOpen())
    {
        throw std::logic_error("Cannot read from a closed MappedFileStream");
    }

    if (m_position >= m_dwFileSize)
    {
        return;
    }

    if (m_hMapFile == nullptr)
    {
        throw std::logic_error("File mapping object is not available");
    }

    /*
     * MapViewOfFile требует, чтобы file offset был кратен
     * allocation granularity.
     *
     * Поэтому:
     *
     * alignedOffset = position - position % granularity
     * delta         = position - alignedOffset
     */
    const uint64_t alignedOffset = m_position - (m_position % m_dwSysGran);

    const uint64_t delta = m_position - alignedOffset;

    const uint64_t remainingBytes = m_dwFileSize - m_position;

    uint64_t usefulBytes = std::min(m_bufsize, remainingBytes);

    /*
     * dwNumberOfBytesToMap имеет тип SIZE_T.
     * На 32-битной сборке SIZE_T имеет ширину 32 бита.
     */
    const uint64_t maxViewSize = static_cast<uint64_t>(std::numeric_limits<SIZE_T>::max());

    if (delta >= maxViewSize)
    {
        throw std::overflow_error("Mapping delta does not fit into SIZE_T");
    }

    usefulBytes = std::min(usefulBytes, maxViewSize - delta);

    if (usefulBytes == 0)
    {
        throw std::overflow_error("Mapped view size is zero");
    }

    const uint64_t completeViewSize = delta + usefulBytes;

    const DWORD offsetHigh = static_cast<DWORD>(alignedOffset >> 32);

    const DWORD offsetLow = static_cast<DWORD>(alignedOffset & 0xFFFFFFFFULL);

    void* mappedAddress = ::MapViewOfFile(
        m_hMapFile,
        FILE_MAP_READ,
        offsetHigh,
        offsetLow,
        static_cast<SIZE_T>(completeViewSize));

    if (mappedAddress == nullptr)
    {
        ThrowLastWindowsError("Cannot map file view");
    }

    m_lpMapAddress = static_cast<uint8_t*>(mappedAddress);

    m_dwFileMapStart = alignedOffset;
    m_dwViewDelta = delta;
    m_dwMapViewSize = completeViewSize;

    m_pbegin = m_lpMapAddress + static_cast<size_t>(delta);

    m_pcurr = m_pbegin;

    m_pend = m_pbegin + static_cast<size_t>(usefulBytes);
}


bool MappedFileStream::ReadByte(uint8_t& data)
{
    if (!IsOpen())
    {
        return false;
    }

    if (m_position >= m_dwFileSize)
    {
        return false;
    }

    if (m_pcurr == nullptr || m_pcurr == m_pend)
    {
        fetchmore();
    }

    if (m_pcurr == nullptr || m_pcurr == m_pend)
    {
        return false;
    }

    data = *m_pcurr;

    ++m_pcurr;
    ++m_position;

    return true;
}


uint64_t MappedFileStream::Read(uint8_t* buffer, const uint64_t count)
{
    if (count == 0)
    {
        return 0;
    }

    if (buffer == nullptr)
    {
        throw std::invalid_argument("MappedFileStream::Read: buffer is null");
    }

    if (!IsOpen())
    {
        return 0;
    }

    uint64_t totalRead = 0;

    while (totalRead < count && m_position < m_dwFileSize)
    {
        if (m_pcurr == nullptr || m_pcurr == m_pend)
        {
            fetchmore();
        }

        if (m_pcurr == nullptr || m_pcurr == m_pend)
        {
            break;
        }

        const uint64_t bytesAvailable = static_cast<uint64_t>(m_pend - m_pcurr);

        const uint64_t bytesRequested = count - totalRead;

        const uint64_t bytesToCopy = std::min(bytesAvailable, bytesRequested);

        std::memcpy(
            buffer + static_cast<size_t>(totalRead),
            m_pcurr,
            static_cast<size_t>(bytesToCopy));

        m_pcurr += static_cast<size_t>(bytesToCopy);
        m_position += bytesToCopy;
        totalRead += bytesToCopy;
    }

    return totalRead;
}