#include <iostream>
//#include <chrono>
#include "Utils.h"
#include "string_utils.h"


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
    for (size_t i = 0; i < str.length(); i++)
    {
        if (str[i] != '\'') res += str[i];
    }

    return res;
}


// parses one value either m_realStart or m_realLength in format with Factor multiplier: B, K, M, G, T, P
// examples: 100G, 5T, 20000G, 400M, 0B
// returns back symFactor and factor values, and real calculated value as uint64 as a result of function call 
uint64_t ParseNumber(std::string num, char& FactorSym, uint64_t& FactorInt)
{
    const std::string SYMBOLS = "BKMGTP";
    const uint64_t F_B = 1ULL;
    const uint64_t F_K = 1ULL<<10;
    const uint64_t F_M = 1ULL<<20;
    const uint64_t F_G = 1ULL<<30;
    const uint64_t F_T = 1ULL<<40;
    const uint64_t F_P = 1ULL<<50;

    // remove any leading and traling spaces, just in case.
    TrimAndUpper(num);

    FactorSym = num.at(num.length() - 1); // we need symFACTOR later for output filename

    uint64_t ffactor = 0;
    if (SYMBOLS.find(FactorSym) != std::string::npos) // factor is present in num
    {
        switch (FactorSym)
        {
        case 'B': ffactor = F_B; break;
        case 'K': ffactor = F_K; break;
        case 'M': ffactor = F_M; break;
        case 'G': ffactor = F_G; break;
        case 'T': ffactor = F_T; break;
        case 'P': ffactor = F_P; break;
        default:
            throw std::invalid_argument("[ParseNumber] String value should be a number with factor (one letter from list: BKMGTP). Factor is incorrect here: '" + num + "'.\n");
        }

        num = num.substr(0, num.length() - 1); // remove letter G at the end. Or T, or P, or M, or K or B.
    }
    else if (std::isdigit(FactorSym)) // looks like num is just number without factor at the end
        ffactor = F_B;
    else
        throw std::invalid_argument("[ParseNumber] Incorrect num parameter '" + num + "'.\n");

    uint64_t number;
    try
    {
        number = std::stoull(num);
    }
    catch (...)
    {
        throw std::invalid_argument("[ParseNumber] String value should be a number: '" + num + "'.\n");
    }

    FactorInt = ffactor;

    return number * ffactor;
}

uint64_t ParseNumber(std::string num)
{
    char FactorSym;
    uint64_t FactorInt;
    return ParseNumber(num, FactorSym, FactorInt);
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
