#pragma once

#include <string>
#include <cassert>
#include <fstream>

#include "DynamicArrays.h"
#include "thread_pool.h"
#include "ThreeN1Task.h"
#include "Utils.h"
#include "include/string_utils.h"
#include "BigInt.h"

template<typename IntImpl>
class ThreeN1Task;

#pragma pack(push, 1)
template<typename IntImpl>
struct ThreeN1Data
{
	// Optimization - initialization of fields intentionally skipped here
	// array of ThreeN1Data will be initialized later by single memset call
	IntImpl maxvalue; // = 0ull; 
	uint16_t steps; // = 0; // looks like number of steps does not exceed 1300. We allocate 0..65535 range for it to save memory.

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

*/


template<typename IntImpl>
class IThreeN1
{
public:
	static const uint64_t MAX_STEPS = 2000; // max number of steps for one number
	using DataType = IntImpl;
	using CalcDataType = ThreeN1Data<IntImpl>;
	using CacheType = THArray<CalcDataType>;

protected:
	bool checkInCache(const IntImpl& curr, CalcDataType& calcResult);
	virtual void calc3p1Cache(const IntImpl& number, CalcDataType& calcResult) = 0;
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

	virtual void Calc3p1(const IntImpl& number, CalcDataType& calcResult) = 0;
	virtual void Calc3p1(const IntImpl& number, std::vector<IntImpl>& chain, uint64_t& steps, IntImpl& maxNum) = 0;
	virtual void Calc3p1Range(const IntImpl& start, const IntImpl& finish) = 0;
	virtual void Calc3p1RangeCache(const IntImpl& start, const IntImpl& finish) = 0;

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

class ThreeN1Int64: public IThreeN1<uint64_t>
{
public:
	using CalcDataType64 = ThreeN1Data<uint64_t>;
	const uint64_t OVERFLOW_LIMIT = std::numeric_limits<uint64_t>::max() / 3;
protected:
	void calc3p1Cache(const uint64_t& number, CalcDataType64& calcResult) override;
public:
	void Calc3p1(const uint64_t& number, CalcDataType64& calcResult) override;
	void Calc3p1(const uint64_t& number, std::vector<uint64_t>& chain, uint64_t& steps, uint64_t& maxNum) override;
	void Calc3p1Range(const uint64_t& start, const uint64_t& finish) override;
	void Calc3p1RangeCache(const uint64_t& start, const uint64_t& finish) override;
};

class ThreeN1BigInt : public IThreeN1<BigInt>
{
public:
	using CalcDataTypeBigInt = ThreeN1Data<BigInt>;
protected:
	void calc3p1Cache(const BigInt& number, CalcDataTypeBigInt& calcResult) override;
public:
	void Calc3p1(const BigInt& number, CalcDataTypeBigInt& calcResult) override;
	void Calc3p1(const BigInt& number, std::vector<BigInt>& chain, uint64_t& steps, BigInt& maxNum) override;
	void Calc3p1Range(const BigInt& start, const BigInt& finish) override;
	void Calc3p1RangeCache(const BigInt& start, const BigInt& finish) override;
};




// check if curr number is in cache.
// if curr is out of cache range checkInCache returns false immediately
// if curr is IN cache range:
//   if curr is found in cache - caclResult is updated and function returns true
//   if curr is NOT found in cache then it calls calc3p1Cache to calc curr, updates calcResult with new data and returns true;
template<typename IntImpl>
bool IThreeN1<IntImpl>::checkInCache(const IntImpl& curr, CalcDataType& calcResult)
{
	if (curr >= m_cacheStart && curr < m_cacheFinish)
	{
		CalcDataType& elem = m_valuesCache[(uint32_t)toULongLong(curr - m_cacheStart)]; //TODO prefomance degradation here!!!
		if (elem.steps == 0) // we didn't meet this number earlier
		{
			CalcDataType calcRes2;
			calc3p1Cache(curr, calcRes2);
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



// calculate big range using threads.
// rance then is divided into subranges 10'000'000 numbers each - each subrange is task for one thread
template<typename IntImpl>
void IThreeN1<IntImpl>::Calc3p1allThreads(const IntImpl& start, const IntImpl& finish, uint64_t threadsCnt)
{
	m_hits = 0;
	//CalcDataType calcData{ 0ull, 0ull };
	IntImpl range = finish - start;

#ifdef USE_VALUES_CACHE
	m_valuesCache.Clear();
	m_valuesCache.SetCapacity((uint32_t)(toULongLong(range);// +2ull));
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
	
	std::locale loc(std::cout.getloc(), new MyGroupSeparator());
	std::osyncstream syncout(std::cout);
	syncout.imbue(loc); //std::locale(std::cout.getloc(), new MyGroupSeparator())

	IntImpl rangeStart = start;
	uint64_t tasksCount = 0;
	while (true) //(rangeStart < finish)
	{
		bool lastRange = false;

		if (thread_pool.task_queue_size() < TASK_POOL_THRESHOLD)
		{
			syncout << std::format(loc, "{:<{}}: {:L}", "Tasks processed", F_WIDTH, tasksCount) << std::endl;
			syncout << std::format(loc, "{:<{}}: {:L}", "Tasks in queue", F_WIDTH, thread_pool.task_queue_size()) << std::endl;
			//syncout << "Tasks completed: " << thread_pool.tasks_completed() << std::endl;
			syncout << std::format(loc, "{:<{}}: {:L}", "Tasks standby", F_WIDTH, stanbyTasks.size()) << std::endl;

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
void IThreeN1<IntImpl>::CacheToFileBin(const IntImpl& start, const std::string& fileName)
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
void IThreeN1<IntImpl>::CacheFromFileBin(const std::string& fileName)
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

	m_valuesCache.SetCapacity((uint32_t)toULongLong(cnt)); //TODO performance degradation here!!! 
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
void IThreeN1<IntImpl>::rangeDataToFile(const std::string& fileName)
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
