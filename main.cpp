#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <fstream>
#include <io.h>
#include <algorithm>
#include <vector>
#include <map>



const int centers_maxize = 4;
const int regs_maxize = 9;
const int segs_maxize = 8;
const uint32_t ram_maxize = 0x7fffff;       // uint8 -> 1 Byte * 0xffffff => ~ 8 MiB
const uint32_t stack_maxize = 0x7fff;       // uint8 -> 1 Byte * 0x7fff => ~ 32 KiB

const uint8_t dead_magicNum = 0xcc;


class Ram
{
private:
    mutable uint32_t read_times = 0;
    mutable uint32_t write_times = 0;

public:
    uint8_t ramD[ram_maxize] = {0};
    Ram() {}

    operator uint8_t() const {return *ramD; }

    uint8_t read(uint32_t index)
    {
        if (index >= ram_maxize) {return dead_magicNum; }
        read_times++;
        return ramD[index];
    }
    void write(uint32_t index, uint8_t val)
    {
        if (index >= ram_maxize) {return; }
        write_times++;
        ramD[index] = val;
    }

    uint32_t get_readTimes() const {return read_times; }
    uint32_t get_writeTimes() const {return write_times; }


    uint8_t operator[](uint32_t index) const
    {
        if (index >= ram_maxize) {return dead_magicNum; }
        read_times++;
        return ramD[index];
    }
    uint8_t& operator[](uint32_t index)
    {
        if (index >= ram_maxize) 
        {
            static uint8_t temp_area = dead_magicNum;
            return temp_area; 
        }
        write_times++;
        return ramD[index];
    }

};


class reg32
{
public:
    class HalfRef
    {
    public:
        HalfRef(uint32_t& whole, int shift) : _whole(whole), _shift(shift) {}

        operator uint16_t() const {return static_cast<uint16_t>((_whole >> _shift) & 0xffff); }

        bool isParity() const
        {
            uint8_t valueT = static_cast<uint16_t>((_whole >> _shift) & 0xffu);
            valueT ^= (valueT >> 4);
            valueT ^= (valueT >> 2);
            valueT ^= (valueT >> 1);
            return (valueT & 0b01u) ==0;
        }
        bool ckbit(int bit_index) const
        {
            bit_index = std::min(15, std::max(0, bit_index));
            return ((_whole >> _shift) & (0b01u << bit_index)) ==1;
        }

        HalfRef& operator=(uint16_t val)
        {
            uint32_t mask = 0xffffu << _shift;
            _whole = (_whole & ~mask) | ((uint32_t)val << _shift); 

            return *this;
        }
        HalfRef& operator^=(uint16_t mask)
        {
            uint32_t fullMask = (uint32_t)mask << _shift;
            _whole ^= fullMask;
            return *this;
        }
        HalfRef& operator|=(uint16_t mask)
        {
            uint32_t fullMask = (uint32_t)mask << _shift;
            _whole |= fullMask;
            return *this;
        }
        HalfRef& operator&=(uint16_t mask)
        {
            uint16_t current = static_cast<uint16_t>((_whole >> _shift) & 0xffffu);
            current &= mask;
            *this = current; 
            return *this;
        }
        HalfRef& operator+=(uint16_t val)
        {
            uint16_t current = static_cast<uint16_t>((_whole >> _shift) & 0xffffu);
            current += val;
            *this = current; 
            return *this;
        }
        HalfRef& operator-=(uint16_t val)
        {
            uint16_t current = static_cast<uint16_t>((_whole >> _shift) & 0xffffu);
            current -= val;
            *this = current;
            return *this;
        }

    private:
        uint32_t& _whole;
        int _shift;
    
    };

    reg32(uint32_t init =0) : _value(init) {}

    HalfRef low()   {return HalfRef(_value, 0); }
    HalfRef high()  {return HalfRef(_value, 16); }

    uint32_t word() const   {return _value; }
    void word(uint32_t Val) {_value = Val; }

    reg32& operator=(uint32_t Val) {_value = Val; return *this; }
    operator uint32_t() const {return _value; }

    void chbit(int bit_index)       // change bit
    {
        bit_index = std::min(31, std::max(0, bit_index));
        _value ^= (0b01u << bit_index);
    }
    bool ckbit(int bit_index) const // check bit
    {
        bit_index = std::min(31, std::max(0, bit_index));
        return ((_value >> bit_index) & 0b01u) ==1;
    }
    void stbit(int bit_index)       // set bit
    {
        bit_index = std::min(31, std::max(0, bit_index));
        _value |= (0b01u << bit_index);
    }
    void clbit(int bit_index)       // clear bit
    {
        bit_index = std::min(31, std::max(0, bit_index));
        _value &= ~(0b01u << bit_index);
    }
    bool isParityL8() const
    {
        uint8_t valueT = _value & 0xffu;
        valueT ^= (valueT >> 4);
        valueT ^= (valueT >> 2);
        valueT ^= (valueT >> 1);
        return (valueT & 0b01u) ==0;
    }

    // Low halfReg
    void chbitL(int bit_index)      
    {
        bit_index = std::min(15, std::max(0, bit_index));
        low() ^= (0b01u << bit_index);
    }
    bool ckbitL(int bit_index) const
    {
        bit_index = std::min(15, std::max(0, bit_index));
        return ((_value >> bit_index) & 0b01u) ==1;
    }
    void stbitL(int bit_index) 
    {
        bit_index = std::min(15, std::max(0, bit_index));
        low() |= (0b01u << bit_index);
    }
    void clbitL(int bit_index)
    {
        bit_index = std::min(15, std::max(0, bit_index));
        low() &= ~(0b01u << bit_index);
    }

    // High halfReg
    void chbitH(int bit_index)    
    {
        bit_index = std::min(15, std::max(0, bit_index));
        high() ^= (0b01u << bit_index);
    }
    bool ckbitH(int bit_index) const
    {
        bit_index = std::min(15, std::max(0, bit_index));
        return ((_value >> (bit_index + 16)) & 0b01u) ==1;
    }
    void stbitH(int bit_index) 
    {
        bit_index = std::min(15, std::max(0, bit_index));
        high() |= (0b01u << bit_index);
    }
    void clbitH(int bit_index)
    {
        bit_index = std::min(15, std::max(0, bit_index));
        high() &= ~(0b01u << bit_index);
    }

private:
    uint32_t _value;

};

class Hasher
{
private:
    const static int max4timesArg = 0xf;
    const static int min4timesArg = 0x2;
    const static uint32_t length_sizeof_uint16 = 0xffff +1;
    
    static uint32_t randNum_arrToT[max4timesArg -1][length_sizeof_uint16];
    static uint32_t randNum_arrHR2[max4timesArg -1][length_sizeof_uint16];

    const static uint32_t xor_seed = 0xdeadc01d;        
    const static uint32_t mul_seeDefault = 0xfcfbfaf9;
    const static uint32_t num1 = 0x9e3779b1;

    static bool tablesInited;
    static void ensure_tableInited(uint32_t seed_4mul = mul_seeDefault)
    {
        if (!tablesInited)
        {
            create_randNumTable(seed_4mul);
            create_randNumTableHR2(seed_4mul);
            tablesInited = true;
        }
    }

public:
    Hasher(uint32_t seed_4mul=mul_seeDefault)
    {
        ensure_tableInited(seed_4mul);
    }

    static uint32_t hash_uint32(uint32_t num, int times =3, uint32_t seed_4mul=mul_seeDefault)
    {
        uint32_t numT = seed_4mul | 0x01;
        times = std::min(0xf, std::max(2, times));
        num ^= xor_seed;
        num *= num1;
        for (int i =0; i < times; ++i)
        {
            num ^= (num >> 16);
            num *= numT;
            numT ^= ((((numT >> 8) | ((num ^ num1) >> 24)) & (numT << 16)) | 0x01);
        }
        if (num == 0 || num == 0xffffffff) {return hash_uint32(num ^ (seed_4mul & numT), times ^ 0x0f, seed_4mul);}
        return num;
    }

    static void create_randNumTable(uint32_t seed_4mul=mul_seeDefault)
    {
        int times = min4timesArg;
        for (uint32_t index1 =0; times < max4timesArg; (++times, ++index1))
        {
            for (uint32_t index2 =0; index2 < length_sizeof_uint16; ++index2)
            {
                randNum_arrToT[index1][index2] = hash_uint32(index2, times, seed_4mul);
            }
        }
    }

    static void create_randNumTableHR2(uint32_t seed_4mul=mul_seeDefault)
    {
        int times = min4timesArg;
        for (uint32_t index1 =0; times < max4timesArg; (++times, ++index1))
        {
            for (uint32_t index2 =0; index2 < length_sizeof_uint16; ++index2)
            {
                uint32_t src_num = randNum_arrToT[index1][index2];
                randNum_arrHR2[index1][index2] = hash_uint32(src_num, times, seed_4mul);
            }
        }
    }

    static uint32_t get_randNum(uint16_t index, int timesD=0)
    {
        timesD = std::min(max4timesArg -2, std::max(0, timesD));
        uint16_t step = randNum_arrToT[timesD][(uint16_t)(index +1)];
        uint16_t indexT = index +1 + step;
        return randNum_arrToT[timesD][(uint16_t)indexT];
    }

    static uint32_t get_randNumHR2(uint16_t index, int timesD=0)
    {
        timesD = std::min(max4timesArg -2, std::max(0, timesD));
        uint16_t step = randNum_arrHR2[timesD][(uint16_t)(index +1)];
        uint16_t indexT = index +1 + step;
        return randNum_arrHR2[timesD][(uint16_t)indexT];
    }

};
bool Hasher::tablesInited = false;
uint32_t Hasher::randNum_arrToT[Hasher::max4timesArg -1][Hasher::length_sizeof_uint16] = {0};
uint32_t Hasher::randNum_arrHR2[Hasher::max4timesArg -1][Hasher::length_sizeof_uint16] = {0};


class Center32x
{
private:
    class commandProcesser
    {
    private:
        reg32* flag;
        uint32_t* timE;
        reg32 copyT1, copyT2, copyT3, copyT4;
        uint16_t copyF1, copyF2, copyF3, copyF4;

    public:
        commandProcesser(reg32* flagN, uint32_t* timEN) : flag(flagN), timE(timEN) {}

        uint32_t nop(uint32_t timE) const
        {
            int temp = timE;
            return temp;
        }
        void nop() const
        {}
        void mov(reg32* dst, reg32 src)
        {
            dst->word(src.word());
        }
        void mov(reg32* dst, reg32::HalfRef src)
        {
            copyF1 = src;
            dst->word(copyF1);
            copyF1 = 0;
        }
        void mov(reg32* dst, uint32_t src)
        {
            dst->word(src);
        }
        void mov(reg32::HalfRef* dst, reg32::HalfRef src)
        {
            copyF1 = src;
            *dst = copyF1;
            copyF1 = 0;
        }
        void mov(reg32::HalfRef* dst, reg32 src)
        {
            copyF1 = src.low();
            *dst = copyF1;
            copyF1 = 0;
        }
        void mov(reg32::HalfRef* dst, uint16_t src)
        {
            *dst = src;
        }
        void xchg(reg32* dst, reg32* src)
        {
            copyT1.word(src->word());
            src->word(dst->word());
            dst->word(copyT1.word());

            copyT1.word(0);
        }
        void xchg(reg32::HalfRef* dst, reg32::HalfRef* src)
        {
            copyF1 = *src;
            copyF2 = *dst;
            *src = copyF2;
            *dst = copyF1;
            copyF1 = copyF2 = 0;
        }
        void flagSet4add(bool sum_betterThan_dst, bool dstParity, bool dstEZ, bool dstMSB, bool OFcheck)
        {
            // CF(0), PF(2), ZF(6), SF(7), OF(11)
            if (sum_betterThan_dst) {flag->stbit(0); } else {flag->clbit(0); }
            if (dstParity) {flag->stbit(2); } else {flag->clbit(2); }
            if (dstEZ) {flag->stbit(6); } else {flag->clbit(6); }
            if (dstMSB) {flag->stbit(7); } else {flag->clbit(7); }
            if (OFcheck) {flag->stbit(11); } else {flag->clbit(11); }
        }
        void add(reg32* dst, reg32 src)
        {
            uint64_t sum = *dst + src;
            copyT1.word(dst->word());
            *dst = static_cast<uint32_t>(sum);

            bool dstMSB(dst->ckbit(31)), srcMSB(src.ckbit(31)), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4add(sum > *dst, dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void add(reg32* dst, uint32_t src)
        {
            uint64_t sum = *dst + src;
            copyT1.word(dst->word());
            *dst = static_cast<uint32_t>(sum);

            bool dstMSB(dst->ckbit(31)), srcMSB(src & 0x80000000), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);
            
            flagSet4add(sum > *dst, dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0); 
        }
        void add(reg32*dst, reg32::HalfRef src)
        {
            uint64_t sum = *dst + src;
            copyT1.word(dst->word());
            *dst = static_cast<uint32_t>(sum);

            bool dstMSB(dst->ckbit(31)), srcMSB(src.ckbit(15)), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4add(sum > *dst, dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void add(reg32::HalfRef* dst, reg32::HalfRef src)
        {
            uint32_t sum = *dst + src;
            copyF1 = *dst;
            *dst = static_cast<uint16_t>(sum);

            bool dstMSB(dst->ckbit(15)), srcMSB(src.ckbit(15)), cpyMSB(copyF1 & 0x8000);
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);
            
            flagSet4add(sum > *dst, dst->isParity(), *dst ==0, dstMSB, OFcheck);
            copyF1 = 0;
        }
        void add(reg32::HalfRef* dst, uint16_t src)
        {
            uint32_t sum = *dst + src;
            copyF1 = *dst;
            *dst = static_cast<uint16_t>(sum);

            bool dstMSB(dst->ckbit(15)), srcMSB(src & 0x8000), cpyMSB(copyF1 & 0x8000);
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);
            
            flagSet4add(sum > *dst, dst->isParity(), *dst ==0, dstMSB, OFcheck);
            copyF1 = 0;
        }
        void flagSet4sub(bool dst_lessThan_src, bool dstParity, bool dstEZ, bool dstMSB, bool OFcheck)
        {
            // CF(0), PF(2), ZF(6), SF(7), OF(11)
            if (dst_lessThan_src) {flag->stbit(0); } else {flag->clbit(0); }
            if (dstParity) {flag->stbit(2); } else {flag->clbit(2); }
            if (dstEZ) {flag->stbit(6); } else {flag->clbit(6); }
            if (dstMSB) {flag->stbit(7); } else {flag->clbit(7); }
            if (OFcheck) {flag->stbit(11); } else {flag->clbit(11); }
        }
        void sub(reg32* dst, reg32 src)
        {
            copyT1.word(dst->word());
            *dst = *dst - src;

            bool dstMSB(dst->ckbit(31)), srcMSB(src.ckbit(31)), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4sub(copyT1 < src, dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void sub(reg32* dst, uint32_t src)
        {
            copyT1.word(dst->word());
            *dst = *dst - src;

            bool dstMSB(dst->ckbit(31)), srcMSB(src & 0x80000000), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4sub(copyT1 < src, dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void sub(reg32* dst, reg32::HalfRef src)
        {
            copyT1.word(dst->word());
            *dst = *dst - src;

            bool dstMSB(dst->ckbit(31)), srcMSB(src.ckbit(15)), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4sub(copyT1 < src, dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void sub(reg32::HalfRef* dst, reg32::HalfRef src)
        {
            copyF1 = *dst;
            *dst = *dst - src;

            bool dstMSB(dst->ckbit(15)), srcMSB(src.ckbit(15)), cpyMSB(copyF1 & 0x8000);
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4sub(copyF1 < src, dst->isParity(), *dst ==0, dstMSB, OFcheck);
            copyF1 = 0;
        }
        void sub(reg32::HalfRef* dst, uint16_t src)
        {
            copyF1 = *dst;
            *dst = *dst - src;

            bool dstMSB(dst->ckbit(15)), srcMSB(src & 0x8000), cpyMSB(copyF1 & 0x8000);
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4sub(copyF1 < src, dst->isParity(), *dst ==0, dstMSB, OFcheck);
            copyF1 = 0;
        }
        void flagSet4adc(bool dst_betterThan_src, bool dstParity, bool dstEZ, bool dstMSB, bool OFcheck)
        {
            flagSet4add(dst_betterThan_src, dstParity, dstEZ, dstMSB, OFcheck);
        }
        void adc(reg32* dst, reg32 src)
        {
            copyT1.word(dst->word());
            // CF(0)
            uint32_t CFval = (flag->ckbit(0)) ? 1 : 0;
            uint64_t sum = *dst + src + CFval;
            *dst = static_cast<uint32_t>(sum);

            bool dstMSB(dst->ckbit(31)), srcMSB(src.ckbit(31)), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4adc(sum > *dst, dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void adc(reg32* dst, uint32_t src)
        {
            copyT1.word(dst->word());
            uint32_t CFval = (flag->ckbit(0)) ? 1 : 0;
            uint64_t sum = *dst + src + CFval;
            *dst = static_cast<uint32_t>(sum);

            bool dstMSB(dst->ckbit(31)), srcMSB(src & 0x80000000), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4adc(sum > *dst, dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void adc(reg32* dst, reg32::HalfRef src)
        {
            copyT1.word(dst->word());
            uint32_t CFval = (flag->ckbit(0)) ? 1 : 0;
            uint64_t sum = *dst + src + CFval;
            *dst = static_cast<uint32_t>(sum);

            bool dstMSB(dst->ckbit(31)), srcMSB(src.ckbit(15)), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4adc(sum > *dst, dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void adc(reg32::HalfRef* dst, reg32::HalfRef src)
        {
            copyF1 = *dst;
            uint16_t CFval = (flag->ckbit(0)) ? 1 : 0;
            uint32_t sum = *dst + src + CFval;
            *dst = static_cast<uint16_t>(sum);

            bool dstMSB(dst->ckbit(15)), srcMSB(src.ckbit(15)), cpyMSB(copyF1 & 0x8000);
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4adc(sum > *dst, dst->isParity(), *dst ==0, dstMSB, OFcheck);
            copyF1 = 0;
        }
        void adc(reg32::HalfRef* dst, uint16_t src)
        {
            copyF1 = *dst;
            uint16_t CFval = (flag->ckbit(0)) ? 1 : 0;
            uint32_t sum = *dst + src + CFval;
            *dst = static_cast<uint16_t>(sum);

            bool dstMSB(dst->ckbit(15)), srcMSB(src & 0x8000), cpyMSB(copyF1 & 0x8000);
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4adc(sum > *dst, dst->isParity(), *dst ==0, dstMSB, OFcheck);
            copyF1 = 0;
        }
        void flagSet4sbb(bool dst_lessThan_src, bool dstParity, bool dstEZ, bool dstMSB, bool OFcheck)
        {
            flagSet4sub(dst_lessThan_src, dstParity, dstEZ, dstMSB, OFcheck);
        }
        void sbb(reg32* dst, reg32 src)
        {
            copyT1.word(dst->word());
            // CF (0)
            uint32_t CFval = (flag->ckbit(0)) ? 1 : 0;
            *dst = *dst - src - CFval;

            bool dstMSB(dst->ckbit(31)), srcMSB(src.ckbit(31)), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4sbb(*dst < (src + CFval), dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void sbb(reg32* dst, uint32_t src)
        {
            copyT1.word(dst->word());
            uint32_t CFval = (flag->ckbit(0)) ? 1 : 0;
            *dst = *dst - src - CFval;

            bool dstMSB(dst->ckbit(31)), srcMSB(src & 0x80000000), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4sbb(*dst < (src + CFval), dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void sbb(reg32* dst, reg32::HalfRef src)
        {
            copyT1.word(dst->word());
            uint32_t CFval = (flag->ckbit(0)) ? 1 : 0;
            *dst = *dst - src - CFval;

            bool dstMSB(dst->ckbit(31)), srcMSB(src.ckbit(15)), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4sbb(*dst < (src + CFval), dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void sbb(reg32::HalfRef* dst, reg32::HalfRef src)
        {
            copyF1 = *dst;
            uint16_t CFval = (flag->ckbit(0)) ? 1 : 0;
            *dst = *dst - src - CFval;

            bool dstMSB(dst->ckbit(15)), srcMSB(src.ckbit(15)), cpyMSB(copyF1 & 0x8000);
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4sbb(*dst < (src + CFval), dst->isParity(), *dst ==0, dstMSB, OFcheck);
            copyF1 = 0;
        }
        void sbb(reg32::HalfRef* dst, uint16_t src)
        {
            copyF1 = *dst;
            uint16_t Cfval = (flag->ckbit(0)) ? 1 : 0;
            *dst = *dst - src - Cfval;

            bool dstMSB(dst->ckbit(15)), srcMSB(src & 0x8000), cpyMSB(copyF1 & 0x8000);
            bool OFcheck = (cpyMSB && srcMSB && !dstMSB) || (!cpyMSB && !srcMSB && dstMSB);

            flagSet4sbb(*dst < (src + Cfval), dst->isParity(), *dst ==0, dstMSB, OFcheck);
            copyF1 = 0;
        }
        void flagSet4inc(bool Parity, bool EZ, bool MSB, bool OFcheck)
        {
            // PF(2), ZF(6), SF(7), OF(11)
            if (Parity) {flag->stbit(2); } else{flag->clbit(2); }
            if (EZ) {flag->stbit(6); } else{flag->clbit(6); }
            if (MSB) {flag->stbit(7); } else{flag->clbit(7); }
            if (OFcheck) {flag->stbit(11); } else{flag->clbit(11); }
        }
        void inc(reg32* dst)
        {
            copyT1.word(dst->word());
            *dst = *dst + 1;

            bool dstMSB(dst->ckbit(31)), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && !dstMSB) || (!cpyMSB && dstMSB);

            flagSet4inc(dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void inc(reg32::HalfRef* dst)
        {
            copyF1 = *dst;
            *dst = *dst + 1;

            bool dstMSB(dst->ckbit(15)), cpyMSB(copyF1 & 0x8000);
            bool OFcheck = (cpyMSB && !dstMSB) || (!cpyMSB && dstMSB);

            flagSet4inc(dst->isParity(), *dst ==0, dstMSB, OFcheck);
            copyF1 = 0;
        }
        void flagSet4dec(bool Parity, bool EZ, bool MSB, bool OFcheck)
        {
            // PF(2), ZF(6), SF(7), OF(11)
            if (Parity) {flag->stbit(2); } else{flag->clbit(2); }
            if (EZ) {flag->stbit(6); } else{flag->clbit(6); }
            if (MSB) {flag->stbit(7); } else{flag->clbit(7); }
            if (OFcheck) {flag->stbit(11); } else{flag->clbit(11); }
        }
        void dec(reg32* dst)
        {
            copyT1.word(dst->word());
            *dst = *dst - 1;

            bool dstMSB(dst->ckbit(31)), cpyMSB(copyT1.ckbit(31));
            bool OFcheck = (cpyMSB && !dstMSB) || (!cpyMSB && dstMSB);

            flagSet4dec(dst->isParityL8(), *dst ==0, dstMSB, OFcheck);
            copyT1.word(0);
        }
        void dec(reg32::HalfRef* dst)
        {
            copyF1 = *dst;
            *dst = *dst - 1;

            bool dstMSB(dst->ckbit(15)), cpyMSB(copyF1 & 0x8000);
            bool OFcheck = (cpyMSB && !dstMSB) || (!cpyMSB && dstMSB);

            flagSet4dec(dst->isParity(), *dst ==0, dstMSB, OFcheck);
            copyF1 = 0;
        }
        
    };

    uint32_t IDt;
    uint32_t EID;
    uint32_t UD;
    uint32_t timE;

    reg32 FLAG = 0;
    commandProcesser calc{&FLAG, &timE};

    uint32_t segs[8] = {0};
    uint32_t& CS = segs[0];
    uint32_t& DS = segs[1];
    uint32_t& SS = segs[2];
    uint32_t& ES = segs[3];
    uint32_t& FS = segs[4];
    uint32_t& GS = segs[5];

    reg32 AX, CX, DX, BX;
    reg32 SP, BP, SI, DI;
    reg32* regs[regs_maxize] = {&AX, &CX, &DX, &BX, &SP, &BP, &SI, &DI, &FLAG};
    
    std::map<uint16_t, devIO*> devsRegistry;

public:
    Center32x(uint32_t ID)
    {
        IDt = ID;
        UD = Hasher::hash_uint32(IDt);
        uint8_t temp = UD & 0b0111;
        EID = Hasher::get_randNumHR2(UD, temp);
    }

    void getSegs(uint32_t* dstArray) const
    {
        for (int index =0; index < 8; ++index)
        {
            dstArray[index] = segs[index];
        }
    }
    void getRegs(reg32* dstArray) const
    {
        for (int index =0; index < regs_maxize; ++index)
        {
            dstArray[index].word(regs[index]->word());
        }
    }

};

class devIO
{};

class rootOP
{};

class debugCMD
{
private:
    Center32x* centers[centers_maxize];
    reg32 regs_0f_centers[centers_maxize][regs_maxize];
    uint32_t segs_0f_centers[centers_maxize][segs_maxize];

public:
    debugCMD(Center32x* centersG[centers_maxize])
    {
        for (int index =0; index < centers_maxize; ++index)
        {
            centers[index] = centersG[index];
            centers[index]->getRegs(regs_0f_centers[index]);
            centers[index]->getSegs(segs_0f_centers[index]);
        }
    }
    void data_update()
    {
        for (int index =0; index < centers_maxize; ++index)
        {
            centers[index]->getRegs(regs_0f_centers[index]);
            centers[index]->getSegs(segs_0f_centers[index]);
        }
    }

};






bool initG()
{
    return true;
}


int main()
{
    initG();
    Hasher ashor;
    system("pause");
    return 0;
}


