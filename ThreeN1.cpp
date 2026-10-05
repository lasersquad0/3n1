

#include "ThreeN1.h"


// еще оптимизация - держать кеш в диапазоне up/2....up. где up верхняя граница кеша.
// держать кеш ниже чем up/2 нету смысла туда никогда не зайдем.
// например диапазон 1G...2G 
// Save cache only when IntImpl=uint64_t. 
// other IntImpl implementation just must be able to read this file properly
template<>
void IThreeN1<uint64_t>::SaveCacheToFileVarLen()
{
    auto fileName = getCacheFileName(".diffvar");

    std::ofstream f;
    f.open(fileName, std::ios::out | std::ios::binary);
    if (f.fail())
        throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");

    const uint64_t BUF_LEN = 100'000'000; // save by blocks of 100М size
    uint8_t* buf = new uint8_t[BUF_LEN];

    uint64_t offset = var_len_encode(buf, m_cacheStart);
    f.write((const char*)buf, offset);  // saving start number, all subsequent numbers will be get by +1 to start

    offset = var_len_encode(buf, m_valuesCache.Count());
    f.write((const char*)buf, offset);  // saving  number of items in cache 

    offset = 0;
    for (uint i = 0; i < m_valuesCache.Count(); ++i)
    {
        CalcDataType val = m_valuesCache[i];
        assert(val.steps < STEPS_MAX);
        offset += var_len_encode(buf + offset, (uint64_t)val.steps);
        offset += var_len_encode(buf + offset, val.maxvalue);

        if (offset > BUF_LEN - 8)
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
