/*
 * Compare.h
 *
 * Copyright 2025, LogEngine2 Project. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef COMPARE_H
#define COMPARE_H

//#include <exception> //unix
#include <string>
//#include "Common.h"


// compares two strings case insensitive
template <typename STRING>
bool EqualNCase(const STRING& str1, const STRING& str2)
{
	// make sure that STRING is one of instantiations of std::string
	static_assert(std::is_base_of<std::basic_string<typename STRING::value_type, typename STRING::traits_type>, STRING>::value);

	if (str1.length() == 0 && str2.length() == 0)
		return true;
	if ((str1.length() == 0) && (str2.length() > 0))
		return false;
	if ((str1.length() > 0) && (str2.length() == 0))
		return false;

	//#ifdef HAVE_STRCASECMP
	//	return strcasecmp(s1, s2) == 0;
	//#else

		//for (; *s1 != '\0' && *s2 != '\0'; s1++, s2++)
	for (size_t i = 0; i < str1.length() && i < str2.length(); i++)
		if (toupper((str1[i])) != toupper((str2[i])))
			return false;

	if (str1.length() == str2.length())
		return true;
	else
		return false;
	//#endif
}

template<class STRING>
int CompareNCase(const STRING& str1, const STRING& str2)
{
	// make sure that STRING is one of instantiations of std::string
	static_assert(std::is_base_of<std::basic_string<typename STRING::value_type, typename STRING::traits_type>, STRING>::value);

	if ((str1.length() == 0) && (str2.length() == 0)) // two empty strings are equal
		return 0;

	if ((str1.length() == 0)) //empty string is less any non empty 
		return -1;

	if (str2.length() == 0) // non-empty string is larger any empty one
		return 1;

	//#ifdef HAVE_STRCASECMP
	//	return strcasecmp(s1, s2);
	//#else
	for (size_t i = 0; i < str1.length() && str2.length(); i++)
	{
		int upper1 = toupper((str1[i]));
		int upper2 = toupper((str2[i]));

		if (upper1 > upper2)
			return 1;
		else if (upper1 < upper2)
			return -1;
	}

	if (str1.length() > str2.length())
		return 1;
	else if (str1.length() < str2.length())
		return -1;
	else
		return 0;
	//#endif
}

/// Class to compare other classes by operators ==, <, >
// Actually it uses only two operators for comparing: == and >. It means that comparing classes need to implement only two compare operators. 
template<class C>
class Compare
{
public:
	virtual bool eq(const C& a, const C& b) const { return a == b; } // returns True when object a is equal b
	virtual bool lt(const C& a, const C& b) const { return b > a;  } // lt=less than
	virtual bool mt(const C& a, const C& b) const { return a > b;  } // mt=more than
	virtual ~Compare() {}
};

template<class C>
class CompareReverse : public Compare<C>
{
public:
	bool eq(const C& a, const C& b) const override 
	{
		return a == b;
	}

	bool lt(const C& a, const C& b) const override
	{
		return a > b;
	}

	bool mt(const C& a, const C& b) const override
	{
		return b > a;
	}
};

// Class to compare std::string WITHOUT CASE sensitivity
class CompareStringNCase: public Compare<std::string>
{
public:
	bool eq(const std::string& a, const std::string& b) const override // returns True when string a is equal b
	{ 
		return EqualNCase(a, b); 
	}

	bool lt(const std::string& a, const std::string& b) const override
	{
		return CompareNCase(a, b) < 0;

		/*std::string aa = a, bb = b;
		std::transform(aa.begin(), aa.end(), aa.begin(), ::toupper);
		std::transform(bb.begin(), bb.end(), bb.begin(), ::toupper);
		return aa.compare(bb) < 0;*/ 
	}

	bool mt(const std::string& a, const std::string& b) const override
	{ 
		return CompareNCase(a, b) > 0;

		/*std::string aa = a, bb = b;
		std::transform(aa.begin(), aa.end(), aa.begin(), ::toupper);
		std::transform(bb.begin(), bb.end(), bb.begin(), ::toupper);
		return aa.compare(bb) > 0;*/ 
	}
};


// Class to compare std::wstring WITHOUT CASE sensitivity
class CompareWStringNCase : public Compare<std::wstring>
{
public:
	bool eq(const std::wstring& a, const std::wstring& b) const override // returns True when string a is equal b
	{
		return EqualNCase(a, b);
	}

	bool lt(const std::wstring& a, const std::wstring& b) const override
	{
		return CompareNCase(a, b) < 0;
	}

	bool mt(const std::wstring& a, const std::wstring& b) const override
	{
		return CompareNCase(a, b) > 0;
	}
};


#endif //COMPARE_H
