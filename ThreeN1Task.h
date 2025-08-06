#pragma once

#include <string>
//#include <shared_mutex>
#include <syncstream>
#include <locale>

#include "BigInt.h"
#include "ThreeN1.h"
#include "thread_pool.h"
#include "Utils.h"


template<typename IntImpl>
class ThreeN1;

//template<typename IntImpl>
//struct ThreeN1Data;

template<typename IntImpl>
struct RangeData;

template<typename IntImpl>
class ThreeN1Task : public MT::Task
{
private:
	IntImpl m_start, m_end; // current range
	IntImpl m_mvnum;        // number from the range that generates max value in 3p1 sequence
	IntImpl m_msnum;        // number from the range that generates max steps in 3p1 sequence
	IntImpl m_maxvalue;     // max value reached during calculating current range
	uint64_t m_maxsteps;         // max value of steps in 3p1 sequence in current range
	ThreeN1<IntImpl>& m_parent;

	static inline uint seq = 0;

public:
	ThreeN1Task(ThreeN1<IntImpl>& parent): Task(std::to_string(++seq)), m_parent(parent)
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
		// std::osyncstream scout(std::cout);
		// scout << "[" << id << "] " << "Starting task: " << std::endl;

		typename ThreeN1<IntImpl>::CalcDataType calcData;
		m_maxvalue = m_start;
		m_mvnum = m_start;
		m_maxsteps = 0;
		m_msnum = m_start;

		std::osyncstream syncout(std::cout);
		std::locale loc(std::cout.getloc(), new MyGroupSeparator());
		syncout.imbue(loc);

		for (IntImpl i = m_start; i < m_end; i++)
		{
			try
			{
				m_parent.Calc3p1(i, calcData);

				if (m_maxvalue < calcData.maxvalue) m_maxvalue = calcData.maxvalue, m_mvnum = i;
				if (m_maxsteps < calcData.steps)    m_maxsteps = calcData.steps,    m_msnum = i;
			}
			catch (std::overflow_error & ex) // add intermediate range results into list and stop calc this range 
			{
				status = TaskStatus::error;
				m_parent.addRangeData(getRangeData(i));	
				syncout << std::setw(5) << "[" << id << "] " << "range: (" << m_start << "," << m_end << ") current number: " << i << " " << ex.what() << std::endl;
				throw;
			}
			catch (...) // any exception means tasks is not finished - error
			{
				status = TaskStatus::error;
				m_parent.addRangeData(getRangeData(i));
				syncout << std::setw(5) << "[" << id << "] " << "range: (" << m_start << "," << m_end << ") current number:" << i << "ERROR during range calculation!" << std::endl;
				throw;
			}
		}

		m_parent.addRangeData(getRangeData());

		//scout << "[" << id << "] " << "Calculated, storing results... " << std::endl;
		syncout << std::format(loc, "[{:2}] Range:({:L}, {:L}) Max steps: {:5L} ({:L}) Max value: {:25L} ({:L})", id, toULongLong(m_start), toULongLong(m_end), m_maxsteps, toULongLong(m_msnum), toULongLong(m_maxvalue), toULongLong(m_mvnum)) << std::endl;
		//syncout << "[" << id << "] " << "range: (" << m_start << "," << m_end << ")  max steps: " << m_maxsteps << " (" << m_msnum << ")" << "  max value: " << m_maxvalue << " (" << m_mvnum << ")" << std::endl;
		
	}

	RangeData<IntImpl> getRangeData(IntImpl errnum = 0ull)
	{
		RangeData<IntImpl> rd;
		rd.start = m_start;
		rd.finish = m_end;
		rd.num1 = m_msnum;
		rd.num2 = m_mvnum;
		rd.num1steps = m_maxsteps;
		rd.num2maxvalue = m_maxvalue;
		rd.errnum = errnum;
		rd.status = status;
		return rd;
	}
};

