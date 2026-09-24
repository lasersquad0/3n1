#pragma once

#include <fstream>
#include "BuferedFileStream.h"


enum class PRIMES_FILE_FORMAT
{
	none,
	txt,
	txtdiff,
	bin,
	bindiff,
	bindiffvar
};

#define FORMAT_TO_STR(arg) (arg==PRIMES_FILE_FORMAT::txt ? "TXT":arg==PRIMES_FILE_FORMAT::bin?"BIN":arg==PRIMES_FILE_FORMAT::txtdiff? \
     "TXTDIFF":arg==PRIMES_FILE_FORMAT::bindiff?"BINDIFF":arg==PRIMES_FILE_FORMAT::bindiffvar?"BINDIFFVAR":"<unrecognized>")

#define STR_TO_FORMAT(arg) (arg=="TXT"?PRIMES_FILE_FORMAT::txt:arg=="BIN"?PRIMES_FILE_FORMAT::bin:arg=="TXTDIFF"?PRIMES_FILE_FORMAT::txtdiff: \
     arg=="BINDIFF"?PRIMES_FILE_FORMAT::bindiff:arg=="BINDIFFVAR"?PRIMES_FILE_FORMAT::bindiffvar:PRIMES_FILE_FORMAT::none)

size_t var_len_encode(uint8_t buf[9], uint64_t num);
size_t var_len_decode(const uint8_t buf[], size_t size_max, uint64_t * num);

// Functions for working with files of various formats containing prime numbers: txt, txtdiff, bin, bindiff, bindiffvar
// Function for reading, writing and converting files between there formats.
class PrimesFIO
{
private:
	// allowed file format extensions (in uppercase for proper comparing)
	inline static const std::string TXT = "TXT" ; //TODO decide whether to use string or string_view
	inline const static std::string_view BIN = "BIN";
	inline static const std::string DIFF = "DIFF";
	inline static const std::string_view DIFFVAR = "DIFFVAR";

public:
	static PRIMES_FILE_FORMAT GetFileType(std::string& fileName);

	//TODO make all these methods static???
	size_t LoadFromTXT(uint64_t* arr, size_t len, std::fstream& f);
	size_t LoadFromTXTDiff(uint64_t* arr, size_t len, uint64_t& lastPrime, std::fstream& f);
	size_t LoadFromBIN(uint64_t* arr, size_t len, std::fstream& f);
	size_t LoadFromBINJava(uint64_t* arr, size_t len, std::fstream& f);
	size_t LoadFromBINDiff(uint64_t* arr, size_t len, uint64_t& lastPrime, std::fstream& f);
	size_t LoadFromBINDiffVar(uint64_t* arr, size_t len, uint64_t& lastPrime, IBuferedFileStream& bf);
	size_t LoadFromBINDiffVarOLD(uint64_t* arr, size_t len, uint64_t& lastPrime, std::fstream& f);

	void SaveAsTXT(uint64_t* arr, size_t len, std::fstream& f);
	void SaveAsTXTDiff(uint64_t* arr, size_t len, uint64_t& lastPrime, uint64_t& maxDiff, std::fstream& f);
	void SaveAsBIN(uint64_t* arr, size_t len, std::fstream& f);
	void SaveAsBINDiff(uint64_t* arr, size_t len, uint64_t& lastPrime, uint64_t& maxDiff, std::fstream& f);
	void SaveAsBINDiffVar(uint64_t* arr, size_t len, uint64_t& lastPrime, uint64_t& maxDiff, std::fstream& f);

	// absolute positioning
	size_t BypassTXT(size_t bypassCount, std::fstream& f);
	size_t BypassTXTDiff(uint64_t bypassCount, uint64_t& lastPrime, std::fstream& f);
	size_t BypassBIN(size_t bypassCount, std::fstream& f);
	size_t BypassBINDiff(size_t bypassCount, uint64_t& lastPrime, std::fstream& f);
	size_t BypassBINDiffVar(uint64_t bypassCount, uint64_t& lastPrime, IBuferedFileStream& bf);
	size_t BypassBINDiffVarOLD(uint64_t bypassCount, uint64_t& lastPrime, std::fstream& f);

	uint64_t readJavaLong(std::fstream& f);

};


