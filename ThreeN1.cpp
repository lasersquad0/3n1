

#include "ThreeN1.h"


// calc ONE number WITHOUT using cache
// used for BigInt ONLY
template<>
void ThreeN1<BigInt>::Calc3p1(const BigInt& number, ThreeN1Data<BigInt>& calcResult)
{
    calcResult.maxvalue = number;
    calcResult.steps = 0ull;
    BigInt curr = number;
    
    if (curr < m_unused.BitsCount()) m_unused.setTrue(curr); //m_paths[curr] = true;

    while (curr != 1ull)
    {
        if (curr.IsEven())
        {
            divide_by_2(curr);
        }
        else
        {
            curr = (3ull * curr + 1ull) / 2; //TODO why dont use divide_by_2() here? 

            calcResult.steps++; // if curr is odd we do 2 operations at once and increase steps twice accordingly
            if (calcResult.maxvalue < curr) calcResult.maxvalue = curr;
        }

        calcResult.steps++;

        if (curr < m_unused.BitsCount()) m_unused.setTrue(curr); //m_paths[curr] = true;
    }
}

// еще оптимизация - держать кеш в диапазоне up/2....up. где up верхняя граница кеша.
// держать кеш ниже чем up/2 нету смысла туда никогда не зайдем.
// например диапазон 1G...2G 
template<>
void ThreeN1<uint64_t>::CacheToFileVarLen(const uint64_t& start, const std::string& fileName)
{
    std::ofstream f;
    f.open(fileName, std::ios::out | std::ios::binary);
    if (f.fail())
    {
        //cout << "Cannot open file '" << fileTo << "' for writing, exiting." << endl;
        throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");
    }

    const uint64_t BUF_LEN = 100'000'000; // записываем в файл блоками по 100М
    uint8_t* buf = new uint8_t[BUF_LEN];

    uint64_t offset = var_len_encode(buf, start);
    f.write((const char*)buf, offset);  // saving start number, all subsequent numbers will be get by +1 to start

    offset = var_len_encode(buf, m_valuesCache.Count());
    f.write((const char*)buf, offset);  // saving expected number of items in a file

    offset = 0;
    for (uint i = 0; i < m_valuesCache.Count(); ++i)
    {
        CalcDataType val = m_valuesCache[i];
        assert(val.steps < 65536);
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


template<>
void ThreeN1<uint64_t>::CacheFromFileVarLen(const std::string& fileName)
{
    std::ifstream f;
    f.open(fileName, std::ios::out | std::ios::binary);
    if (f.fail())
    {
        //cout << "Cannot open file '" << fileTo << "' for writing, exiting." << endl;
        throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");
    }

    uint64_t start, cnt;
    uint8_t buf[9];

    size_t maxSize = VarLenReadBuf(f, buf);
    size_t res = var_len_decode(buf, maxSize, &start);
    assert(res > 0);

    maxSize = VarLenReadBuf(f, buf);
    res = var_len_decode(buf, maxSize, &cnt);
    assert(res > 0);

    m_valuesCache.SetCapacity((uint)(cnt));// +2ull)); // +2 just in case
    CalcDataType val;
    while (true)
    {
        maxSize = VarLenReadBuf(f, buf);
        uint64_t tmp;
        res = var_len_decode(buf, maxSize, &tmp);
        assert(res > 0);
        assert(tmp < 65536ull);
        val.steps = (uint16_t)tmp;

        maxSize = VarLenReadBuf(f, buf);
        res = var_len_decode(buf, maxSize, &val.maxvalue);
        assert(res > 0);

        if (f.eof()) break;

        m_valuesCache.AddValue(val);

        cnt--;
    }

    assert(cnt == 0);

    f.close();
}

template<>
void ThreeN1<uint64_t>::CacheFromFileVarLen2(const std::string& fileName, int64_t itemsToRead)
{
    std::ifstream f;
    f.open(fileName, std::ios::out | std::ios::binary);
    if (f.fail())
        throw std::invalid_argument("Error: cannot open file '" + fileName + "'\n");

    const uint64_t BUF_LEN = 100'000'000; // read file by 100M blocks
    uint8_t* buf = new uint8_t[BUF_LEN];

    uint64_t cnt{};

    size_t maxSize = VarLenReadBuf(f, buf);
    size_t res = var_len_decode(buf, maxSize, &m_cacheStart);
    assert(res > 0);

    maxSize = VarLenReadBuf(f, buf);
    res = var_len_decode(buf, maxSize, &cnt);
    assert(res > 0);
    // reading only itemsToRead items from cache file
    if (itemsToRead != -1) cnt = std::min(cnt, (uint64_t)itemsToRead);
    m_cacheFinish = m_cacheStart + cnt;

    m_valuesCache.Clear();
    m_valuesCache.SetCapacity((uint)(toULongLong(cnt)));// +2ull)); // +2 just in case
    CalcDataType val;
    size_t offset = 0;
    size_t actualBufSize = BUF_LEN;

    f.read((char*)buf, BUF_LEN);

    while (true)
    {
        if ((offset > actualBufSize - 9) && !f.eof())
        {
            size_t remainde = actualBufSize - offset;
            memcpy(buf, buf + offset, remainde); // move remainding bytes into beginning of the buffer
            f.read((char*)(buf + remainde), BUF_LEN - remainde);
            actualBufSize = f.gcount() + remainde; // real number of read bytes
            offset = 0;
        }

        uint64_t tmp;
        res = var_len_decode(buf + offset, 9, &tmp);
        assert(res > 0);
        assert(tmp < 65536ull);
        val.steps = (uint16_t)tmp;
        offset += res;

        res = var_len_decode(buf + offset, 9, &val.maxvalue);
        assert(res > 0);
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
