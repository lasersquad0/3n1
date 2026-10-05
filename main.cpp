
#include "debug.h"
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
#include "utils/include/Ticks.h"
#include "Utils.h"
#include "ttmath/ttmath.h"

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
#define OPT_F _T("f")
#define OPT_N _T("n")
#define OPT_C _T("c")
#define OPT_T _T("t")
#define OPT_U _T("u")
#define OPT_L _T("l")
#define OPT_H _T("h")

static void DefineOptions(COptionsList& options)
{
	COption cc;
	cc.ShortName(OPT_C).LongName(_T("cache")).Descr(_T("Use cache during calculations")).Required(false).NumArgs(0);
	options.AddOption(cc);

	COption rr;
	rr.ShortName(OPT_R).LongName(_T("range")).Descr(_T("Define calculation range by specifying start and end values")).Required(false).RequiredArgs(2);
	options.AddOption(rr);

	COption ff;
	ff.ShortName(OPT_F).LongName("from").Descr(_T("Define the calculation range by specifying start and length of the range")).RequiredArgs(2).Required(false);
	options.AddOption(ff);

	COption nn;
	nn.ShortName(OPT_N).LongName(_T("number")).Descr(_T("Caclcualte one number and show full the chain of 3n1 numbers till '1'")).Required(false).RequiredArgs(1);
	options.AddOption(nn);

	COption tt;
	tt.ShortName(OPT_T).LongName(_T("threads")).Descr(_T("Calculate with specified number of threads")).Required(false).NumArgs(1).RequiredArgs(1);
	options.AddOption(tt);

	COption uu;
	uu.ShortName(OPT_U).LongName(_T("unused")).Descr(_T("Track unused numbers during calculations. Define range of unusued numbers. Range always starts from 0.")).Required(false).NumArgs(1).RequiredArgs(1);
	options.AddOption(uu);

	options.AddOption(OPT_L, "long", _T("Force use long arithmetic. Long arithmetic will be used even for small numbers."), 0, false);
	options.AddOption(OPT_H, _T("help"), _T("Show help"), 0);
	
	options.MutuallyExclusive(OPT_R, OPT_N, OPT_F); // cannot have any both (or all three) options -r, -f and -n together in one cmd line
}

template<typename ThreeN1IntImpl>
void ProcessOptionN(typename ThreeN1IntImpl::DataType number);

template<typename ThreeN1IntImpl>
void ProcessOptionR(typename ThreeN1IntImpl::DataType start, typename ThreeN1IntImpl::DataType finish, bool useCache);

#define SetParam(_) try { _; } catch (...) { /* nothing to do, because Parameters::XXXXX remains unchanged in case of exception */ }

int _tmain(int argc, TCHAR* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtMemState s1, s2, s3;
	_CrtMemCheckpoint(&s1); // Take a snapshot at the start of main()

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
	
	std::cout.imbue(ThreeN1::Locale);

	Ticks::Start("totaltime");

	try
	{
		// one option either -r or -n must be in cmd line
		if (!cmd.HasOption(OPT_N) && !cmd.HasOption(OPT_R) && !cmd.HasOption(OPT_F))
		{
			std::cout << "Error: One of these three options has to be in command line: -r, -f, -n." << std::endl;
			return 1;
		}


		if (cmd.HasOption(OPT_U)) // track unused ONLY when -u option is specified in cmd
		{
			SetParam(Parameters::UNUSED_SIZE = ParseNumber<uint64_t>(cmd.GetOptionValue(OPT_U))); // index=0 by default
			//auto unusedRange = std::min(Parameters::UNUSED_SIZE, toUInt64(finish));
			//calc.TrackUnused(unusedRange);
			//std::cout << std::format(ThreeN1::Locale, "{:<{}}: 1..{:L}", "Track unused (range)", F_WIDTH, unusedRange) << std::endl;
		}
		else
		{
			std::cout << std::format(ThreeN1::Locale, "{:<{}}: OFF", "Track unused", F_WIDTH) << std::endl;
		}

		if (cmd.HasOption(OPT_T))
		{
			SetParam(Parameters::THREADS = std::stoul(cmd.GetOptionValue(OPT_T))); // index=0 by default
		}

		if (cmd.HasOption(OPT_N))
		{
			std::cout << "Calculating chain for single number using Collatz rules." << std::endl << std::endl;

			BigInt number = ParseNumber<BigInt>(cmd.GetOptionValue(OPT_N, 0)); // parses values like 1G, 100T, 200M, 150K together with 1'000'000, 100, 1234567890, etc.
			
			if (number < BIGINT_THRESHOLD && !cmd.HasOption(OPT_L))
			{
				ProcessOptionN<ThreeN1Int64>(toUInt64(number));
			}
			else
			{
				ProcessOptionN<ThreeN1BigInt>(number);
			}
		}
		else if (cmd.HasOption(OPT_R))
		{
			//by default parse range as BigInt
			BigInt start = ParseNumber<BigInt>(cmd.GetOptionValue(OPT_R, 0)); // parses values like 1G, 100T, 200M, 150K together with 1'000'000, 100, 1234567890, etc.
			BigInt finish = ParseNumber<BigInt>(cmd.GetOptionValue(OPT_R, 1));

			if (finish < BIGINT_THRESHOLD && !cmd.HasOption(OPT_L))
			{
				ProcessOptionR<ThreeN1Int64>(toUInt64(start), toUInt64(finish), cmd.HasOption(OPT_C));
			}
			else
			{
				ProcessOptionR<ThreeN1BigInt>(start, finish, cmd.HasOption(OPT_C));
			}
		}
		else if (cmd.HasOption(OPT_F))
		{
			//by default parse range as BigInt
			BigInt start = ParseNumber<BigInt>(cmd.GetOptionValue(OPT_F, 0));
			BigInt length = ParseNumber<BigInt>(cmd.GetOptionValue(OPT_F, 1));
			BigInt finish = start + length;

			if (finish < BIGINT_THRESHOLD && !cmd.HasOption(OPT_L))
			{
				ProcessOptionR<ThreeN1Int64>(toUInt64(start), toUInt64(finish), cmd.HasOption(OPT_C));
			}
			else
			{
				ProcessOptionR<ThreeN1BigInt>(start, finish, cmd.HasOption(OPT_C));
			}
		}

		std::cout << std::format("{:<{}}: {}", "Total time spent", F_WIDTH, MillisecToStr(Ticks::Finish("totaltime"))) << std::endl;
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

	_CrtMemCheckpoint(&s2); // Take a snapshot at the end of main()
	_CrtMemCheckpoint(&s2); // Take a snapshot at the end of main()
	if (_CrtMemDifference(&s3, &s1, &s2)) _CrtMemDumpStatistics(&s3); // Dump memory statistics excluding global variables

}


template<typename ThreeN1IntImpl>
void ProcessOptionN(typename ThreeN1IntImpl::DataType number)
{
	using IntImpl = typename ThreeN1IntImpl::DataType;
	static_assert(std::is_same<uint64_t, IntImpl>::value || std::is_same<BigInt, IntImpl>::value);

	ThreeN1IntImpl calc;
	uint64_t steps;
	IntImpl maxNumber;
	std::vector<IntImpl> chain;
	calc.Calc3p1(number, chain, steps, maxNumber);

	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Starting number", F_WIDTH, number) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: ", "Chain of numbers", F_WIDTH);

	//TODO accumulate may be slow for large number of items in vector
	std::string s = std::accumulate(std::next(chain.begin()), chain.end(), std::format(ThreeN1::Locale, "{:L}", chain.front()),
		[](std::string acc, IntImpl x) {
			return std::move(acc) + "," + std::format(ThreeN1::Locale, "{:L}", x);
		});

	std::cout << s << std::endl;

	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Steps", F_WIDTH, steps) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Max number in chain", F_WIDTH, maxNumber) << std::endl;
}

template<typename ThreeN1IntImpl>
void ProcessOptionR(typename ThreeN1IntImpl::DataType start, typename ThreeN1IntImpl::DataType finish, bool useCache)
{
	using IntImpl = typename ThreeN1IntImpl::DataType;
	static_assert(std::is_same<IntImpl, uint64_t>::value || std::is_same<IntImpl, BigInt>::value);

	ThreeN1IntImpl calc;

	if (start > finish)
	{
		IntImpl tmp = start;
		start = finish;
		finish = tmp;
	}

	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} - {:L}", "Calculation Range", F_WIDTH, start, finish) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Count of numbers", F_WIDTH, finish - start) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Threads", F_WIDTH, Parameters::THREADS) << std::endl;
	if constexpr (std::is_same_v<IntImpl, BigInt>)
		std::cout << std::format(ThreeN1::Locale, "{:<{}}: {} (much slower then 64bit arithmetic, use it only when numbers exceed 64bit max value)", "Use long arithmetic", F_WIDTH, "YES") << std::endl;
	if constexpr (std::is_same_v<IntImpl, uint64_t>)
		std::cout << std::format(ThreeN1::Locale, "{:<{}}: {} (the fastest one, use it only for numbers less than ~18*10^18", "Use 64bit arithmetic", F_WIDTH, "YES") << std::endl;

	// exclude 0 and 1 from calc
	if (start < 2) start = 2;
	assert(start <= finish);

	if (Parameters::THREADS > 1)
	{
		if (useCache)
		{
			//NOTE!!! Calculations in threads do NOT use CACHE at the moment
			std::cout << std::format("{:<{}}: {}", "Use Cache", F_WIDTH, "YES") << std::endl;
			calc.Calc3p1allThreads(start, finish, Parameters::THREADS);
		}
		else
		{
			std::cout << std::format("{:<{}}: {}", "Use Cache", F_WIDTH, "NO") << std::endl;
			calc.Calc3p1allThreads(start, finish, Parameters::THREADS);
		}
	}
	else //threads=1
	{
		if (useCache)
		{
			std::cout << std::format("{:<{}}: {}", "Use Cache", F_WIDTH, "YES") << std::endl;

			Ticks::Start("loading cache file");
			std::cout << "Loading cache data...";

			calc.LoadCacheFromFileVarLen2("ThreeN1 cache - 300000000-600000000.diffvar");//(CACHE_FILE_BIN);

			std::cout << "\r";

			//auto stop = std::chrono::high_resolution_clock::now();
			std::cout << std::format("{:<{}}: {:L}", "Loaded cache count", F_WIDTH, calc.m_valuesCache.Count()) << std::endl;
			std::cout << std::format("{:<{}}: {}", "Loading cache time", F_WIDTH, MillisecToStr(Ticks::Finish("loading cache file"))) << std::endl;

			//calc.Calc3p1RangeCacheUpdate(start, finish);

			std::cout << "Verifying cache...";
			//std::string emptyNums;
			uint64_t emptyCount = 0;
			for (uint32_t v = 0; v < calc.m_valuesCache.Count(); v++)
			{
				if (calc.m_valuesCache[v].steps == 0) emptyCount++;
				//emptyNums.append(v).append(",");

			}
			std::cout << "\r" << std::format(ThreeN1::Locale, "{:<{}}: {:L} ({}%)", "Empty numbers in cache", F_WIDTH, emptyCount, emptyCount * 100 / calc.m_valuesCache.Count()) << std::endl;

			//save cache data only for uint64_t specialization
			if constexpr (std::is_same_v<IntImpl, uint64_t>)
			{
				//Ticks::Start("save file");

				//std::cout << "Saving cache file...";
				//calc.SaveCacheToFileBin();
				//calc.SaveCacheToFileVarLen();

				//std::cout << "\r" << std::format("{:<{}}: {}", "Time spent for file saving", F_WIDTH, MillisecToStr(Ticks::Finish("save file"))) << std::endl;
			}
		}
		else // here goes option -r which is mandatory
		{
			std::cout << std::format("{:<{}}: {}", "Use Cache", F_WIDTH, "NO") << std::endl << std::endl;
			calc.Calc3p1Range(start, finish);
		}
	}


}