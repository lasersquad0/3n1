
#include "ThreeN1.h"
#include "utils/include/Ticks.h"

// calculates ONE number 
// retuns list of all 3n1 numbers produces by this number.
void ThreeN1TTMath::Calc3p1(const TTMathBigInt& number, std::vector<TTMathBigInt>& chain, uint64_t& steps, TTMathBigInt& maxNum) const
{
	steps = 0;
	maxNum = number;
	TTMathBigInt curr = number;

	chain.clear();
	chain.reserve(IThreeN1::MAX_STEPS);
	chain.push_back(curr);

	while (curr != 1) 
	{
		if (curr % 2 == 0) //TODO we can use GetBit() method here
		{
			curr /= 2;
			chain.push_back(curr);
		}
		else
		{
			curr = curr * 3 + 1;
			chain.push_back(curr);
			if (maxNum < curr) maxNum = curr;

			curr /= 2;
			chain.push_back(curr);

			steps++; // if curr is odd we do 2 operations at once and increase steps twice accordingly
		}

		steps++;
	}
}

// calculates ONE number WITHOUT using cache
// returns number of steps and max number reached in the chain
void ThreeN1TTMath::Calc3p1(const TTMathBigInt& number, CalcDataTypeTTMath& calcResult)
{
	calcResult.maxvalue = number;
	calcResult.steps = 0ull;
	TTMathBigInt curr = number;

	//auto& unused = m_unused;
	//const auto bitsCount = unused.BitsCount();

	//if (curr < bitsCount) unused.setTrue(curr);

	while (curr != 1)
	{
		if (curr % 2 == 1)
		{
			curr = curr*3 + 1;
			if (calcResult.maxvalue < curr) calcResult.maxvalue = curr;
			calcResult.steps++;
		}

		while (curr % 2 == 0)
		{
			curr /= 2;
			calcResult.steps++;
		}

		//auto tz = curr.TrailingZeroBits();
		//assert(tz > 0);
		//calcResult.steps += (uint16_t)tz;
		//while (tz-- > 0) {
		//	assert(curr.IsEven());
		//	divide_by_2(curr); 
		//}

		//if (curr < bitsCount) unused.setTrue(curr);
	}
}

// calculates ONE number WITH using cache
// all intermediate numbers are added to the cache if they are in cache range
void ThreeN1TTMath::calc3p1Cache(const TTMathBigInt& number, CalcDataTypeTTMath& calcResult)
{
	TTMathBigInt curr = number;

	//auto& unused = m_unused;
	//const auto bitsCount = unused.BitsCount();
	//if (curr < bitsCount) unused.setTrue(curr);

	TTMathBigInt chain[IThreeN1::MAX_STEPS];

	uint16_t index = 0;
	chain[index++] = number;

	while (curr != 1)
	{
		if ((curr % 2 == 0)) // is even
		{
			curr /= 2;
			chain[index++] = curr;
		}
		else
		{
			curr = curr * 3 + 1;
			//if (curr < bitsCount) unused.setTrue(curr);
			chain[index++] = curr;
			curr /= 2;
			chain[index++] = curr;
		}

		//if (curr < bitsCount) unused.setTrue(curr);

		// check if curr is in cache
		// if yes, add to cache all collected numbers chain and exit from the function
		if (curr >= m_cacheStart && curr < m_cacheFinish)
		{
			CalcDataTypeTTMath& elem = m_valuesCache[(curr - m_cacheStart).ToUInt()];
			if (elem.steps > 0) // found in cache
			{
				m_hits++; //for each number we come here once
				TTMathBigInt mxval = elem.maxvalue;
				assert(index - 1 > 0);
				for (int i = index - 2; i >= 0; i--)
				{
					auto& num = chain[i];
					if (mxval < num) mxval = num;

					if (num >= m_cacheStart && num < m_cacheFinish)
					{
						CalcDataTypeTTMath res;
						res.steps = (uint16_t)(index - 1 - i + elem.steps);
						res.maxvalue = mxval;
						m_valuesCache.SetValue((uint32_t)(num - m_cacheStart).ToUInt(), res);
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
		TTMathBigInt mxval(1);
		for (int i = index - 1; i >= 0; i--)
		{
			auto& num = chain[i];
			if (mxval < num) mxval = num;

			if (num >= m_cacheStart && num < m_cacheFinish)
			{
				CalcDataTypeTTMath res;
				res.steps = (uint16_t)(index - 1 - i);
				res.maxvalue = mxval;
				m_valuesCache.SetValue((uint32_t)(num - m_cacheStart).ToUInt(), res);
			}
		}
		calcResult.steps = index - 1;
		calcResult.maxvalue = mxval;
	}
}

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
// calculate range of numbers WITHOUT using cache, collect and print some statistic
void ThreeN1TTMath::Calc3p1Range(const TTMathBigInt& start, const TTMathBigInt& finish)
{
	uint64_t maxsteps = 0,lineCnt = 0;
	uint64_t sumsteps = 0; // sumsteps is not going to exceed uint64_t because max range we are able to calculate is ~10^12 * avg_steps = 10^15. it still less than uint64_t
	TTMathBigInt num1(0ull);
	TTMathBigInt num2(0ull), maxmaxv(0ull);
	CalcDataTypeTTMath calcData{ 0ull, 0ull };

	//std::locale loc(std::cout.getloc(), new MyGroupSeparator());

#ifdef _DEBUG
	const uint64_t PRINT_VALUE = 20'000; // print "progress" on each 50Kth number
#else
	const uint64_t PRINT_VALUE = 100'000; // print "progress" on each 100Kth number
#endif

	uint64_t printCounter = PRINT_VALUE;
	Ticks::Start("calctime");
	auto speedStart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point speedStop;

	std::string fstr = finish.ToString();
	size_t fieldWidth = fstr.size() + 2; //2 extra spaces
	TTMathBigInt range = finish - start;

	for (TTMathBigInt i = start; i < finish; i++)
	{
		if (--printCounter == 0) // show progress
		{
			printCounter = PRINT_VALUE;
			speedStop = std::chrono::high_resolution_clock::now();
			auto speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(speedStop - speedStart).count();
			//TODO calc length of the longest line here and use it below for clearing all the line 
			std::cout << '\r' << std::format(ThreeN1::Locale, "{} | {}% | speed: {:L} num/sec", i + 1, (i - start) * 100 / range, speed) << '\r'; // i+1 is to avoid showing ... 999 999 in progress print
			speedStart = std::chrono::high_resolution_clock::now();
		}

		Calc3p1(i, calcData);

		sumsteps += calcData.steps;

		if (maxmaxv < calcData.maxvalue)
		{
			num1 = i;
			maxmaxv = calcData.maxvalue;
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}} | steps: {:>5L} | MAX VALUE: {:>26}", std::format("[{:L}]", lineCnt++),
				i, fieldWidth, calcData.steps, calcData.maxvalue) << std::endl;
		}

		if (maxsteps < calcData.steps)
		{
			num2 = i;
			maxsteps = calcData.steps;
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}} | STEPS: {:>5L} | max value: {:>26}", std::format("[{:L}]", lineCnt++),
				i, fieldWidth, calcData.steps, calcData.maxvalue) << std::endl;
		}
	}

	//stop = std::chrono::high_resolution_clock::now();

	std::cout << std::string(50, ' ') << "\r" << std::endl; // clear progress counter

	auto calcTime = Ticks::Finish("calctime"); //std::chrono::duration_cast<std::chrono::milliseconds>(stop - start0).count();
	std::cout << std::format("{:<{}}: {}", "Calculation time", F_WIDTH, MillisecToStr(calcTime)) << std::endl;

	/*auto num = std::max(num1, num2);
	uint64_t dig;
	if constexpr (std::is_same<IntImpl, BigInt>::value) // for BigInt only
		dig = (uint64_t)(Length(num) * 1.35);
	else
		dig = (uint64_t)((log10(num) + 1)*1.35); // +30% for spaces between groups by 3 digits
	*/
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} for number {:}", "Max Steps", F_WIDTH, maxsteps, num2) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:} for number {:}", "Max Value", F_WIDTH, maxmaxv, num1) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Total Steps", F_WIDTH, sumsteps) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:}", "Average Steps", F_WIDTH, sumsteps / range.ToUInt()) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:} num/sec", "Average Speed", F_WIDTH, range * 1000 / calcTime) << std::endl;

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
}

// calculate range of numbers WITH cache, collect and print some statistic
// might be faster but I am not sure
void ThreeN1TTMath::Calc3p1RangeCache(const TTMathBigInt& start, const TTMathBigInt& finish)
{
	uint64_t maxsteps = 0, sumsteps = 0, lineCnt = 0;
	TTMathBigInt num1(0ull);
	TTMathBigInt num2(0ull), maxmaxv(0ull);
	CalcDataType calcData{ 0ull, 0ull };

	m_hits = 0;
	TTMathBigInt range = finish - start + 1ull;
	m_cacheStart = 5;
	m_cacheFinish = m_cacheStart + range;
	m_valuesCache.Clear();
	Ticks::Start("memalloc");
	m_valuesCache.SetCapacity((uint32_t)range.ToUInt());
	m_valuesCache.SetCount((uint32_t)range.ToUInt()); // array is full of trash data after this call, use memset to set array elements into required values
	std::cout << std::format("{:<{}}: {}", "Mem alloc time", F_WIDTH, MillisecToStr(Ticks::Finish("memalloc"))) << std::endl;
	//memset(m_valuesCache.GetValuePointer(0), 0, sizeof(decltype(m_valuesCache)::item_type) * range);
	
	//std::locale loc(std::cout.getloc(), new MyGroupSeparator());

#ifdef _DEBUG
	const uint64_t PRINT_VALUE = 20'000; // print "progress" on each 50Kth number
#else
	const uint64_t PRINT_VALUE = 100'000; // print "progress" on each 100Kth number
#endif

	uint64_t printCounter = PRINT_VALUE;
	Ticks::Start("calctime");
	auto speedStart = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point speedStop;

	std::string fstr = finish.ToString();
	size_t fieldWidth = fstr.length() + 2; //2 extra spaces

	for (TTMathBigInt i = start; i < finish; i++)
	{
		if (--printCounter == 0)
		{
			printCounter = PRINT_VALUE;
			speedStop = std::chrono::high_resolution_clock::now();
			auto speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(speedStop - speedStart).count();
			//TODO calc length of the longest line here and use it below for clearing all the line 
			std::cout << '\r' << i + 1 << " | " << (i - start) * 100 / (finish - start) << "% | speed: " << speed << " num/sec" << '\r'; // i+1 is to avoid showing ... 999 999 in progress print
			speedStart = std::chrono::high_resolution_clock::now();
		}

		calc3p1Cache(i, calcData);
		//m_valuesCache.SetValue((uint32_t)(i - m_cacheStart), calcData); // works quicker than .AddValue()

		sumsteps += calcData.steps;

		if (maxmaxv < calcData.maxvalue)
		{
			num1 = i;
			maxmaxv = calcData.maxvalue;
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}} | steps: {:>5L} | MAX VALUE: {:>26}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				i, fieldWidth, calcData.steps, calcData.maxvalue) << std::endl;
		}

		if (maxsteps < calcData.steps)
		{
			num2 = i;
			maxsteps = calcData.steps;
			std::cout << std::format(ThreeN1::Locale, "{:>5} Number: {:>{}} | STEPS: {:>5L} | max value: {:>26}", std::format(ThreeN1::Locale, "[{:L}]", lineCnt++),
				i, fieldWidth, calcData.steps, calcData.maxvalue) << std::endl;
		}
	}

	std::cout << std::string(50, ' ') << "\r" << std::endl; // clear progress counter

	auto calcTime = Ticks::Finish("calctime"); //std::chrono::duration_cast<std::chrono::milliseconds>(stop - start0).count();
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {}", "Calculation time", F_WIDTH, MillisecToStr(calcTime)) << std::endl;

	/*auto num = std::max(num1, num2);
	uint64_t dig;
	if constexpr (std::is_same<IntImpl, BigInt>::value) // for BigInt only
		dig = (uint64_t)(Length(num) * 1.35);
	else
		dig = (uint64_t)((log10(num) + 1) * 1.35); // +30% for spaces between groups by 3 digits

	std::cout << std::format(loc, "Number: {:>{}L} | max steps: {}", toULongLong(num2), dig, maxsteps) << std::endl;
	std::cout << std::format(loc, "Number: {:>{}L} | max value: {}", toULongLong(num1), dig, maxmaxv) << std::endl;
*/
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} | for number: {:}", "Max Steps", F_WIDTH, maxsteps, num2) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:} | for number: {:}", "Max Value", F_WIDTH, maxmaxv, num1) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} steps", "Total Steps", F_WIDTH, sumsteps) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:} steps", "Average Steps", F_WIDTH, sumsteps / range.ToUInt()) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:} num/sec", "Average Speed", F_WIDTH, range * 1000 / calcTime) << std::endl;

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
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} ({}%)", "Cache Hits", F_WIDTH, m_hits, (100 * m_hits) / range.ToUInt()) << std::endl;

}

