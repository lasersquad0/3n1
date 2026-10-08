
#include "ThreeN1.h"
#include "utils/include/Ticks.h"

// calculates ONE number 
// retuns list of all 3n1 numbers produces by this number.
void ThreeN1Int64::Calc3p1(const uint64_t& number, std::vector<uint64_t>& chain, uint64_t& steps, uint64_t& maxNum) const
{
	steps = 0;
	maxNum = number;
	uint64_t curr = number;

	auto overflow = OVERFLOW_LIMIT; //local var
	chain.clear();
	chain.reserve(IThreeN1::MAX_STEPS);
	chain.push_back(curr);

	while (curr != 1ull)
	{
		if ((curr & 1ull) == 0) // is even
		{
			curr >>= 1;
			chain.push_back(curr);
		}
		else
		{
			if (curr >= overflow)
				throw std::overflow_error("Overflow detected!");

			curr = (3ull * curr + 1ull);
			chain.push_back(curr);
			if (maxNum < curr) maxNum = curr;

			curr >>= 1;
			chain.push_back(curr);

			steps++; // if curr is odd we do 2 operations at once and increase steps twice accordingly
		}

		steps++;
	}
}

// calculates ONE number WITHOUT using cache
// returns number of steps and max number reached in the chain
void ThreeN1Int64::Calc3p1(const uint64_t& number, CalcDataType64& calcResult)
{
	calcResult.maxvalue = number;
	calcResult.steps = 0ull;
	uint64_t curr = number;
	auto& unused = m_unused;
	const auto bitsCount = unused.BitsCount();

	if (curr < bitsCount) unused.setTrue(curr);

	auto overflow = OVERFLOW_LIMIT;

	while (curr != 1ull)
	{
		if ((curr & 1ull) == 0) // is even
		{
			curr >>= 1;
		}
		else
		{
			if (curr >= overflow)
				throw std::overflow_error("Overflow detected!");

			curr = (3ull * curr + 1ull);
			if (calcResult.maxvalue < curr) calcResult.maxvalue = curr;

			int tz = std::countr_zero(curr);
			curr >>= tz;
			calcResult.steps += (uint16_t)tz; // if curr is odd we do 2 operations at once and increase steps twice accordingly
		}

		calcResult.steps++;

		if (curr < bitsCount) unused.setTrue(curr);
	}
}


// calculates ONE number WITH using cache
// DOES NOT add items to the cache. Assumes that cache is full and loaded from a file.
void ThreeN1Int64::Calc3p1Cache(const uint64_t& number, CalcDataType64& calcResult)
{
	uint64_t curr = number;
	uint64_t mxval = number;
	uint16_t steps = 0;

	auto& unused = m_unused;
	const auto bitsCount = unused.BitsCount();
	if (curr < bitsCount) unused.setTrue(curr);

	while (curr != 1)
	{
		if ((curr & 0x01) == 0)
		{
			curr >>= 1;
		}
		else
		{
			if (curr >= OVERFLOW_LIMIT)
				throw std::overflow_error("Overflow detected!");

			curr = 3 * curr + 1;
			if (mxval < curr) mxval = curr;
			if (curr < bitsCount) unused.setTrue(curr);
			curr >>= 1;
			steps++;
		}
		steps++;

		if (curr < bitsCount) unused.setTrue(curr);

		// check if curr is in cache range
		if (curr >= m_cacheStart && curr < m_cacheFinish)
		{
			auto& elem = m_valuesCache[(uint32_t)(curr - m_cacheStart)];
			assert(elem.steps > 0);

			m_hits++; //for each number we come here once
			if (mxval < elem.maxvalue) mxval = elem.maxvalue;

			steps += elem.steps;
			break;
		}
	}

	calcResult.steps = steps;
	calcResult.maxvalue = mxval;
}

// calculates ONE number WITH using cache
// all intermediate numbers are added to the cache if they are in cache range
void ThreeN1Int64::calc3p1CacheUpdate(const uint64_t& number, CalcDataType64& calcResult)
{
	uint64_t curr = number;
	auto& unused = m_unused;
	const auto bitsCount = unused.BitsCount();

	if (curr < bitsCount) unused.setTrue(curr);

	uint64_t chain[IThreeN1::MAX_STEPS];

	uint16_t index = 0;
	chain[index++] = number;

	while (curr != 1ull)
	{
		if ((curr & 0x01) == 0) // is even
		{
			curr >>= 1;
			chain[index++] = curr;
		}
		else
		{
			if (curr >= OVERFLOW_LIMIT)
				throw std::overflow_error("Overflow detected!");

			curr = 3ull * curr + 1ull;
			if (curr < bitsCount) unused.setTrue(curr);
			chain[index++] = curr;
			curr >>= 1;
			chain[index++] = curr;

		}

		if (curr < bitsCount) unused.setTrue(curr);

		// check if curr is in cache
		// if yes, add to cache all collected numbers chain and exit from the function
		if (curr >= m_cacheStart && curr < m_cacheFinish)
		{
			auto& elem = m_valuesCache[(uint32_t)(curr - m_cacheStart)];
			if (elem.steps > 0) // found in cache
			{
				m_hits++; //for each number we come here once
				uint64_t mxval = elem.maxvalue;
				assert(index - 1 > 0);
				for (int i = index - 2; i >= 0; i--)
				{
					auto num = chain[i];
					if (mxval < num) mxval = num;

					if (num >= m_cacheStart && num < m_cacheFinish)
					{
						CacheItemType res;
						res.steps = (uint16_t)(index - 1 - i + elem.steps);
						res.maxvalue = mxval;
						m_valuesCache.SetValue((uint32_t)(num - m_cacheStart), res);
					}
				}
				calcResult.steps = (index - 1 + elem.steps);
				calcResult.maxvalue = mxval;
				break;
			}
		}
	}

	if (curr == 1)
	{
		uint64_t mxval = 1;
		for (int i = index - 1; i >= 0; i--)
		{
			auto num = chain[i];
			if (mxval < num) mxval = num;

			if (num >= m_cacheStart && num < m_cacheFinish)
			{
				CacheItemType res;
				res.steps = (uint16_t)(index - 1 - i);
				res.maxvalue = mxval;
				m_valuesCache.SetValue((uint32_t)(num - m_cacheStart), res);
			}
		}
		calcResult.steps = index - 1;
		calcResult.maxvalue = mxval;
	}
};

// calculates RANGE of numbers WITHOUT using cache
// collects and print some statistic
void ThreeN1Int64::Calc3p1Range(const uint64_t& start, const uint64_t& finish)
{

#ifdef _DEBUG
	const uint64_t PRINT_VALUE = 1'000'000; // print "progress" on each 1Mth number
#else
	const uint64_t PRINT_VALUE = 4'000'000; // print "progress" on each 4Mth number
#endif

	Ticks::Start("calctime");
	auto speedStart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point speedStop;

	StatDataType stat{};
	CalcDataType calcData{ 0ull, 0ull };

	uint64_t printCounter = PRINT_VALUE;
	size_t fieldWidth = NumLen(finish);
	fieldWidth += (fieldWidth - 1) / 3; // count thousands separators in length
	uint64_t range = finish - start;
	uint64_t maxValWidth = MAX_VALUE_WIDTH;
	uint64_t maxProgressLen = 0;
	uint64_t speed, avgsteps, lineCnt = 0;
	std::string maxvalueStr, progressStr;
	progressStr.reserve(200);
	
	std::cout << std::endl;

	for (uint64_t i = start; i < finish; i++)
	{
		if (--printCounter == 0) // show progress
		{
			speedStop = std::chrono::high_resolution_clock::now();
			printCounter = PRINT_VALUE;
			speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(speedStop - speedStart).count();
			avgsteps = stat.sumsteps / (i - start);
			progressStr.clear();
			std::format_to(std::back_inserter(progressStr), ThreeN1::Locale, "{:L} | {}% | Speed: {:L} nums/sec | Average Steps: {:L}", 
				i + 1, (i - start) * 100 / range, speed, avgsteps);
			if (maxProgressLen < progressStr.size()) maxProgressLen = progressStr.size();
			//progressStr.append(maxProgressLen - progressStr.size(), ' ');
			std::cout << progressStr << "\r";
			speedStart = std::chrono::high_resolution_clock::now();
		}

		Calc3p1(i, calcData);

		stat.sumsteps += calcData.steps;

		if (stat.num2maxvalue < calcData.maxvalue)
		{
			stat.num2 = i;
			stat.num2maxvalue = calcData.maxvalue;
			maxvalueStr = std::format(ThreeN1::Locale, "{:L}", calcData.maxvalue);
			if (maxvalueStr.size() > maxValWidth) maxValWidth = (uint32_t)maxvalueStr.size();
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}L} | steps:     {:>5L} | MAX VALUE: {:>{}}", 
				std::format(ThreeN1::Locale, "[{:L}]", lineCnt++), i, fieldWidth, calcData.steps, maxvalueStr, maxValWidth) << std::endl;
		}

		if (stat.num1steps < calcData.steps)
		{
			stat.num1 = i;
			stat.num1steps = calcData.steps;
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}L} | MAX STEPS: {:>5L} | value:     {:>{}L}", 
				std::format(ThreeN1::Locale, "[{:L}]", lineCnt++), i, fieldWidth, calcData.steps, calcData.maxvalue, maxValWidth) << std::endl;
		}
	}

	std::cout << std::string(maxProgressLen, ' ') << "\r" << std::endl; // clear progress counter

	stat.calctime = Ticks::Finish("calctime");
	stat.numcount = range;

	printCalcResults(stat);

}

// calculates range of numbers WITH cache
// requires much memory for storing cache, but really faster. 
// cache stores pairs steps (uint16_t) and maxvalue (uint64_t)
// collects and print some statistic
// assumes that CACHE is EMPTY and fills it during its calculation process  
ThreeN1Int64::StatDataType ThreeN1Int64::Calc3p1RangeCacheUpdate(const uint64_t& start, const uint64_t& finish)
{
	/*
	* Looks like it is better to put cache in the bottom, let say from 4 to 1G or to 200M depending on available memory.
	* (for 1G range it will require 10G of memory for cache)
	* In this case hit rate % will be aroud 100%.
	* 
	* When cache equals start-finish then hit rate is very low for big numbers 
	* Hit rate statistics when cache is: start ... finish
	* range: 100M 200M,     hit rate: 75%, speed: 2 567 000 num/sec
	* range: 900M 1000M,    hit rate: 24%, speed: 944 00 num/sec
	* range: 5000M 5100M,   hit rate: 2.17%, speed: 886 000 num/sec
	* range: 10000M 10100M, hit rate: 0.25%, speed: 909 500 num/sec 
	* range: 100000M 100100M, hit rate: 0.0%, speed: 450 600 num/sec
	*
	* It is bad idea to has cache here: start-(finish-start) ... start.
	* In this case hit rate = 0%
	* 
	* Statistics when cache is here: start/2 ... start/2 + (finish-start)
	* range: 100M 200M,     hit rate: 92%, speed: 6 750 000 num/sec
	* range: 900M 1000M,    hit rate: 52%, speed: 1 800 000 num/sec
	* range: 5000M 5100M,   hit rate: 12%, speed: 642 000 num/sec
	* range: 10000M 10100M, hit rate: 5%, speed: 603 269 num/sec 	
	* 
	* Statistics when cache is here: 5 ... finish-start
	* range: 100M 200M,     hit rate: 100%, speed: 10 400 000 num/sec
	* range: 900M 1000M,    hit rate: 100%, speed: 5 136 000 num/sec
	* range: 5000M 5100M,   hit rate: 100%, speed: 2 607 000 num/sec
	* range: 10000M 10100M, hit rate: 100%, speed: 3 307 000 num/sec
	* range: 100000M 100100M, hit rate: 100%, speed: 2 160 000 num/sec
	*
	* Statistics when cache is here: finish-start ... 2*(finish-start)
	* range: 100M 200M,     hit rate: 75%, speed: 2 550 000 num/sec
	* range: 900M 1000M,    hit rate: 90%, speed: 4 400 000 num/sec
	* range: 5000M 5100M,   hit rate: 93%, speed: 2 788 000 num/sec
	* range: 10000M 10100M, hit rate: 94%, speed: 2 533 000 num/sec
	* range: 100000M 100100M, hit rate: 98%, speed: 2 107 000 num/sec 
	*/

#ifdef _DEBUG
	const uint64_t PRINT_VALUE = 1'000'000; // print "progress" on each 1Mth number
#else
	const uint64_t PRINT_VALUE = 4'000'000; // print "progress" on each 4Mth number
#endif

	StatDataType stat{};
	CalcDataType calcData{ 0ull, 0ull };

	m_hits = 0;
	auto crange = ThreeN1::GetCacheRange(start, finish);
	m_cacheStart = crange.Start;
	m_cacheFinish = crange.Finish;
	auto cacheRange = m_cacheFinish - m_cacheStart;
	
	Ticks::Start("memalloc");
	m_valuesCache.Clear();
	m_valuesCache.SetCapacity((uint32_t)cacheRange);
	m_valuesCache.SetCount((uint32_t)cacheRange); // array is full of trash data after this call, we use memset below to set array elements into required values
	memset(m_valuesCache.GetValuePointer(0), 0, sizeof(decltype(m_valuesCache)::item_type) * cacheRange);
	std::cout << std::format("{:<{}}: {}", "Cache mem alloc time", F_WIDTH, MillisecToStr(Ticks::Finish("memalloc"))) << std::endl;

	uint64_t range = finish - start;

	Ticks::Start("calctime");
	auto speedStart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point speedStop;

	size_t fieldWidth = NumLen(finish);
	fieldWidth += (fieldWidth - 1) / 3; // count thousands separators in length
	uint64_t printCounter = PRINT_VALUE;
	uint64_t MaxValWidth = MAX_VALUE_WIDTH;
	uint64_t maxProgressLen = 0;
	uint64_t speed, avgsteps, lineCnt = 0;
	std::string maxvalueStr, progressStr;
	progressStr.reserve(200);

	std::cout << std::endl;

	for (uint64_t i = start; i <= finish; i++)
	{
		if (--printCounter == 0)
		{
			printCounter = PRINT_VALUE;
			speedStop = std::chrono::high_resolution_clock::now();
			speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(speedStop - speedStart).count();
			avgsteps = stat.sumsteps / (i - start);
			progressStr.clear();
			std::format_to(std::back_inserter(progressStr), ThreeN1::Locale, "{:L} | {}% | Speed: {:L} nums/sec | Average Steps: {:L} | Hits: {}%   ", 
				i + 1, (i - start) * 100 / range, speed, avgsteps, (100 * m_hits) / (i - start));
			if (maxProgressLen < progressStr.size()) maxProgressLen = progressStr.size();
			//progressStr.append(maxProgressLen - progressStr.size(), ' ');
			std::cout << progressStr << "\r";
			speedStart = std::chrono::high_resolution_clock::now();
		}

		calc3p1CacheUpdate(i, calcData);

		stat.sumsteps += calcData.steps;

		if (stat.num2maxvalue < calcData.maxvalue)
		{
			stat.num2 = i;
			stat.num2maxvalue = calcData.maxvalue;
			maxvalueStr = std::format(ThreeN1::Locale, "{:L}", calcData.maxvalue);
			if (maxvalueStr.size() > MaxValWidth) MaxValWidth = (uint32_t)maxvalueStr.size();
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}L} | steps:     {:>5L} | MAX VALUE: {:>{}}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				i, fieldWidth, calcData.steps, maxvalueStr, MaxValWidth) << std::endl;
		}

		if (stat.num1steps < calcData.steps)
		{
			stat.num1 = i;
			stat.num1steps = calcData.steps;
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}L} | MAX STEPS: {:>5L} | value:     {:>{}L}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				i, fieldWidth, calcData.steps, calcData.maxvalue, MaxValWidth) << std::endl;
		}
	}

	std::cout << std::string(maxProgressLen, ' ') << "\r" << std::endl; // clear progress counter

	stat.calctime = Ticks::Finish("calctime");
	stat.numcount = range;

	printCalcResults(stat);

	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {} - {}", "Cache Range", F_WIDTH, ReduceNumber(m_cacheStart), ReduceNumber(m_cacheFinish)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {} ", "Cache Size", F_WIDTH, ReduceNumber(cacheRange)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} ({:.2f}%)", "Cache Hits", F_WIDTH, m_hits, (double)(100 * m_hits) / range) << std::endl;

	//std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "MAXULONGLONG", F_WIDTH, std::numeric_limits<uint64_t>::max()/* ULLONG_MAX*/) << std::endl;

	return stat;
}

// calculates range of numbers WITH pre-loaded cache
// requires much memory for storing cache, but really faster. 
// cache stores pairs steps (uint16_t) and maxvalue (uint64_t)
// collects and print some statistic
// assumes that CACHE is PRE-LOADED and function DOES NOT change cache data  
void ThreeN1Int64::Calc3p1RangeCache(const uint64_t& start, const uint64_t& finish)
{
#ifdef _DEBUG
	const uint64_t PRINT_VALUE = 1'000'000; // print "progress" on each 1Mth number
#else
	const uint64_t PRINT_VALUE = 4'000'000; // print "progress" on each 4Mth number
#endif

	StatDataType stat{};
	CalcDataType calcData{ 0ull, 0ull };

	m_hits = 0;
	uint64_t range = finish - start; 

	Ticks::Start("calctime");
	auto speedStart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point speedStop;

	size_t fieldWidth = NumLen(finish);
	fieldWidth += (fieldWidth - 1) / 3; // count thousands separators in length
	uint64_t printCounter = PRINT_VALUE;
	uint64_t MaxValWidth = MAX_VALUE_WIDTH;
	uint64_t maxProgressLen = 0;
	uint64_t speed, avgsteps, lineCnt = 0;
	std::string maxvalueStr, progressStr;
	progressStr.reserve(200);
	
	std::cout << std::endl;

	for (uint64_t i = start; i <= finish; i++)
	{
		if (--printCounter == 0)
		{
			printCounter = PRINT_VALUE;
			speedStop = std::chrono::high_resolution_clock::now();
			speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(speedStop - speedStart).count();
			avgsteps = stat.sumsteps / (i - start);
			progressStr.clear();
			std::format_to(std::back_inserter(progressStr), ThreeN1::Locale, "{:L} | {}% | Speed: {:L} nums/sec | Average Steps: {:L} | Hits: {}%   ", 
				i + 1, (i - start) * 100 / range, speed, avgsteps, (100 * m_hits) / (i - start));
			if (maxProgressLen < progressStr.size()) maxProgressLen = progressStr.size();
			//progressStr.append(maxProgressLen - progressStr.size(), ' ');
			std::cout << progressStr << "\r";
			speedStart = std::chrono::high_resolution_clock::now();
		}

		Calc3p1Cache(i, calcData);

		stat.sumsteps += calcData.steps;

		if (stat.num2maxvalue < calcData.maxvalue)
		{
			stat.num2 = i;
			stat.num2maxvalue = calcData.maxvalue;
			maxvalueStr = std::format(ThreeN1::Locale, "{:L}", calcData.maxvalue);
			if (maxvalueStr.size() > MaxValWidth) MaxValWidth = (uint32_t)maxvalueStr.size();
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}L} | steps:     {:>5L} | MAX VALUE: {:>{}}", 
				std::format(ThreeN1::Locale, "[{:L}]", lineCnt++), i, fieldWidth, calcData.steps, maxvalueStr, MaxValWidth) << std::endl;
		}

		if (stat.num1steps < calcData.steps)
		{
			stat.num1 = i;
			stat.num1steps = calcData.steps;
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}L} | MAX STEPS: {:>5L} | value:     {:>{}L}", 
				std::format(ThreeN1::Locale, "[{:L}]", lineCnt++), i, fieldWidth, calcData.steps, calcData.maxvalue, MaxValWidth) << std::endl;
		}
	}

	std::cout << std::string(maxProgressLen, ' ') << "\r" << std::endl; // clear progress counter

	stat.calctime = Ticks::Finish("calctime");
	stat.numcount = range;

	printCalcResults(stat);

	auto cacheRange = m_cacheFinish - m_cacheStart;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {} - {}", "Cache Range", F_WIDTH, ReduceNumber(m_cacheStart), ReduceNumber(m_cacheFinish)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {} ", "Cache Size", F_WIDTH, ReduceNumber(cacheRange)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} ({:.2f}%)", "Cache Hits", F_WIDTH, m_hits, (double)(100 * m_hits) / range) << std::endl;

	//std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "MAXULONGLONG", F_WIDTH, std::numeric_limits<uint64_t>::max()/* ULLONG_MAX*/) << std::endl;
}

