
#include "debug.h"
#include <locale>
#include <string>
#include <vector>
#include <numeric>
#include <unordered_set>
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
#define OPT_G _T("g")
#define OPT_S _T("s")
#define OPT_H _T("h")

static void DefineOptions(COptionsList& options)
{
	COption cc;
	cc.ShortName(OPT_C).LongName(_T("cache")).Descr(_T("Use cache during calculations")).Required(false).NumArgs(0);
	options.AddOption(cc);

	COption rr;
	rr.ShortName(OPT_R).LongName(_T("range")).Descr(_T("Define calculation range by specifying start and end values of the range")).Required(false).RequiredArgs(2);
	options.AddOption(rr);

	COption ss;
	ss.ShortName(OPT_S).LongName("size").Descr(_T("Define the calculation range by specifying start and length of the range")).RequiredArgs(2).Required(false);
	options.AddOption(ss);

	COption nn;
	nn.ShortName(OPT_N).LongName(_T("number")).Descr(_T("Caclcualte one number and show full the chain of 3n1 numbers till '1'")).Required(false).RequiredArgs(1);
	options.AddOption(nn);

	COption tt;
	tt.ShortName(OPT_T).LongName(_T("threads")).Descr(_T("Calculate with specified number of threads")).Required(false).NumArgs(1).RequiredArgs(1);
	options.AddOption(tt);

	COption uu;
	uu.ShortName(OPT_U).LongName(_T("unused")).Descr(_T("Track unused numbers during calculations. Define range of unusued numbers. Range always starts from 0.")).Required(false).NumArgs(1).RequiredArgs(1);
	options.AddOption(uu);

	COption ff;
	ff.ShortName(OPT_F).LongName(_T("cachefile")).Descr(_T("Specifies file name where cache will be saved. Valid only with options -c and -g. Otherwise ignored.")).Required(false).RequiredArgs(1);
	options.AddOption(ff); 

	options.AddOption(OPT_L, "long", _T("Force use long arithmetic. Long arithmetic will be used even for small numbers."), 0, false);
	options.AddOption(OPT_G, "gen", _T("Valid only when option -c (cache) is specified. Causes cache to be generated and saved into file. Cache range is specified with -r option here."), 0, false);
	
	options.AddOption(OPT_H, _T("help"), _T("Show help"), 0);
	
	options.MutuallyExclusive(OPT_R, OPT_N, OPT_S); // cannot have any both (or all three) options -r, -f and -n together in one cmd line
}

template<typename ThreeN1IntImpl>
void ProcessOptionN(typename ThreeN1IntImpl::DataType number);

template<typename ThreeN1IntImpl>
void ProcessOptionR(typename ThreeN1IntImpl::DataType start, typename ThreeN1IntImpl::DataType finish);

//TODO shall we show error message if exception thrown in try..catch?
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
		// one option -r, -n or -s must be in cmd line
		if (!cmd.HasOption(OPT_N) && !cmd.HasOption(OPT_R) && !cmd.HasOption(OPT_S))
		{
			std::cout << "Error: One of these three options has to be in command line: -r, -s, -n." << std::endl;
			return 1;
		}

		if (cmd.HasOption(OPT_U)) // track unused ONLY when -u option is specified in cmd
		{
			Parameters::UNUSED_SIZE = ParseNumber<uint64_t>(cmd.GetOptionValue(OPT_U)); // index=0 by default
			//auto unusedRange = std::min(Parameters::UNUSED_SIZE, toUInt64(finish));
			//calc.TrackUnused(unusedRange);
			//std::cout << std::format(ThreeN1::Locale, "{:<{}}: 1..{:L}", "Track unused (range)", F_WIDTH, unusedRange) << std::endl;
		}

		if (cmd.HasOption(OPT_T))
		{
			SetParam(Parameters::THREADS = std::stoul(cmd.GetOptionValue(OPT_T))); // index=0 by default
		}
		

		Parameters::CACHE_FILENAME     = cmd.GetOptionValue(OPT_F, 0, CACHE_FILE_VARLEN);
		Parameters::HAS_CACHE_FILENAME = cmd.HasOption(OPT_F);
		Parameters::USE_LONG_ARITHM    = cmd.HasOption(OPT_L);
		Parameters::GENERATE_CACHE     = cmd.HasOption(OPT_G);
		Parameters::USE_CACHE          = cmd.HasOption(OPT_C);
		
		if (cmd.HasOption(OPT_N))
		{
			std::cout << "Calculating chain for single number using Collatz rules." << std::endl << std::endl;
			//by default parse number as BigInt
			BigInt number = ParseNumber<BigInt>(cmd.GetOptionValue(OPT_N, 0));
			
			if (number < BIGINT_THRESHOLD && !cmd.HasOption(OPT_L))
				ProcessOptionN<ThreeN1Int64>(toUInt64(number));
			else
				ProcessOptionN<ThreeN1BigInt>(number);
		}
		else if (cmd.HasOption(OPT_R))
		{
			//by default parse range as BigInt
			BigInt start = ParseNumber<BigInt>(cmd.GetOptionValue(OPT_R, 0)); 
			BigInt finish = ParseNumber<BigInt>(cmd.GetOptionValue(OPT_R, 1));

			if (finish < BIGINT_THRESHOLD && !cmd.HasOption(OPT_L))
				ProcessOptionR<ThreeN1Int64>(toUInt64(start), toUInt64(finish));
			else
				ProcessOptionR<ThreeN1BigInt>(start, finish);
		}
		else if (cmd.HasOption(OPT_S))
		{
			//by default parse range as BigInt
			BigInt start = ParseNumber<BigInt>(cmd.GetOptionValue(OPT_S, 0));
			BigInt length = ParseNumber<BigInt>(cmd.GetOptionValue(OPT_S, 1));
			BigInt finish = start + length;
			
			if (finish < BIGINT_THRESHOLD && !cmd.HasOption(OPT_L))
				ProcessOptionR<ThreeN1Int64>(toUInt64(start), toUInt64(finish));
			else
				//ProcessOptionR<ThreeN1BigInt>(start, finish);
				ProcessOptionR<ThreeN1TTMath>((std::string)start, (std::string)finish);
		}
		else
		{
			throw std::exception("There is no required option specified.");
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
void ProcessOptionR(typename ThreeN1IntImpl::DataType start, typename ThreeN1IntImpl::DataType finish)
{
	using IntImpl = typename ThreeN1IntImpl::DataType;
	using CacheItemType = typename ThreeN1IntImpl::CacheItemType;
	static_assert(std::is_same<IntImpl, uint64_t>::value || std::is_same<IntImpl, BigInt>::value || std::is_same<IntImpl, TTMathBigInt>::value);

	ThreeN1IntImpl calc;

	if (start > finish)
	{
		IntImpl tmp = start;
		start = finish;
		finish = tmp;
	}

	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {} - {}", "Calculation Range", F_WIDTH, ReduceNumber(start), ReduceNumber(finish)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {}", "Count of numbers", F_WIDTH, ReduceNumber(finish - start)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Threads", F_WIDTH, Parameters::THREADS) << std::endl;
	if constexpr (std::is_same_v<IntImpl, BigInt>)
		std::cout << std::format("{:<{}}: {} (much slower then 64bit arithmetic, use it only when numbers exceed 64bit max value)", "Use long arithmetic", F_WIDTH, "YES") << std::endl;
	if constexpr (std::is_same_v<IntImpl, uint64_t>)
		std::cout << std::format("{:<{}}: {} (the fastest one, use it only for numbers less than ~18*10^18)", "Use 64bit arithmetic", F_WIDTH, "YES") << std::endl;
	std::cout << std::format("{:<{}}: {}", "Track unused", F_WIDTH, Parameters::UNUSED_SIZE == UNUSED_DEF_SIZE?"NO":"YES") << std::endl;
	std::cout << std::format("{:<{}}: {}", "Use Cache", F_WIDTH, Parameters::USE_CACHE?"YES":"NO") << std::endl;
	if (Parameters::USE_CACHE)
	{
		std::cout << std::format("{:<{}}: {}", "Generate Cache File", F_WIDTH, Parameters::GENERATE_CACHE?"YES (you can define your own cache file name using option -f)":"NO") << std::endl;
		std::cout << std::format("{:<{}}:'{}'", "Cache File", F_WIDTH, ThreeN1::GetCacheFileName(start, finish, VARLEN_EXT)) << std::endl;
	}

	// exclude 0 and 1 from calc
	if (start < 2) start = 2;
	assert(start <= finish);

	if (Parameters::THREADS > 1)
	{
		if (Parameters::USE_CACHE)
		{
			Ticks::Start("loading cache file");
			std::cout << "Loading cache data...";

			calc.LoadCacheFromFileVarLen(Parameters::CACHE_FILENAME);

			std::cout << "\r";

			std::cout << std::format("{:<{}}: {} numbers", "Loaded Cache Size", F_WIDTH, ReduceNumber((uint64_t)calc.m_valuesCache.Count())) << std::endl;
			std::cout << std::format("{:<{}}: {}", "Loading Cache Time", F_WIDTH, MillisecToStr(Ticks::Finish("loading cache file"))) << std::endl;

			calc.Calc3p1allThreads(start, finish, Parameters::THREADS);
		}
		else
		{
			calc.Calc3p1allThreads(start, finish, Parameters::THREADS);
		}
	}
	else //threads=1
	{
		if (Parameters::USE_CACHE)
		{		
			if (Parameters::GENERATE_CACHE)
			{
				calc.Calc3p1RangeCacheUpdate(start, finish);
				
				std::cout << "Verifying generated cache...";
				uint64_t emptyCount = 0;

				//std::unordered_set<uint64_t> unique;

				for (uint32_t v = 0; v < calc.m_valuesCache.Count(); v++)
				{
					auto& val = calc.m_valuesCache[v];
					//unique.insert(val.maxvalue);
					if (val.steps == 0) emptyCount++;
				}

				assert(emptyCount == 0);
				auto filled = calc.m_valuesCache.Count() - emptyCount;

				std::cout << "\r" << std::format(ThreeN1::Locale, "{:<{}}: {}% ({:L})", "Cache fullness", F_WIDTH, filled * 100 / calc.m_valuesCache.Count(), filled) << std::endl;

				//auto count = std::count_if(calc.m_valuesCache.begin(), calc.m_valuesCache.end(), [&calc](CacheItemType& v) { return v.maxvalue < calc.m_cacheFinish; });
				//std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Unique nums", F_WIDTH, unique.size()) << std::endl;


				Ticks::Start("save file");

				std::cout << "Saving cache file...";
				//calc.SaveCacheToFileBin(ThreeN1::GetCacheFileName(start, finish, BIN_EXT));
				calc.SaveCacheToFileVarLen(ThreeN1::GetCacheFileName(start, finish, VARLEN_EXT));

				std::cout << "\r" << std::format("{:<{}}: {}", "Time spent for file saving", F_WIDTH, MillisecToStr(Ticks::Finish("save file"))) << std::endl;
			}
			else
			{
				Ticks::Start("loading cache file");
				std::cout << "Loading cache data...";

				calc.LoadCacheFromFileVarLen(Parameters::CACHE_FILENAME);

				std::cout << "\r";

				std::cout << std::format("{:<{}}: {} numbers", "Loaded Cache Size", F_WIDTH, ReduceNumber((uint64_t)calc.m_valuesCache.Count())) << std::endl;
				std::cout << std::format("{:<{}}: {}", "Loading Cache Time", F_WIDTH, MillisecToStr(Ticks::Finish("loading cache file"))) << std::endl;

				calc.Calc3p1RangeCache(start, finish);
			}
		}
		else // -r without cache option
		{
			calc.Calc3p1Range(start, finish);
		}
	}
}