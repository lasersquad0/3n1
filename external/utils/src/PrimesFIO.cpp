
#include <string>
#include <cassert>
#include <algorithm>
#include "BuferedFileStream.h"
#include "PrimesFIO.h"

using namespace std;

// Loading either entire file or up to len prime numbers from TXT file
size_t PrimesFIO::LoadFromTXT(uint64_t* arr, size_t len, fstream& f)
{
	string line;
	line.reserve(50);

	uint32_t cnt = 0;
	while (cnt < len)
	{
		getline(f, line, ',');
		if (f.eof()) break;
		arr[cnt++] = atoll(line.c_str());
	}

	return cnt;
}

size_t PrimesFIO::BypassTXT(size_t bypassCount, fstream& f)
{
	if (bypassCount == 0) return 0;

	string line;
	line.reserve(50);

	uint32_t cnt = 0;
	while (bypassCount > 0)
	{
		getline(f, line, ',');
		if (f.eof()) break;
		bypassCount--;
		cnt++;
	}

	return cnt;
}

size_t PrimesFIO::LoadFromTXTDiff(uint64_t* arr, size_t len, uint64_t& lastPrime, fstream& f)
{
	string line;
	line.reserve(50);

	uint32_t cnt = 0;

	while (cnt < len)
	{
		getline(f, line, ',');
		if (f.eof()) break;
	
		// diff between 2 and 3 is 1. we do not divide it by 2 during saving, all the other diffs are divided by 2.
		if (lastPrime < 3)
			arr[cnt] = lastPrime + atoll(line.c_str()); 
		else
			arr[cnt] = lastPrime + atoll(line.c_str()) * 2;
		
		lastPrime = arr[cnt++];
	}

	return cnt;
}

size_t PrimesFIO::BypassTXTDiff(uint64_t bypassCount, uint64_t& lastPrime, fstream& f)
{
	if (bypassCount == 0) return 0;

	string line;
	line.reserve(50);

	uint32_t cnt = 0;

	while (bypassCount > 0)
	{
		getline(f, line, ',');
		if (f.eof()) break;

		// diff between 2 and 3 is 1. we do not divide it by 2 during saving, all the other diffs are divided by 2.
		if (lastPrime < 3)
			lastPrime += atoll(line.c_str());
		else
			lastPrime += atoll(line.c_str()) * 2;
		
		bypassCount--;
		cnt++;
	}

	return cnt;
}

size_t PrimesFIO::LoadFromBIN(uint64_t* arr, size_t len, fstream& f)
{
	f.read((char*)arr, sizeof(uint64_t)*len);

	return f.gcount()/sizeof(uint64_t);

	/*uint32_t cnt = 0;
	while (cnt < len)
	{
	
		f.read((char*)(arr + cnt), sizeof(uint64_t));
		if (f.eof()) break;
		cnt++;
	}

	return cnt;*/
}

size_t PrimesFIO::BypassBIN(size_t bypassCount, fstream& f)
{
	if (bypassCount == 0) return 0;

	f.seekg(bypassCount * sizeof(uint64_t));
	return bypassCount;
}

size_t PrimesFIO::LoadFromBINJava(uint64_t* arr, size_t len, fstream& f)
{
	uint32_t cnt = 0;
	while (cnt < len)
	{
		arr[cnt] = readJavaLong(f);
		if (f.eof()) break;
		cnt++;
	}

	return cnt;
}

size_t PrimesFIO::LoadFromBINDiff(uint64_t* arr, size_t len, uint64_t& lastPrime, fstream& f)
{
	uint32_t cnt = 0;
	while (cnt < len)
	{
		if (lastPrime == 0)
		{
			f.read((char*)(arr + cnt), sizeof(uint64_t));
			if (f.eof()) break;
		}
		else
		{
			uint16_t diff = 0;
			f.read((char*)&diff, sizeof(uint16_t));
			if (f.eof()) break;

			// diff between 2 and 3 is 1. we do not divide it by 2 during saving, all the other diffs are divided by 2.
			if(lastPrime < 3)
				arr[cnt] = lastPrime + (uint64_t)diff;
			else
				arr[cnt] = lastPrime + (uint64_t)diff * 2;
		}

		lastPrime = arr[cnt++];
	}

	return cnt;
}

size_t PrimesFIO::BypassBINDiff(size_t bypassCount, uint64_t& lastPrime, fstream& f)
{
	if (bypassCount == 0) return 0;

	uint32_t cnt = 0;
	uint16_t diff = 0;
	while (true)
	{
		if (lastPrime == 0)
		{
			f.read((char*)&lastPrime, sizeof(uint64_t));
			if (f.eof()) break;
		}
		else
		{
			f.read((char*)&diff, sizeof(uint16_t));
			if (f.eof()) break;

			// diff between 2 and 3 is 1. we do not divide it by 2 during saving, all the other diffs are divided by 2.
			if (lastPrime < 3)
				lastPrime += (uint64_t)diff;
			else 
				lastPrime += (uint64_t)diff*2;
		}

		cnt++;
	}

	return cnt;
}

void PrimesFIO::SaveAsTXT(uint64_t* arr, size_t len, fstream& f)
{
	uint64_t chunk = 10'000'000;

	string s, ss;
	s.reserve(chunk);

	for (uint64_t i = 0; i < len; ++i)
	{
		ss = to_string(arr[i]);
		s.append(ss);
		s.append(",");

		if (s.size() > chunk - 20ull) // when we are close to capacity but еще НЕ перепрыгнули ее
		{
			f.write(s.c_str(), s.length());
			// f.flush();
			s.clear(); // make string empty leaving capacity as is
		}
	}

	f.write(s.c_str(), s.length());
	f.flush();

	//printf("Primes %llu were saved to file '%s'.\n", cntPrimes, outputFilename.c_str());
}


void PrimesFIO::SaveAsTXTDiff(uint64_t* arr, size_t len, uint64_t& lastPrime, uint64_t& maxDiff, fstream& f)
{
	uint64_t chunk = 10'000'000;

	string s;
	s.reserve(chunk);

	for (uint64_t i = 0; i < len; ++i)
	{
		uint64_t diff = arr[i] - lastPrime;
		if (lastPrime > 2) diff /= 2;

		if ((maxDiff < diff) && (lastPrime != 0)) maxDiff = diff;

		s.append(to_string(diff));
		s.append(",");
		lastPrime = arr[i];

		if (s.size() > chunk - 20ull) // when we are close to capacity but еще НЕ перепрыгнули ее
		{
			f.write(s.c_str(), s.length());
			// f.flush();
			s.clear(); // make string empty leaving capacity as is
		}
	}

	f.write(s.c_str(), s.length());
	f.flush();
}

void PrimesFIO::SaveAsBIN(uint64_t* arr, size_t len, fstream& f)
{
	f.write((char*)arr, sizeof(uint64_t)*len);

	/*for (uint64_t i = 0; i < len; i++)
	{
		f.write((char*)(arr + i), sizeof(uint64_t));
	}*/

	f.flush();
}

void PrimesFIO::SaveAsBINDiff(uint64_t* arr, size_t len, uint64_t& lastPrime, uint64_t& maxDiff, fstream& f)
{
	maxDiff = 0;

	for (uint64_t i = 0; i < len; i++)
	{
		if (lastPrime == 0)
		{
			f.write((char*)(arr + i), sizeof(uint64_t));
		}
		else
		{
			uint16_t diff = (uint16_t)(arr[i] - lastPrime); // diff should fit into 2 bytes
			if (lastPrime > 2) diff /= 2;

			if (maxDiff < diff) maxDiff = diff;

			f.write((char*)&diff, sizeof(uint16_t));
		}

		lastPrime = arr[i];
	}

	f.flush();
}

void PrimesFIO::SaveAsBINDiffVar(uint64_t* arr, size_t len, uint64_t& lastPrime, uint64_t& maxDiff, fstream& f)
{
	uint8_t buf[9];
	uint64_t diff;
	maxDiff = 0;

	for (uint64_t i = 0; i < len; i++)
	{
		diff = arr[i] - lastPrime;

		if (lastPrime > 2) // special case when diff is odd when lastPrime==2 (1=3-2)
		{
			diff /= 2; // divide by 2 everything except first number which is prime (not diff) and diff between 2 and 3
			if (maxDiff < diff) maxDiff = diff;
		}

		size_t difflen = var_len_encode(buf, diff);
		assert(difflen > 0);
		//assert(difflen <= 2);

		f.write((char*)buf, difflen);
		lastPrime = arr[i];
	}

	f.flush();
}

size_t PrimesFIO::LoadFromBINDiffVar(uint64_t* arr, size_t len, uint64_t& lastPrime, IBuferedFileStream& bf)
{
	uint64_t diff;
	uint8_t buf[9]{0};
	size_t maxSize;

	uint32_t cnt = 0;
	while (cnt < len)
	{
		maxSize = 0;

		while (true)
		{
			//f.read((char*)(buf + maxSize), 1);
			bf.ReadByte(*(buf + maxSize));
			if (bf.Eof()) break;
			if ((buf[maxSize++] & 0x80) == 0) break;
		}

		if (bf.Eof()) break;

		size_t res = var_len_decode(buf, maxSize, &diff);
		
		if (lastPrime > 0)
		{
			assert(maxSize == 1 || maxSize == 2);
			assert(res == 1 || res == 2);
			assert(diff < 1000);
		}

		if (lastPrime > 2) diff *= 2; // multiply by 2 everying except first number (which is not diff) and when lastPrime==2

		arr[cnt] = diff + lastPrime;
		lastPrime = arr[cnt++];
	}

	return cnt;
}

size_t PrimesFIO::BypassBINDiffVar(uint64_t bypassCount, uint64_t& lastPrime, IBuferedFileStream& bf)
{
	if (bypassCount == 0) return 0;

	uint64_t diff;
	uint8_t buf[9];
	size_t maxSize;
	
	uint64_t cnt = 0;
	while (true)
	{
		maxSize = 0;
		
		while (true)
		{
			//f.read((char*)(buf + maxSize), 1);
			bf.ReadByte(*(buf + maxSize));
			if (bf.Eof()) break;
			if ((buf[maxSize++] & 0x80) == 0) break;
		}

		if (bf.Eof()) break;

		size_t res = var_len_decode(buf, maxSize, &diff);
		if (lastPrime > 0) 
		{
			assert(maxSize == 1 || maxSize == 2);
			assert(res == 1 || res == 2);
			assert(diff < 1000);
		}

		if (lastPrime > 2) diff *= 2; // multiply by 2 everying except first number (which is not diff) and when lastPrime==2

		lastPrime = diff + lastPrime;
		cnt++;

		if (--bypassCount == 0) break;
	}

	return cnt;
}

// ***!!!! OLD version without multiplier by 2 !!!! *****
size_t PrimesFIO::BypassBINDiffVarOLD(uint64_t bypassCount, uint64_t& lastPrime, fstream& f)
{
	if (bypassCount == 0) return 0;

	uint64_t diff;
	uint8_t buf[9];
	size_t maxSize;

	uint64_t cnt = 0;
	while (true)
	{
		maxSize = 0;

		while (true)
		{
			f.read((char*)(buf + maxSize), 1);
			if (f.eof()) break;
			if ((buf[maxSize++] & 0x80) == 0) break;
		}

		if (f.eof()) break;
		size_t res = var_len_decode(buf, maxSize, &diff);

		assert(res > 0);

		// old format, without multiplication by 2 

		lastPrime = diff + lastPrime;
		cnt++;

		if (--bypassCount == 0) break;
	}

	return cnt;
}

// ***!!!! OLD version without multiplier by 2 !!!! *****
size_t PrimesFIO::LoadFromBINDiffVarOLD(uint64_t* arr, size_t len, uint64_t& lastPrime, fstream& f)
{
	uint64_t diff;
	uint8_t buf[9];
	size_t maxSize;

	uint32_t cnt = 0;
	while (cnt < len)
	{
		maxSize = 0;

		while (true)
		{
			f.read((char*)(buf + maxSize), 1);
			if (f.eof()) break;
			if ((buf[maxSize++] & 0x80) == 0) break;
		}

		if (f.eof()) break;

		[[maybe_unused]] size_t res = var_len_decode(buf, maxSize, &diff);
		
		assert(res > 0);

		arr[cnt] = diff + lastPrime;
		lastPrime = arr[cnt++];
	}

	return cnt;
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

uint64_t PrimesFIO::readJavaLong(fstream& f)
{
	uint64_t res = 0;
	unsigned char b = 0;
	for (uint32_t i = 0; i < 8; i++)
	{
		f.read((char*)&b, sizeof(unsigned char));
		res <<= 8;
		res |= b;
	}
	return res;
}


static char mytoupper(int c) // to eliminate compile warning "warning C4244: '=': conversion from 'int' to 'char', possible loss of data"
{
	return (char)toupper(c);
}

PRIMES_FILE_FORMAT PrimesFIO::GetFileType(std::string& fileName)
{
	std::string fn1 = fileName;

	// make it uppercase
	std::transform(fn1.begin(), fn1.end(), fn1.begin(), ::mytoupper);

	size_t index1 = fn1.find_last_of('.'); // search for rightmost extension

	if (index1 == std::string::npos) // file has no extension at all
		return PRIMES_FILE_FORMAT::none;

	string ext1 = fn1.substr(index1 + 1);

	string fn2 = fn1.substr(0, index1); // preparing for searching second rightmost extension (if any)
	size_t index2 = fn2.find_last_of('.');
	string ext2 = "";
	if (index2 != string::npos)
		ext2 = fn2.substr(index2 + 1);

	if (ext1 == TXT)
	{
		if (ext2 == DIFF)
			return PRIMES_FILE_FORMAT::txtdiff;
		else
			return PRIMES_FILE_FORMAT::txt;
	}

	if (ext1 == BIN)
	{
		if (ext2 == DIFF)
			return PRIMES_FILE_FORMAT::bindiff;
		else if (ext2 == DIFFVAR)
			return PRIMES_FILE_FORMAT::bindiffvar;
		else
			return PRIMES_FILE_FORMAT::bin;
	}

	return PRIMES_FILE_FORMAT::none; // extension is not recognized
}
