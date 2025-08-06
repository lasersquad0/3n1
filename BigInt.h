#pragma once

// C++ program to implement the above approach
// 
//#include <bits/stdc++.h>
#include <string>
#include <iostream>
#include <sstream>
#include <format>
#include <algorithm>

//using namespace std;


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
    std::string digits;
public:

    //Constructors:
    BigInt(unsigned long long nr = 0ull);
    BigInt(std::string& s);
    BigInt(const char* s);
    BigInt(const BigInt& a);

    bool IsEven() { if (digits.length() == 0) return true; else return digits[0] % 2 == 0; };
    bool HasTrailingZeros(unsigned int zeroes = 1) { for (unsigned int i = 0; i < zeroes; i++) if (digits[i] != 0) return false; return true; }

    //Helper Functions:
    friend void divide_by_2(BigInt& a);
    friend bool Null(const BigInt& a);
    friend int Length(const BigInt& a);
    int operator[](const int index)const;

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

    //operator unsigned long long() const;

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

// Specialization std::formatter to use BigInt variables in std:format() calls
template <>
struct std::formatter<BigInt> : std::formatter<std::string> //: std::formatter<std::string_view>
{
    /*constexpr auto parse(std::format_parse_context& ctx) 
    {
        return ctx.begin();
    }*/

    auto format(const BigInt& p, std::format_context& ctx) const
    {
        const char separator = ' ';
        std::string digits = p;
        std::string result;
        int count = 0;
        for (auto it = digits.rbegin(); it != digits.rend(); ++it) 
        {
            if (count && count % 3 == 0) result += separator;
            result += *it;
            ++count;
        }

        std::reverse(result.begin(), result.end());
       // return std::format_to(ctx.out(), "{}", result);
        return std::formatter<std::string>::format(result, ctx);
    }
};


