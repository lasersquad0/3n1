#pragma once

#include <fstream>
#include "BuferedFileStream.h"

size_t var_len_encode(uint8_t buf[9], uint64_t num);
size_t var_len_decode(const uint8_t buf[], size_t size_max, uint64_t* num);

// Functions for working with files of various formats containing prime numbers: txt, txtdiff, bin, bindiff, bindiffvar
// Function for reading, writing and converting files between there formats.
namespace PrimesFIO
{

enum class PRIMES_FILE_FORMAT
{
	none,
	txt,
	txtdiff,
	bin,
	bindiff,
	bindiffvar,
	nFormats
};

using PFF = PRIMES_FILE_FORMAT;

//#define FORMAT_TO_STR(arg) (arg==PRIMES_FILE_FORMAT::txt ? "TXT":arg==PRIMES_FILE_FORMAT::bin?"BIN":arg==PRIMES_FILE_FORMAT::txtdiff? \
//     "TXTDIFF":arg==PRIMES_FILE_FORMAT::bindiff?"BINDIFF":arg==PRIMES_FILE_FORMAT::bindiffvar?"BINDIFFVAR":"<unrecognized>")

//#define STR_TO_FORMAT(arg) (arg=="TXT"?PRIMES_FILE_FORMAT::txt:arg=="BIN"?PRIMES_FILE_FORMAT::bin:arg=="TXTDIFF"?PRIMES_FILE_FORMAT::txtdiff: \
//     arg=="BINDIFF"?PRIMES_FILE_FORMAT::bindiff:arg=="BINDIFFVAR"?PRIMES_FILE_FORMAT::bindiffvar:PRIMES_FILE_FORMAT::none)

constexpr int iff(PFF ff) { return static_cast<int>(ff); }
inline const char* FORMATS[iff(PFF::nFormats)]={"NONE", "TXT", "TXTDIFF", "BIN", "BINDIFF", "BINDIFFVAR" };
inline const char* PFFToStr(PFF ff) { return FORMATS[iff(ff)]; }
PFF StrToPFF(const std::string& ff);

// this stuct is returned by some PrimesFIO functions to allow showing progress during load operations
struct LoadPrimesResult
{
	size_t PrimesLoaded;
	size_t BytesRead;
};


	PRIMES_FILE_FORMAT GetFileType(const std::string& fileName);

	LoadPrimesResult LoadFromTXT(std::fstream& f, uint64_t* arr, size_t len);
	LoadPrimesResult LoadFromTXTDiff(std::fstream& f, uint64_t* arr, size_t len, uint64_t& lastPrime);
	LoadPrimesResult LoadFromBIN(std::fstream& f, uint64_t* arr, size_t len);
	LoadPrimesResult LoadFromBINDiff(std::fstream& f, uint64_t* arr, size_t len, uint64_t& lastPrime);
	LoadPrimesResult LoadFromBINDiffVar(IBuferedFileStream& bf, uint64_t* arr, size_t len, uint64_t& lastPrime);

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

	// positioning without actual reading data
	LoadPrimesResult BypassTXT(std::fstream& f, size_t bypassCount);
	LoadPrimesResult BypassTXTDiff(std::fstream& f, uint64_t bypassCount, uint64_t& lastPrime);
	LoadPrimesResult BypassBIN(std::fstream& f, size_t bypassCount);
	LoadPrimesResult BypassBINDiff(std::fstream& f, size_t bypassCount, uint64_t& lastPrime);
	LoadPrimesResult BypassBINDiffVar(IBuferedFileStream& bf, uint64_t bypassCount, uint64_t& lastPrime);

	size_t BypassTXT(size_t bypassCount, std::fstream& f);
	size_t BypassTXTDiff(uint64_t bypassCount, uint64_t& lastPrime, std::fstream& f);
	size_t BypassBIN(size_t bypassCount, std::fstream& f);
	size_t BypassBINDiff(size_t bypassCount, uint64_t& lastPrime, std::fstream& f);
	size_t BypassBINDiffVar(uint64_t bypassCount, uint64_t& lastPrime, IBuferedFileStream& bf);
	size_t BypassBINDiffVarOLD(uint64_t bypassCount, uint64_t& lastPrime, std::fstream& f);

	uint64_t readJavaLong(std::fstream& f);

};


