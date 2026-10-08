#pragma once

#include <string>
#include <cassert>
#include <fstream>

#include "utils/include/string_utils.h"
#include "utils/include/Ticks.h"
#include "DynamicArrays.h"
#include "thread_pool.h"
#include "ThreeN1Task.h"
#include "Utils.h"
#include "BigInt.h"
#include "ttmath/ttmath.h"

#pragma pack(push, 1)
template<typename IntImpl>
struct ThreeN1Data
{
	// Optimization - initialization of fields intentionally skipped here
	// array of ThreeN1Data will be initialized later by single memset call
	IntImpl maxvalue; // = 0ull; 
	uint16_t steps = 0; // looks like number of steps does not exceed 1300. We allocate 0..65535 range for it to save memory.

	//Read and Write
	template<typename U>
	friend std::ostream& operator<<(std::ostream& out, const struct ThreeN1Data<U>& d);

	template<typename U>
	friend std::istream& operator>>(std::istream& in, struct ThreeN1Data<U>& d);
};
#pragma pack(pop)


template<typename U>
std::ostream& operator<<(std::ostream& out, const struct ThreeN1Data<U>& d)
{
	out << d.maxvalue; // we have overloaded operator << for BigInt that saves number in binary format
	out << d.steps;
	return out;
}

template<typename U>
std::istream& operator>>(std::istream& in, struct ThreeN1Data<U>& d)
{
	in >> d.maxvalue;// we have overloaded operator >> for BigInt that can properly load number in binary format
	in >> d.steps;
	return in;
}

// Oprator == is needed by THArray to successfully compile functions like IndexOf()
template<typename IntImpl>
bool operator==(const struct ThreeN1Data<IntImpl>& a, const struct ThreeN1Data<IntImpl>& b)
{
	return a.steps == b.steps && a.maxvalue == b.maxvalue;
}

template<typename IntImpl>
struct StatData
{
	IntImpl num1;
	uint64_t num1steps;
	IntImpl num2;
	IntImpl num2maxvalue;
	uint64_t sumsteps; // sumsteps is not going to exceed uint64_t because max range we are able to calculate is ~10^12 * avg_steps = 10^15. it still less than uint64_t
	uint64_t numcount; // =range
	long long calctime;
};

template<typename IntImpl>
class IThreeN1
{
public:
	static const uint64_t MAX_STEPS = 2000; // max number of steps for one number

	using DataType = IntImpl;
	using StatDataType = StatData<DataType>;
	using CalcDataType = ThreeN1Data<IntImpl>;
	using CacheItemType = ThreeN1Data<uint64_t>;
	using CacheType = THArray<CacheItemType>; // cache is always uintt64_t
protected:
	std::mutex m_rangeMutex;
	MyBitset m_unused; // false in this array means that cpecified number is unused, true - is used.
	uint64_t m_hits = 0;
	THArraySorted<RangeData<IntImpl>> m_rangeData;

	virtual void calc3p1CacheUpdate(const IntImpl& number, CalcDataType& calcResult) = 0;
	void rangeDataToFile(const std::string& fileName);
	virtual void printCalcResults(StatDataType& stat);
public:
	CacheType m_valuesCache;
	uint64_t m_cacheStart = 0;  // this is range of cached values pre-loaded from file
	uint64_t m_cacheFinish = 0;

	virtual void Calc3p1(const IntImpl& number, CalcDataType& calcResult) = 0;
	virtual void Calc3p1(const IntImpl& number, std::vector<IntImpl>& chain, uint64_t& steps, IntImpl& maxNum) const = 0;
	virtual void Calc3p1Cache(const IntImpl& number, CalcDataType& calcResult) = 0;
	virtual void Calc3p1Range(const IntImpl& start, const IntImpl& finish) = 0;
	virtual void Calc3p1RangeCache(const IntImpl& start, const IntImpl& finish) = 0;
	virtual StatDataType Calc3p1RangeCacheUpdate(const IntImpl& start, const IntImpl& finish) = 0;

	void Calc3p1allThreads(const IntImpl& start, const IntImpl& finish, uint64_t threadsCnt);
	
	void SaveCacheToFileVarLen(const std::string& fileName);
	void SaveCacheToFileBin(const std::string& fileName);
	void SaveCacheToFileTxt(const std::string& fileName);
	void LoadCacheFromFileVarLen(const std::string& fileName, int64_t itemsToRead = -1);
	void LoadCacheFromFileTxt(const std::string& fileName);
	//void LoadCacheFromFileVarLen1(const std::string& fileName);

	void AddRangeData(const RangeData<IntImpl>& data)
	{
		std::lock_guard<std::mutex> lock(m_rangeMutex);
		m_rangeData.AddValue(data);
	}

	void TrackUnused(uint64_t value) { m_unused.Init(value); }
};

class ThreeN1Int64: public IThreeN1<uint64_t>
{
public:
	using CalcDataType64 = ThreeN1Data<DataType>;
	const uint64_t OVERFLOW_LIMIT = std::numeric_limits<uint64_t>::max() / 3;
protected:
	void calc3p1CacheUpdate(const uint64_t& number, CalcDataType64& calcResult) override;
public:
	void Calc3p1(const uint64_t& number, CalcDataType64& calcResult) override;
	void Calc3p1(const uint64_t& number, std::vector<DataType>& chain, uint64_t& steps, uint64_t& maxNum) const override;
	void Calc3p1Cache(const uint64_t& number, CalcDataType64& calcResult) override;
	void Calc3p1Range(const uint64_t& start, const uint64_t& finish) override;
	void Calc3p1RangeCache(const uint64_t& start, const uint64_t& finish) override;
	StatDataType Calc3p1RangeCacheUpdate(const uint64_t& start, const uint64_t& finish) override;
};

class ThreeN1BigInt : public IThreeN1<BigInt>
{
public:
	using CalcDataTypeBigInt = ThreeN1Data<BigInt>;
protected:
	void calc3p1CacheUpdate(const BigInt& number, CalcDataTypeBigInt& calcResult) override;
public:
	void Calc3p1(const BigInt& number, CalcDataTypeBigInt& calcResult) override;
	void Calc3p1(const BigInt& number, std::vector<BigInt>& chain, uint64_t& steps, BigInt& maxNum) const override;
	void Calc3p1Cache(const BigInt& number, CalcDataTypeBigInt& calcResult) override;
	void Calc3p1Range(const BigInt& start, const BigInt& finish) override;
	void Calc3p1RangeCache(const BigInt& start, const BigInt& finish) override;
	StatDataType Calc3p1RangeCacheUpdate(const BigInt& start, const BigInt& finish) override;
};

class ThreeN1TTMath : public IThreeN1<TTMathBigInt>
{
public:
	using CalcDataTypeTTMath = ThreeN1Data<TTMathBigInt>;
protected:
	void calc3p1CacheUpdate(const TTMathBigInt& number, CalcDataTypeTTMath& calcResult) override;
public:
	void Calc3p1(const TTMathBigInt& number, CalcDataTypeTTMath& calcResult) override;
	void Calc3p1(const TTMathBigInt& number, std::vector<TTMathBigInt>& chain, uint64_t& steps, TTMathBigInt& maxNum) const override;
	void Calc3p1Cache(const TTMathBigInt& number, CalcDataTypeTTMath& calcResult) override;
	void Calc3p1Range(const TTMathBigInt& start, const TTMathBigInt& finish) override;
	void Calc3p1RangeCache(const TTMathBigInt& start, const TTMathBigInt& finish) override;
	StatDataType Calc3p1RangeCacheUpdate(const TTMathBigInt& start, const TTMathBigInt& finish) override;
};


// calculate big range using threads.
// rance then is divided into subranges 10'000'000 numbers each - each subrange is task for one thread
template<typename IntImpl>
void IThreeN1<IntImpl>::Calc3p1allThreads(const IntImpl& start, const IntImpl& finish, uint64_t threadsCnt)
{
	Ticks::Start("calctime");
	m_hits = 0;
	IntImpl range = finish - start;

	//const uint64_t MIN_RANGE = 2'000'000; // if range is less than this value - use single thread mode for calculation
	//if (range < MIN_RANGE)
	//{
	//	calc3p1Range(start, finish);
	//	return;
	//}

	MT::ThreadPool thread_pool((int)threadsCnt);
	//thread_pool.set_logger_flag(true);

	std::vector<ThreeN1Task<IntImpl>*> standbyTasks;
	const uint64_t TASK_POOL_SIZE = 100;
	const uint64_t TASK_POOL_INITIAL_SIZE = 3ull * TASK_POOL_SIZE / 2 + (threadsCnt+1); // one extra task just for sure
	const uint64_t TASK_POOL_THRESHOLD = TASK_POOL_SIZE / 2; // add more tasks into pool when queue of tasks becomes less than this threshold
#ifdef _DEBUG
	const uint64_t ONE_TASK_RANGE = 1'000'000ull;
#else
	const uint64_t ONE_TASK_RANGE = 100'000ull;
#endif

	//ThreeN1Task<IntImpl> toCopy(*this);
	//standbyTasks.insert(standbyTasks.begin(), TASK_POOL_INITIAL_SIZE, &toCopy);

	for (int j = 0; j < TASK_POOL_INITIAL_SIZE; j++) // total number of tasks need to be 1.5 times larger 
	{
		standbyTasks.push_back(new ThreeN1Task<IntImpl>(*this));
	}

	std::osyncstream syncout(std::cout);
	syncout.imbue(ThreeN1::Locale);

	IntImpl rangeCount = range / ONE_TASK_RANGE;
	if(rangeCount > 1'000'000)
		std::cout << "Too wide range (" << start << "," << finish << "). It may take too much time to calculate." << std::endl;

	// remove the pool from a pause, allowing streams to take on the tasks on the fly
	thread_pool.start();

	std::cout << std::endl;

	IntImpl rangeStart = start;
	uint64_t tasksCount = 0;
	
	auto cycleStart = std::chrono::high_resolution_clock::now();
	while (true)
	{
		bool lastRange = false;

		if (thread_pool.task_queue_size() < TASK_POOL_THRESHOLD)
		{
			for (int j = 0; j < TASK_POOL_SIZE; j++, rangeStart+=ONE_TASK_RANGE) // total number of tasks need to be 1.5 times larger 
			{
				IntImpl rangeFinish = rangeStart + ONE_TASK_RANGE;
				if (rangeFinish >= finish)
				{
					rangeFinish = finish;
					lastRange = true;
				}

				auto task = standbyTasks.back();
				standbyTasks.pop_back();
				task->InitTask(rangeStart, rangeFinish); // put new range into exisiting (cached) task object
				thread_pool.add_task(task);
				tasksCount++;
				if (lastRange) break; // we've added the last range for processing, stopping the loop then
			}

			auto tasksInQueue = thread_pool.task_queue_size();
			syncout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Tasks total", F_WIDTH, rangeCount) << std::endl;
			syncout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Tasks finished", F_WIDTH, tasksCount - tasksInQueue) << std::endl;
			syncout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Tasks in queue", F_WIDTH, tasksInQueue) << std::endl;
			syncout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Tasks standby", F_WIDTH, standbyTasks.size()) << std::endl;
			syncout << std::format(ThreeN1::Locale, "{:<{}}: {}",   "Time spent", F_WIDTH, MillisecToStr(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - cycleStart).count())) << std::endl;
			syncout << std::endl;
			syncout.emit();
			cycleStart = std::chrono::high_resolution_clock::now();
		}

		if (lastRange) break;

		std::this_thread::sleep_for(std::chrono::seconds(1));
		
		thread_pool.move_completed(standbyTasks); // move completed tasks back to standbyTasks and clean up completed tasks list		
	}

	syncout << "All tasks added. Waiting till they finished." << std::endl;
	syncout.emit();

	thread_pool.wait();
	thread_pool.move_completed(standbyTasks);

	assert(standbyTasks.size() == TASK_POOL_INITIAL_SIZE);

	for (auto item: standbyTasks) delete item;

	standbyTasks.clear();

	std::cout << "ALL TASKS COMPLETED" << std::endl;

	thread_pool.stop();

	uint64_t maxsteps = 0;
	uint64_t sumsteps = 0;
	IntImpl maxmaxv{};
	IntImpl snum{}, vnum{};

	for (auto& item : m_rangeData)
	{
		if (maxsteps < item.num1steps)   maxsteps = item.num1steps, snum = item.num1;
		if (maxmaxv < item.num2maxvalue) maxmaxv = item.num2maxvalue, vnum = item.num2;
		sumsteps += item.sumsteps;
	}

	IntImpl bigss = sumsteps; // to avoid compiler error when uint64_t divided by IntImpl
	auto calcTime = Ticks::Finish("calctime"); 
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {}", "Calculation time", F_WIDTH, MillisecToStr(calcTime)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} for number: {:L}", "Max Steps", F_WIDTH, maxsteps, snum) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} for number: {:L}", "Max Value", F_WIDTH, maxmaxv, vnum) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Total Steps", F_WIDTH, sumsteps) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} steps", "Average Steps", F_WIDTH,  bigss / range) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} num/sec", "Average Speed", F_WIDTH, range * 1000 / calcTime) << std::endl;

	bool showErrMess = true;
	for (auto& item : m_rangeData)
	{
		if (item.status != MT::Task::TaskStatus::completed)
		{
			if (showErrMess) std::cout << "There are tasks finished wiht a error" << std::endl, showErrMess = false;
			std::cout << std::format(ThreeN1::Locale, "Number {:L} cannot be calculated, approriate task #{:L} has finished with error", item.errnum, item.taskid) << std::endl;
		}
	}
	//std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "MAXULONGLONG", F_WIDTH, std::numeric_limits<uint64_t>::max()) << std::endl;
}

template<typename IntImpl>
void IThreeN1<IntImpl>::printCalcResults(StatDataType& stat)
{
	std::cout << std::format("{:<{}}: {}", "Calculation time", F_WIDTH, MillisecToStr(stat.calctime)) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} for number {:L}", "Max Steps", F_WIDTH, stat.num1steps, stat.num1) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} for number {:L}", "Max Value", F_WIDTH, stat.num2maxvalue, stat.num2) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L}", "Total Steps", F_WIDTH, stat.sumsteps) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} steps", "Average Steps", F_WIDTH, stat.sumsteps / stat.numcount) << std::endl;
	std::cout << std::format(ThreeN1::Locale, "{:<{}}: {:L} numbers per sec", "Average Speed", F_WIDTH, stat.numcount * 1000 / stat.calctime) << std::endl;
	
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
		std::cout << std::format("{:<{}}: {}", std::format(ThreeN1::Locale, "Unused numbers (first {:L})", SHOW_FIRST_UNUSED), F_WIDTH, str) << std::endl;
	}
}

// еще оптимизация - держать кеш в диапазоне up/2....up. где up верхняя граница кеша.
// держать кеш ниже чем up/2 нету смысла туда никогда не зайдем.
// например диапазон 1G...2G 
template<typename IntImpl>
void IThreeN1<IntImpl>::SaveCacheToFileVarLen(const std::string& fileName)
{
	std::ofstream f;
	f.open(fileName, std::ios::out | std::ios::binary);
	if (f.fail())
		throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");

	const uint64_t BUF_LEN = 100'000'000; // save by blocks of 100М size
	const uint64_t WRITE_THRESHOLD = 9 + 1; // 9 is a max length of encoded uin64_t number, add one extra byte for sure

	uint8_t* buf = new uint8_t[BUF_LEN];

	uint64_t offset = var_len_encode(buf, m_cacheStart);
	f.write((const char*)buf, offset);  // saving start number, all subsequent numbers will be get by +1 to start

	uint64_t cnt = m_valuesCache.Count();
	offset = var_len_encode(buf, cnt);
	f.write((const char*)buf, offset);  // saving  number of items in cache 

	offset = 0;
	for (uint i = 0; i < m_valuesCache.Count(); ++i)
	{
		CacheItemType& val = m_valuesCache[i];
		assert(val.steps < MAX_STEPS);
		offset += var_len_encode(buf + offset, (uint64_t)val.steps); //TODO replace buf+offset by bufcurr pointer
		offset += var_len_encode(buf + offset, val.maxvalue);

		if (offset > BUF_LEN - WRITE_THRESHOLD)
		{
			f.write((char*)buf, offset);
			offset = 0;
		}
	}

	f.write((char*)buf, offset);

	delete[] buf;

	f.flush();
	f.close();
}

/*
template<typename IntImpl>
void IThreeN1<IntImpl>::LoadCacheFromFileVarLen1(const std::string& fileName)
{
	std::ifstream f;
	f.open(fileName, std::ios::in | std::ios::binary);
	if (f.fail())
		throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");

	uint64_t start, cnt;
	uint8_t buf[9];

	size_t maxSize = VarLenReadBuf(f, buf);
	size_t res = var_len_decode(buf, maxSize, &start);
	assert(res > 0);
	m_cacheStart = start;

	maxSize = VarLenReadBuf(f, buf);
	res = var_len_decode(buf, maxSize, &cnt);
	assert(res > 0);
	m_cacheFinish = start + cnt;

	m_valuesCache.SetCapacity((uint)(cnt));
	CacheItemType val;	
	while (true)
	{
		maxSize = VarLenReadBuf(f, buf);
		if (f.eof()) break;

		uint64_t tmp;
		res = var_len_decode(buf, maxSize, &tmp);
		assert(res > 0);
		assert(tmp < MAX_STEPS);
		val.steps = (uint16_t)tmp;

		maxSize = VarLenReadBuf(f, buf);
		if (f.eof()) break;

		res = var_len_decode(buf, maxSize, &tmp);
		assert(res > 0);
		assert(tmp >= start);
		val.maxvalue = tmp;

		m_valuesCache.AddValue(val);

		cnt--;
	}

	assert(cnt == 0);

	f.close();
}
*/

// optimised method for loading big files.
// it loads data by big chunks and then works with data in memory
template<typename IntImpl>
void IThreeN1<IntImpl>::LoadCacheFromFileVarLen(const std::string& fileName, int64_t itemsToRead)
{
	std::ifstream f;
	f.open(fileName, std::ios::out | std::ios::binary);
	if (f.fail())
		throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");

	const uint64_t BUF_LEN = 100'000'000; // read file by 100M blocks
	const uint64_t READ_THRESHOLD = 9 + 1; // 9 is a max length of encoded uin64_t number, add one extra byte for sure

	uint8_t* buf = new uint8_t[BUF_LEN];

	uint64_t start, cnt = 0;

	size_t maxSize = VarLenReadBuf(f, buf); // read one number
	size_t res = var_len_decode(buf, maxSize, &start);
	assert(res > 0);
	m_cacheStart = start;

	maxSize = VarLenReadBuf(f, buf); // read one number
	res = var_len_decode(buf, maxSize, &cnt);
	assert(res > 0);

	// reading up to itemsToRead items from cache file
	if (itemsToRead != -1) cnt = std::min(cnt, (uint64_t)itemsToRead);
	m_cacheFinish = m_cacheStart + cnt;

	m_valuesCache.Clear();
	m_valuesCache.SetCapacity((uint)cnt);
	
	if (cnt == 0ull) return;

	CacheItemType val;
	size_t offset = 0;
	size_t actualBufSize = BUF_LEN;

	f.read((char*)buf, BUF_LEN);

	while (true)
	{
		assert(actualBufSize >= READ_THRESHOLD);
		if ((offset > actualBufSize - READ_THRESHOLD) && !f.eof())
		{
			size_t remainde = actualBufSize - offset;
			memcpy(buf, buf + offset, remainde); // move remainding bytes into beginning of the buffer
			f.read((char*)(buf + remainde), BUF_LEN - remainde);
			actualBufSize = f.gcount() + remainde; // real number of bytes read
			offset = 0;
		}

		uint64_t tmp;
		res = var_len_decode(buf + offset, 9, &tmp);
		assert(res > 0);
		assert(tmp < MAX_STEPS);
		val.steps = (uint16_t)tmp;
		offset += res;

		res = var_len_decode(buf + offset, 9, &tmp);
		assert(res > 0);
		assert(tmp >= start);
		val.maxvalue = tmp;

		offset += res;

		m_valuesCache.AddValue(val);

		cnt--;
		if (cnt == 0ull) break;
		if (f.eof() && (offset >= actualBufSize)) break;
	}

	delete[] buf;

	assert(cnt == 0ull);

	f.close();
}

template<typename IntImpl>
void IThreeN1<IntImpl>::SaveCacheToFileBin(const std::string& fileName)
{
	//auto fileName = getCacheFileName(BIN_EXT);

	std::ofstream f;
	f.open(fileName, std::ios::out | std::ios::binary);
	if (f.fail())
		throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");

	uint64_t cnt = m_valuesCache.Count();
	f.write((char*)&m_cacheStart, sizeof(m_cacheStart));
	f.write((char*)&cnt, sizeof(cnt));  // saving expected number of items in a file

	for (uint32_t i = 0; i < m_valuesCache.Count(); ++i)
	{
		CacheItemType& val = m_valuesCache[i];
		assert(val.steps < MAX_STEPS);
		f.write((char*)&val.maxvalue, sizeof(val.maxvalue));
		f.write((char*)&val.steps, sizeof(val.steps));
	}

	f.flush();
	f.close();
}

template<typename IntImpl>
void IThreeN1<IntImpl>::SaveCacheToFileTxt(const std::string& fileName)
{
	std::ofstream f;
	f.open(fileName, std::ios::out | std::ios::binary);
	if (f.fail())
		throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");

	f << m_cacheStart; 

	uint64_t cnt = m_valuesCache.Count();
	f << cnt;
	
	for (uint32_t i = 0; i < m_valuesCache.Count(); ++i)
	{
		CacheItemType& val = m_valuesCache[i];
		assert(val.steps < MAX_STEPS);
		f << val;
	}

	f.flush();
	f.close();
}

template<typename IntImpl>
void IThreeN1<IntImpl>::LoadCacheFromFileTxt(const std::string& fileName)
{
	std::ifstream f;
	f.open(fileName, std::ios::in | std::ios::binary);
	if (f.fail())
		throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");

	uint64_t start, cnt; // cnt has IntImpl type intentionally
	f >> start;
	f >> cnt;

	m_valuesCache.SetCapacity(cnt);
	CacheItemType val;
	while (true)
	{
		f >> val; // CacheItemType has overloaded operator >>
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



template<>
struct std::formatter<TTMathBigInt> : std::formatter<std::string_view>
{
private:
	static constexpr std::size_t max_spec_size = 256;
public:
	constexpr auto parse(std::format_parse_context& ctx)
	{
		auto begin = ctx.begin();
		const auto end = ctx.end();

		// Находим закрывающую '}' именно текущего replacement field.
		// Вложенные {} возможны, например, в динамической ширине.
		auto close = begin;
		std::size_t nested = 0;

		for (; close != end; ++close)
		{
			if (*close == '{')
			{
				++nested;
			}
			else if (*close == '}')
			{
				if (nested == 0)
					break;

				--nested;
			}
		}

		if (close == end)
			throw std::format_error("unterminated BigInt format specification");

		/*
		 * Для строковой спецификации L может находиться:
		 *
		 *   непосредственно перед '}'
		 *   непосредственно перед 's'
		 *   непосредственно перед '?'
		 *
		 * Примеры:
		 *
		 *   {:L}
		 *   {:>20L}
		 *   {:>20Ls}
		 */
		auto localeSpecifier = close;

		if (localeSpecifier != begin)
		{
			--localeSpecifier;

			// Если указан строковый presentation type,
			// проверяем символ перед ним.
			if ((*localeSpecifier == 's' || *localeSpecifier == '?') && localeSpecifier != begin)
			{
				--localeSpecifier;
			}
		}

		// Если L отсутствует, можно напрямую вызвать базовый parse().
		// Это важно в том числе для динамической ширины и точности.
		if (localeSpecifier == close || *localeSpecifier != 'L')
		{
			return std::formatter<std::string_view>::parse(ctx);
		}

		/*
		 * Создаём копию спецификации без L.
		 *
		 * Например:
		 *
		 *   >20L}  -> >20}
		 *   *^20Ls} -> *^20s}
		 */
		std::array<char, max_spec_size> filtered{};
		std::size_t size = 0;

		for (auto it = begin; it != close; ++it)
		{
			if (it == localeSpecifier)
				continue;

			if (size + 1 >= filtered.size())
			{
				throw std::format_error("TTMathBigInt format specification is too long");
			}

			filtered[size++] = *it;
		}

		// Базовый formatter ожидает увидеть закрывающую '}'.
		filtered[size++] = '}';

		std::format_parse_context filteredContext{ std::string_view{filtered.data(), size} };

		// Базовый string formatter сохраняет внутри себя ширину, выравнивание, fill, precision и presentation type.
		const auto result = std::formatter<std::string_view>::parse(filteredContext);

		// Убеждаемся, что базовый formatter дошёл до '}'.
		if (result == filteredContext.end() || *result != '}')
		{
			throw std::format_error("invalid TTMathBigInt format specification");
		}

		// Возвращаем итератор исходного контекста, а не временного.
		return close;
	}

	auto format(const TTMathBigInt& value, std::format_context& ctx) const
	{
		const std::string digits = value.ToString();

		std::string result;
		result.reserve(digits.size() + digits.size() / 3);

		std::size_t firstDigit = 0;

		// Знак не должен участвовать в группировке цифр.
		if (!digits.empty() && (digits.front() == '-' || digits.front() == '+'))
		{
			result += digits.front();
			firstDigit = 1;
		}

		const std::size_t digitCount = digits.size() - firstDigit;

		for (std::size_t i = 0; i < digitCount; ++i)
		{
			if (i != 0 && (digitCount - i) % 3 == 0)
				result += ' ';

			result += digits[firstDigit + i];
		}

		/*
		 * Здесь применяются настройки, которые базовый
		 * formatter<string_view> сохранил в parse():
		 *
		 * - fill;
		 * - align;
		 * - width;
		 * - precision;
		 * - presentation type.
		 */
		return std::formatter<std::string_view>::format(result, ctx);
	}
};
