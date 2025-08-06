
#include <chrono>
#include <locale>
#include <string>
#include "DynamicArrays.h"
#include "BigInt.h"
#include "CommandLine.h"
#include "DefaultParser.h"
#include "HelpFormatter.h"
#include "ThreeN1.h"
#include "string_utils.h"


static void PrintUsage(COptionsList& options)
{
	std::cout << CHelpFormatter::Format(_T("ThreeN1"), &options);
}

#define OPT_C _T("c")
#define OPT_R _T("r")
#define OPT_T _T("t")
#define OPT_U _T("u")
#define OPT_H _T("h")

static void DefineOptions(COptionsList& options)
{
	COption cc;
	cc.ShortName(OPT_C).LongName(_T("cache")).Descr(_T("Use cache during calculations")).Required(false).NumArgs(0);
	options.AddOption(cc);

	COption rr;
	rr.ShortName(OPT_R).LongName(_T("range")).Descr(_T("Define range for calculations")).Required(true).NumArgs(2).RequiredArgs(2);
	options.AddOption(rr);

	COption tt;
	tt.ShortName(OPT_T).LongName(_T("threads")).Descr(_T("Calculate with specified number of threads")).Required(false).NumArgs(1).RequiredArgs(1);
	options.AddOption(tt);

	COption uu;
	uu.ShortName(OPT_U).LongName(_T("unused")).Descr(_T("Track unused numbers during calculations. Define range of unusued numbers. Range always starts from 0.")).Required(false).NumArgs(1).RequiredArgs(1);
	options.AddOption(uu);

	options.AddOption(OPT_H, _T("help"), _T("Show help"), 0);
}

int _tmain(int argc, TCHAR* argv[])
//int main(int argc,char* argv[])
{
	CDefaultParser defaultParser;
	CCommandLine cmd;
	COptionsList options;

	DefineOptions(options);

	if (!defaultParser.Parse(&options, &cmd, argv, argc))
	{
		std::cout << defaultParser.GetLastError() << std::endl;
		PrintUsage(options);
		return 1;
	}
	
	if (cmd.HasOption(OPT_H))
	{
		PrintUsage(options);
		return 0;
	}

	std::cout << "START - Collatz conjecture solver (3n+1)" << std::endl;

	std::cout.imbue(std::locale(std::cout.getloc(), new MyGroupSeparator()));

	auto start1 = std::chrono::high_resolution_clock::now();
	auto startFS = start1; // just to do not write long type definitions
	
	try
	{
		//ThreeN1<uint64_t> calc1;
		ThreeN1<BigInt> calc1;
		
		using IntImpl = decltype(calc1)::DataType;
		
		// default values
		const uint64_t THREADS_DEF = 4;
		const uint64_t UNUSED_DEF = 1'000'000'000;
		
		IntImpl start{}, finish{};

		if (cmd.HasOption(OPT_R)) // we must have option -r in cmd
		{
			std::string sstart = RemoveApo(cmd.GetOptionValue(OPT_R, 0, _T("some v") )); // "3'000'000'000"));
			std::string sfinish = RemoveApo(cmd.GetOptionValue(OPT_R, 1,_T("some v") )); // "4'000'000'000"));

			start = ParseNumber(sstart); // parses values like 1G, 100T, 200M, 150K together with 1000000, 100, 1234567890, etc.
			finish = ParseNumber(sfinish);

			if (start > finish)
			{
				IntImpl tmp = start;
				start = finish;
				finish = tmp;
			}
		}

		std::cout << "Range to calculate: " << start << " - " << finish << std::endl;

		uint64_t unused = UNUSED_DEF;
		if (cmd.HasOption(OPT_U)) // track unused ONLY when -u option is specified in cmd
		{
			try
			{
				unused = ParseNumber(cmd.GetOptionValue(OPT_U, 0, "defau"));
			}
			catch (...)
			{
				// nothing to do, unused remains unchanged in case of exception
			}

			auto unusedRange = std::min(unused, toULongLong(finish));
			calc1.TrackUnused(unusedRange);
			std::cout << "Track unused is ON. Range: 1.." << unusedRange << std::endl;
		}
		
		if (cmd.HasOption(OPT_T))
		{
			//Calculations in threads do NOT use CACHE at the moment
			uint64_t threads = THREADS_DEF;
			try
			{
				threads = std::stoull(cmd.GetOptionValue(OPT_T, 0, "def"));
			}
			catch (...)
			{
				// nothing to do, unused remains unchanged in case of exception
			}

			std::cout << "Calculation is done in threads (" << threads << ")" << std::endl;

			if (cmd.HasOption(OPT_C))
			{
				//NOTE!!! Calculations in threads do NOT use CACHE at the moment
				std::cout << "Using CACHE for claculations." << std::endl;
				calc1.Calc3p1allThreads(start, finish, threads);
			}
			else
			{
				std::cout << "Calculating WITHOUT cache." << std::endl;
				calc1.Calc3p1allThreads(start, finish, threads);
			}
		}
		else
		{
			if (cmd.HasOption(OPT_C))
			{
				startFS = std::chrono::high_resolution_clock::now();
				std::cout << "Loading cache data..." << std::endl;
				
				if constexpr (std::is_same<decltype(calc1)::DataType, BigInt>::value) // for BigInt only
					calc1.CacheFromFileBin("3-1G.bin");
				else
					calc1.CacheFromFileVarLen2("3-1G.binvar", toULongLong(finish));

				auto stop = std::chrono::high_resolution_clock::now();
				std::cout << "Loaded cache count:" << calc1.m_valuesCache.Count() << std::endl;
				std::cout << "Loading cache time:" << MillisecToStr(std::chrono::duration_cast<std::chrono::milliseconds>(stop - startFS).count()) << std::endl;
				
				std::cout << "Using CACHE for claculations." << std::endl << std::endl;
				calc1.Calc3p1RangeCache(start, finish);
			}
			else // here goes option -r which is mandatory
			{
				std::cout << "Calculating WITHOUT cache." << std::endl << std::endl;
				calc1.Calc3p1Range(start, finish);
			}
		}

		startFS = std::chrono::high_resolution_clock::now();

		//calc1.valuesCacheToFileBin(start, "3-1G.bin");
		//calc1.valuesCacheToFileVarLen(start, "3-1G.binvar");
		
		//calc1.rangeDataToFile("31b-32b.txt");

		// ThreeN1<BigInt> calc2;
		// BigInt start2("12980000000"), finish2("12990000000");
		// calc2.calc3p1allThreads(start2, finish2, 10);
		// startFS = chrono::high_resolution_clock::now();
		// calc2.rangeDataToFile("1298b-1399b BigInt.txt");

	}
	catch (THArrayException& ex)
	{
		std::cout << ex.getErrorMessage() << std::endl;
	}
	catch (TBigIntException& ex)
	{
		std::cout << ex.getErrorMessage() << std::endl;
	}
	catch (std::exception& ex)
	{
		std::cout << ex.what() << std::endl;
	}
	catch (...)
	{
		std::cout << "UNKNOWN EXCEPTION" << std::endl;
	}


	auto stop = std::chrono::high_resolution_clock::now();
	std::cout << "Time spent:" << MillisecToStr(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start1).count()) << std::endl;
	std::cout << "Time spent for file saving:" << MillisecToStr(std::chrono::duration_cast<std::chrono::milliseconds>(stop - startFS).count()) << std::endl;

	std::cout << "FINISH" << std::endl;
}

