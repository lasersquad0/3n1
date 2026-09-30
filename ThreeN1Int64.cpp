
#include "ThreeN1.h"
#include "utils/include/Ticks.h"

// calculates ONE number 
// retuns list of all 3n1 numbers produces by this number.
void ThreeN1Int64::Calc3p1(const uint64_t& number, std::vector<uint64_t>& chain, uint64_t& steps, uint64_t& maxNum) const
{
	steps = 0;
	maxNum = number;
	uint64_t curr = number;

	auto overflow = OVERFLOW_LIMIT;
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
// all intermediate numbers are added to the cache if they are in cache range
void ThreeN1Int64::calc3p1Cache(const uint64_t& number, CalcDataType64& calcResult)
{
	//uint32_t steps = 0ull;
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
			
			//steps++; // if curr is odd we do 2 operations at once and increase steps twice accordingly
		}

		//steps++; //TODO we can remove steps and use only index

		if (curr < bitsCount) unused.setTrue(curr);

		// check if curr is in cache
		// if yes, add to cache all collected numbers chain and exit from the function
		if (curr >= m_cacheStart && curr < m_cacheFinish)
		{
			CalcDataType64& elem = m_valuesCache[(uint32_t)(curr - m_cacheStart)];
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
						CalcDataType64 res;
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
				CalcDataType64 res;
				res.steps = (uint16_t)(index - 1 - i);
				res.maxvalue = mxval;
				m_valuesCache.SetValue((uint32_t)(num - m_cacheStart), res);
			}
		}
		calcResult.steps = index - 1;
		calcResult.maxvalue = mxval;
	}
}

// calculate range of numbers WITHOUT using cache, collect and print some statistic
void ThreeN1Int64::Calc3p1Range(const uint64_t& start, const uint64_t& finish)
{
	uint64_t maxsteps = 0, sumsteps = 0, lineCnt = 0;
	uint64_t num1 = 0ull;
	uint64_t num2 = 0ull, maxmaxv = 0ull;
	CalcDataType calcData{ 0ull, 0ull };
	//std::locale loc(std::cout.getloc(), new MyGroupSeparator());

#ifdef _DEBUG
	const uint64_t PRINT_VALUE = 1'000'000; // print "progress" on each 1Mth number
#else
	const uint64_t PRINT_VALUE = 4'000'000; // print "progress" on each 4Mth number
#endif

	uint64_t printCounter = PRINT_VALUE;
	Ticks::Start("calctime");
	auto speedStart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point speedStop;

	size_t fieldWidth = NumLen(finish) + 2; //2 extra spaces
	uint64_t range = finish - start;
	uint32_t MaxValWidth = MAX_VALUE_WIDTH;
	std::string maxvalueStr;

	for (uint64_t i = start; i < finish; i++)
	{
		if (--printCounter == 0) // show progress
		{
			printCounter = PRINT_VALUE;
			speedStop = std::chrono::high_resolution_clock::now();
			auto speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(speedStop - speedStart).count();
			//TODO calc length of the longest line here and use it below for clearing all the line 
			std::cout << '\r' << i + 1 << " | " << (i - start) * 100 / range << "% | speed: " << speed << " num/sec " << '\r'; // i+1 is to avoid showing ... 999 999 in progress print
			speedStart = std::chrono::high_resolution_clock::now();
		}

		Calc3p1(i, calcData);

		sumsteps += calcData.steps;

		if (maxmaxv < calcData.maxvalue)
		{
			num1 = i;
			maxmaxv = calcData.maxvalue;
			maxvalueStr = std::format(ThreeN1::Locale, "{:L}", calcData.maxvalue);
			if (maxvalueStr.size() > MaxValWidth) MaxValWidth = (uint32_t)maxvalueStr.size();
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}L} | steps: {:>5L} | MAX VALUE: {:>{}}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				i, fieldWidth, calcData.steps, maxvalueStr, MaxValWidth) << std::endl;
		}

		if (maxsteps < calcData.steps)
		{
			num2 = i;
			maxsteps = calcData.steps;
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}L} | STEPS: {:>5L} | max value: {:>{}L}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				i, fieldWidth, calcData.steps, calcData.maxvalue, MaxValWidth) << std::endl;
		}
	}

	//stop = std::chrono::high_resolution_clock::now();

	std::cout << std::string(50, ' ') << "\r" << std::endl; // clear progress counter

	auto calcTime = Ticks::Finish("calctime");//std::chrono::duration_cast<std::chrono::milliseconds>(stop - start0).count();
	std::cout << std::format("{:<{}}: {}", "Calculation time", F_WIDTH, MillisecToStr(calcTime)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} for number {:L}", "Max Steps", F_WIDTH, maxsteps, num2) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} for number {:L}", "Max Value", F_WIDTH, maxmaxv, num1) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Total Steps", F_WIDTH, sumsteps) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} steps", "Average Steps", F_WIDTH, sumsteps / range) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} num/sec", "Average Speed", F_WIDTH, range * 1000 / calcTime) << std::endl;

	if (m_unused.BitsCount() > 0)
	{
		const uint32_t SHOW_FIRST_UNUSED = 30;
		uint32_t unused = 0;
		uint32_t numOfFirst = SHOW_FIRST_UNUSED;
		std::string str;
		for (size_t i = 1; i < m_unused.BitsCount(); i++) // bypass 0 number, it is never touched 
		{
			if (m_unused.get(i) == false)
			{
				if (numOfFirst > 0)
				{
					str = str + "," + std::to_string(i);
					numOfFirst--;
				}
				unused++;
			}
		}

		std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Unused numbers total", F_WIDTH, unused) << std::endl;
		std::cout << std::format(ThreeN1::Locale, "{:<{}}: {}", std::format(ThreeN1::Locale, "Unused numbers (first {:L})", SHOW_FIRST_UNUSED), F_WIDTH, str) << std::endl;
	}

	//if (std::is_same<IntImpl, uint64_t>::value)
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "MAXULONGLONG", F_WIDTH, std::numeric_limits<uint64_t>::max()/* ULLONG_MAX*/) << std::endl;
}



// calculate range of numbers WITH cache, collect and print some statistic
// requires much memory for storing cache but really faster. 
// cache stores pairs steps (uint16_t) and maxvalue (uint64_t)
void ThreeN1Int64::Calc3p1RangeCache(const uint64_t& start, const uint64_t& finish)
{
	uint64_t maxsteps = 0, sumsteps = 0, lineCnt = 0;
	uint64_t num1 = 0ull;
	uint64_t num2 = 0ull, maxmaxv = 0ull;
	CalcDataType calcData{ 0ull, 0ull };

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

	m_hits = 0;
	uint64_t range = finish - start + 1ull; // becuse both finish and start are valid numbers for checking
	m_cacheStart = start;
	m_cacheFinish = m_cacheStart + range;
	m_valuesCache.Clear();
	Ticks::Start("memalloc");
	m_valuesCache.SetCapacity((uint32_t)range);
	m_valuesCache.SetCount((uint32_t)range); // array is full of trash data after this call, use memset to set array elements into required values
	memset(m_valuesCache.GetValuePointer(0), 0, sizeof(decltype(m_valuesCache)::item_type) * range);
	std::cout << std::format("{:<{}}: {}", "Mem alloc time", F_WIDTH, MillisecToStr(Ticks::Finish("memalloc"))) << std::endl;

	//std::locale loc(std::cout.getloc(), new MyGroupSeparator());

#ifdef _DEBUG
	const uint64_t PRINT_VALUE = 1'000'000; // print "progress" on each 1Mth number
#else
	const uint64_t PRINT_VALUE = 4'000'000; // print "progress" on each 4Mth number
#endif

	uint64_t printCounter = PRINT_VALUE;
	Ticks::Start("calctime");
	auto speedStart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point speedStop;

	size_t fieldWidth = NumLen(finish) + 2; //2 extra spaces
	uint32_t MaxValWidth = MAX_VALUE_WIDTH;
	std::string maxvalueStr;

	for (uint64_t i = start; i < finish; i++)
	{
		if (--printCounter == 0)
		{
			printCounter = PRINT_VALUE;
			speedStop = std::chrono::high_resolution_clock::now();
			auto speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(speedStop - speedStart).count();
			//TODO calc length of the longest line here and use it below for clearing all the line 
			std::cout << '\r' << i + 1 << " | " << (i - start)*100 / range << "% | speed: " << speed << " num/sec" << '\r'; // i+1 is to avoid showing ... 999 999 in progress print
			speedStart = std::chrono::high_resolution_clock::now();
		}

		calc3p1Cache(i, calcData);
		//m_valuesCache.SetValue((uint32_t)(i - m_cacheStart), calcData); // works quicker than .AddValue()

		sumsteps += calcData.steps;

		if (maxmaxv < calcData.maxvalue)
		{
			num1 = i;
			maxmaxv = calcData.maxvalue;
			maxvalueStr = std::format(ThreeN1::Locale, "{:L}", calcData.maxvalue);
			if (maxvalueStr.size() > MaxValWidth) MaxValWidth = (uint32_t)maxvalueStr.size();
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}L} | steps: {:>5L} | MAX VALUE: {:>{}}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				i, fieldWidth, calcData.steps, maxvalueStr, MaxValWidth) << std::endl;
		}

		if (maxsteps < calcData.steps)
		{
			num2 = i;
			maxsteps = calcData.steps;
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}L} | STEPS: {:>5L} | max value: {:>{}L}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				i, fieldWidth, calcData.steps, calcData.maxvalue, MaxValWidth) << std::endl;
		}
	}

	std::cout << std::string(50, ' ') << "\r" << std::endl; // clear progress counter

	auto calcTime = Ticks::Finish("calctime"); //std::chrono::duration_cast<std::chrono::milliseconds>(stop - start0).count();
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {}", "Calculation time", F_WIDTH, MillisecToStr(calcTime)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} for number: {:L}", "Max Steps", F_WIDTH, maxsteps, num2) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} for number: {:L}", "Max Value", F_WIDTH, maxmaxv, num1) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Total Steps", F_WIDTH, sumsteps) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} steps", "Average Steps", F_WIDTH, sumsteps / range) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} num/sec", "Average Speed", F_WIDTH, range * 1000 / calcTime) << std::endl;

	if (m_unused.BitsCount() > 0)
	{
		const uint32_t SHOW_FIRST_UNUSED = 30;
		uint32_t unused = 0;
		uint32_t numOfFirst = SHOW_FIRST_UNUSED;
		//uint32_t numOfFirstBy3 = 10;
		std::string str;
		for (size_t i = 1; i < m_unused.BitsCount(); i++) // bypass 0 number, it is never touched 
		{
			if (m_unused.get(i) == false)
			{
				if (numOfFirst > 0)
				{
					str = str + "," + std::to_string(i);
					numOfFirst--;
				}
				unused++;
			}
		}

		std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Unused Numbers Total", F_WIDTH, unused) << std::endl;
		std::cout << std::format(ThreeN1::Locale, "{:<{}}: {}", std::format(ThreeN1::Locale, "Unused Numbers (first {:L})", SHOW_FIRST_UNUSED), F_WIDTH, str) << std::endl;
	}

	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} items", "Cache Size", F_WIDTH, m_valuesCache.Count()) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} ({:.6f}%)", "Cache Hits", F_WIDTH, m_hits, (double)(100 * m_hits) / range) << std::endl;

	//if (std::is_same<IntImpl, uint64_t>::value)
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "MAXULONGLONG", F_WIDTH, std::numeric_limits<uint64_t>::max()/* ULLONG_MAX*/) << std::endl;
}
