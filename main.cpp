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



// ___<funcName> means been deprecated or its purpose is unclear, so better not to use  xP;
// <<varName>T> or <<VARnAME>t> means Temp, Test or Total;
// <<varName>G> means Global;
// <<varName>P> means it Points to;
// <<varName>E> means it s about Equipment;
// <<varName>C> means current;
// <<varName>N> means New;

/* ModR/M - modified --
    Mod     7       0 is mem32, 1 is mem16;
    Reg     3-6     choose full or half register as source register;
    Mem     0-2     choose full register as base register;
*/


const int centers_maxize = 4;
const int regs_maxize = 9;
const int segs_maxize = 8;
const int commandsArr_maxize = 10;
const uint32_t ram_maxize = 0x7fffff;       // uint8 -> 1 Byte * 0xffffff => ~ 8 MiB
const uint32_t stack_maxize = 0x7fff;       // uint8 -> 1 Byte * 0x7fff => ~ 32 KiB
const uint32_t ramSegs_maxize = 256;

const uint8_t dead_magicNum = 0xcc;


class Ram
{
private:
    mutable uint32_t read_times = 0;
    mutable uint32_t write_times = 0;
    mutable bool deadState_temp = false;

public:
    uint8_t ramD[ram_maxize] = {0};

    Ram() {}

    operator uint8_t() const {return *ramD; }

    uint8_t read(uint32_t index) const
    {
        if (index >= ram_maxize) 
        {
            deadState_temp = true; 
            return dead_magicNum; 
        }
        deadState_temp = false;
        read_times++;
        return ramD[index];
    }
    void write(uint32_t index, uint8_t val)
    {
        if (index >= ram_maxize) 
        {
            deadState_temp = true;
            return; 
        }
        deadState_temp = false;
        write_times++;
        ramD[index] = val;
    }
    uint16_t read16(uint32_t index) const
    {
        if (index +1 >= ram_maxize)
        {
            deadState_temp = true; 
            return (uint32_t)dead_magicNum; 
        }
        deadState_temp = false;
        read_times++;
        return ((uint16_t)ramD[index] | (uint16_t)ramD[index +1] << 8);
    }
    uint32_t read32(uint32_t index) const
    {
        if (index +3 >= ram_maxize)
        {
            deadState_temp = true; 
            return (uint32_t)dead_magicNum; 
        }
        deadState_temp = false;
        read_times++;
        return ((uint32_t)ramD[index] | (uint32_t)ramD[index +1] << 8 | (uint32_t)ramD[index +2] << 16 | (uint32_t)ramD[index +3] << 24);
    }
    void write16(uint32_t index, uint16_t val)
    {
        if (index +1 >= ram_maxize)
        {
            deadState_temp = true; 
            return; 
        }
        deadState_temp = false;
        write_times++;
        ramD[index] = val;
        ramD[index +1] = val >> 8;
    }
    void write32(uint32_t index, uint32_t val)
    {
        if (index +3 >= ram_maxize)
        {
            deadState_temp = true; 
            return; 
        }
        deadState_temp = false;
        write_times++;
        ramD[index] = val;
        ramD[index +1] = val >> 8;
        ramD[index +2] = val >> 16;
        ramD[index +3] = val >> 24;
    }

    bool get_deadState() const {return deadState_temp; }

    uint32_t get_readTimes() const {return read_times; }
    uint32_t get_writeTimes() const {return write_times; }


    /*
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
    } //*/

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


class devIO
{
private:
    uint32_t UEID;

public:
    devIO(uint32_t ID) : UEID(ID) {}

};

class Center32x
{
private:
    class commandProcesser
    {
    private:
        reg32* flag;
        reg32* ACC;     // AX
        reg32* DRC;     // DX
        uint32_t* timE;
        Ram* ramP;


        // for temporary use
        reg32 copyT1, copyT2, copyT3, copyT4;
        uint16_t copyF1, copyF2, copyF3, copyF4;

    public:
        commandProcesser(reg32* flagN, reg32* AX, reg32* DX, uint32_t* timEN, Ram* ramG)
         : flag(flagN), ACC(AX), DRC(DX), timE(timEN), ramP(ramG) {}

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
        void cbw()
        {
            bool SignAL = ACC->ckbit(15);       // check the sign of AL
            if (SignAL) {ACC->high() = 0xffff; } else {ACC->high() = 0; }
        }
        void cwd()
        {
            bool SignAX = ACC->ckbit(31);       // check the sign of AX
            if (SignAX) {DRC->word(0xffffffff); } else {DRC->word(0); }
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
        void add(reg32* dst, reg32::HalfRef src)
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
        void flagSet4mul(bool CFcheck, bool OFcheck)
        {
            // CF(0), OF(11)
            if (CFcheck) {flag->stbit(0); } else {flag->clbit(0); }
            if (OFcheck) {flag->stbit(11); } else {flag->clbit(11); }
        }
        void mul(reg32 src)
        {
            uint64_t temp = ((uint64_t)ACC->word() * (uint64_t)src.word());
            ACC->word((uint32_t)temp);
            DRC->word((uint32_t)(temp >> 32));

            bool CFcheck, OFcheck;
            if (DRC->word() ==0) {CFcheck = OFcheck = false; } else {CFcheck = OFcheck = true; }

            flagSet4mul(CFcheck, OFcheck);
        }
        void mul(uint32_t ops32)
        {
            uint64_t temp =  ((uint64_t)ACC->word() * (uint64_t)ops32);
            ACC->word((uint32_t)temp);
            DRC->word((uint32_t)(temp >> 32));

            bool CFcheck, OFcheck;
            if (DRC->word() ==0) {CFcheck = OFcheck = false; } else {CFcheck = OFcheck = true; }

            flagSet4mul(CFcheck, OFcheck);
        }
        void mul(reg32::HalfRef src)
        {
            ACC->word((uint32_t)(ACC->low() * src));

            bool CFcheck, OFcheck;
            if (ACC->high() ==0) {CFcheck = OFcheck = false; } else {CFcheck = OFcheck = true; }

            flagSet4mul(CFcheck, OFcheck);
        }
        void mul(uint16_t ops16)
        {
            ACC->word((uint32_t)(ACC->low() * ops16));

            bool CFcheck, OFcheck;
            if (ACC->high() ==0) {CFcheck = OFcheck = false; } else {CFcheck = OFcheck = true; }

            flagSet4mul(CFcheck, OFcheck);
        }
        void flagSet4div()
        {
            // pass--
        }
        void div(reg32 src)
        {
            if (src.word() ==0) 
            {
                flag->stbit(1);     // Divide by zero
                return;
            }

            uint64_t divD = ((uint64_t)DRC->word() << 32) + ACC->word();
            ACC->word((uint32_t)(divD / src));
            DRC->word((uint32_t)(divD % src));

            flagSet4div();
        }
        void div(uint32_t ops32)
        {
            if (ops32 ==0)
            {
                flag->stbit(1);     // Divide by zero
                return;
            }

            uint64_t divD = ((uint64_t)DRC->word() << 32) + ACC->word();
            ACC->word((uint32_t)(divD / ops32));
            DRC->word((uint32_t)(divD % ops32));

            flagSet4div();
        }
        void div(reg32::HalfRef src)
        {
            if (src ==0)
            {
                flag->stbit(1);     // Divide by zero
                return;
            }

            copyT1.word(ACC->word());
            ACC->low() = copyT1.word() / src;
            ACC->high() = copyT1.word() % src;

            flagSet4div();
            copyT1.word(0);
        }
        void div(uint16_t ops16)
        {
            if (ops16 ==0)
            {
                flag->stbit(1);     // Divide by zero
                return;
            }

            copyT1.word(ACC->word());
            ACC->low() = copyT1.word() / ops16;
            ACC->high() = copyT1.word() % ops16;

            flagSet4div();
            copyT1.word(0);
        }
        void flagSet4imul(bool CFcheck, bool OFcheck)
        {
            flagSet4mul(CFcheck, OFcheck);
        }
        void imul(reg32 src)
        {
            int32_t AX_signed = static_cast<int32_t>(ACC->word());
            int32_t src_signed = static_cast<int32_t>(src.word());
            int64_t temp = (int64_t)AX_signed * src_signed;
            uint64_t tempBits = *(int64_t*)&temp;
            ACC->word(static_cast<uint32_t>(tempBits));
            DRC->word(static_cast<uint32_t>(tempBits >> 32));

            bool CFcheck, OFcheck;
            if (DRC->word() ==0) {CFcheck = OFcheck = false; } else {CFcheck = OFcheck = true; }

            flagSet4imul(CFcheck, OFcheck);
        }
        void imul(uint32_t ops32)
        {
            int32_t AX_signed = static_cast<int32_t>(ACC->word());
            int32_t src_signed = static_cast<int32_t>(ops32);
            int64_t temp = (int64_t)AX_signed * src_signed;
            uint64_t tempBits = *(int64_t*)&temp;
            ACC->word(static_cast<uint32_t>(tempBits));
            DRC->word(static_cast<uint32_t>(tempBits >> 32));

            bool CFcheck, OFcheck;
            if (DRC->word() ==0) {CFcheck = OFcheck = false; } else {CFcheck = OFcheck = true; }

            flagSet4imul(CFcheck, OFcheck);
        }
        void imul(reg32::HalfRef src)
        {
            int16_t AL_signed = static_cast<int16_t>(ACC->low());
            int16_t src_signed = static_cast<int16_t>(src);
            int32_t temp = (int32_t)AL_signed * src_signed;
            ACC->word(*(int32_t*)&temp);

            bool CFcheck, OFcheck;
            if (ACC->high() ==0) {CFcheck = OFcheck = false; } else {CFcheck = OFcheck = true; }

            flagSet4imul(CFcheck, OFcheck);
        }
        void imul(uint16_t ops16)
        {
            int16_t AL_signed = static_cast<int16_t>(ACC->low());
            int16_t src_signed = static_cast<int16_t>(ops16);
            int32_t temp = (int32_t)AL_signed * src_signed;
            ACC->word(*(int32_t*)&temp);

            bool CFcheck, OFcheck;
            if (ACC->high() ==0) {CFcheck = OFcheck = false; } else {CFcheck = OFcheck = true; }

            flagSet4imul(CFcheck, OFcheck);
        }
        void flagSet4idiv()
        {
            // pass--
        }
        void idiv(reg32 src)
        {
            if (src.word() ==0)
            {
                flag->stbit(1);     // Divide by zero
                return;
            }

            int64_t divD = static_cast<int64_t>(((uint64_t)DRC->word() << 32) + ACC->word());
            int32_t divS = static_cast<int32_t>(src.word());

            ACC->word((uint32_t)(int32_t)(divD / divS));
            DRC->word((uint32_t)(int32_t)(divD % divS));

            flagSet4idiv();
        }
        void idiv(uint32_t ops32)
        {
            if (ops32 ==0)
            {
                flag->stbit(1);     // Divide by zero
                return;
            }

            int64_t divD = static_cast<int64_t>(((uint64_t)DRC->word() << 32) + ACC->word());
            int32_t divS = static_cast<int32_t>(ops32);

            ACC->word((uint32_t)(int32_t)(divD / divS));
            DRC->word((uint32_t)(int32_t)(divD % divS));

            flagSet4idiv();
        }
        void idiv(reg32::HalfRef src)
        {
            if (src ==0)
            {
                flag->stbit(1);     // Divide by zero
                return;
            }

            int32_t divD = static_cast<int32_t>(ACC->word());
            int16_t divS = static_cast<int16_t>(src);

            ACC->low() = (uint16_t)(int16_t)(divD / divS);
            ACC->high() = (uint16_t)(int16_t)(divD % divS);

            flagSet4idiv();
        }
        void idiv(uint16_t ops16)
        {
            if (ops16 ==0)
            {
                flag->stbit(1);     // Divide by zero
                return;
            }

            int32_t divD = static_cast<int32_t>(ACC->word());
            int16_t divS = static_cast<int16_t>(ops16);

            ACC->low() = (uint16_t)(int16_t)(divD / divS);
            ACC->high() = (uint16_t)(int16_t)(divD % divS);

            flagSet4idiv();
        }
        void flagSet4logic(bool isParity, bool EZ, bool dstMSB)
        {
            // CF(0)=0, PF(2), ZF(6), SF(7), OF(11)=0
            flag->clbit(0);
            if (isParity) {flag->stbit(2); } else {flag->clbit(2); }
            if (EZ) {flag->stbit(6); } else {flag->clbit(6); }
            if (dstMSB) {flag->stbit(7); } else {flag->clbit(7); }
            flag->clbit(11);
        }
        void andL(reg32* dst, reg32 src)
        {
            dst->word(dst->word() & src.word());

            bool dstMSB = (dst->ckbit(31));

            flagSet4logic(dst->isParityL8(), *dst ==0, dstMSB);
        }
        void andL(reg32* dst, uint32_t src)
        {
            dst->word(dst->word() & src);

            bool dstMSB = (dst->ckbit(31));

            flagSet4logic(dst->isParityL8(), *dst ==0, dstMSB);
        }
        void andL(reg32* dst, reg32::HalfRef src)
        {
            dst->word(dst->word() & (uint32_t)src);

            bool dstMSB = (dst->ckbit(31));

            flagSet4logic(dst->isParityL8(), *dst ==0, dstMSB);
        }
        void andL(reg32::HalfRef* dst, reg32::HalfRef src)
        {
            *dst = *dst & src;

            bool dstMSB = (dst->ckbit(15));

            flagSet4logic(dst->isParity(), *dst ==0, dstMSB);
        }
        void andL(reg32::HalfRef* dst, uint16_t src)
        {
            *dst = *dst & src;

            bool dstMSB = (dst->ckbit(15));

            flagSet4logic(dst->isParity(), *dst ==0, dstMSB);
        }
        void orL(reg32* dst, reg32 src)
        {
            dst->word(dst->word() | src.word());

            bool dstMSB = (dst->ckbit(31));

            flagSet4logic(dst->isParityL8(), *dst ==0, dstMSB);
        }
        void orL(reg32* dst, uint32_t src)
        {
            dst->word(dst->word() | src);

            bool dstMSB = (dst->ckbit(31));

            flagSet4logic(dst->isParityL8(), *dst ==0, dstMSB);
        }
        void orL(reg32* dst, reg32::HalfRef src)
        {
            dst->word(dst->word() | (uint32_t)src);

            bool dstMSB = (dst->ckbit(31));

            flagSet4logic(dst->isParityL8(), *dst ==0, dstMSB);
        }
        void orL(reg32::HalfRef* dst, reg32::HalfRef src)
        {
            *dst = *dst | src;

            bool dstMSB = (dst->ckbit(15));

            flagSet4logic(dst->isParity(), *dst ==0, dstMSB);
        }
        void orL(reg32::HalfRef* dst, uint16_t src)
        {
            *dst = *dst | src;

            bool dstMSB = (dst->ckbit(15));

            flagSet4logic(dst->isParity(), *dst ==0, dstMSB);
        }
        void xorL(reg32* dst, reg32 src)
        {
            dst->word(dst->word() ^ src.word());

            bool dstMSB = (dst->ckbit(31));

            flagSet4logic(dst->isParityL8(), *dst ==0, dstMSB);
        }
        void xorL(reg32* dst, uint32_t src)
        {
            dst->word(dst->word() ^ src);

            bool dstMSB = (dst->ckbit(31));

            flagSet4logic(dst->isParityL8(), *dst ==0, dstMSB);
        }
        void xorL(reg32* dst, reg32::HalfRef src)
        {
            dst->word(dst->word() ^ (uint32_t)src);

            bool dstMSB = (dst->ckbit(31));

            flagSet4logic(dst->isParityL8(), *dst ==0, dstMSB);
        }
        void xorL(reg32::HalfRef* dst, reg32::HalfRef src)
        {
            *dst = *dst ^ src;

            bool dstMSB = (dst->ckbit(15));

            flagSet4logic(dst->isParity(), *dst ==0, dstMSB);
        }
        void xorL(reg32::HalfRef* dst, uint16_t src)
        {
            *dst = *dst ^ src;

            bool dstMSB = (dst->ckbit(15));

            flagSet4logic(dst->isParity(), *dst ==0, dstMSB);
        }
        void test(reg32 dst, reg32 src)
        {
            copyT1.word(dst.word());
            andL(&copyT1, src);

            copyT1.word(0);
        }
        void test(reg32 dst, uint32_t src)
        {
            copyT1.word(dst.word());
            andL(&copyT1, src);

            copyT1.word(0);
        }
        void test(reg32 dst, reg32::HalfRef src)
        {
            copyT1.word(dst.word());
            andL(&copyT1, src);

            copyT1.word(0);
        }
        void test(reg32::HalfRef dst, reg32::HalfRef src)
        {
            copyT1.low() = (uint16_t)dst;
            reg32::HalfRef dstT = copyT1.low();
            andL(&dstT, src);

            copyT1.low() = 0;
        }
        void test(reg32::HalfRef dst, uint16_t src)
        {
            copyT1.low() = (uint16_t)dst;
            reg32::HalfRef dstT = copyT1.low();
            andL(&dstT, src);

            copyT1.low() = 0;
        }
        void cmp(reg32 dst, reg32 src)
        {
            copyT1.word(dst.word());
            sub(&copyT1, src);

            copyT1.word(0);
        }
        void cmp(reg32 dst, uint32_t src)
        {
            copyT1.word(dst.word());
            sub(&copyT1, src);

            copyT1.word(0);
        }
        void cmp(reg32 dst, reg32::HalfRef src)
        {
            copyT1.word(dst.word());
            sub(&copyT1, src);

            copyT1.word(0);
        }
        void cmp(reg32::HalfRef dst, reg32::HalfRef src)
        {
            copyT1.low() = (uint16_t)dst;
            reg32::HalfRef dstT = copyT1.low();
            sub(&dstT, src);

            copyT1.low() = 0;
        }
        void cmp(reg32::HalfRef dst, uint16_t src)
        {
            copyT1.low() = (uint16_t)dst;
            reg32::HalfRef dstT = copyT1.low();
            sub(&dstT, src);

            copyT1.low() = 0;
        }
        
    };

    class commandDecoder
    {
    private:
        Center32x::commandProcesser* cmdPcsr;
        reg32* regs;
        reg32* segs;
        uint32_t timE;
        Ram* ramP;

        reg32::HalfRef* halfRegs[regs_maxize];

    public:
        commandDecoder(Center32x::commandProcesser* cmds, reg32* regsN, reg32* segsN, uint32_t timEN, Ram* ramG)
         : cmdPcsr(cmds), regs(regsN), segs(segsN), timE(timEN), ramP(ramG) 
        {
            int halfRegsLength = regs_maxize / 2;
            for (int i =0; i < halfRegsLength; ++i)
            {
                reg32::HalfRef low(regs[i].low()), high(regs[i].high());
                halfRegs[i] = &low;
                halfRegs[i + halfRegsLength] = &high;
            }
        }

        
        /*
            >   number happens to be the index in the Array regs (without FLAG)
            0b0000 -> AX,   0b0001 -> CX,   0b0010 -> DX,   0b0011 -> BX,
            0b0100 -> SP,   0b0101 -> BP,   0b0110 -> SI,   0b0111 -> DI,
                <
            
            0b1000 -> AL,   0b1001 -> CL,   0b1010 -> DL,   0b1011 -> BL,
            0b1100 -> AH,   0b1101 -> CH,   0b1110 -> DH,   0b1111 -> BH

            >   0xf0 + index
            0b0000 -> CS,   0b0001 -> DS,   0b0010 -> SS,   0b0011 -> ES,
            0b0100 -> FS,   0b0101 -> GS
                <

        */

        bool get_deadState_set_MOFstate()
        {
            bool isDead = ramP->get_deadState();
            if (isDead)
            {
                // Memory Overflow
                regs[8].stbit(31);
            }
            return isDead;
        }

        void decode(uint8_t* commands[commandsArr_maxize])
        {
            uint8_t opcode = *commands[0];

            switch (opcode)
            {
                
                case 0x00: // add reg reg, Bytes 2, [add, reg reg]
                {
                    uint8_t ops1base = (*commands[1] & 0x0f);
                    uint8_t ops2base = (*commands[1] >> 4);
                    bool ops1isHalf = (ops1base & 0b1000) != 0;
                    bool ops2isHalf = (ops2base & 0b1000) != 0;

                    if (ops1isHalf) // ops1 is HalfReg
                    {
                        uint8_t ops1index = std::min(ops1base & 0b0111, regs_maxize -1);

                        if (ops2isHalf) // ops2 is HalfReg
                        {
                            uint8_t ops2index = std::min(ops2base & 0b0111, regs_maxize -1);
                            cmdPcsr->add(halfRegs[ops1index], *halfRegs[ops2index]);
                        } 
                        else // ops2 is FullReg
                        {
                            uint8_t ops2index = std::min(ops2base & 0b0111, regs_maxize -1);
                            cmdPcsr->add(halfRegs[ops1index], (uint16_t)regs[ops2index]);
                        }
                    }
                    else // ops1 is FullReg
                    {
                        uint8_t ops1index = std::min(ops1base & 0b0111, regs_maxize -1);

                        if (ops2isHalf) // ops2 is HalfReg
                        {
                            uint8_t ops2index = std::min(ops2base & 0b0111, regs_maxize -1);
                            cmdPcsr->add(&regs[ops1index], *halfRegs[ops2index]);
                        } 
                        else // ops2 is FullReg
                        {
                            uint8_t ops2index = std::min(ops2base & 0b0111, regs_maxize -1);
                            cmdPcsr->add(&regs[ops1index], regs[ops2index]);
                        }
                    }

                    break;
                }
                
                case 0x02: // add mem32/16 r32, Bytes 3, [add, ModR/M, s32]
                {
                    uint8_t mode = (*commands[1] >> 7) & 0b1;
                    uint8_t srcRegIndex = (*commands[1] >> 3) & 0b0111;
                    uint8_t baseRegIndex = *commands[1] & 0b111;
                    uint8_t segIndex = *commands[2] & 0b111;
                    bool isHalf = (*commands[1] & 0b1000) != 0;

                    uint32_t memAddr = 0;
                    reg32 memVal = 0;

                    if (mode == 0b0) // add mem32 reg32
                    {
                        memAddr = regs[baseRegIndex].word() + segs[segIndex].word() * ramSegs_maxize;
                        memVal.word(ramP->read32(memAddr));

                        if (get_deadState_set_MOFstate()) {return; }
                        
                        // HalfReg
                        if (isHalf) {cmdPcsr->add(&memVal, *halfRegs[srcRegIndex]); }
                        // FullReg
                        else {cmdPcsr->add(&memVal, regs[srcRegIndex]); }

                        ramP->write32(memAddr, memVal.word());

                    }
                    else // add mem16 reg32
                    {
                        memAddr = regs[baseRegIndex].word() + segs[segIndex].word() * ramSegs_maxize;
                        memVal.low() = ramP->read16(memAddr);

                        if (get_deadState_set_MOFstate()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->add(&memVal, *halfRegs[srcRegIndex]); }
                        // FullReg
                        else {cmdPcsr->add(&memVal, regs[srcRegIndex]); }

                        ramP->write16(memAddr, memVal.low());

                    }

                    break;
                }

                case 0x03: // add r32 mem32/16, Bytes 3, [add, ModR/M, s32]
                {
                    uint8_t mode = (*commands[1] >> 7) & 0b1;
                    uint8_t srcRegIndex = (*commands[1] >> 3) & 0b0111;
                    uint8_t baseRegIndex = *commands[1] & 0b111;
                    uint8_t segIndex = *commands[2] & 0b111;
                    bool isHalf = (*commands[1] & 0b1000) != 0;

                    uint32_t memAddr = 0;
                    reg32 memVal = 0;

                    break;
                }
                
                case 0x04: // add AL imm16, Bytes 3, [add, immL, immH]
                {
                    uint16_t imm = ((uint16_t)*commands[1] | (uint16_t)*commands[2] << 8);
                    cmdPcsr->add(halfRegs[0], imm);

                    break;
                }
                
                case 0x05: // add AX imm32, Bytes 5, [add, immLL, immLH, immHL, immHH]
                {
                    uint32_t imm = ((uint32_t)*commands[1] | (uint32_t)*commands[2] << 8 | (uint32_t)*commands[3] << 16 | (uint32_t)*commands[4] << 24);
                    cmdPcsr->add(&regs[0], imm);

                    break;
                }
            }
        }
    
    };

    uint32_t IDt;
    uint32_t EID;
    uint32_t UD;
    uint32_t timE;
    Ram* ramP;
    bool runningState = false;

    reg32 FLAG = 0;
    /*  flags   index   bits    description
        CF      0       1       Carry Flag;
    >>  DBZ     1       1       Divide By Zero;
        PF      2       1       Parity Flag;
        ZF      6       1       Zero Flag;
        SF      7       1       Sign Flag;
        IF      9       1       Interrupt Flag;
        DF      10      1       Direction Flag;
        OF      11      1       Overflow Flag;
        IOPL    12-13   2       I/O Privilege Level;
        NT      14      1       Nested Task;
        RF      16      1       Resume Flag;
    >>  MOF     31      1       Memory Overflow Flag;
    */

    reg32 segs[8];
    reg32& CS = segs[0];
    reg32& DS = segs[1];
    reg32& SS = segs[2];
    reg32& ES = segs[3];
    reg32& FS = segs[4];
    reg32& GS = segs[5];

    reg32 AX, CX, DX, BX;
    reg32 SP, BP, SI, DI;
    reg32* regs[regs_maxize] = {&AX, &CX, &DX, &BX, &SP, &BP, &SI, &DI, &FLAG};

    reg32 IP;
    
    commandProcesser cmdPcsr{&FLAG, &AX, &DX, &timE, ramP};
    
    std::map<uint16_t, devIO*> devsRegistry;

public:
    Center32x(uint32_t ID, Ram* ramG)
    {
        IDt = ID;
        UD = Hasher::hash_uint32(IDt);
        uint8_t temp = UD & 0b0111;
        EID = Hasher::get_randNumHR2(UD, temp);
        ramP = ramG;
    }

    void start() {runningState = true; }
    void stop() {runningState = false; }
    void interruptStateSetd() 
    {
        if (FLAG.ckbit(9))
        {
            FLAG.clbit(9);
            runningState = false;
        }
    }
    void ___interruptStateClrd()
    {
        if (!FLAG.ckbit(9))
        {
            FLAG.stbit(9);
            runningState = true;
        }
    }

    void getSegs(reg32* dstArray) const
    {
        for (int index =0; index < segs_maxize; ++index)
        {
            dstArray[index].word(segs[index].word());
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

class rootOP
{};

class debugCMD
{
private:
    Center32x* centers[centers_maxize];
    reg32 regs_0f_centers[centers_maxize][regs_maxize];
    reg32 segs_0f_centers[centers_maxize][segs_maxize];

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

void test() 
{
    return;
}


int main()
{
    initG();
    Hasher ashor;


    //* test
    test();
    //*/
    system("pause");
    return 0;
}


