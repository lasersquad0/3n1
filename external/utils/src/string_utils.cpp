
#include <format>
#include <stdexcept>
#include <cassert>
#include <algorithm>

#include "../include/string_utils.h"

#ifdef WIN32
// special non-template function for wchar_t*
//template<>
/*std::string wtos(const wchar_t* wstr)
{
    std::wstring wwstr(wstr);
    int len = WideCharToMultiByte(CP_UTF8, 0 /*WC_NO_BEST_FIT_CHARS*//*, wwstr.data(), (int)wwstr.length(), nullptr, 0, nullptr, nullptr);

    if (len == 0)
        throw std::runtime_error(std::format("Error in WideCharToMultiByte 1. Error code: {}", GetLastError()));

    std::string dest(len, 0);
    //dest.resize(len);
    int err = WideCharToMultiByte(CP_UTF8, 0 /*WC_NO_BEST_FIT_CHARS*//*, wwstr.data(), (int)wwstr.size(), dest.data(), len, nullptr, nullptr);

    if (!err)
        throw std::runtime_error(std::format("Error in WideCharToMultiByte 2. Error code: {}", GetLastError()));

    return dest;
}*/

/*std::string wtos(const char* str)
{
    return str;
}*/


std::wstring stow(const char* astr)
{
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, astr, -1, nullptr, 0); // Get the required buffer size
    if (bufferSize == 0)
        throw std::runtime_error(std::format("Error in MultiByteToWideChar 1. Error code: {}", GetLastError()));

    std::wstring wstr(bufferSize, 0);

    int result = MultiByteToWideChar(CP_UTF8, 0, astr, -1, &wstr[0], bufferSize);
    if (result == 0)
        throw std::runtime_error(std::format("Error in MultiByteToWideChar 2. Error code: {}", GetLastError()));

    wstr.resize(bufferSize - 1);
    return wstr;
}
#endif

void Trim(string_t& str) //Note: str will be changed
{
    // remove any leading and traling spaces and tabs.
    size_t strBegin = str.find_first_not_of(U(" \t"));
    if (strBegin == string_t::npos) return;

    size_t strEnd = str.find_last_not_of(U(" \t"));
    assert(strEnd != string_t::npos);

    str.erase(strEnd + 1 /*, str.size() - strEnd*/);
    str.erase(0, strBegin);
}

static char mytoupper(int c) // to eliminate compile warning "warning C4244: '=': conversion from 'int' to 'char', possible loss of data"
{
    return (char)toupper(c);
}

void TrimAndUpper(string_t& str) //Note: str will be changed
{
    Trim(str);

    // to uppercase
    transform(str.begin(), str.end(), str.begin(), ::mytoupper);
}


// extracts filename from path with filename
std::string ExtractFileName(const std::string& FileName)
{
	size_t i = FileName.length();
	if (i == 0) return "";

	do
	{
		i--;
		if ((FileName[i] == '\\') || (FileName[i] == '/'))
		{
			i++;
			break;
		}
	} while (i > 0);

	return FileName.substr(i);
}

// extracts file dir from path with filename
std::string ExtractFileDir(const std::string& FileName)
{
	size_t i = FileName.length();
	if (i == 0) return "";

	do
	{
		i--;
		if ((FileName[i] == '\\') || (FileName[i] == '/'))
		{
			i++;
			break;
		}
	} while (i > 0);

	return FileName.substr(0, i);
}

std::string ExtractFileExt(const std::string& FileName)
{
	size_t i = FileName.length();
	if (i == 0) return "";

	bool f = false;
	do
	{
		i--;
		if (FileName[i] == '.')
		{
			f = true; // we found a dot
			break;
		}

		if (FileName[i] == '\\' || FileName[i] == '/') // stop is folder delimiter found
			break;

	} while (i > 0);

	if (f)
		return FileName.substr(i);
	else
		return "";
}

std::string StripFileExt(const std::string& FileName)
{
	size_t i = FileName.length();
	if (i == 0) return "";

	bool f = false;
	do
	{
		i--;
		if (FileName[i] == '.')
		{
			f = true; // we found a dot
			break;
		}

		if (FileName[i] == '\\' || FileName[i] == '/') // stop is folder delimiter found
			break;

	} while (i > 0);

	if (f)
		return FileName.substr(0, i);
	else
		return FileName;
}

