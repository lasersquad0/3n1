#pragma once

#include <string>
#include <syncstream>
#include <locale>
#include "BigInt.h"
#include "ThreeN1.h"
#include "thread_pool.h"
#include "Utils.h"


template<typename IntImpl>
class IThreeN1;

template<typename IntImpl>
struct StatData;

template<typename IntImpl>
struct RangeData
{
	IntImpl start;
	IntImpl finish;
	IntImpl num1;
	uint64_t num1steps;
	IntImpl num2;
	IntImpl num2maxvalue;
	uint64_t sumsteps;
	IntImpl errnum;
	enum MT::Task::TaskStatus status;
};


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

template<typename IntImpl>
class ThreeN1Task : public MT::Task
{
private:
	IntImpl m_start{}, m_end{};  // current range
	IntImpl m_mvnum{};           // number from the range that generates max value in 3p1 sequence
	IntImpl m_msnum{};           // number from the range that generates max steps in 3p1 sequence
	IntImpl m_maxvalue{};        // max value reached during calculating current range
	uint64_t m_maxsteps{};       // max value of steps in 3p1 sequence in current range
	uint64_t m_sumsteps{};       // sum of all steps in current range
	IThreeN1<IntImpl>& m_parent;

	static inline uint seq = 0;

public:
	ThreeN1Task(const ThreeN1Task<IntImpl>& other) = delete;
	ThreeN1Task<IntImpl>& operator=(const ThreeN1Task<IntImpl>& other) = delete;

	ThreeN1Task(IThreeN1<IntImpl>& parent): Task(std::to_string(++seq)), m_parent(parent)
	{
	};

	void InitTask(IntImpl start, IntImpl end)
	{
		m_start = start;
		m_end = end;
		status = TaskStatus::awaiting;
	}

	void one_thread_method() override
	{
		typename IThreeN1<IntImpl>::CalcDataType calcData;
		m_maxvalue = m_start;
		m_mvnum = m_start;
		m_maxsteps = 0;
		m_msnum = m_start;
		m_sumsteps = 0;

		std::osyncstream syncout(std::cout);
		//std::locale loc(std::cout.getloc(), new MyGroupSeparator());
		syncout.imbue(ThreeN1::Locale);
		uint32_t MaxValWidth = MAX_VALUE_WIDTH;

		for (IntImpl i = m_start; i < m_end; i++)
		{
			try
			{
				m_parent.Calc3p1(i, calcData);

				if (m_maxvalue < calcData.maxvalue) m_maxvalue = calcData.maxvalue, m_mvnum = i;
				if (m_maxsteps < calcData.steps)    m_maxsteps = calcData.steps,    m_msnum = i;

				m_sumsteps += calcData.steps;
			}
			catch (std::overflow_error & ex) // add intermediate range results into list and stop calc this range 
			{
				//TODO who can generate overflow_error?? nobody?
				status = TaskStatus::error; //TODO it does not make much sense to set ststus here because it is set in MT::Task::one_thread_pre_method() method
				m_parent.addRangeData(getRangeData(TaskStatus::error, i));
				//syncout << std::setw(5) << "[" << id << "] " << "range: (" << m_start << "," << m_end << ") current number: " << i << " " << ex.what() << std::endl;
				syncout << std::format(ThreeN1::Locale, "{:>4} | Range: {:L}-{:L} | Current num: {:L}. {}", std::format(ThreeN1::Locale, "#{:L}", id), m_start, m_end, i, ex.what()) << std::endl;
				throw;
			}
			catch (...) // any exception means tasks is not finished - error
			{
				status = TaskStatus::error; //TODO it does not make much sense to set status here because it is set in MT::Task::one_thread_pre_method() method
				m_parent.addRangeData(getRangeData(TaskStatus::error, i));
				//syncout << std::setw(5) << "[" << id << "] " << "range: (" << m_start << "," << m_end << ") current number:" << i << "ERROR during range calculation!" << std::endl;
				syncout << std::format(ThreeN1::Locale, "{:>4} | Range: {:L}-{:L} | Current num: {:L}. Error during range calculation!", std::format(ThreeN1::Locale, "#{:L}", id), m_start, m_end, i) << std::endl;
				throw;
			}
		}
		
		m_parent.addRangeData(getRangeData(TaskStatus::completed));

		std::string strID = std::format(ThreeN1::Locale, "#{:L}", id);
		std::string maxvalueStr = std::format(ThreeN1::Locale, "{:L}", m_maxvalue);
		syncout << std::format(ThreeN1::Locale, "{:>4} | Range: {:L}-{:L} | Max steps: {:>5L} ({:L}) | Max value: {:>{}} ({:L})",
			strID, m_start, m_end, m_maxsteps, m_msnum, maxvalueStr, MaxValWidth, m_mvnum) << std::endl;
	}

	RangeData<IntImpl> getRangeData(TaskStatus st, IntImpl errnum = 0ull)
	{
		RangeData<IntImpl> rd;
		rd.start = m_start;
		rd.finish = m_end;
		rd.num1 = m_msnum;
		rd.num2 = m_mvnum;
		rd.num1steps = m_maxsteps;
		rd.num2maxvalue = m_maxvalue;
		rd.sumsteps = m_sumsteps;
		rd.errnum = errnum;
		rd.status = st;
		return rd;
	}
};

