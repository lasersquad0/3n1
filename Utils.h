#pragma once

#include <string>
#include <locale>
#include <fstream>
#include <cassert>
#include "BigInt.h"
#include "utils/include/string_utils.h"
#include "ttmath/ttmath.h"


using TTMathBigInt = ttmath::UInt<10>;

const uint64_t BIGINT_THRESHOLD = std::numeric_limits<uint64_t>::max() / 10;
const uint32_t F_WIDTH = 27;
const uint32_t MAX_VALUE_WIDTH = 27;

#define DEFAULT_THREADS 1
#define UNUSED_DEF_SIZE 0 // size of array in bits for tracking unused numbers, 0 means - no tracking of unused numbers
#define APP_NAME "ThreeN1"
#define APP_EXE_NAME APP_NAME ".exe"
//#define PERCENT_FACTOR 100
#define CACHE_FILE_BIN "3-1G.bin"
#define CACHE_FILE_VARLEN "ThreeN1 cache - 300M-600M.varlen"
#define VARLEN_EXT ".varlen"
#define BIN_EXT ".bin"

class Parameters
{
public:
	static uint32_t THREADS;
	static uint64_t UNUSED_SIZE;
	static bool USE_CACHE;
	static bool GENERATE_CACHE;
	static bool USE_LONG_ARITHM;
	static std::string CACHE_FILENAME;
	static bool HAS_CACHE_FILENAME;  // true when filename is specified in command line (option -f)
};

// value passed by value intentionally
template<typename IntImpl>
std::string ReduceNumber(IntImpl value, bool thouSep = true);

std::string ReduceNumber(uint64_t value, bool thouSep = true);

struct CacheRange
{
	uint64_t Start;
	uint64_t Finish;
};

namespace ThreeN1
{
	struct MyGroupSeparator : std::numpunct<char>
	{
		char do_thousands_sep() const override { return '\''; } // thousands separator
		std::string do_grouping() const override { return "\3"; } // group by 3
	};

	inline std::locale Locale(std::cout.getloc(), new MyGroupSeparator());

	inline CacheRange GetCacheRange(uint64_t start, uint64_t finish) { auto range = finish - start; return finish / 2 > range ? CacheRange{ finish / 2 - range, finish / 2 } : CacheRange{ 0, finish / 2 }; };
	//TODO for BigInt we should make sure that cache is placed somewhere inside uint64_t range. Need another formular here
	inline CacheRange GetCacheRange(const BigInt& start, const BigInt& finish) { return GetCacheRange(toUInt64(start), toUInt64(finish)); };
	inline CacheRange GetCacheRange(const TTMathBigInt& start, const TTMathBigInt& finish) { return GetCacheRange(start.ToUInt(), finish.ToUInt()); };

	//this method is called only when we need to save cache data into file
	// start and finish is calculation range here, not a cache range
	template<typename IntImpl>
	std::string GetCacheFileName(IntImpl start, IntImpl finish, const std::string& ext)
	{
		// if user has specified cache file name return it as is
		if (Parameters::HAS_CACHE_FILENAME)
			return Parameters::CACHE_FILENAME;
		// if user is not provided cache filename then
		//   - if -g present: build it from cache start and finish values
		//   - if -g is not present: return default cache file name from Parameters::CACHE_FILENAME
		else
			if (Parameters::GENERATE_CACHE)
			{
				auto cr = GetCacheRange(start, finish);
				return std::format("ThreeN1 cache - {}-{}{}", ReduceNumber(cr.Start, false), ReduceNumber(cr.Finish, false), ext);
			}
			else
				return Parameters::CACHE_FILENAME;
	}

};

// value passed by value intentionally
template<typename IntImpl>
std::string ReduceNumber(IntImpl value, bool thouSep)
{
	if (value == 1ull) return "0";

	static constexpr std::array<const char*, 9> suffixes = { "", "K", "M", "G", "T", "P", "E", "Z", "Y" };

	static const std::array<IntImpl, 9> powers = {
		1ULL,                        // 10^0
		1'000ULL,                    // 10^3  (K)
		1'000'000ULL,                // 10^6  (M)
		1'000'000'000ULL,            // 10^9  (G)
		1'000'000'000'000ULL,        // 10^12 (T)
		1'000'000'000'000'000ULL,    // 10^15 (P)
		1'000'000'000'000'000'000ULL,// 10^18 (E)
		"1000000000000000000000",    // 10^21 (Z)
		"1000000000000000000000000"  // 10^24 (Y)
	};

	size_t group;
	// Find max group that divides the number without a remainder
	for (group = powers.size() - 1; group > 0; --group)
	{
		if (value % powers[group] == 0ull)
		{
			value /= powers[group];
			break;
		}
	}

	const std::string digits = value.ToString(); // convert BigInt into string
	assert(digits.size() > 0);

	if (!thouSep) return digits + suffixes[group];

	const auto& punct = std::use_facet<std::numpunct<char>>(ThreeN1::Locale);
	const char separator = punct.thousands_sep();
	const std::string grouping = punct.grouping();

	if (grouping.empty() || (grouping.front() == '\0'))
	{
		// if no groupping in locale
		return digits + suffixes[group];
	}
	else
	{
		std::string result;
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

		return result + suffixes[group];
	}
}

uint64_t NumLen(uint64_t num);
uint64_t NumLen(const BigInt& num);

std::string RemoveApo(const std::string& str);

size_t VarLenReadBuf(std::ifstream& fin, uint8_t* buf);
size_t var_len_encode(uint8_t buf[9], uint64_t num);
size_t var_len_decode(const uint8_t buf[], size_t size_max, uint64_t* num);

// this is concept that checks that T can be constructed from a string.
// e.g. BigInt can be constructed from a string while uint64_t cannot.
template<typename T>
concept StringConstructible = requires(const std::string& s) 
{
	{ T(s) } -> std::same_as<T>;
};

/**
* @brief Parses string in format with Factor multiplier suffix: B, K, M, G, T, P, E, Z, Y
* @details Examples: 100G, 5T, 20000G, 5'000'000G, 400M, 0B, 1234 (no suffix)
*   num is passed by value intentionally
* @return A value of IntImpl type constructed from a string 'num' with factor suffx applied
*/
template<typename IntImpl>
IntImpl ParseNumber(std::string num/*, char& FactorSym, uint64_t& FactorInt*/)
{
	const std::string SYMBOLS = "BKMGTPEZY";
	const uint64_t F_B = 1;
	const uint64_t F_K = 1000;
	const uint64_t F_M = F_K * 1000;
	const uint64_t F_G = F_M * 1000;
	const uint64_t F_T = F_G * 1000;
	const uint64_t F_P = F_T * 1000;
	const BigInt   F_E = F_P * 1000;
	const BigInt   F_Z = F_E * 1000;
	const BigInt   F_Y = F_Z * 1000;

	// remove any leading and traling spaces, just in case.
	TrimAndUpper(num);

	// remove apostrophes from number
	std::string num2 = RemoveApo(num);

	char FactorSym = num2.at(num2.length() - 1);

	BigInt ffactor;
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
		case 'E': ffactor = F_E; break;
		case 'Z': ffactor = F_Z; break;
		case 'Y': ffactor = F_Y; break;
		default:
			throw std::invalid_argument("[ParseNumber] String value should be a number with factor (one letter from list: BKMGTP). Factor is incorrect here: '" + num + "'.\n");
		}

		num2.resize(num2.length() - 1); // remove letter G at the end. Or T, or P, or M, or K or B.
	}
	else 
		if (std::isdigit(FactorSym)) // looks like num is just number without factor at the end
			ffactor = F_B;
		else
			throw std::invalid_argument("[ParseNumber] Incorrect num parameter '" + num + "'.\n");


	if constexpr (StringConstructible<IntImpl>) 
	{
		return IntImpl(num2) * ffactor;  // BigInt
	}
	else
	{
		uint64_t number;

		try
		{
			number = std::stoull(num2);
		}
		catch (...)
		{
			throw std::invalid_argument("[ParseNumber] String value should be a number: '" + num + "'.\n");
		}

		// also it is possible to convert to uint64_t from string as shown below
		/*uint64_t value = 0;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
		if (ec != std::errc{} || ptr != str.data() + str.size()) {
			throw std::invalid_argument("Invalid number: " + str);
			*/

		//FactorInt = ffactor;

		return toUInt64(number * ffactor);
	}
}


class MyBitset
{
private:
	static const uint64_t WORD_2POWER = 6ULL; // 2^6==sizeof(uint64_t)*8 = 64;
	static const uint64_t BITS_IN_WORD = 1ULL << WORD_2POWER; //==sizeof(uint64_t)*8 = 64;
	static const uint64_t WORD_MASK = BITS_IN_WORD - 1ULL; // =63=0x3F

	uint64_t m_bits = 0;
	uint64_t* m_arr = nullptr; 

	//	bool get_bit(uint64_t word, uint32_t offset)
	//	{
	//		assert(offset < WORD_IN_BITS);
	//		uint64_t tmp = (word >> offset) & 0x01;
	//		//tmp &= 0x01;
	//		return tmp == 1;
	//	}

		//uint64_t set_bit(uint64_t word, uint32_t offset, bool bit)
		//{
		//	assert(offset < BITS_IN_WORD);
		//	uint64_t mask = bit ? 0x01 : 0x00;
		//	//mask <<= offset;

		//	return word | (mask << offset); // don't work well if 1 is already there and we want to put 0 now.
		//}

public:
	MyBitset()
	{
		//two fields are initialised inline above
		//m_bits = 0;
		//m_arr = nullptr;
	}

	MyBitset(uint64_t bitsCount)
	{
		Init(bitsCount);
	}

	~MyBitset()
	{
		free(m_arr);
		m_arr = nullptr;
	}

	void Init(uint64_t bitsCount)
	{
		if (m_arr) free(m_arr); // free previously allocated memory.
		m_bits = bitsCount;
		uint64_t wordsCnt = (bitsCount + (BITS_IN_WORD - 1ULL)) / BITS_IN_WORD;
		m_arr = (uint64_t*)calloc(wordsCnt, sizeof(uint64_t)); // initialises memory to zero
	}

	inline bool get(uint64_t bitIndex) const
	{
		assert(bitIndex < m_bits);

		uint64_t index2 = bitIndex >> WORD_2POWER; 
		uint64_t offset = bitIndex & WORD_MASK; 

		return ((m_arr[index2] >> offset) & 0x01) == 1ULL;
	}

	inline void setTrue(BigInt& bitIndex)
	{
		setTrue(toUInt64(bitIndex));
	}

	inline void setTrue(uint64_t bitIndex)
	{
		assert(bitIndex < m_bits);

		uint64_t index2 = bitIndex >> WORD_2POWER; // index/BITS_IN_WORD;
		uint64_t offset = bitIndex & WORD_MASK; // index % BITS_IN_WORD;

		m_arr[index2] |= (1ull << offset); //TODO don't work well if 1 is already there and we want to put 0 now.
		//arr[index2] = set_bit(arr[index2], offset, value);
	}

	inline uint64_t BitsCount() const
	{
		return m_bits;
	}
};

