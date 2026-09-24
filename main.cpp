
#include <chrono>
#include <locale>
#include <string>
#include <vector>
#include <numeric>
#include "DynamicArrays.h"
#include "BigInt.h"
#include "cli/CommandLine.h"
#include "cli/DefaultParser.h"
#include "cli/HelpFormatter.h"
#include "ThreeN1.h"
#include "utils/include/string_utils.h"
#include "Utils.h"

#define DEFAULT_THREADS 1
#define UNUSED_DEF_SIZE 1'000'000'000 // size of array in bits for tracking unused numbers
#define APP_NAME "ThreeN1"
#define APP_EXE_NAME APP_NAME ".exe"
#define PERCENT_FACTOR 100
#define CACHE_FILE_BIN "3-1G.bin"
#define CACHE_FILE_BINVAR "3-1G.binvar"


class Parameters
{
public:
	static uint32_t THREADS;
	static uint64_t UNUSED_SIZE;
};

uint32_t Parameters::THREADS = DEFAULT_THREADS;
uint64_t Parameters::UNUSED_SIZE = UNUSED_DEF_SIZE;

static void PrintUsage(COptionsList& options)
{
	std::cout << CHelpFormatter::Format(_T(APP_NAME), &options);
}

#define OPT_R _T("r")
#define OPT_N _T("n")
#define OPT_C _T("c")
#define OPT_T _T("t")
#define OPT_U _T("u")
#define OPT_H _T("h")

static void DefineOptions(COptionsList& options)
{
	COption cc;
	cc.ShortName(OPT_C).LongName(_T("cache")).Descr(_T("Use cache during calculations")).Required(false).NumArgs(0);
	options.AddOption(cc);

	COption rr;
	rr.ShortName(OPT_R).LongName(_T("range")).Descr(_T("Define range for calculations")).Required(false).NumArgs(2).RequiredArgs(2);
	options.AddOption(rr);

	COption nn;
	nn.ShortName(OPT_N).LongName(_T("number")).Descr(_T("Caclcualte one number and show full the chain of 3n1 numbers till '1'")).Required(false).RequiredArgs(1);
	options.AddOption(nn);

	COption tt;
	tt.ShortName(OPT_T).LongName(_T("threads")).Descr(_T("Calculate with specified number of threads")).Required(false).NumArgs(1).RequiredArgs(1);
	options.AddOption(tt);

	COption uu;
	uu.ShortName(OPT_U).LongName(_T("unused")).Descr(_T("Track unused numbers during calculations. Define range of unusued numbers. Range always starts from 0.")).Required(false).NumArgs(1).RequiredArgs(1);
	options.AddOption(uu);

	options.AddOption(OPT_H, _T("help"), _T("Show help"), 0);
	
	options.MutuallyExclusive(OPT_R, OPT_N); // cannot have both options -r and -n together in one cmd line
}

int _tmain(int argc, TCHAR* argv[])
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

	std::cout << std::endl << "Collatz conjecture solver (3n+1)" << std::endl << std::endl;

	auto loc(std::locale(std::cout.getloc(), new MyGroupSeparator()));
	std::cout.imbue(loc);

	auto start1 = std::chrono::high_resolution_clock::now();
	auto startFS = start1; // just to do not write long type definitions
	
	try
	{
		ThreeN1Int64 calc1;
		//ThreeN1BigInt calc1;

		using IntImpl = decltype(calc1)::DataType;

		// one option either -r or -n must be in cmd line
		if (!cmd.HasOption(OPT_N) && !cmd.HasOption(OPT_R))
		{
			std::cout << "Error: One of these two options has to be in command line: -r, -n." << std::endl;
			return 1;
		}


		if (cmd.HasOption(OPT_N))
		{
			IntImpl number = ParseNumber(RemoveApo(cmd.GetOptionValue(OPT_N, 0)));
			uint64_t steps;
			IntImpl maxNumber;
			std::vector<IntImpl> chain;
			calc1.Calc3p1(number, chain, steps, maxNumber);

			std::cout << "Calculating chain for single number using Collatz rules." << std::endl << std::endl;

			std::cout << std::format(loc, "{:<{}}: {}", "Starting number", F_WIDTH, number) << std::endl;
			std::cout << std::format(loc, "{:<{}}: ", "Chain of numbers", F_WIDTH);

			//TODO accumulate may be slow for large number of items in vector
			std::string s = std::accumulate(std::next(chain.begin()), chain.end(), std::format(loc, "{}", chain.front()),
				[&loc](std::string acc, uint64_t x) {
					return std::move(acc) + "," + std::format(loc, "{:L}", x);
				});

			std::cout << s << std::endl;

			std::cout << std::format(loc, "{:<{}}: {:L}", "Steps", F_WIDTH, steps) << std::endl;
			std::cout << std::format(loc, "{:<{}}: {:L}", "Max number in chain", F_WIDTH, maxNumber) << std::endl;

		}
		else if (cmd.HasOption(OPT_R))
		{
			IntImpl start{}, finish{};

			//option -r should always present
			std::string sstart = RemoveApo(cmd.GetOptionValue(OPT_R, 0, _T("some v"))); // "3'000'000'000"));
			std::string sfinish = RemoveApo(cmd.GetOptionValue(OPT_R, 1, _T("some v"))); // "4'000'000'000"));

			start = ParseNumber(sstart); // parses values like 1G, 100T, 200M, 150K together with 1000000, 100, 1234567890, etc.
			finish = ParseNumber(sfinish);

			if (start > finish)
			{
				IntImpl tmp = start;
				start = finish;
				finish = tmp;
			}

			if constexpr (std::is_same<decltype(calc1)::DataType, BigInt>::value) // for BigInt only
				std::cout << std::format(loc, "{:<{}}: {} - {}", "Calculation Range", F_WIDTH, start, finish) << std::endl;
			else
				std::cout << std::format(loc, "{:<{}}: {} - {}", "Calculation Range", F_WIDTH, start, finish) << std::endl;

			// exclude 0 and 1 from calc
			if (start < 2) start = 2;

			assert(start <= finish);

			if (cmd.HasOption(OPT_U)) // track unused ONLY when -u option is specified in cmd
			{
				try
				{
					Parameters::UNUSED_SIZE = ParseNumber(cmd.GetOptionValue(OPT_U, 0, "defau"));
				}
				catch (...)
				{
					// nothing to do, unused remains unchanged in case of exception
				}

				auto unusedRange = std::min(Parameters::UNUSED_SIZE, toULongLong(finish));
				calc1.TrackUnused(unusedRange);

				std::cout << std::format(loc, "{:<{}}: 1..{:L}", "Track unused (range)", F_WIDTH, unusedRange) << std::endl;
			}
			else
			{
				std::cout << std::format(loc, "{:<{}}: OFF", "Track unused", F_WIDTH) << std::endl;
			}


			if (cmd.HasOption(OPT_T))
			{
				try
				{
					Parameters::THREADS = std::stoul(cmd.GetOptionValue(OPT_T, 0, "def"));
				}
				catch (...)
				{
					// nothing to do, because Parameters::THREADS remains unchanged in case of exception
				}
			}

			std::cout << std::format(loc, "{:<{}}: {:L}", "Calculation Threads", F_WIDTH, Parameters::THREADS) << std::endl;


			if (Parameters::THREADS > 1)
			{
				if (cmd.HasOption(OPT_C))
				{
					//NOTE!!! Calculations in threads do NOT use CACHE at the moment
					std::cout << std::format("{:<{}}: {}", "Use CACHE", F_WIDTH, "YES") << std::endl;
					calc1.Calc3p1allThreads(start, finish, Parameters::THREADS);
				}
				else
				{
					std::cout << std::format("{:<{}}: {}", "Use CACHE", F_WIDTH, "NO") << std::endl;
					calc1.Calc3p1allThreads(start, finish, Parameters::THREADS);
				}
			}
			else //threads=1
			{
				if (cmd.HasOption(OPT_C))
				{
					std::cout << std::format("{:<{}}: {}", "Use CACHE", F_WIDTH, "YES") << std::endl << std::endl;

					startFS = std::chrono::high_resolution_clock::now();

					std::cout << "Loading cache data...";

					/*if constexpr (std::is_same<decltype(calc1)::DataType, BigInt>::value) // for BigInt only
						calc1.CacheFromFileBin(CACHE_FILE_BIN);
					else
						calc1.CacheFromFileVarLen2(CACHE_FILE_BINVAR, toULongLong(finish));
					*/
					std::cout << "\r";


					auto stop = std::chrono::high_resolution_clock::now();
					std::cout << std::format("{:<{}}: {}", "Loaded cache count", F_WIDTH, calc1.m_valuesCache.Count()) << std::endl;
					std::cout << std::format("{:<{}}: {}", "Loading cache time", F_WIDTH, MillisecToStr(std::chrono::duration_cast<std::chrono::milliseconds>(stop - startFS).count())) << std::endl;

					calc1.Calc3p1RangeCache(start, finish);
				}
				else // here goes option -r which is mandatory
				{
					std::cout << std::format("{:<{}}: {}", "Use CACHE", F_WIDTH, "NO") << std::endl << std::endl;
					calc1.Calc3p1Range(start, finish);
				}
			}

			startFS = std::chrono::high_resolution_clock::now();

			//calc1.valuesCacheToFileBin(start, "3-1G.bin");
			//calc1.CacheToFileVarLen(start, "3-1G.binvar");

			//calc1.rangeDataToFile("31b-32b.txt");

			// ThreeN1<BigInt> calc2;
			// BigInt start2("12980000000"), finish2("12990000000");
			// calc2.calc3p1allThreads(start2, finish2, 10);
			// startFS = chrono::high_resolution_clock::now();
			// calc2.rangeDataToFile("1298b-1399b BigInt.txt");

			//auto stop = std::chrono::high_resolution_clock::now();
			//std::cout << std::format("{:<{}}: {}", "Time spent for file saving", F_WIDTH, MillisecToStr(std::chrono::duration_cast<std::chrono::milliseconds>(stop - startFS).count())) << std::endl;
		}

		auto stop = std::chrono::high_resolution_clock::now();
		std::cout << std::format("{:<{}}: {}", "Total time spent", F_WIDTH, MillisecToStr(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start1).count())) << std::endl;
	}
	catch (THArrayException& ex)
	{
		std::cout << "Error: " << ex.getErrorMessage() << std::endl;
		std::cout << "Aborting." << std::endl;
	}
	catch (TBigIntException& ex)
	{
		std::cout << "Error: " << ex.getErrorMessage() << std::endl;
		std::cout << "Aborting." << std::endl;
	}
	catch (std::exception& ex)
	{
		std::cout << "Error: " << ex.what() << std::endl;
		std::cout << "Aborting." << std::endl;
	}
	catch (...)
	{
		std::cout << "UNKNOWN EXCEPTION" << std::endl;
	}
}

