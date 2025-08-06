#pragma once

#include <string>
#include <cassert>
#include <fstream>

#include "DynamicArrays.h"
#include "thread_pool.h"
#include "ThreeN1Task.h"
#include "Utils.h"
#include "string_utils.h"
#include "BigInt.h"

template<typename IntImpl>
class ThreeN1Task;

#pragma pack(push, 1)
template<typename IntImpl>
struct ThreeN1Data
{
public:
	IntImpl maxvalue = 0ull;
	uint16_t steps = 0; // looks like number of steps does not exceed 1300. We allocate 0..65535 range for it to save memory.

	//Read and Write
	template<typename U>
	friend std::ostream& operator<<(std::ostream& out, const struct ThreeN1Data<U>& d);
	template<typename U>
	friend std::istream& operator>>(std::istream& in, struct ThreeN1Data<U>& d);
};
#pragma pack(pop)

template<typename IntImpl>
struct RangeData
{
	IntImpl start;
	IntImpl finish;
	IntImpl num1;
	uint64_t num1steps;
	IntImpl num2;
	IntImpl num2maxvalue;
	IntImpl errnum;
	enum ThreeN1Task<IntImpl>::TaskStatus status;
};

template<typename U>
std::ostream& operator<<(std::ostream& out, const struct ThreeN1Data<U>& d)
{
	out << d.maxvalue;
	out << d.steps;
	return out;
}

template<typename U>
std::istream& operator>>(std::istream& in, struct ThreeN1Data<U>& d)
{
	in >> d.maxvalue;
	in >> d.steps;
	return in;
}

template<typename IntImpl>
bool operator==(const struct ThreeN1Data<IntImpl>& a, const struct ThreeN1Data<IntImpl>& b)
{
	return a.steps == b.steps && a.maxvalue == b.maxvalue;
}


template<typename IntImpl>
bool operator>(const struct RangeData<IntImpl>& a, const struct RangeData<IntImpl>& b)
{
	return a.start > b.start;
}

template<typename IntImpl>
bool operator==(const struct RangeData<IntImpl>& a, const struct RangeData<IntImpl>& b)
{
	return a.start == b.start;
}

/*template<typename IntImpl>
class Compare<ThreeN1Data<IntImpl>>
{
	typedef ThreeN1Data<IntImpl> TNV;
public:
	virtual bool eq(const TNV& a, const TNV& b) const { return a.steps == b.steps && a.maxvalue == b.maxvalue; };
	virtual bool lt(const TNV& a, const TNV& b) const { return a.steps < b.steps && a.maxvalue < b.maxvalue; };
	virtual bool mt(const TNV& a, const TNV& b) const { return a.steps > b.steps && a.maxvalue > b.maxvalue; };
	virtual ~Compare() {};
};


template<typename IntImpl>
inline bool isEven(IntImpl& v)
{
	if constexpr (std::is_same<IntImpl, BigInt>::value)
		return v.IsEven();
	else
		return (v & 1ull) == 0;
}
*/



template<typename IntImpl>
class ThreeN1
{
public:
	using DataType = IntImpl;
	using CalcDataType = ThreeN1Data<IntImpl>;
	using CacheType = THArray<CalcDataType>;
private:
	bool checkInCache(const IntImpl& curr, const IntImpl& start, const IntImpl& finish, CalcDataType& calcResult);
	void calc3p1Cache(const IntImpl& start, const IntImpl& finish, const IntImpl& number, CalcDataType& calcResult);
	void rangeDataToFile(const std::string& fileName);

public:
	THArraySorted<RangeData<IntImpl>> m_rangeData;
	CacheType m_valuesCache;
	IntImpl m_cacheStart; // this is range of cached values pre-loaded from file
	IntImpl m_cacheFinish;

	uint64_t m_hits = 0;
	std::mutex m_cacheMutex;
	//const uint64_t PATHS_SIZE = 1'000'000'000ull;
	//bool* m_paths;
	MyBitset m_unused; // false in this array means that cpecified number is unused, true - is used.

	void Calc3p1(const IntImpl& number, CalcDataType& calcResult);
	void Calc3p1Range(const IntImpl& start, const IntImpl& finish);
	void Calc3p1RangeCache(const IntImpl& start, const IntImpl& finish);

	void Calc3p1allThreads(const IntImpl& start, const IntImpl& finish, uint64_t threadsCnt);
	void CacheToFileVarLen(const IntImpl& start, const std::string& fileName);
	void CacheToFileBin(const IntImpl& start, const std::string& fileName);
	void CacheFromFileVarLen(const std::string& fileName);
	void CacheFromFileVarLen2(const std::string& fileName, int64_t itemsToRead = -1);
	void CacheFromFileBin(const std::string& fileName);

	void addRangeData(RangeData<IntImpl> data)
	{
		std::lock_guard<std::mutex> lock(m_cacheMutex);
		m_rangeData.AddValue(data);
	}

	void TrackUnused(uint64_t value)
	{
		m_unused.Init(value);
	}
};

// calc ONE number WITHOUT using cache
template<typename IntImpl>
//__declspec(noinline) 
void ThreeN1<IntImpl>::Calc3p1(const IntImpl& number, CalcDataType& calcResult)
{
	calcResult.maxvalue = number;
	calcResult.steps = 0ull;
	IntImpl curr = number;

	if (curr < m_unused.BitsCount()) m_unused.setTrue(curr); //m_paths[curr] = true;

	const IntImpl OVERFLOW_LIMIT = std::numeric_limits<IntImpl>::max() / 3;
	if constexpr (std::is_same<IntImpl, BigInt>::value) // for BigInt only
	{
		while (curr != 1ull)
		{
			if (curr.IsEven() )
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

			if (curr < m_unused.BitsCount()) m_unused.setTrue(curr); //m_paths[curr] = true;
		}
	}
	else
	{
		static_assert(std::is_same<IntImpl, uint64_t>::value); // supported types only uint64_t and BigInt now

		while (curr != 1ull)
		{
			if ((curr & 1ull) == 0) // is even
			{
				curr >>= 1;
			}
			else
			{
				if(curr >= OVERFLOW_LIMIT) //if (curr >= ULLONG_MAX / 3)
					throw std::overflow_error("Overflow detected!");

				curr = (3ull * curr + 1ull) / 2;
				calcResult.steps++; // if curr is odd we do 2 operations at once and increase steps twice accordingly
				if (calcResult.maxvalue < curr) calcResult.maxvalue = curr;
			}

			calcResult.steps++;

			if (curr < m_unused.BitsCount()) m_unused.setTrue(curr); // m_paths[curr] = true;
		}
	}
}

// Declaration of BigInt specialization of template method
// Implementation should be in .cpp file
// calcs ONE number WITHOUT using cache
template<>
void ThreeN1<BigInt>::Calc3p1(const BigInt& number, CalcDataType& calcResult);


// calc ONE number WITH using cache
// that might be faster than without cache, but not sure
template<typename IntImpl>
void ThreeN1<IntImpl>::calc3p1Cache(const IntImpl& start, const IntImpl& finish, const IntImpl& number, CalcDataType& calcResult)
{
	calcResult.maxvalue = number;
	calcResult.steps = 0ull;
	IntImpl curr = number;

	if (curr < m_unused.BitsCount()) m_unused.setTrue(curr); //m_paths[curr] = true;

	const IntImpl OVERFLOW_LIMIT = std::numeric_limits<IntImpl>::max() / 3;

	if constexpr (std::is_same<IntImpl, BigInt>::value)
	{
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

			if (curr < m_unused.BitsCount()) m_unused.setTrue(curr); //m_paths[curr] = true;

			if (checkInCache(curr, start, finish, calcResult)) break;
		}
	}
	else
	{
		static_assert(std::is_same<IntImpl, uint64_t>::value); // supported types only uint64_t and BigInt now

		while (curr != 1ull)
		{
			if ((curr & 0x01) == 0) // is even
			{
				curr >>= 1;
			}
			else
			{
				if (curr >= OVERFLOW_LIMIT) //if (curr >= ULLONG_MAX / 3)
					throw std::overflow_error("Overflow detected!");

				curr = (3ull * curr + 1ull) / 2;
				calcResult.steps++; // if curr is odd we do 2 operations at once and increase steps twice accordingly
				if (calcResult.maxvalue < curr) calcResult.maxvalue = curr;
			}

			calcResult.steps++;

			if (curr < m_unused.BitsCount()) m_unused.setTrue(curr); //m_paths[curr] = true;

			if (checkInCache(curr, start, finish, calcResult)) break;
		}
	}
}


// check if curr number is in cache.
// if curr is out of cache range checkInCache returns false immediately
// if curr is IN cache range:
//   if curr is found in cache - caclResult is updated and function returns true
//   if curr is NOT found in cache then it calls calc3p1Cache to calc curr, updates calcResult with new data and returns true;
template<typename IntImpl>
bool ThreeN1<IntImpl>::checkInCache(const IntImpl& curr, const IntImpl& start, const IntImpl& finish, CalcDataType& calcResult)
{
	if (curr >= m_cacheStart && curr < m_cacheFinish)
	{
		CalcDataType& elem = m_valuesCache[(uint)toULongLong(curr - m_cacheStart)]; //TODO prefomance degradation here!!!
		if (elem.steps == 0) // we didn't meet this number earlier
		{
			CalcDataType calcRes2;
			calc3p1Cache(start, finish, curr, calcRes2);
			calcResult.steps += calcRes2.steps;
			if (calcResult.maxvalue < calcRes2.maxvalue) calcResult.maxvalue = calcRes2.maxvalue; // one if should be faster than std::max()
			//calcResult.maxvalue = std::max(calcResult.maxvalue, calcRes2.maxvalue);
			return true;
		}
		else
		{
			calcResult.steps += elem.steps;
			if (calcResult.maxvalue < elem.maxvalue) calcResult.maxvalue = elem.maxvalue; // one if should be faster than std::max()
			//calcResult.maxvalue = std::max(calcResult.maxvalue, elem.maxvalue);

			m_hits++;
			return true; // we already know steps/maxv for curr number.
		}
	}

	return false;
}

// calculate range of numbers WITHOUT using cache, collect and print some statistic
template<typename IntImpl>
void ThreeN1<IntImpl>::Calc3p1Range(const IntImpl& start, const IntImpl& finish)
{
	uint64_t maxsteps = 0, sumsteps = 0, lineCnt = 0;
	IntImpl num1 = 0ull;
	IntImpl num2 = 0ull, maxmaxv = 0ull;
	CalcDataType calcData{ 0ull, 0ull };
	std::locale loc(std::cout.getloc(), new MyGroupSeparator());

	const uint64_t PRINT_VALUE = 1'000'000; // print "progress" on each 1Mth number
	uint64_t printCounter = PRINT_VALUE;
	auto start0 = std::chrono::high_resolution_clock::now();
	auto start1 = start0;
	std::chrono::high_resolution_clock::time_point stop;

	for (IntImpl i = start; i < finish; i++)
	{
		if (--printCounter == 0) // show progress
		{
			printCounter = PRINT_VALUE;
			stop = std::chrono::high_resolution_clock::now();
			auto speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(stop - start1).count();
			std::cout << '\r' << i+1 << " (speed: " << speed <<" num/sec) " << '\r'; // i+1 is to avoid showing ... 999 999 in progress print
			start1 = std::chrono::high_resolution_clock::now();
		}

		Calc3p1(i, calcData);

		sumsteps += calcData.steps;

		if (maxmaxv < calcData.maxvalue)
		{
			num1 = i;
			maxmaxv = calcData.maxvalue;
			std::cout << std::format("[{:3}] Number: {:>25} | steps: {:>5L} | MAX VALUE: {:>25}", lineCnt++, i, calcData.steps, calcData.maxvalue) << std::endl;
			//std::cout << "number:" << i << "  steps:" << calcData.steps << "  MAX VALUE:" << calcData.maxvalue << std::endl;
		}

		if (maxsteps < calcData.steps)
		{
			num2 = i;
			maxsteps = calcData.steps;
			std::cout << std::format(loc, "[{:3}] Number: {:>25} | STEPS: {:>5L} | max value: {:>25}", lineCnt++, i, calcData.steps, calcData.maxvalue) << std::endl;
			//std::cout << "number:" << i << "  STEPS:" << calcData.steps << "  max value:" << calcData.maxvalue << std::endl;
		}
	}

	stop = std::chrono::high_resolution_clock::now();

	std::cout << "                                             \r" << std::endl; // clear progress counter

	auto calcTime = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start0).count();
	std::cout << "Total Steps: " << sumsteps << std::endl;
	std::cout << "Average Steps: " << sumsteps / (finish - start) << std::endl;
	std::cout << "Average Speed: " << (finish - start) * 1000 / calcTime << " num/sec" << std::endl;
	std::cout << "Calculation time: " << MillisecToStr(calcTime) << std::endl;

	auto num = std::max(num1, num2);
	uint64_t dig;
	if constexpr (std::is_same<IntImpl, BigInt>::value) // for BigInt only
		dig = (uint64_t)(Length(num) * 1.35);
	else
		dig = (uint64_t)((log10(num) + 1)*1.35); // +30% for spaces between groups by 3 digits 
	
	std::cout << std::format(loc, "Number: {:>{}} | max steps: {}", num2, dig, maxsteps) << std::endl;
	std::cout << std::format(loc, "Number: {:>{}} | max value: {}", num1, dig, maxmaxv) << std::endl;
	
	const uint SHOW_FIRST_UNUSED = 30;
	uint unused = 0;
	uint numOfFirst = SHOW_FIRST_UNUSED;
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

	std::cout << "Unused numbers total: " << unused << std::endl;
	std::cout << "Unused numbers (first " << SHOW_FIRST_UNUSED << "): " << str << std::endl;
	//if (std::is_same<IntImpl, uint64_t>::value)
	std::cout << "MAXULONGLONG: " << std::numeric_limits<IntImpl>::max()/* ULLONG_MAX*/ << std::endl;
}

// calculate range of numbers WITH cache, collect and print some statistic
// might be faster but I am not sure
template<typename IntImpl>
void ThreeN1<IntImpl>::Calc3p1RangeCache(const IntImpl& start, const IntImpl& finish)
{
	uint64_t maxsteps = 0, sumsteps = 0, lineCnt = 0;
	IntImpl num1 = 0ull;
	IntImpl num2 = 0ull, maxmaxv = 0ull;
	CalcDataType calcData{ 0ull, 0ull };

	m_hits = 0;
	/*IntImpl range = finish - start + 1ull; // becuse both finish and start are valid numbers for checking
	m_cacheStart = start;
	m_cacheFinish = finish;
	m_valuesCache.Clear();
	m_valuesCache.SetCapacity((uint)toULongLong(range));
	m_valuesCache.SetCount((uint)range); // array is full of trash data after this call, use memset to set array elements into required values
	memset(m_valuesCache.GetValuePointer(0), 0, sizeof(decltype(m_valuesCache)::item_type) * range);
	*/
	std::locale loc(std::cout.getloc(), new MyGroupSeparator());

	const uint64_t PRINT_VALUE = 1'000'000;
	uint64_t printCounter = PRINT_VALUE;
	auto start0 = std::chrono::high_resolution_clock::now();
	auto start1 = start0; 
	std::chrono::high_resolution_clock::time_point stop;

	for (IntImpl i = start; i < finish; i++)
	{
		if (--printCounter == 0)
		{
			printCounter = PRINT_VALUE;
			stop = std::chrono::high_resolution_clock::now();
			auto speed = PRINT_VALUE * 1000 / std::chrono::duration_cast<std::chrono::milliseconds>(stop - start1).count();
			std::cout << '\r' << i + 1 << " (speed: " << speed << " num/sec) " << '\r'; // i+1 is to avoid showing ... 999 999 in progress print
			start1 = std::chrono::high_resolution_clock::now();
		}

		calc3p1Cache(start, finish, i, calcData);
		//m_valuesCache.SetValue((uint)(i - m_cacheStart), calcData); // works quicker than .AddValue()
		
		sumsteps += calcData.steps;

		if (maxmaxv < calcData.maxvalue)
		{
			num1 = i;
			maxmaxv = calcData.maxvalue;
			std::cout << std::format(loc, "[{:3}] Number: {:>25} Steps: {:>5L} MAX VALUE: {:>25}", lineCnt++, i, calcData.steps, calcData.maxvalue) << std::endl;
			//std::cout << "number:" << i << "  steps:" << calcData.steps << "  MAX VALUE:" << calcData.maxvalue << std::endl;
		}

		if (maxsteps < calcData.steps)
		{
			num2 = i;
			maxsteps = calcData.steps;
			std::cout << std::format(loc, "[{:3}] Number: {:>25} STEPS: {:>5L} Max value: {:>25}", lineCnt++, i, calcData.steps, calcData.maxvalue) << std::endl;
			//std::cout << "number:" << i << "  STEPS:" << calcData.steps << "  max value:" << calcData.maxvalue << std::endl;
		}
	}

	std::cout << "                                             \r" << std::endl; // clear progress counter

	auto calcTime = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start0).count();
	std::cout << "Total Steps: " << sumsteps << std::endl;
	std::cout << "Average Steps: " << sumsteps / (finish - start) << std::endl;
	std::cout << "Average Speed: " << (finish - start) * 1000 / calcTime << " num/sec" << std::endl;
	std::cout << "Calculation time: " << MillisecToStr(calcTime) << std::endl;

	auto num = std::max(num1, num2);

	uint64_t dig;
	if constexpr (std::is_same<IntImpl, BigInt>::value) // for BigInt only
		dig = (uint64_t)(Length(num) * 1.35);
	else
		dig = (uint64_t)((log10(num) + 1) * 1.35); // +30% for spaces between groups by 3 digits 
	
	std::cout << std::format(loc, "Number: {:>{}L} | max steps: {}", toULongLong(num2), dig, maxsteps) << std::endl;
	std::cout << std::format(loc, "Number: {:>{}L} | max value: {}", toULongLong(num1), dig, maxmaxv) << std::endl;

	const uint SHOW_FIRST_UNUSED = 30;
	uint unused = 0;
	uint numOfFirst = SHOW_FIRST_UNUSED;
	//uint numOfFirstBy3 = 10;
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

	std::cout << "Unused numbers total: " << unused << std::endl;
	std::cout << "Unused numbers (first " << SHOW_FIRST_UNUSED << "): " << str << std::endl;
	std::cout << "Cache size: " << m_valuesCache.Count() << std::endl;
	std::cout << "Cache Hits: " << m_hits << " (" << (double)(100 * m_hits) / m_valuesCache.Count() << "%)" << std::endl;

	//if (std::is_same<IntImpl, uint64_t>::value)
	std::cout << "MAXULONGLONG: " << std::numeric_limits<IntImpl>::max()/* ULLONG_MAX*/ << std::endl;
}


// calculate big range using threads.
// rance then is divided into subranges 10'000'000 numbers each - each subrange is task for one thread
template<typename IntImpl>
void ThreeN1<IntImpl>::Calc3p1allThreads(const IntImpl& start, const IntImpl& finish, uint64_t threadsCnt)
{
	m_hits = 0;
	//CalcDataType calcData{ 0ull, 0ull };
	IntImpl range = finish - start;

#ifdef USE_VALUES_CACHE
	m_valuesCache.Clear();
	m_valuesCache.SetCapacity((uint)(toULongLong(range);// +2ull));
#endif

	//const uint64_t MIN_RANGE = 2'000'000; // if range is less than this value - use single thread mode for calculation
	//if (range < MIN_RANGE)
	//{
	//	calc3p1Range(start, finish);
	//	return;
	//}

	MT::ThreadPool thread_pool((int)threadsCnt);
	//thread_pool.set_logger_flag(true);

	// remove the pool from a pause, allowing streams to take on the tasks on the fly
	thread_pool.start();

	std::vector<std::shared_ptr<ThreeN1Task<IntImpl>>> stanbyTasks;
	const uint64_t TASK_POOL_SIZE = 100;
	const uint64_t TASK_POOL_INITIAL_SIZE = 3ull * TASK_POOL_SIZE / 2 + (threadsCnt+1); // one extra task just for sure
	const uint64_t TASK_POOL_THRESHOLD = TASK_POOL_SIZE / 2;
	const uint64_t ONE_TASK_RANGE = 10'000'000ull;

	for (int j = 0; j < TASK_POOL_INITIAL_SIZE; j++) // total number of tasks need to be 1.5 times larger 
	{
		stanbyTasks.push_back(std::make_shared<ThreeN1Task<IntImpl>>(*this));
	}

	IntImpl rangeCount = range / ONE_TASK_RANGE;

	if(rangeCount > 1'000'000)
		std::cout << "Too wide range (" << start << "," << finish << "). It may take too much time to calculate." << std::endl;

	std::osyncstream syncout(std::cout);
	syncout.imbue(std::locale(std::cout.getloc(), new MyGroupSeparator()));

	IntImpl rangeStart = start;
	uint64_t tasksCount = 0;
	while (true) //(rangeStart < finish)
	{
		bool lastRange = false;

		if (thread_pool.task_queue_size() < TASK_POOL_THRESHOLD)
		{
			syncout << "Tasks processed: " << tasksCount << std::endl;
			syncout << "Tasks in queue: " << thread_pool.task_queue_size() << std::endl;
			//syncout << "Tasks completed: " << thread_pool.tasks_completed() << std::endl;
			syncout << "Tasks standby: " << stanbyTasks.size() << std::endl;

			for (int j = 0; j < TASK_POOL_SIZE; j++, rangeStart+=ONE_TASK_RANGE) // total number of tasks need to be 1.5 times larger 
			{
				IntImpl rangeFinish = rangeStart + ONE_TASK_RANGE;
				if (rangeFinish >= finish)
				{
					rangeFinish = finish;
					lastRange = true;
				}

				std::shared_ptr<ThreeN1Task<IntImpl>> task = stanbyTasks.back(); // [stanbyTasks.size() - 1];
				stanbyTasks.pop_back();
				task->InitTask(rangeStart, rangeFinish); // put new range into exisiting (cached) task object
				thread_pool.add_task(*task);
				tasksCount++;
				if (lastRange) break; // we've added the last range for processing, stopping the loop then
			}
		}

		if (lastRange) break;

		std::this_thread::sleep_for(std::chrono::seconds(1));
		
		thread_pool.move_completed(stanbyTasks); // move completed tasks back to standbyTasks and clean up completed tasks list		
	}

	syncout << std::endl << "All tasks added. Waiting till they finished." << std::endl;

	thread_pool.wait();
	thread_pool.move_completed(stanbyTasks);

	assert(stanbyTasks.size() == TASK_POOL_INITIAL_SIZE);

	syncout << "ALL TASKS COMPLETED" << std::endl;

	thread_pool.stop();

	//syncout << "number:" << num2 << "  max steps:" << maxsteps << std::endl;
	//syncout << "number:" << num1 << "  max value:" << maxmaxv << std::endl;
#ifdef USE_VALUES_CACHE
	syncout << "valuesCache.size:" << m_valuesCache.Count() << std::endl;
	syncout << "hits:" << m_hits << std::endl;
#endif
	//syncout << "sumsteps:" << sumsteps << std::endl;
	syncout << "MAXULONGLONG:" << std::numeric_limits<IntImpl>::max()/* ULLONG_MAX*/ << std::endl;
}

template<typename IntImpl>
void ThreeN1<IntImpl>::CacheToFileBin(const IntImpl& start, const std::string& fileName)
{
	std::ofstream f;
	f.open(fileName, std::ios::out | std::ios::binary);
	if (f.fail())
	{
		//cout << "Cannot open file '" << fileTo << "' for writing, exiting." << endl;
		throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");
	}

	f << start;
	//f.write((const char*)&start, sizeof(start));  // saving start number, all subsequent numbers will be get by +1 to start
	IntImpl cnt = (IntImpl)m_valuesCache.Count();
	f << cnt;
	//f.write((const char*)&cnt, sizeof(cnt));  // saving expected number of items in a file

	for (uint64_t i = 0; i < cnt; ++i)
	{
		CalcDataType& val = m_valuesCache[i];
		assert(val.steps < 65536);
		f << val;
		//f.write((char*)&val, sizeof(val));
	}

	f.flush();
	f.close();
}

template<typename IntImpl>
void ThreeN1<IntImpl>::CacheFromFileBin(const std::string& fileName)
{
	std::ifstream f;
	f.open(fileName, std::ios::out | std::ios::binary);
	if (f.fail())
	{
		//cout << "Cannot open file '" << fileTo << "' for writing, exiting." << endl;
		throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");
	}

	IntImpl start, cnt;
	f >> start;
	f >> cnt;
	//f.read(&start, sizeof(IntImpl));
	//f.read(&cnt, sizeof(IntImpl));

	m_valuesCache.SetCapacity((uint)toULongLong(cnt)); //TODO performance degradation here!!! 
	CalcDataType val;
	while (true)
	{
		f >> val;
		//f.read(&val, sizeof(CalcDataType));
		m_valuesCache.AddValue(val);
		cnt--;
		if (f.eof()) break; // if we've met EOF earlier than expected 
	}

	assert(cnt == 0ull);

	f.close();
}

template<typename IntImpl>
void ThreeN1<IntImpl>::rangeDataToFile(const std::string& fileName)
{
	std::ofstream f;
	//fout.exceptions(/*ifstream::failbit |*/ ifstream::badbit);
	f.open(fileName, std::ios::out | std::ios::binary);
	if (f.fail())
		throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");

	for (uint64_t i = 0; i < m_rangeData.Count(); ++i)
	{
		RangeData<IntImpl> data = m_rangeData[i];

		f << data.start;
		f << ',';
		f << data.finish;
		f << ',';
		f << data.num1;
		f << '=';
		f << data.num1steps;
		f << ',';
		f << data.num2;
		f << '=';
		f << data.num2maxvalue;
		f << " (";
		f << NumLen(data.num2maxvalue);
		f << "dig)";
		if (data.status == ThreeN1Task<IntImpl>::TaskStatus::error) f << " *ERROR* errnum:" << data.errnum;
		f << "\n";
	}

	f.flush();
	f.close();
}
