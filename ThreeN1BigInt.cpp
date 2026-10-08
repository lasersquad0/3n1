
#include "ThreeN1.h"
#include "utils/include/Ticks.h"

// calculates ONE number 
// retuns list of all 3n1 numbers produces by this number.
void ThreeN1BigInt::Calc3p1(const BigInt& number, std::vector<BigInt>& chain, uint64_t& steps, BigInt& maxNum) const
{
	steps = 0;
	maxNum = number;
	BigInt curr = number;

	chain.clear();
	chain.reserve(IThreeN1::MAX_STEPS);
	chain.push_back(curr);

	while (curr != BigInt::ONE)
	{
		if (curr.IsEven())
		{
			divide_by_2(curr);
			chain.push_back(curr);
		}
		else
		{
			curr += curr + curr;
			curr++;
			chain.push_back(curr);
			if (maxNum < curr) maxNum = curr;

			divide_by_2(curr);
			chain.push_back(curr);

			steps++; // if curr is odd we do 2 operations at once and increase steps twice accordingly
		}

		steps++;
	}
}

// calculates ONE number WITHOUT using cache
// returns number of steps and max number reached in the chain
void ThreeN1BigInt::Calc3p1(const BigInt& number, CalcDataTypeBigInt& calcResult)
{
	calcResult.maxvalue = number;
	calcResult.steps = 0ull;
	BigInt curr = number;

	auto& unused = m_unused;
	const auto bitsCount = unused.BitsCount();

	if (curr < bitsCount) unused.setTrue(curr);

	while (curr != BigInt::ONE)
	{
		if (curr.IsOdd())
		{
			curr += curr + curr;
			curr++;

			if (calcResult.maxvalue < curr) calcResult.maxvalue = curr;
			calcResult.steps++;
		}

		while (curr.IsEven())
		{
			divide_by_2(curr);
			calcResult.steps++;
		}

		//auto tz = curr.TrailingZeroBits();
		//assert(tz > 0);
		//calcResult.steps += (uint16_t)tz;
		//while (tz-- > 0) {
		//	assert(curr.IsEven());
		//	divide_by_2(curr); 
		//}

		if (curr < bitsCount) unused.setTrue(curr);
	}
}

// calculates ONE number WITH using cache
// DOES NOT add items to the cache. Assumes that cache is full and loaded from a file.
void ThreeN1BigInt::Calc3p1Cache(const BigInt& number, CalcDataTypeBigInt& calcResult)
{
	BigInt curr = number;
	BigInt mxval = number;
	uint16_t steps = 0;

	auto& unused = m_unused;
	const auto bitsCount = unused.BitsCount();
	if (curr < bitsCount) unused.setTrue(curr);

	while (curr != BigInt::ONE)
	{
		if ((curr.IsEven())) 
		{
			divide_by_2(curr);
		}
		else
		{
			curr += curr + curr;
			curr++;
			if (mxval < curr) mxval = curr;
			if (curr < bitsCount) unused.setTrue(curr);
			divide_by_2(curr);
			steps++;
		}
		steps++;

		if (curr < bitsCount) unused.setTrue(curr);

		// check if curr is in cache
		// if yes, add to cache all collected numbers chain and exit from the function
		if (curr >= m_cacheStart && curr < m_cacheFinish)
		{			
			auto& elem = m_valuesCache[(uint32_t)toUInt64(curr - m_cacheStart)];
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
void ThreeN1BigInt::calc3p1CacheUpdate(const BigInt& number, CalcDataTypeBigInt& calcResult)
{
	BigInt curr = number;
	auto& unused = m_unused;
	const auto bitsCount = unused.BitsCount();

	if (curr < bitsCount) unused.setTrue(curr);

	BigInt chain[IThreeN1::MAX_STEPS];

	uint16_t index = 0;
	chain[index++] = number;

	while (curr != BigInt::ONE)
	{
		if (curr.IsEven()) // is even
		{
			divide_by_2(curr);
			chain[index++] = curr;
		}
		else
		{
			curr += curr + curr;
			curr++;
			if (curr < bitsCount) unused.setTrue(curr);
			chain[index++] = curr;
			divide_by_2(curr);
			chain[index++] = curr;
		}

		if (curr < bitsCount) unused.setTrue(curr);

		// check if curr is in cache
		// if yes, add to cache all collected numbers chain and exit from the function
		if (curr >= m_cacheStart && curr < m_cacheFinish)
		{
			CacheItemType& elem = m_valuesCache[(uint32_t)(toUInt64(curr - m_cacheStart))];

			if (elem.steps > 0) // found in cache
			{
				m_hits++; //for each number we come here once
				BigInt mxval = elem.maxvalue;
				assert(index - 1 > 0);
				for (int i = index - 2; i >= 0; i--)
				{
					auto& num = chain[i];
					if (mxval < num) mxval = num;

					if (num >= m_cacheStart && num < m_cacheFinish)
					{
						CacheItemType res;
						res.steps = (uint16_t)(index - 1 - i + elem.steps);
						res.maxvalue = toUInt64(mxval);
						m_valuesCache.SetValue((uint32_t)toUInt64(num - m_cacheStart), res);
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
		BigInt mxval = 1;
		for (int i = index - 1; i >= 0; i--)
		{
			auto& num = chain[i];
			if (mxval < num) mxval = num;

			if (num >= m_cacheStart && num < m_cacheFinish)
			{
				CacheItemType res;
				res.steps = (uint16_t)(index - 1 - i);
				res.maxvalue = toUInt64(mxval);
				m_valuesCache.SetValue((uint32_t)toUInt64(num - m_cacheStart), res);
			}
		}
		calcResult.steps = index - 1;
		calcResult.maxvalue = mxval;
	}
};


// calc ONE number WITH using cache
// that might be faster than without cache, but not sure
/*void ThreeN1BigInt::calc3p1Cache(const BigInt& number, CalcDataTypeBigInt& calcResult)
{
	calcResult.maxvalue = number;
	calcResult.steps = 0ull;
	BigInt curr = number;
	auto& unused = m_unused;
	const auto bitsCount = unused.BitsCount();

	if (curr < bitsCount) unused.setTrue(curr);

	while (curr != 1ull)
	{
		if (curr.IsEven())
		{
			divide_by_2(curr);
		}
		else
		{
			curr = (curr + curr + curr + 1ull);
			divide_by_2(curr);
			calcResult.steps++; // if curr is odd we do 2 operations at once and increase steps twice accordingly
			if (calcResult.maxvalue < curr) calcResult.maxvalue = curr;
		}

		calcResult.steps++;

		if (curr < bitsCount) unused.setTrue(curr);

		if (checkInCache(curr, calcResult)) break;
	}
}
*/
// calculates RANGE of numbers WITHOUT using cache
// collects and print some statistic
void ThreeN1BigInt::Calc3p1Range(const BigInt& start, const BigInt& finish)
{
#ifdef _DEBUG
	const uint64_t PRINT_VALUE = 20'000; // print "progress" on each 50Kth number
#else
	const uint64_t PRINT_VALUE = 100'000; // print "progress" on each 100Kth number
#endif
	
	Ticks::Start("calctime");
	auto speedStart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point speedStop;

	StatDataType stat{};
	CalcDataType calcData{ 0ull, 0ull };

	size_t fieldWidth = NumLen(finish);
	fieldWidth += (fieldWidth - 1) / 3; // count thousands separators in length
	BigInt range = finish - start;
	uint64_t printCounter = PRINT_VALUE;
	uint64_t maxValWidth = MAX_VALUE_WIDTH;
	uint64_t maxProgressLen = 0;
	uint64_t speed, avgsteps, lineCnt = 0;
	std::string maxvalueStr, progressStr;
	progressStr.reserve(200);

	std::cout << std::endl;

	for (BigInt i = start; i < finish; i++)
	{
		if (--printCounter == 0) // show progress
		{
			printCounter = PRINT_VALUE;
			speedStop = std::chrono::high_resolution_clock::now();
			speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(speedStop - speedStart).count();
			avgsteps = toUInt64(stat.sumsteps / (i - start));
			progressStr.clear();
			std::format_to(std::back_inserter(progressStr), ThreeN1::Locale, "{:L} | {}% | Speed: {:L} nums/sec | Average Steps: {:L}", i + 1, (i - start) * 100 / range, speed, avgsteps);
			if (maxProgressLen < progressStr.size()) maxProgressLen = progressStr.size();
			//progressStr.append(maxProgressLen - progressStr.size(), ' ');
			std::cout << progressStr << "\r";
			//std::cout << std::format(ThreeN1::Locale, "{:L} | {}% | Speed: {:L} num/sec | Average Steps: {:L}", i + 1, (i - start) * 100 / range, speed, stepsavg) << '\r'; // i+1 is to avoid showing ... 999 999 in progress print
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
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}} | steps:     {:>5L} | MAX VALUE: {:>{}}", 
				std::format(ThreeN1::Locale, "[{:L}]", lineCnt++), std::format(ThreeN1::Locale, "{:L}", i), fieldWidth, calcData.steps, maxvalueStr, maxValWidth) << std::endl;
		}

		if (stat.num1steps < calcData.steps)
		{
			stat.num1 = i;
			stat.num1steps = calcData.steps;
			maxvalueStr = std::format(ThreeN1::Locale, "{:L}", calcData.maxvalue);
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}} | MAX STEPS: {:>5L} | value:     {:>{}}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				std::format(ThreeN1::Locale, "{:L}", i), fieldWidth, calcData.steps, maxvalueStr, maxValWidth) << std::endl;
		}
	}

	std::cout << std::string(maxProgressLen, ' ') << "\r" << std::endl; // clear progress counter 
	
	stat.calctime = Ticks::Finish("calctime");
	stat.numcount = toUInt64(range);

	printCalcResults(stat);
}

// calculates range of numbers WITH cache
// If item is not in cache -> adds it into cache
// might be faster that "no cache" methods, but I am not sure
// requires much memory for storing cache. 
// cache stores pairs steps (uint16_t) and maxvalue (BigInt)
// assumes that CACHE is EMPTY and fills it during its calculation process  
ThreeN1BigInt::StatDataType ThreeN1BigInt::Calc3p1RangeCacheUpdate(const BigInt& start, const BigInt& finish)
{
#ifdef _DEBUG
	const uint64_t PRINT_VALUE = 20'000; // print "progress" on each 50Kth number
#else
	const uint64_t PRINT_VALUE = 100'000; // print "progress" on each 100Kth number
#endif

	StatDataType stat{};
	CalcDataType calcData{ 0ull, 0ull };

	m_hits = 0;
	BigInt range = finish - start;
	assert(range < std::numeric_limits<uint32_t>::max()); // assuming range is less than 4G
	//uint32_t range32 = (uint32_t)toUInt64(range);

	auto crange = ThreeN1::GetCacheRange(start, finish);
	m_cacheStart = crange.Start;
	m_cacheFinish = crange.Finish;
	auto cacheRange = m_cacheFinish - m_cacheStart;

	Ticks::Start("memalloc");
	m_valuesCache.Clear();
	m_valuesCache.SetCapacity((uint32_t)cacheRange); //TODO SetCapacity calls constructors for each BigInt in the array. That takes a time. 
	m_valuesCache.SetCount((uint32_t)cacheRange);
	std::cout << std::format("{:<{}}: {}", "Cache mem alloc time", F_WIDTH, MillisecToStr(Ticks::Finish("memalloc"))) << std::endl;
	//memset(m_valuesCache.GetValuePointer(0), 0, sizeof(decltype(m_valuesCache)::item_type) * range);

	uint64_t printCounter = PRINT_VALUE;
	Ticks::Start("calctime");
	auto speedStart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point speedStop;

	size_t fieldWidth = NumLen(finish);
	fieldWidth += (fieldWidth - 1) / 3; // count thousands separators in length
	uint64_t maxValWidth = MAX_VALUE_WIDTH;
	uint64_t maxProgressLen = 0;
	uint64_t speed, avgsteps, lineCnt = 0;
	std::string maxvalueStr, progressStr;
	progressStr.reserve(200);

	std::cout << std::endl;

	for (BigInt i = start; i <= finish; i++)
	{
		if (--printCounter == 0)
		{
			printCounter = PRINT_VALUE;
			speedStop = std::chrono::high_resolution_clock::now();
			speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(speedStop - speedStart).count();
			avgsteps = toUInt64(stat.sumsteps / (i - start));
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
			if (maxvalueStr.size() > maxValWidth) maxValWidth = (uint32_t)maxvalueStr.size();
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}} | steps:     {:>5L} | MAX VALUE: {:>{}}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				std::format(ThreeN1::Locale, "{:L}", i), fieldWidth, calcData.steps, maxvalueStr, maxValWidth) << std::endl;
		}

		if (stat.num1steps < calcData.steps)
		{
			stat.num1 = i;
			stat.num1steps = calcData.steps;
			maxvalueStr = std::format(ThreeN1::Locale, "{:L}", calcData.maxvalue);
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}} | MAX STEPS: {:>5L} | value:     {:>{}}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				std::format(ThreeN1::Locale, "{:L}", i), fieldWidth, calcData.steps, maxvalueStr, maxValWidth) << std::endl;
		}
	}

	std::cout << std::string(maxProgressLen, ' ') << "\r" << std::endl; // clear progress counter

	stat.calctime = Ticks::Finish("calctime");
	stat.numcount = toUInt64(range);

	printCalcResults(stat);

	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {} - {}", "Cache Range", F_WIDTH, ReduceNumber(m_cacheStart), ReduceNumber(m_cacheFinish)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {} ", "Cache Size", F_WIDTH, ReduceNumber(cacheRange)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} ({:.2f}%)", "Cache Hits", F_WIDTH, m_hits, (double)(100 * m_hits) / toUInt64(range)) << std::endl;

	return stat;
}

// calculates range of numbers WITH cache
// If item is not in cache -> adds it into cache
// might be faster that "no cache" methods, but I am not sure
// requires much memory for storing cache. 
// cache stores pairs steps (uint16_t) and maxvalue (BigInt)
// assumes that CACHE is PRE-LOADED and function DOES NOT change cache data
void ThreeN1BigInt::Calc3p1RangeCache(const BigInt& start, const BigInt& finish)
{
#ifdef _DEBUG
	const uint64_t PRINT_VALUE = 20'000; // print "progress" on each 50Kth number
#else
	const uint64_t PRINT_VALUE = 100'000; // print "progress" on each 100Kth number
#endif

	StatDataType stat{};
	CalcDataType calcData{ 0ull, 0ull };

	m_hits = 0;
	BigInt range = finish - start;
	assert(range < std::numeric_limits<uint32_t>::max()); // assuming range is less than 4G

	uint64_t printCounter = PRINT_VALUE;
	Ticks::Start("calctime");
	auto speedStart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point speedStop;

	size_t fieldWidth = NumLen(finish);
	fieldWidth += (fieldWidth - 1) / 3; // count thousands separators in length
	uint64_t maxValWidth = MAX_VALUE_WIDTH;
	uint64_t maxProgressLen = 0;
	uint64_t speed, avgsteps, lineCnt = 0;
	std::string maxvalueStr, progressStr;
	progressStr.reserve(200);

	std::cout << std::endl;

	for (BigInt i = start; i <= finish; i++)
	{
		if (--printCounter == 0)
		{
			printCounter = PRINT_VALUE;
			speedStop = std::chrono::high_resolution_clock::now();
			speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(speedStop - speedStart).count();
			avgsteps = toUInt64(stat.sumsteps / (i - start));
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
			if (maxvalueStr.size() > maxValWidth) maxValWidth = (uint32_t)maxvalueStr.size();
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}} | steps:     {:>5L} | MAX VALUE: {:>{}}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				std::format(ThreeN1::Locale, "{:L}", i), fieldWidth, calcData.steps, maxvalueStr, maxValWidth) << std::endl;
		}

		if (stat.num1steps < calcData.steps)
		{
			stat.num1 = i;
			stat.num1steps = calcData.steps;
			maxvalueStr = std::format(ThreeN1::Locale, "{:L}", calcData.maxvalue);
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}} | MAX STEPS: {:>5L} | value:     {:>{}}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				std::format(ThreeN1::Locale, "{:L}", i), fieldWidth, calcData.steps, maxvalueStr, maxValWidth) << std::endl;
		}
	}

	std::cout << std::string(maxProgressLen, ' ') << "\r" << std::endl; // clear progress counter

	stat.calctime = Ticks::Finish("calctime");
	stat.numcount = toUInt64(range);

	printCalcResults(stat);

	auto cacheRange = m_cacheFinish - m_cacheStart;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {} - {}", "Cache Range", F_WIDTH, ReduceNumber(m_cacheStart), ReduceNumber(m_cacheFinish)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {} ", "Cache Size", F_WIDTH, ReduceNumber(cacheRange)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} ({:.2f}%)", "Cache Hits", F_WIDTH, m_hits, (double)(100 * m_hits) / toUInt64(range)) << std::endl;

}
