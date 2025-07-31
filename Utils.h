#pragma once

#include <string>
#include <locale>
#include <fstream>
#include <cassert>

#include "BigInt.h"

struct MyGroupSeparator : std::numpunct<char>
{
	char do_thousands_sep() const override { return ' '; } // разделитель тысяч
	std::string do_grouping() const override { return "\3"; } // группировка по 3
};

uint64_t ParseNumber(std::string num, char& FactorSym, uint64_t& FactorInt);
uint64_t ParseNumber(std::string num);

//std::string millisecToStr(long long ms);
uint64_t NumLen(uint64_t num);
uint64_t NumLen(const BigInt& num);

std::string RemoveApo(const std::string& str);
size_t VarLenReadBuf(std::ifstream& fin, uint8_t* buf);
size_t var_len_encode(uint8_t buf[9], uint64_t num);
size_t var_len_decode(const uint8_t buf[], size_t size_max, uint64_t* num);


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

		//	return word | (mask << offset); // не сработает правильно если ранее туда записана 1 и мы сейчас хотим заисать 0.
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

	inline void setTrue(uint64_t bitTndex)
	{
		assert(bitTndex < m_bits);

		uint64_t index2 = bitTndex >> WORD_2POWER; // index/BITS_IN_WORD;
		uint64_t offset = bitTndex & WORD_MASK; // index % BITS_IN_WORD;

		m_arr[index2] |= (1ull << offset); //TODO не сработает правильно если ранее туда записана 1 и мы сейчас хотим записать 0.
		//arr[index2] = set_bit(arr[index2], offset, value);
	}

	inline uint64_t BitsCount() const
	{
		return m_bits;
	}
};

