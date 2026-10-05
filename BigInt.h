#pragma once

// C++ program to implement the above approach
// 
//#include <bits/stdc++.h>
#include <string>
#include <iostream>
#include <sstream>
#include <format>
#include <algorithm>
#include <cassert>


class TBigIntException :public std::exception
{
public:
    TBigIntException(const char* Message) { Text = Message; }
    TBigIntException(const std::string& Message) { Text = Message; }
    virtual ~TBigIntException() noexcept {} ;
    virtual std::string getErrorMessage() const { return Text; }
    const char* what() const noexcept { return Text.c_str(); };
protected:
   // TBigIntException() {}
private:
    std::string Text;
};

class BigInt 
{
private:
    // digits[0] here is a least significant decimal digit
    // each string item contains one decial digit as is e.g. if i'th digit is 5 then digits[i]='\05' (not '5' which is code of symbol '5') 
    std::string digits;
public:
    // constants for quick compare with common values
    static const BigInt ONE;
    static const BigInt TWO;
    static const BigInt THREE;
    static const BigInt TEN;

    //Constructors:
    BigInt(unsigned long long nr = 0ull);
    BigInt(std::string& s);
    BigInt(const char* s);
    BigInt(const BigInt& a);
    
    const std::string& GetDigits() const { return digits; };
    void SetDigits(std::string& dig) { digits = dig; }; // sets digits in internal format

    bool IsEven() { if (digits.length() == 0) return true;  else return (digits[0] & 1) == 0; };
    bool IsOdd()  { if (digits.length() == 0) return false; else return (digits[0] & 1) == 1; };

    void RemoveLeadingZeros()
    {
        auto i = digits.length();
        while (i > 0) 
            if (digits[--i] != 0) break;
        digits.resize(i + 1);
    }

    bool HasTrailingZeros(unsigned int zeroes = 1) { for (unsigned int i = 0; i < zeroes; i++) if (digits[i] != 0) return false; return true; }
    
    // returns number of trailing bits set to zero (least significant bits)
    //TODO *** IT DOES NOT WORK!!! NEED TO BE REWRITTEN ****
    /*int TrailingZeroBits()
    { 
        if (digits.length() == 0) return 0;

        int i = 0;
        while (digits[i] == 0 && (i < digits.length())) i++; //number of trailing zero decimal digits

        unsigned char zeroBits = std::countr_zero((unsigned char)(digits[i]));
        assert(zeroBits < 8);

        return (i << 3) + zeroBits;
    }*/

    //Helper Functions:
    friend uint64_t toUInt64(const BigInt& v);
    friend void divide_by_2(BigInt& a);
    friend bool Null(const BigInt& a);
    friend int Length(const BigInt& a);
    int operator[](const int index) const;

    /* * * * Operator Overloading * * * */

    //Direct assignment
    BigInt& operator=(const BigInt&);

    //Post/Pre - Incrementation
    BigInt& operator++();
    BigInt operator++(int temp);
    BigInt& operator--();
    BigInt operator--(int temp);

    //Addition and Subtraction
    friend BigInt& operator+=(BigInt&, const BigInt&);
    friend BigInt operator+(const BigInt&, const BigInt&);
    friend BigInt operator-(const BigInt&, const BigInt&);
    friend BigInt& operator-=(BigInt&, const BigInt&);

    //Comparison operators
    friend bool operator==(const BigInt&, const BigInt&);
    friend bool operator!=(const BigInt&, const BigInt&);

    friend bool operator>(const BigInt&, const BigInt&);
    friend bool operator>=(const BigInt&, const BigInt&);
    friend bool operator<(const BigInt&, const BigInt&);
    friend bool operator<=(const BigInt&, const BigInt&);

    //Multiplication and Division
    friend BigInt& operator*=(BigInt&, const BigInt&);
    friend BigInt operator*(const BigInt&, const BigInt&);
    friend BigInt& operator/=(BigInt&, const BigInt&);
    friend BigInt operator/(const BigInt&, const BigInt&);

    //Modulo
    friend BigInt operator%(const BigInt&, const BigInt&);
    friend BigInt& operator%=(BigInt&, const BigInt&);

    //Power Function
    friend BigInt& operator^=(BigInt&, const BigInt&);
    friend BigInt operator^(BigInt&, const BigInt&);

    //operator uint64_t() const { }

    operator std::string() const 
    {
        // implementation #1
        std::string dig = digits;
        std::reverse(dig.begin(), dig.end());
        std::transform(dig.begin(), dig.end(), dig.begin(), [](int ch) -> char { return (char)ch + '0'; });
        return dig;
        
        /*
        // implementation #2, is this faster?
        std::stringstream s;
        s << *this;
        return s.str();*/
    }

    //Square Root Function
    friend BigInt sqrt(BigInt& a);

    //Read and Write
    friend std::ostream& operator<<(std::ostream&, const BigInt&);
    friend std::istream& operator>>(std::istream&, BigInt&);

    //Others
    friend BigInt NthCatalan(int n);
    friend BigInt NthFibonacci(int n);
    friend BigInt Factorial(int n);
};

void divide_by_2(BigInt& a);
uint64_t toUInt64(const BigInt& v);

/*
template<class IntImpl>
unsigned long long toULongLong(IntImpl b)
{
    std::stringstream strs;
    strs << b;

    unsigned long long res;
    strs >> res;
    return res;
}

// mitigate performance degradation of main ULongUlong function for uint64_t type parameter
template<>
inline unsigned long long toULongLong<uint64_t>(uint64_t b)
{
    return b;
}
*/

/*
// Specialization std::formatter to use BigInt variables in std:format() calls
template <>
struct std::formatter<BigInt> : std::formatter<std::string> //: std::formatter<std::string_view>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto it = ctx.begin();
        const auto end = ctx.end();

        if (it != end && *it == 'L')
        {
            //localized = true;
            ++it;
        }

        //if (it != end && *it != '}')
        //    throw std::format_error("invalid BigInt format specifier");

        return std::formatter<std::string>::parse(ctx);
    }

    auto format(const BigInt& p, std::format_context& ctx) const
    {
        const char separator = ' ';
        std::string digits = p;
        std::string result;
        int count = 0;
        for (auto it = digits.rbegin(); it != digits.rend(); ++it) 
        {
            if (count && ((count % 3) == 0)) result += separator;
            result += *it;
            ++count;
        }

        std::reverse(result.begin(), result.end());
        //return std::format_to(ctx.out(), "{}", result);
        return std::formatter<std::string>::format(result, ctx);
    }
};
*/

#include <array>
#include <string_view>

template<>
struct std::formatter<BigInt> : std::formatter<std::string_view>
{
private:
    static constexpr std::size_t max_spec_size = 256;
    bool haveLocaleSpecifier = false;
public:
    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto begin = ctx.begin();
        const auto end = ctx.end();

        // Find closing '}' of current replacement field.
        // Nested {} are possible, e.g. when dynamic field width is used 
        auto close = begin;
        std::size_t nested = 0;

        for (; close != end; ++close)
        {
            if (*close == '{')
            {
                ++nested;
            }
            else if (*close == '}')
            {
                if (nested == 0)
                    break;

                --nested;
            }
        }

        assert(*close == '}');

        // close must refer to '}' here, throw an exception if not.
        if (close == end)
            throw std::format_error("Unterminated BigInt format specification");

        /*
         * For string values L can be:
         *
         *   right before '}'
         *   right before 's'
         *   right before '?'
         *
         * Examples:
         *
         *   {:L}
         *   {:>20L}
         *   {:>20Ls}
         */
        auto localeSpecifier = close;
        
        if (localeSpecifier != begin)
        {
            --localeSpecifier;

            // In case of string presentation type, check symbol before it.
            if ((*localeSpecifier == 's' || *localeSpecifier == '?') && localeSpecifier != begin)
            {
                --localeSpecifier;
            }
        }

        // If L is not found we are just calling base parse().
        // This is important for proper handling dynamic width and prescision specifiers.
        if (localeSpecifier == close || *localeSpecifier != 'L')
        {
            return std::formatter<std::string_view>::parse(ctx);
        }

        /*
         * Create a copy without L.
         *
         * Example:
         *
         *   >20L}  -> >20}
         *   *^20Ls} -> *^20s}
         */
        haveLocaleSpecifier = true;
        std::array<char, max_spec_size> filtered{};
        std::size_t size = 0;

        for (auto it = begin; it != close; ++it)
        {
            if (it == localeSpecifier)
                continue;

            if (size + 1 >= filtered.size())
            {
                throw std::format_error("BigInt format specification is too long");
            }

            filtered[size++] = *it;
        }

        // Base formatter expects to see closing '}'.
        filtered[size++] = '}';

        //TODO problem with temporary context is that it does not contain proper value for _Num_args
        // that is why formats like {:>{}L} will not compile while formats {:L} {:{}} (without L) will work well
        std::format_parse_context filteredContext{std::string_view{filtered.data(), size} };

        // Base string formatter stores width, alignment, fill, precision and presentation type in its private fields.
        const auto result = std::formatter<std::string_view>::parse(filteredContext);

        // Make sure that base formatter stays on '}'.
        if (result == filteredContext.end() || *result != '}')
        {
            throw std::format_error("Invalid BigInt format specification");
        }

        // return iterator of original context instead of temporary.
        return close;
    }

    auto format(const BigInt& value, std::format_context& ctx) const
    {
        const std::string digits = value; // convert BigInt into string
        assert(digits.size() > 0);

        const auto& punct = std::use_facet<std::numpunct<char>>(ctx.locale());
        const char separator = punct.thousands_sep();
        const std::string grouping = punct.grouping();
        std::string result;

        if (!haveLocaleSpecifier || grouping.empty() || (grouping.front() == '\0'))
        {
            // if no groupping in locale
            return std::formatter<std::string_view>::format(digits, ctx);
        }
        else
        {
            const uint32_t group1 = grouping.front();

            result.reserve(digits.size() + digits.size() / 3);

            std::size_t firstDigit = 0;

            // Sign is not part of groupping.
            if (!digits.empty() && (digits.front() == '-' || digits.front() == '+'))
            {
                result += digits.front();
                firstDigit = 1;
            }

            const std::size_t digitCount = digits.size() - firstDigit;

            for (std::size_t i = 0; i < digitCount; ++i)
            {
                if (i != 0 && (digitCount - i) % group1 == 0)
                    result += separator;

                result += digits[firstDigit + i];
            }
        }
        /*
         * Here will be applied parameters which base formatter<string_view> saved in parse():
         *
         * - fill;
         * - align;
         * - width;
         * - precision;
         * - presentation type.
         */
        return std::formatter<std::string_view>::format(result, ctx);
    }
};