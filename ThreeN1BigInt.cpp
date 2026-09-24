
#include "ThreeN1.h"

// calc ONE number and retun list of all 3n1 numbers produces by number.
void ThreeN1BigInt::Calc3p1(const BigInt& number, std::vector<BigInt>& chain, uint64_t& steps, BigInt& maxNum)
{
	steps = 0;
	maxNum = number;
	BigInt curr = number;

	chain.clear();
	chain.reserve(IThreeN1::MAX_STEPS);
	chain.push_back(curr);

	while (curr != 1ull)
	{
		if (curr.IsEven())
		{
			divide_by_2(curr);
			chain.push_back(curr);
		}
		else
		{
			curr = (curr + curr + curr + 1ull);
			chain.push_back(curr);
			if (maxNum < curr) maxNum = curr;

			divide_by_2(curr);
			chain.push_back(curr);

			steps++; // if curr is odd we do 2 operations at once and increase steps twice accordingly
		}

		steps++;
	}
}

// calc ONE number WITHOUT using cache
void ThreeN1BigInt::Calc3p1(const BigInt& number, CalcDataTypeBigInt& calcResult)
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
	}
}

// calc ONE number WITH using cache
// that might be faster than without cache, but not sure
void ThreeN1BigInt::calc3p1Cache(/*const BigInt& start, const BigInt& finish,*/ const BigInt& number, CalcDataTypeBigInt& calcResult)
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

		if (checkInCache(curr, /*start, finish,*/ calcResult)) break;
	}
}

// calculate range of numbers WITHOUT using cache, collect and print some statistic
void ThreeN1BigInt::Calc3p1Range(const BigInt& start, const BigInt& finish)
{
	uint64_t maxsteps = 0, sumsteps = 0, lineCnt = 0;
	BigInt num1 = 0ull;
	BigInt num2 = 0ull, maxmaxv = 0ull;
	CalcDataType calcData{ 0ull, 0ull };
	std::locale loc(std::cout.getloc(), new MyGroupSeparator());

#ifdef _DEBUG
	const uint64_t PRINT_VALUE = 50'000; // print "progress" on each 50Kth number
#else
	const uint64_t PRINT_VALUE = 100'000; // print "progress" on each 100Kth number
#endif

	uint64_t printCounter = PRINT_VALUE;
	auto start0 = std::chrono::high_resolution_clock::now();
	auto start1 = start0;
	std::chrono::high_resolution_clock::time_point stop;

	size_t fieldWidth = NumLen(finish) + 2; //2 extra spaces

	for (BigInt i = start; i < finish; i++)
	{
		if (--printCounter == 0) // show progress
		{
			printCounter = PRINT_VALUE;
			stop = std::chrono::high_resolution_clock::now();
			auto speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(stop - start1).count();
			std::cout << '\r' << std::format(loc, "{} {}% (speed: {:L} num/sec)", i + 1, (i - start) * 100 / (finish - start), speed) << '\r'; // i+1 is to avoid showing ... 999 999 in progress print
			start1 = std::chrono::high_resolution_clock::now();
		}

		Calc3p1(i, calcData);

		sumsteps += calcData.steps;

		if (maxmaxv < calcData.maxvalue)
		{
			num1 = i;
			maxmaxv = calcData.maxvalue;
			std::cout << std::format(loc, "{:5} Number: {:>{}} | steps: {:>5L} | MAX VALUE: {:>26}", std::format("[{}]", lineCnt++),
				i, fieldWidth, calcData.steps, calcData.maxvalue) << std::endl;
		}

		if (maxsteps < calcData.steps)
		{
			num2 = i;
			maxsteps = calcData.steps;
			std::cout << std::format(loc, "{:5} Number: {:>{}} | STEPS: {:>5L} | max value: {:>26}", std::format("[{}]", lineCnt++),
				i, fieldWidth, calcData.steps, calcData.maxvalue) << std::endl;
		}
	}

	stop = std::chrono::high_resolution_clock::now();

	std::cout << "                                             \r" << std::endl; // clear progress counter

	auto calcTime = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start0).count();
	std::cout << std::format("{:<{}}: {}", "Calculation time", F_WIDTH, MillisecToStr(calcTime)) << std::endl;

	/*auto num = std::max(num1, num2);
	uint64_t dig;
	if constexpr (std::is_same<IntImpl, BigInt>::value) // for BigInt only
		dig = (uint64_t)(Length(num) * 1.35);
	else
		dig = (uint64_t)((log10(num) + 1)*1.35); // +30% for spaces between groups by 3 digits
	*/
	std::cout << std::format(loc, "{:<{}}: {:L} for number {:}", "Max Steps", F_WIDTH, maxsteps, num2) << std::endl;
	std::cout << std::format(loc, "{:<{}}: {:} for number {:}", "Max Value", F_WIDTH, maxmaxv, num1) << std::endl;
	std::cout << std::format(loc, "{:<{}}: {:L}", "Total Steps", F_WIDTH, sumsteps) << std::endl;
	std::cout << std::format(loc, "{:<{}}: {:}", "Average Steps", F_WIDTH, sumsteps / (finish - start)) << std::endl;
	std::cout << std::format(loc, "{:<{}}: {:} num/sec", "Average Speed", F_WIDTH, (finish - start) * 1000 / calcTime) << std::endl;

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

		std::cout << std::format(loc, "{:<{}}: {:L}", "Unused numbers total", F_WIDTH, unused) << std::endl;
		std::cout << std::format(loc, "{:<{}}: {}", std::format("Unused numbers (first {})", SHOW_FIRST_UNUSED), F_WIDTH, str) << std::endl;
	}
}

// calculate range of numbers WITH cache, collect and print some statistic
// might be faster but I am not sure
void ThreeN1BigInt::Calc3p1RangeCache(const BigInt& start, const BigInt& finish)
{
	uint64_t maxsteps = 0, sumsteps = 0, lineCnt = 0;
	BigInt num1 = 0ull;
	BigInt num2 = 0ull, maxmaxv = 0ull;
	CalcDataType calcData{ 0ull, 0ull };

	m_hits = 0;
	/*IntImpl range = finish - start + 1ull; // becuse both finish and start are valid numbers for checking
	m_cacheStart = start;
	m_cacheFinish = finish;
	m_valuesCache.Clear();
	m_valuesCache.SetCapacity((uint32_t)toULongLong(range));
	m_valuesCache.SetCount((uint32_t)range); // array is full of trash data after this call, use memset to set array elements into required values
	memset(m_valuesCache.GetValuePointer(0), 0, sizeof(decltype(m_valuesCache)::item_type) * range);
	*/
	std::locale loc(std::cout.getloc(), new MyGroupSeparator());

#ifdef _DEBUG
	const uint64_t PRINT_VALUE = 50'000; // print "progress" on each 50Kth number
#else
	const uint64_t PRINT_VALUE = 100'000; // print "progress" on each 100Kth number
#endif
	uint64_t printCounter = PRINT_VALUE;
	auto start0 = std::chrono::high_resolution_clock::now();
	auto start1 = start0;
	std::chrono::high_resolution_clock::time_point stop;

	size_t fieldWidth = NumLen(finish) + 2; //2 extra spaces

	for (BigInt i = start; i < finish; i++)
	{
		if (--printCounter == 0)
		{
			printCounter = PRINT_VALUE;
			stop = std::chrono::high_resolution_clock::now();
			auto speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(stop - start1).count();
			std::cout << '\r' << i + 1 << "  " << (i - start) * 100 / (finish - start) << "% (speed: " << speed << " num/sec) " << '\r'; // i+1 is to avoid showing ... 999 999 in progress print
			start1 = std::chrono::high_resolution_clock::now();
		}

		calc3p1Cache(i, calcData);
		//m_valuesCache.SetValue((uint32_t)(i - m_cacheStart), calcData); // works quicker than .AddValue()

		sumsteps += calcData.steps;

		if (maxmaxv < calcData.maxvalue)
		{
			num1 = i;
			maxmaxv = calcData.maxvalue;
			std::cout << std::format(loc, "{:5} Number: {:>{}} | steps: {:>5L} | MAX VALUE: {:>26}", std::format("[{}]", lineCnt++),
				static_cast<std::string>(i), fieldWidth, calcData.steps, static_cast<std::string>(calcData.maxvalue)) << std::endl;
		}

		if (maxsteps < calcData.steps)
		{
			num2 = i;
			maxsteps = calcData.steps;
			std::cout << std::format(loc, "{:5} Number: {:>{}} | STEPS: {:>5L} | max value: {:>26}", std::format("[{}]", lineCnt++),
				static_cast<std::string>(i), fieldWidth, calcData.steps, static_cast<std::string>(calcData.maxvalue)) << std::endl;
		}
	}

	std::cout << "                                               \r" << std::endl; // clear progress counter

	auto calcTime = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start0).count();
	std::cout << std::format(loc, "{:<{}}: {}", "Calculation time", F_WIDTH, MillisecToStr(calcTime)) << std::endl;

	/*auto num = std::max(num1, num2);
	uint64_t dig;
	if constexpr (std::is_same<IntImpl, BigInt>::value) // for BigInt only
		dig = (uint64_t)(Length(num) * 1.35);
	else
		dig = (uint64_t)((log10(num) + 1) * 1.35); // +30% for spaces between groups by 3 digits

	std::cout << std::format(loc, "Number: {:>{}L} | max steps: {}", toULongLong(num2), dig, maxsteps) << std::endl;
	std::cout << std::format(loc, "Number: {:>{}L} | max value: {}", toULongLong(num1), dig, maxmaxv) << std::endl;
*/
	std::cout << std::format(loc, "{:<{}}: {:L} | for number: {:}", "Max Steps", F_WIDTH, maxsteps, num2) << std::endl;
	std::cout << std::format(loc, "{:<{}}: {:} | for number: {:}", "Max Value", F_WIDTH, maxmaxv, num1) << std::endl;
	std::cout << std::format(loc, "{:<{}}: {:L} steps", "Total Steps", F_WIDTH, sumsteps) << std::endl;
	std::cout << std::format(loc, "{:<{}}: {:} steps", "Average Steps", F_WIDTH, sumsteps / (finish - start)) << std::endl;
	std::cout << std::format(loc, "{:<{}}: {:} num/sec", "Average Speed", F_WIDTH, (finish - start) * 1000 / calcTime) << std::endl;

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

		std::cout << std::format(loc, "{:<{}}: {:L}", "Unused Numbers Total", F_WIDTH, unused) << std::endl;
		std::cout << std::format(loc, "{:<{}}: {}", std::format(loc, "Unused Numbers (first {:L})", SHOW_FIRST_UNUSED), F_WIDTH, str) << std::endl;
	}

	std::cout << std::format(loc, "{:<{}}: {:L} items", "Cache Size", F_WIDTH, m_valuesCache.Count()) << std::endl;
	std::cout << std::format(loc, "{:<{}}: {:L} ({:.2f}%)", "Cache Hits", F_WIDTH, m_hits, (double)(100 * m_hits) / m_valuesCache.Count()) << std::endl;

}
