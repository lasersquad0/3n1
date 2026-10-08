#include <iostream>
#include "Utils.h"


uint32_t Parameters::THREADS = DEFAULT_THREADS;
uint64_t Parameters::UNUSED_SIZE = UNUSED_DEF_SIZE;
bool Parameters::USE_CACHE = false;
bool Parameters::GENERATE_CACHE = false;
bool Parameters::USE_LONG_ARITHM = false;
bool Parameters::HAS_CACHE_FILENAME = false;
std::string Parameters::CACHE_FILENAME = CACHE_FILE_VARLEN;


uint64_t NumLen(uint64_t num)
{
    uint64_t result = 1;
    while (num /= 10) ++result;
    return result;
}

uint64_t NumLen(const BigInt& num)
{
    return Length(num);
}

// reading one varlen number from file
size_t VarLenReadBuf(std::ifstream& fin, uint8_t* buf)
{
    size_t maxSize = 0;
    while (true)
    {
        fin.read((char*)(buf + maxSize), 1);
        if (fin.eof()) break;
        if ((buf[maxSize++] & 0x80) == 0) break;
    }

    return maxSize;
}

std::string RemoveApo(const std::string& str)
{
    std::string res;
    res.reserve(str.size());
    for (size_t i = 0; i < str.length(); i++)
    {
        if (str[i] != '\'') res += str[i];
    }

    return res;
}

std::string ReduceNumber(uint64_t value, bool thouSep) 
{
    if (value == 0) return "0";

    static constexpr std::array<const char*, 9> suffixes = { "", "K", "M", "G", "T", "P", "E", "Z", "Y" };

    static constexpr std::array<uint64_t, 7> powers = {
    1ULL,                        // 10^0
    1'000ULL,                    // 10^3  (K)
    1'000'000ULL,                // 10^6  (M)
    1'000'000'000ULL,            // 10^9  (G)
    1'000'000'000'000ULL,        // 10^12 (T)
    1'000'000'000'000'000ULL,    // 10^15 (P)
    1'000'000'000'000'000'000ULL,// 10^18 (E)
    };

    // Find max group that divides the number without a remainder
    for (size_t group = powers.size() - 1; group > 0; --group) 
    {
        if (value % powers[group] == 0)
            if (thouSep)
                return std::format(ThreeN1::Locale, "{:L}{}", value / powers[group], suffixes[group]);
            else
                return std::to_string(value / powers[group]) + suffixes[group];
    }

    // no group of zeros that is a multiple of 3.
    return thouSep ? std::format(ThreeN1::Locale, "{:L}", value) : std::to_string(value);
}


size_t var_len_encode(uint8_t buf[9], uint64_t num)
{
    if (num > UINT64_MAX / 2)
        return 0;

    size_t i = 0;

    while (num >= 0x80)
    {
        buf[i++] = (uint8_t)(num) | 0x80;
        num >>= 7;
    }

    buf[i++] = (uint8_t)(num);

    return i;
}

size_t var_len_decode(const uint8_t buf[], size_t size_max, uint64_t* num)
{
    if (size_max == 0)
        return 0;

    if (size_max > 9)
        size_max = 9;

    *num = buf[0] & 0x7F;
    size_t i = 0;

    while (buf[i++] & 0x80)
    {
        if (i >= size_max || buf[i] == 0x00)
            return 0;

        *num |= (uint64_t)(buf[i] & 0x7F) << (i * 7);
    }

    return i;
}
