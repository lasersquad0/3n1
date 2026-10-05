#pragma once

#include <string>
#include <locale>
#include <fstream>
#include <cassert>
#include "BigInt.h"
#include "utils/include/string_utils.h"

const uint64_t BIGINT_THRESHOLD = std::numeric_limits<uint64_t>::max() / 10;
const uint32_t STEPS_MAX = std::numeric_limits<uint16_t>::max();
const uint32_t F_WIDTH = 27;
const uint32_t MAX_VALUE_WIDTH = 27;


namespace ThreeN1
{
	struct MyGroupSeparator : std::numpunct<char>
	{
		char do_thousands_sep() const override { return '\''; } // thousands separator
		std::string do_grouping() const override { return "\3"; } // group by 3
	};

	inline std::locale Locale(std::cout.getloc(), new MyGroupSeparator());

};

//uint64_t ParseNumber(std::string num, char& FactorSym, uint64_t& FactorInt);
//uint64_t ParseNumber(std::string num);

//std::string millisecToStr(long long ms);
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
* @brief Parses string in format with Factor multiplier suffix: B, K, M, G, T, P
* @details Examples: 100G, 5T, 20000G, 5'000'000G, 400M, 0B, 1234 (no suffix)
*   num is passed by value intentionally
* @return A value of IntImpl type constructed from a string 'num' with factor suffx applied
*/
template<typename IntImpl>
IntImpl ParseNumber(std::string num/*, char& FactorSym, uint64_t& FactorInt*/)
{
	const std::string SYMBOLS = "BKMGTP";
	const uint64_t F_B = 1;
	const uint64_t F_K = 1000;
	const uint64_t F_M = F_K * 1000;
	const uint64_t F_G = F_M * 1000;
	const uint64_t F_T = F_G * 1000;
	const uint64_t F_P = F_T * 1000;

	// remove any leading and traling spaces, just in case.
	TrimAndUpper(num);

	// remove apostrophes from number
	std::string num2 = RemoveApo(num);

	char FactorSym = num2.at(num2.length() - 1);

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

		num2.resize(num2.length() - 1); // remove letter G at the end. Or T, or P, or M, or K or B.
	}
	else if (std::isdigit(FactorSym)) // looks like num is just number without factor at the end
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

		return number * ffactor;
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

