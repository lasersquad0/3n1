
#include <chrono>
#include <locale>
#include <string>
#include "DynamicArrays.h"
//#include "BigInt.h"
#include "CommandLine.h"
#include "DefaultParser.h"
#include "HelpFormatter.h"
#include "ThreeN1.h"
#include "string_utils.h"


/*#define X1(x) X2(x)+X2(x)
#define X2(x) X3(x)+X3(x)
#define X3(x) X4(x)+X4(x)
#define X4(x) X5(x)+X5(x)
#define X5(x) X6(x)+X6(x)
#define X6(x) X7(x)+X7(x)
#define X7(x) X8(x)+X8(x)
#define X8(x) X9(x)+X9(x)
#define X9(x) X10(x)+X10(x)
#define X10(x) X11(x)+X11(x)
#define X11(x) X12(x)+X12(x)
#define X12(x) X13(x)+X13(x)
#define X13(x) X14(x)+X14(x)
#define X14(x) X15(x)+X15(x)
#define X15(x) X16(x)+X16(x)
#define X16(x) X17(x)+X17(x)
#define X17(x) X18(x)+X18(x)
#define X18(x) X19(x)+X19(x)
#define X19(x) X20(x)+X20(x)
#define X20(x) X21(x)+X21(x)
#define X21(x) X22(x)+X22(x)
#define X22(x) X23(x)+X23(x)
#define X23(x) X24(x)+X24(x)
#define X24(x) X25(x)+X25(x)
#define X25(x) X26(x)+X26(x)
#define X26(x) X27(x)+X27(x)
#define X27(x) X28(x)+X28(x)
#define X28(x) X29(x)+X29(x)
#define X29(x) X30(x)+X30(x)
#define X30(x) X31(x)+X31(x)
#define X31(x) X32(x)+X32(x)
#define X32(x) X33(x)+X33(x)
#define X33(x) X34(x)+X34(x)
#define X34(x) X35(x)+X35(x)
#define X35(x) X36(x)+X36(x)
#define X36(x) X37(x)+X37(x)
#define X37(x) X38(x)+X38(x)
#define X38(x) X39(x)+X39(x)
#define X39(x) X40(x)+X40(x)
#define X40(x) X41(x)+X41(x)
#define X41(x) X42(x)+X42(x)
#define X42(x) X43(x)+X43(x)
#define X43(x) X44(x)+X44(x)
#define X44(x) X45(x)+X45(x)
#define X45(x) X46(x)+X46(x)
#define X46(x) X47(x)+X47(x)
#define X47(x) X48(x)+X48(x)
#define X48(x) X49(x)+X49(x)
#define X49(x) X50(x)+X50(x)
#define X50(x) X51(x)+X51(x)
#define X51(x) X52(x)+X52(x)
#define X52(x) X53(x)+X53(x)
#define X53(x) X54(x)+X54(x)
#define X54(x) X55(x)+X55(x)
#define X55(x) X56(x)+X56(x)
#define X56(x) X57(x)+X57(x)
#define X57(x) X58(x)+X58(x)
#define X58(x) X59(x)+X59(x)
#define X59(x) X60(x)+X60(x)
#define X60(x) X61(x)+X61(x)
#define X61(x) X62(x)+X62(x)
#define X62(x) X63(x)+X63(x)
#define X63(x) X64(x)+X64(x)
#define X64(x) x+x*/

static void PrintUsage(COptionsList& options)
{
	std::cout << CHelpFormatter::Format("ThreeN1", &options);
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


int main(int argc,char* argv[])
{
	//return X44(0);

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
		// default values
		const uint64_t THREADS_DEF = 4;
		const uint64_t UNUSED_DEF = 1'000'000'000;
		uint64_t start{}, finish{};

		if (cmd.HasOption(OPT_R)) // we must have option -r in cmd
		{
			std::string sstart = RemoveApo(cmd.GetOptionValue(OPT_R, 0, "some v" )); // "3'000'000'000"));
			std::string sfinish = RemoveApo(cmd.GetOptionValue(OPT_R, 1, "some v")); // "4'000'000'000"));

			start = ParseNumber(sstart); // parses values like 1G, 100T, 200M, 150K together with 1000000, 100, 1234567890, etc.
			finish = ParseNumber(sfinish);

			if (start > finish)
			{
				uint64_t tmp = start;
				start = finish;
				finish = tmp;
			}
		}

		std::cout << "Range to calculate: " << start << " - " << finish << std::endl;

		ThreeN1<uint64_t> calc1;

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

			auto unusedRange = std::min(unused, finish);
			calc1.TrackUnused(unusedRange);
			std::cout << "Track unused is ON. Range: 1.." << unusedRange << std::endl;
		}

		//ThreeN1<BigInt> calc1; //TODO BigInt specialisation will not compile at the moment, need to work on std::format(BigInt) to make it compile
		
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
				calc1.CacheFromFileVarLen2("3-1G.binvar", finish);
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

