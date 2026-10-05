#include <iostream>
#include "Utils.h"


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




/*uint64_t ParseNumber(std::string num)
{
    char FactorSym;
    uint64_t FactorInt;
    return ParseNumber<uint64_t>(num, FactorSym, FactorInt);
}*/

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
