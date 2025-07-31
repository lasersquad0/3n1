

#include "ThreeN1.h"



//template<typename IntImpl>
//std::mutex ThreeN1Task<IntImpl>::m_cacheLock;

//bool operator==(const struct ThreeN1ValueBigInt& a, const struct ThreeN1ValueBigInt& b)
//{
//    return a.steps == b.steps && a.maxvalue == b.maxvalue;
//}
//
//template<>
//class Compare<ThreeN1ValueBigInt>
//{
//public:
//    virtual bool eq(const ThreeN1ValueBigInt& a, const ThreeN1ValueBigInt& b) const { return a.steps == b.steps && a.maxvalue == b.maxvalue; };
//    virtual bool lt(const ThreeN1ValueBigInt& a, const ThreeN1ValueBigInt& b) const { return a.steps < b.steps && a.maxvalue < b.maxvalue; };
//    virtual bool mt(const ThreeN1ValueBigInt& a, const ThreeN1ValueBigInt& b) const { return a.steps > b.steps && a.maxvalue > b.maxvalue; };
//    virtual ~Compare() {};
//};

/*
void calc3p1(BigInt number, BigInt& steps, BigInt& maxvalue)
{
    maxvalue = number;
    steps = 0ull;

    //BigInt max_value = ULLONG_MAX / 3;
    BigInt curr = number;

    while (curr != 1)
    {
        //cout << curr << endl;
        if (curr % 2 == 0ull)
        {
            curr /= 2;
        }
        else
        {
            //if (curr >= max_value)
            //    throw overflow_error("Overflow detected!");

            curr = 3 * curr + 1;
            if (maxvalue < curr) maxvalue = curr;
        }

        steps++;
    }

    //cout << curr << endl;
}

void calc3p1(BigInt number, BigInt& steps, BigInt& maxvalue, const maptn1b& values, ull& hits)
{
    maxvalue = number;
    steps = 0ull;

    BigInt curr = number;

    while (curr != 1ull)
    {
        if (curr.IsEven())
        {
            divide_by_2(curr);
            if (curr < number)
            {
                auto elem = values.GetValuePointer(curr);
                if (elem != nullptr)
                {
                    steps += elem->steps + 1;
                    maxvalue = max(maxvalue, elem->maxvalue);

                    hits++;
                    return; // got value less than initial, we already know steps/maxv for this value.
                }
            }
        }
        else
        {
            curr = 3 * curr + 1ull;
            if (maxvalue < curr) maxvalue = curr;
        }

        steps++;
    }

    //cout << curr << endl;
}

void calc3p1Range(BigInt start, BigInt finish)
{
    BigInt steps, num1, maxsteps;
    BigInt maxv, num2, maxmaxv;
    BigInt sumsteps;
    ull hits = 0;

    maptn1b values;

    BigInt cap = finish - start + 2ull;
    ull capacity = toULongLong(cap);
    values.SetCapacity((unsigned int)capacity);

    for (BigInt i = start; i < finish; i++)
    {
        if (i.HasTrailingZeros(3)) // for optimization print every 1000th number only
            cout << '\r' << i << '\r';

        calc3p1(i, steps, maxv, values, hits);
        values.SetValue(i, { steps, maxv });

        //calc3p1(i, steps, maxv);

        sumsteps += steps;

        if (maxmaxv < maxv)
        {
            num1 = i;
            maxmaxv = maxv;
            cout << "number:" << i << "  steps:" << steps << "  max value:" << maxv << " (" << Length(maxmaxv) << " digits)" << endl;
        }

        if (maxsteps < steps)
        {
            num2 = i;
            maxsteps = steps;
            //cout << "number:" << i << "  steps:" << steps << "  max value:" << maxv << endl;
        }
    }

    cout << "Totals:" << endl;
    cout << "number:" << num2 << "  max steps:" << maxsteps << endl;
    cout << "number:" << num1 << "  max value:" << maxmaxv << " (" << Length(maxmaxv) << " digits)" << endl;
    cout << "values.size:" << values.Count() << endl;
    cout << "sumsteps:" << sumsteps << endl;
    cout << "hits:" << hits << endl;
}
*/
