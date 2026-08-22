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
#include <thread>
#include <bitset>
#include <mutex>
#include <atomic>
#include <fluidsynth.h>
#include <stdexcept>

/*
#ifdef _WIN32
#include <windows.h>
HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
DWORD dwMode = 0;
GetConsoleMode(hOut, &dwMode);
dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
SetConsoleMode(hOut, dwMode);
#endif
//*/



// ___<funcName> means been deprecated or its purpose is unclear, so better not to use  xP;
// <<varName>T> or <<VARnAME>t> means Temp, Test or Total;
// <<varName>G> means Global;
// <<varName>P> means it Points to;
// <<varName>E> means it s about Equipment;
// <<varName>C> means current;
// <<varName>N> means New;

/* ModR/M - modified --
    Mod     7       0 is mem32, 1 is mem16;
    Reg     3-6     choose full or half register;
    Mem     0-2     choose full register as base register;
*/


const int centers_maxize = 2;//4;
const int regs_maxize = 9;
const int segs_maxize = 8;
const int commandsArr_maxize = 10;
const uint32_t ram_maxize = 0x7fffffff;     // uint8 -> 1 Byte * 0x7fffffff => ~ 2 GiB
const uint32_t stack_maxize = 0xfffff;      // uint8 -> 1 Byte * 0xfffff => ~ 1 MiB
const uint32_t gram_maxize = 0xfffffff;     // uint8 -> 1 Byte * 0xfffffff => ~ 256 MiB
const uint32_t ramSegPer_maxize = 262144;   // 256 KiB
const uint16_t opc_maxize = 0xff;

const uint8_t dead_magicNum = 0xcc;
uint8_t* opCode_decodeTable = new uint8_t[opc_maxize +1]();

const uint32_t UKED_EntropyDev = 0xffe7c099;
const uint32_t UKED_GraphicDev = 0xf6ca951c;
const uint32_t UKED_Center2OPtransferDev = 0x7ca5fc20;
const uint32_t UKED_Center2centerTransferDev = 0x7ca5fc2c;
const uint32_t UKED_SoundCarDev = 0xf50dcacd;


class Ram
{
private:
    uint32_t ram_size;
    const uint32_t *ramSizeP = &ram_size;
    const static uint32_t ramSegs_size = ramSegPer_maxize;
    const static uint32_t ramSegs_maxcount = ram_maxize / ramSegPer_maxize;
    const static uint32_t PID_maxize = 0xffff;
    const static uint16_t rootPID = 0x00, constPID = 0x01, publicPID = 0xffff;
    mutable uint32_t read_times = 0;
    mutable uint32_t write_times = 0;
    mutable bool deadState_temp = false;
    mutable bool UAstate_0f_PID[PID_maxize +1] = {false};
    mutable uint8_t* ramD;
    
    // 0x00 is root PID, same means it s locked for other Processes tha cant visit;
    // 0x01 is const PID, same means every Process can visit tha ram segmentation locked by it but read only except root PID;
    // 0xffff is public PID, same means every Process can visit the ram segmentation locked by it;
    mutable uint8_t ramSgmtation_LocktPID[ramSegs_maxcount] = {0x00};

public:
    std::mutex raMutexC;


    Ram(uint32_t ramSizeT=ram_maxize) : ram_size(std::min(ram_maxize, std::max(ramSegPer_maxize, ramSizeT))), ramD(new uint8_t[ram_size]()) {}

    bool PIDcanRead(uint32_t index, uint16_t PID) const
    {
        uint32_t index_ramSeg = index / ramSegs_size;
        uint8_t ramPID = ramSgmtation_LocktPID[index_ramSeg];
        if (ramPID == rootPID) {return false; }
        if (ramPID == constPID) {return true; }
        if (ramPID == publicPID) {return true; }
        return PID == ramPID;
    }
    bool PIDcanWrite(uint32_t index, uint16_t PID) const
    {
        uint32_t index_ramSeg = index / ramSegs_size;
        uint8_t ramPID = ramSgmtation_LocktPID[index_ramSeg];
        if (ramPID == rootPID) {return false; }
        if (ramPID == constPID) {return false; }
        if (ramPID == publicPID) {return true; }
        return PID == ramPID;
    }

    operator uint8_t() const {return *ramD; }

    uint8_t read(uint32_t index, uint16_t PID=publicPID) const
    {
        bool canVisit = true; // root PID can pass
        if (!(PID == rootPID)) {canVisit = PIDcanRead(index, PID); }
        if (!canVisit) 
        {
            UAstate_0f_PID[PID] = true;
            return (uint8_t)rootPID;
        }
        if (index > ram_size) 
        {
            deadState_temp = true; 
            return dead_magicNum; 
        }
        deadState_temp = false;
        read_times++;
        return ramD[index];
    }
    void write(uint32_t index, uint8_t val, uint16_t PID=publicPID)
    {
        bool canVisit = true; // root PID can pass
        if (!(PID == rootPID)) {canVisit = PIDcanWrite(index, PID); }
        if (!canVisit) 
        {
            UAstate_0f_PID[PID] = true;
            return; 
        }
        if (index > ram_size) 
        {
            deadState_temp = true;
            return; 
        }
        deadState_temp = false;
        write_times++;
        ramD[index] = val;
    }
    uint16_t read16(uint32_t index, uint16_t PID=publicPID) const
    {
        bool canVisit = true; // root PID can pass
        if (!(PID == rootPID)) {canVisit = PIDcanRead(index, PID) && PIDcanRead(index +1, PID); }
        if (!canVisit) 
        {
            UAstate_0f_PID[PID] = true;
            return rootPID;
        }
        if (index +1 > ram_size)
        {
            deadState_temp = true; 
            return (uint32_t)dead_magicNum; 
        }
        deadState_temp = false;
        read_times++;
        return ((uint16_t)ramD[index] | (uint16_t)ramD[index +1] << 8);
    }
    uint32_t read32(uint32_t index, uint16_t PID=publicPID) const
    {
        bool canVisit = true; // root PID can pass
        if (!(PID == rootPID)) {canVisit = PIDcanRead(index, PID) && PIDcanRead(index +3, PID); }
        if (!canVisit) 
        {
            UAstate_0f_PID[PID] = true;
            return (uint32_t)rootPID;
        }
        if (index +3 > ram_size)
        {
            deadState_temp = true; 
            return (uint32_t)dead_magicNum; 
        }
        deadState_temp = false;
        read_times++;
        return ((uint32_t)ramD[index] | (uint32_t)ramD[index +1] << 8 | (uint32_t)ramD[index +2] << 16 | (uint32_t)ramD[index +3] << 24);
    }
    void write16(uint32_t index, uint16_t val, uint16_t PID=publicPID)
    {
        bool canVisit = true; // root PID can pass
        if (!(PID == rootPID)) {canVisit = PIDcanWrite(index, PID) && PIDcanWrite(index +1, PID); }
        if (!canVisit) 
        {
            UAstate_0f_PID[PID] = true;
            return; 
        }
        if (index +1 > ram_size)
        {
            deadState_temp = true; 
            return; 
        }
        deadState_temp = false;
        write_times++;
        ramD[index] = val;
        ramD[index +1] = val >> 8;
    }
    void write32(uint32_t index, uint32_t val, uint16_t PID=publicPID)
    {
        bool canVisit = true; // root PID can pass
        if (!(PID == rootPID)) {canVisit = PIDcanWrite(index, PID) && PIDcanWrite(index +3, PID); }
        if (!canVisit) 
        {
            UAstate_0f_PID[PID] = true;
            return; 
        }
        if (index +3 > ram_size)
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
    bool get_UAstate(uint16_t PID) const {return UAstate_0f_PID[PID]; }

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
        HalfRef(reg32& reg, int shift) : _whole(reg._value), _shift(shift) {}

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
        HalfRef& operator=(const HalfRef& otherHalfReg) 
        {
            uint32_t mask = 0xffffu << _shift;
            _whole = (_whole & ~mask) | (otherHalfReg._whole & mask); 

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
    uint32_t UKUEUD;

public:
    devIO(uint32_t ID, uint32_t UKED) : UEID(ID), UKUEUD(UKED) {}

    uint32_t get_UEID() const {return UEID; }
    uint32_t get_UKED() const {return UKUEUD; }

};

class EntropyDev : public devIO
{
private:
    Hasher* hashP;

public:
    EntropyDev(uint32_t ID, Hasher* hashG)
     : hashP(hashG), devIO(hashP->get_randNumHR2(ID), UKED_EntropyDev) {}

};

class GraphicDev : public devIO
{
private:
    uint8_t GramD[gram_maxize] = {0};

public:
    GraphicDev(uint32_t ID, Hasher* hashG)
     : devIO(hashG->get_randNumHR2(ID), UKED_GraphicDev) {}

};

class SoundCarDev : public devIO
{
private:
    std::string SFpathP;
    fluid_settings_t* settings = nullptr;
    fluid_synth_t* synthP = nullptr;
    int sfont_id = -1;

public:
    SoundCarDev(uint32_t ID, Hasher* hashG, std::string SFpath)
     : devIO(hashG->get_randNumHR2(ID), UKED_SoundCarDev), SFpathP(SFpath) 
    {
        settings = new_fluid_settings(); // init the settings
        fluid_settings_setnum(settings, "synth.sample-rate", 44100.0); // set the sampling rate
        fluid_settings_setint(settings, "synth.verbose", 0); // turn off extra output

        synthP = new_fluid_synth(settings); // chreate the synthesizer instance
        if (!synthP) throw std::runtime_error("Failed to create synthesizer");

        sfont_id = fluid_synth_sfload(synthP, SFpathP.c_str(), 1); // load the SF2 file
        if (sfont_id == -1) throw std::runtime_error("Failed to load SF2 file");
    }

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

        void nop(uint8_t time) const
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(time));
        }
        void nop() const
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(32)); 
        }
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
        commandProcesser* cmdPcsr;
        uint16_t* PID;
        reg32* regs;
        reg32* segs;
        uint32_t* timE;
        Ram* ramP;

        reg32::HalfRef halfRegs[8];

    public:
        commandDecoder(commandProcesser* cmds, reg32* regsN, reg32* segsN, uint32_t* timEN, Ram* ramG, uint16_t* PIDN)
         : cmdPcsr(cmds), regs(regsN), segs(segsN), timE(timEN), ramP(ramG), PID(PIDN), halfRegs{
            reg32::HalfRef(regsN[0], 0), reg32::HalfRef(regsN[1], 0), reg32::HalfRef(regsN[2], 0), reg32::HalfRef(regsN[3], 0),
            reg32::HalfRef(regsN[0], 16), reg32::HalfRef(regsN[1], 16), reg32::HalfRef(regsN[2], 16), reg32::HalfRef(regsN[3], 16)
         }
        {}

        
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

        bool postProcessing_afterRamRW()
        {
            bool isDead = ramP->get_deadState();
            bool isDead2;
            bool isUA = ramP->get_UAstate(*PID);
            if (isUA)
            {
                // Unauthorized Access
                regs[8].stbit(30);
            }
            if (isDead)
            {
                isDead2 = ramP->get_deadState();
                if (isDead2)
                {
                    // Memory Overflow
                    regs[8].stbit(31);
                } else 
                {return true; }
            }
            return isDead || isUA;
        }

        void decode(uint8_t* commands)
        {
            uint8_t opcode = commands[0];
            std::unique_lock<std::mutex> ram_lock(ramP->raMutexC);

            switch (opcode)
            {
                default:
                {
                    break;
                }
                
                case 0x00: // add reg reg, Bytes 2, [add, src(4)|dst(4)]
                {
                    uint8_t dstBase = (commands[1] & 0x0f);
                    uint8_t srcBase = (commands[1] >> 4);
                    bool dstIsHalf = (dstBase & 0b1000) != 0;
                    bool srcIsHalf = (srcBase & 0b1000) != 0;

                    if (dstIsHalf) // dst is HalfReg
                    {
                        uint8_t dstIndex = std::min(dstBase & 0b0111, regs_maxize -1);

                        if (srcIsHalf) // src is HalfReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->add(&halfRegs[dstIndex], halfRegs[srcIndex]);
                        } 
                        else // src is FullReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->add(&halfRegs[dstIndex], (uint16_t)regs[srcIndex]);
                        }
                    }
                    else // dst is FullReg
                    {
                        uint8_t dstIndex = std::min(dstBase & 0b0111, regs_maxize -1);

                        if (srcIsHalf) // src is HalfReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->add(&regs[dstIndex], halfRegs[srcIndex]);
                        } 
                        else // src is FullReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->add(&regs[dstIndex], regs[srcIndex]);
                        }
                    }

                    break;
                }

                case 0x01: // add r/m32/16 m/r32/16, Bytes 7, [add, modRWR(1)|mod32/16(1)|(2)|reg(4), s32, addrLL, addrLH, addrHL, addrHH]
                {
                    uint8_t mode_RM_MR = (commands[1] >> 7) & 0b1;
                    uint8_t mode_32_16 = (commands[1] >> 6) & 0b1;
                    uint8_t regIndex = (commands[1] & 0b0111);
                    uint8_t segIndex = commands[2] & 0b111;
                    bool isHalf = (commands[1] & 0b1000) != 0;

                    uint32_t memAddr = ((uint32_t)commands[3] | ((uint32_t)commands[4] << 8) 
                        | ((uint32_t)commands[5] << 16) | ((uint32_t)commands[6] << 24)) + segs[segIndex].word() * ramSegPer_maxize;
                    reg32 memVal = 0;

                    if (mode_RM_MR == 0b0) // add r32/16 m32/16
                    {
                        if (mode_32_16 == 0b0) // add r32 m32
                        {
                            memVal.word(ramP->read32(memAddr, *PID));
                            if (postProcessing_afterRamRW()) {return; }
                            cmdPcsr->add(&regs[regIndex], memVal);
                        }
                        else // add r32/16 m16
                        {
                            memVal.low() = ramP->read16(memAddr, *PID);
                            if (postProcessing_afterRamRW()) {return; }
                            uint16_t memVal16 = memVal.low();

                            if (isHalf) {cmdPcsr->add(&halfRegs[regIndex], memVal16); }
                            else {cmdPcsr->add(&regs[regIndex], memVal16); }
                        }
                    }
                    else // add m32/16 r32/16
                    {
                        if (mode_32_16 == 0b0) // add m32 r32
                        {
                            memVal.word(ramP->read32(memAddr, *PID));
                            if (postProcessing_afterRamRW()) {return; }
                            cmdPcsr->add(&memVal, regs[regIndex]);
                            ramP->write32(memAddr, memVal.word(), *PID);
                        }
                        else // add m16 r32/16(->val16)
                        {
                            memVal.low() = ramP->read16(memAddr, *PID);
                            if (postProcessing_afterRamRW()) {return; }
                            reg32::HalfRef memVal16 = memVal.low();
                            
                            if (isHalf) {cmdPcsr->add(&memVal16, halfRegs[regIndex]); }
                            else {cmdPcsr->add(&memVal16, (uint16_t)regs[regIndex]); }
                            ramP->write16(memAddr, memVal16, *PID);
                        }
                    }
                    
                    break;
                }
                
                case 0x02: // add m32/16 r32/16, Bytes 3, [add, ModR/M, s32]
                {
                    uint8_t mode = (commands[1] >> 7) & 0b1;
                    uint8_t srcRegIndex = (commands[1] >> 3) & 0b0111;
                    uint8_t baseRegIndex = commands[1] & 0b111;
                    uint8_t segIndex = commands[2] & 0b111;
                    bool isHalf = (commands[1] & 0b1000) != 0;

                    uint32_t memAddr = regs[baseRegIndex].word() + segs[segIndex].word() * ramSegPer_maxize;
                    reg32 memVal = 0;

                    if (mode == 0b0) // add mem32 r32/16
                    {
                        memVal.word(ramP->read32(memAddr, *PID));

                        if (postProcessing_afterRamRW()) {return; }
                        
                        // HalfReg
                        if (isHalf) {cmdPcsr->add(&memVal, halfRegs[srcRegIndex]); }
                        // FullReg
                        else {cmdPcsr->add(&memVal, regs[srcRegIndex]); }

                        ramP->write32(memAddr, memVal.word(), *PID);

                    }
                    else // add mem16 r32/16
                    {
                        memVal.low() = ramP->read16(memAddr, *PID);

                        if (postProcessing_afterRamRW()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->add(&memVal, halfRegs[srcRegIndex]); }
                        // FullReg
                        else {cmdPcsr->add(&memVal, regs[srcRegIndex]); }

                        ramP->write16(memAddr, memVal.low(), *PID);

                    }

                    break;
                }

                case 0x03: // add r32/16 m32/16, Bytes 3, [add, ModR/M, s32]
                {
                    uint8_t mode = (commands[1] >> 7) & 0b1;
                    uint8_t dstRegIndex = (commands[1] >> 3) & 0b0111;
                    uint8_t baseRegIndex = commands[1] & 0b111;
                    uint8_t segIndex = commands[2] & 0b111;
                    bool isHalf = (commands[1] & 0b1000) != 0;

                    uint32_t memAddr = regs[baseRegIndex].word() + segs[segIndex].word() * ramSegPer_maxize;
                    uint32_t memVal = 0;

                    if (mode == 0b0) // add r32/16 mem32
                    {
                        memVal |= ramP->read32(memAddr, *PID);

                        if (postProcessing_afterRamRW()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->add(&halfRegs[dstRegIndex], (uint16_t)memVal); }
                        // FullReg
                        else {cmdPcsr->add(&regs[dstRegIndex], memVal); }
                    }
                    else // add r32/16 mem16
                    {
                        memVal |= ramP->read16(memAddr, *PID);

                        if (postProcessing_afterRamRW()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->add(&halfRegs[dstRegIndex], (uint16_t)memVal); }
                        // FullReg
                        else {cmdPcsr->add(&regs[dstRegIndex], memVal); }
                    }

                    break;
                }
                
                case 0x04: // add AL imm16, Bytes 3, [add, immL, immH]
                {
                    uint16_t imm = ((uint16_t)commands[1] | ((uint16_t)commands[2] << 8));
                    cmdPcsr->add(&halfRegs[0], imm);

                    break;
                }
                
                case 0x05: // add AX imm32, Bytes 5, [add, immLL, immLH, immHL, immHH]
                {
                    uint32_t imm = ((uint32_t)commands[1] | ((uint32_t)commands[2] << 8) 
                        | ((uint32_t)commands[3] << 16) | ((uint32_t)commands[4] << 24));
                    cmdPcsr->add(&regs[0], imm);

                    break;
                }
            
                case 0x08: // or reg reg, Bytes 2, [or, src(4)|dst(4)]
                {
                    uint8_t dstBase = (commands[1] & 0x0f);
                    uint8_t srcBase = (commands[1] >> 4);
                    bool dstIsHalf = (dstBase & 0b1000) != 0;
                    bool srcIsHalf = (srcBase & 0b1000) != 0;

                    if (dstIsHalf) // dst is HalfReg
                    {
                        uint8_t dstIndex = std::min(dstBase & 0b0111, regs_maxize -1);

                        if (srcIsHalf) // src is HalfReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->orL(&halfRegs[dstIndex], halfRegs[srcIndex]);
                        } 
                        else // src is FullReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->orL(&halfRegs[dstIndex], (uint16_t)regs[srcIndex]);
                        }
                    }
                    else // dst is FullReg
                    {
                        uint8_t dstIndex = std::min(dstBase & 0b0111, regs_maxize -1);

                        if (srcIsHalf) // src is HalfReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->orL(&regs[dstIndex], halfRegs[srcIndex]);
                        } 
                        else // src is FullReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->orL(&regs[dstIndex], regs[srcIndex]);
                        }
                    }

                    break;
                }

                case 0x09: // or r/m32/16 m/r32/16, Bytes 7, [or, modRWR(1)|mod32/16(1)|(2)|reg(4), s32, addrLL, addrLH, addrHL, addrHH]
                {
                    uint8_t mode_RM_MR = (commands[1] >> 7) & 0b1;
                    uint8_t mode_32_16 = (commands[1] >> 6) & 0b1;
                    uint8_t regIndex = (commands[1] & 0b0111);
                    uint8_t segIndex = commands[2] & 0b111;
                    bool isHalf = (commands[1] & 0b1000) != 0;

                    uint32_t memAddr = ((uint32_t)commands[3] | ((uint32_t)commands[4] << 8) 
                        | ((uint32_t)commands[5] << 16) | ((uint32_t)commands[6] << 24)) + segs[segIndex].word() * ramSegPer_maxize;
                    reg32 memVal = 0;

                    if (mode_RM_MR == 0b0) // or r32/16 m32/16
                    {
                        if (mode_32_16 == 0b0) // or r32 m32
                        {
                            memVal.word(ramP->read32(memAddr, *PID));
                            if (postProcessing_afterRamRW()) {return; }
                            cmdPcsr->orL(&regs[regIndex], memVal);
                        }
                        else // or r32/16 m16
                        {
                            memVal.low() = ramP->read16(memAddr, *PID);
                            if (postProcessing_afterRamRW()) {return; }
                            uint16_t memVal16 = memVal.low();

                            if (isHalf) {cmdPcsr->orL(&halfRegs[regIndex], memVal16); }
                            else {cmdPcsr->orL(&regs[regIndex], memVal16); }
                        }
                    }
                    else // or m32/16 r32/16
                    {
                        if (mode_32_16 == 0b0) // or m32 r32
                        {
                            memVal.word(ramP->read32(memAddr, *PID));
                            if (postProcessing_afterRamRW()) {return; }
                            cmdPcsr->orL(&memVal, regs[regIndex]);
                            ramP->write32(memAddr, memVal.word(), *PID);
                        }
                        else // or m16 r32/16(->val16)
                        {
                            memVal.low() = ramP->read16(memAddr, *PID);
                            if (postProcessing_afterRamRW()) {return; }
                            reg32::HalfRef memVal16 = memVal.low();
                            
                            if (isHalf) {cmdPcsr->orL(&memVal16, halfRegs[regIndex]); }
                            else {cmdPcsr->orL(&memVal16, (uint16_t)regs[regIndex]); }
                            ramP->write16(memAddr, memVal16, *PID);
                        }
                    }
                    
                    break;
                }

                case 0x0a: // or m32/16 r32/16, Bytes 3, [or, ModR/M, s32]
                {
                    uint8_t mode = (commands[1] >> 7) & 0b1;
                    uint8_t srcRegIndex = (commands[1] >> 3) & 0b0111;
                    uint8_t baseRegIndex = commands[1] & 0b111;
                    uint8_t segIndex = commands[2] & 0b111;
                    bool isHalf = (commands[1] & 0b1000) != 0;

                    uint32_t memAddr = regs[baseRegIndex].word() + segs[segIndex].word() * ramSegPer_maxize;
                    reg32 memVal = 0;

                    if (mode == 0b0) // or mem32 r32/16
                    {
                        memVal.word(ramP->read32(memAddr, *PID));

                        if (postProcessing_afterRamRW()) {return; }
                        
                        // HalfReg
                        if (isHalf) {cmdPcsr->orL(&memVal, halfRegs[srcRegIndex]); }
                        // FullReg
                        else {cmdPcsr->orL(&memVal, regs[srcRegIndex]); }

                        ramP->write32(memAddr, memVal.word(), *PID);

                    }
                    else // or mem16 r32/16
                    {
                        memVal.low() = ramP->read16(memAddr, *PID);

                        if (postProcessing_afterRamRW()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->orL(&memVal, halfRegs[srcRegIndex]); }
                        // FullReg
                        else {cmdPcsr->orL(&memVal, regs[srcRegIndex]); }

                        ramP->write16(memAddr, memVal.low(), *PID);

                    }

                    break;
                }

                case 0x0b: // or r32/16 m32/16, Bytes 3, [or, ModR/M, s32]
                {
                    uint8_t mode = (commands[1] >> 7) & 0b1;
                    uint8_t dstRegIndex = (commands[1] >> 3) & 0b0111;
                    uint8_t baseRegIndex = commands[1] & 0b111;
                    uint8_t segIndex = commands[2] & 0b111;
                    bool isHalf = (commands[1] & 0b1000) != 0;

                    uint32_t memAddr = regs[baseRegIndex].word() + segs[segIndex].word() * ramSegPer_maxize;
                    uint32_t memVal = 0;

                    if (mode == 0b0) // or r32/16 mem32
                    {
                        memVal |= ramP->read32(memAddr, *PID);

                        if (postProcessing_afterRamRW()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->orL(&halfRegs[dstRegIndex], (uint16_t)memVal); }
                        // FullReg
                        else {cmdPcsr->orL(&regs[dstRegIndex], memVal); }
                    }
                    else // or r32/16 mem16
                    {
                        memVal |= ramP->read16(memAddr, *PID);

                        if (postProcessing_afterRamRW()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->orL(&halfRegs[dstRegIndex], (uint16_t)memVal); }
                        // FullReg
                        else {cmdPcsr->orL(&regs[dstRegIndex], memVal); }
                    }

                    break;
                }

                case 0x0c: // or AL imm16, Bytes 3, [or, immL, immH]
                {
                    uint16_t imm = ((uint16_t)commands[1] | ((uint16_t)commands[2] << 8));
                    cmdPcsr->orL(&halfRegs[0], imm);
                }

                case 0x0d: // or AX imm32, Bytes 5, [or, immLL, immLH, immHL, immHH]
                {
                    uint32_t imm = ((uint32_t)commands[1] | ((uint32_t)commands[2] << 8)
                        | ((uint32_t)commands[3] << 16) | ((uint32_t)commands[4] << 24));
                    cmdPcsr->orL(&regs[0], imm);
                }

                case 0x10: // adc reg reg, Bytes 2, [adc, src(4)|dst(4)]
                {
                    uint8_t dstBase = (commands[1] & 0x0f);
                    uint8_t srcBase = (commands[1] >> 4);
                    bool dstIsHalf = (dstBase & 0b1000) != 0;
                    bool srcIsHalf = (srcBase & 0b1000) != 0;

                    if (dstIsHalf) // dst is HalfReg
                    {
                        uint8_t dstIndex = std::min(dstBase & 0b0111, regs_maxize -1);

                        if (srcIsHalf) // src is HalfReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->adc(&halfRegs[dstIndex], halfRegs[srcIndex]);
                        } 
                        else // src is FullReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->adc(&halfRegs[dstIndex], (uint16_t)regs[srcIndex]);
                        }
                    }
                    else // dst is FullReg
                    {
                        uint8_t dstIndex = std::min(dstBase & 0b0111, regs_maxize -1);

                        if (srcIsHalf) // src is HalfReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->adc(&regs[dstIndex], halfRegs[srcIndex]);
                        } 
                        else // src is FullReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->adc(&regs[dstIndex], regs[srcIndex]);
                        }
                    }

                    break;
                }

                case 0x11: // adc r/m32/16 m/r32/16, Bytes 7, [adc, modRWR(1)|mod32/16(1)|(2)|reg(4), s32, addrLL, addrLH, addrHL, addrHH]
                {
                    uint8_t mode_RM_MR = (commands[1] >> 7) & 0b1;
                    uint8_t mode_32_16 = (commands[1] >> 6) & 0b1;
                    uint8_t regIndex = (commands[1] & 0b0111);
                    uint8_t segIndex = commands[2] & 0b111;
                    bool isHalf = (commands[1] & 0b1000) != 0;

                    uint32_t memAddr = ((uint32_t)commands[3] | ((uint32_t)commands[4] << 8) 
                        | ((uint32_t)commands[5] << 16) | ((uint32_t)commands[6] << 24)) + segs[segIndex].word() * ramSegPer_maxize;
                    reg32 memVal = 0;

                    if (mode_RM_MR == 0b0) // adc r32/16 m32/16
                    {
                        if (mode_32_16 == 0b0) // adc r32 m32
                        {
                            memVal.word(ramP->read32(memAddr, *PID));
                            if (postProcessing_afterRamRW()) {return; }
                            cmdPcsr->adc(&regs[regIndex], memVal);
                        }
                        else // adc r32/16 m16
                        {
                            memVal.low() = ramP->read16(memAddr, *PID);
                            if (postProcessing_afterRamRW()) {return; }
                            uint16_t memVal16 = memVal.low();

                            if (isHalf) {cmdPcsr->adc(&halfRegs[regIndex], memVal16); }
                            else {cmdPcsr->adc(&regs[regIndex], memVal16); }
                        }
                    }
                    else // adc m32/16 r32/16
                    {
                        if (mode_32_16 == 0b0) // adc m32 r32
                        {
                            memVal.word(ramP->read32(memAddr, *PID));
                            if (postProcessing_afterRamRW()) {return; }
                            cmdPcsr->adc(&memVal, regs[regIndex]);
                            ramP->write32(memAddr, memVal.word(), *PID);
                        }
                        else // adc m16 r32/16(->val16)
                        {
                            memVal.low() = ramP->read16(memAddr, *PID);
                            if (postProcessing_afterRamRW()) {return; }
                            reg32::HalfRef memVal16 = memVal.low();
                            
                            if (isHalf) {cmdPcsr->adc(&memVal16, halfRegs[regIndex]); }
                            else {cmdPcsr->adc(&memVal16, (uint16_t)regs[regIndex]); }
                            ramP->write16(memAddr, memVal16, *PID);
                        }
                    }
                    
                    break;
                }

                case 0x12: // adc m32/16 r32/16, Bytes 3, [adc, ModR/M, s32]
                {
                    uint8_t mode = (commands[1] >> 7) & 0b1;
                    uint8_t srcRegIndex = (commands[1] >> 3) & 0b0111;
                    uint8_t baseRegIndex = commands[1] & 0b111;
                    uint8_t segIndex = commands[2] & 0b111;
                    bool isHalf = (commands[1] & 0b1000) != 0;

                    uint32_t memAddr = regs[baseRegIndex].word() + segs[segIndex].word() * ramSegPer_maxize;
                    reg32 memVal = 0;

                    if (mode == 0b0) // adc mem32 r32/16
                    {
                        memVal.word(ramP->read32(memAddr, *PID));

                        if (postProcessing_afterRamRW()) {return; }
                        
                        // HalfReg
                        if (isHalf) {cmdPcsr->adc(&memVal, halfRegs[srcRegIndex]); }
                        // FullReg
                        else {cmdPcsr->adc(&memVal, regs[srcRegIndex]); }

                        ramP->write32(memAddr, memVal.word(), *PID);

                    }
                    else // adc mem16 r32/16
                    {
                        memVal.low() = ramP->read16(memAddr, *PID);

                        if (postProcessing_afterRamRW()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->adc(&memVal, halfRegs[srcRegIndex]); }
                        // FullReg
                        else {cmdPcsr->adc(&memVal, regs[srcRegIndex]); }

                        ramP->write16(memAddr, memVal.low(), *PID);

                    }

                    break;
                }

                case 0x13: // adc r32/16 m32/16, Bytes 3, [adc, ModR/M, s32]
                {
                    uint8_t mode = (commands[1] >> 7) & 0b1;
                    uint8_t dstRegIndex = (commands[1] >> 3) & 0b0111;
                    uint8_t baseRegIndex = commands[1] & 0b111;
                    uint8_t segIndex = commands[2] & 0b111;
                    bool isHalf = (commands[1] & 0b1000) != 0;

                    uint32_t memAddr = regs[baseRegIndex].word() + segs[segIndex].word() * ramSegPer_maxize;
                    uint32_t memVal = 0;

                    if (mode == 0b0) // adc r32/16 mem32
                    {
                        memVal |= ramP->read32(memAddr, *PID);

                        if (postProcessing_afterRamRW()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->adc(&halfRegs[dstRegIndex], (uint16_t)memVal); }
                        // FullReg
                        else {cmdPcsr->adc(&regs[dstRegIndex], memVal); }
                    }
                    else // adc r32/16 mem16
                    {
                        memVal |= ramP->read16(memAddr, *PID);

                        if (postProcessing_afterRamRW()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->adc(&halfRegs[dstRegIndex], (uint16_t)memVal); }
                        // FullReg
                        else {cmdPcsr->adc(&regs[dstRegIndex], memVal); }
                    }

                    break;
                }

                case 0x14: // adc AL imm16, Bytes 3, [adc, immL, immH]
                {
                    uint16_t imm = ((uint16_t)commands[1] | ((uint16_t)commands[2] << 8));
                    cmdPcsr->adc(&halfRegs[0], imm);

                    break;
                }

                case 0x15: // adc AX imm32, Bytes 5, [adc, immLL, immLH, immHL, immHH]
                {
                    uint32_t imm = ((uint32_t)commands[1] | ((uint32_t)commands[2] << 8) 
                        | ((uint32_t)commands[3] << 16) | ((uint32_t)commands[4] << 24));
                    cmdPcsr->adc(&regs[0], imm);

                    break;
                }

                case 0x18: // sbb reg reg, Bytes 2, [sbb, src(4)|dst(4)]
                {
                    uint8_t dstBase = (commands[1] & 0x0f);
                    uint8_t srcBase = (commands[1] >> 4);
                    bool dstIsHalf = (dstBase & 0b1000) != 0;
                    bool srcIsHalf = (srcBase & 0b1000) != 0;

                    if (dstIsHalf) // dst is HalfReg
                    {
                        uint8_t dstIndex = std::min(dstBase & 0b0111, regs_maxize -1);

                        if (srcIsHalf) // src is HalfReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->sbb(&halfRegs[dstIndex], halfRegs[srcIndex]);
                        } 
                        else // src is FullReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->sbb(&halfRegs[dstIndex], (uint16_t)regs[srcIndex]);
                        }
                    }
                    else // dst is FullReg
                    {
                        uint8_t dstIndex = std::min(dstBase & 0b0111, regs_maxize -1);

                        if (srcIsHalf) // src is HalfReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->sbb(&regs[dstIndex], halfRegs[srcIndex]);
                        } 
                        else // src is FullReg
                        {
                            uint8_t srcIndex = std::min(srcBase & 0b0111, regs_maxize -1);
                            cmdPcsr->sbb(&regs[dstIndex], regs[srcIndex]);
                        }
                    }

                    break;
                }

                case 0x19: // sbb r/m32/16 m/r32/16, Bytes 7, [adc, modRWR(1)|mod32/16(1)|(2)|reg(4), s32, addrLL, addrLH, addrHL, addrHH]
                {
                    uint8_t mode_RM_MR = (commands[1] >> 7) & 0b1;
                    uint8_t mode_32_16 = (commands[1] >> 6) & 0b1;
                    uint8_t regIndex = (commands[1] & 0b0111);
                    uint8_t segIndex = commands[2] & 0b111;
                    bool isHalf = (commands[1] & 0b1000) != 0;

                    uint32_t memAddr = ((uint32_t)commands[3] | ((uint32_t)commands[4] << 8) 
                        | ((uint32_t)commands[5] << 16) | ((uint32_t)commands[6] << 24)) + segs[segIndex].word() * ramSegPer_maxize;
                    reg32 memVal = 0;

                    if (mode_RM_MR == 0b0) // sbb r32/16 m32/16
                    {
                        if (mode_32_16 == 0b0) // sbb r32 m32
                        {
                            memVal.word(ramP->read32(memAddr, *PID));
                            if (postProcessing_afterRamRW()) {return; }
                            cmdPcsr->sbb(&regs[regIndex], memVal);
                        }
                        else // sbb r32/16 m16
                        {
                            memVal.low() = ramP->read16(memAddr, *PID);
                            if (postProcessing_afterRamRW()) {return; }
                            uint16_t memVal16 = memVal.low();

                            if (isHalf) {cmdPcsr->sbb(&halfRegs[regIndex], memVal16); }
                            else {cmdPcsr->sbb(&regs[regIndex], memVal16); }
                        }
                    }
                    else // sbb m32/16 r32/16
                    {
                        if (mode_32_16 == 0b0) // sbb m32 r32
                        {
                            memVal.word(ramP->read32(memAddr, *PID));
                            if (postProcessing_afterRamRW()) {return; }
                            cmdPcsr->sbb(&memVal, regs[regIndex]);
                            ramP->write32(memAddr, memVal.word(), *PID);
                        }
                        else // sbb m16 r32/16(->val16)
                        {
                            memVal.low() = ramP->read16(memAddr, *PID);
                            if (postProcessing_afterRamRW()) {return; }
                            reg32::HalfRef memVal16 = memVal.low();
                            
                            if (isHalf) {cmdPcsr->sbb(&memVal16, halfRegs[regIndex]); }
                            else {cmdPcsr->sbb(&memVal16, (uint16_t)regs[regIndex]); }
                            ramP->write16(memAddr, memVal16, *PID);
                        }
                    }
                    
                    break;
                }

                case 0x1a: // sbb m32/16, r32/16, Bytes 3, [sbb, ModR/M, s32]
                {
                    uint8_t mode = (commands[1] >> 7) & 0b1;
                    uint8_t srcRegIndex = (commands[1] >> 3) & 0b0111;
                    uint8_t baseRegIndex = commands[1] & 0b111;
                    uint8_t segIndex = commands[2] & 0b111;
                    bool isHalf = (commands[1] & 0b1000) != 0;

                    uint32_t memAddr = regs[baseRegIndex].word() + segs[segIndex].word() * ramSegPer_maxize;
                    reg32 memVal = 0;

                    if (mode == 0b0) // sbb mem32 r32/16
                    {
                        memVal.word(ramP->read32(memAddr, *PID));

                        if (postProcessing_afterRamRW()) {return; }
                        
                        // HalfReg
                        if (isHalf) {cmdPcsr->sbb(&memVal, halfRegs[srcRegIndex]); }
                        // FullReg
                        else {cmdPcsr->sbb(&memVal, regs[srcRegIndex]); }

                        ramP->write32(memAddr, memVal.word(), *PID);

                    }
                    else // sbb mem16 r32/16
                    {
                        memVal.low() = ramP->read16(memAddr, *PID);

                        if (postProcessing_afterRamRW()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->sbb(&memVal, halfRegs[srcRegIndex]); }
                        // FullReg
                        else {cmdPcsr->sbb(&memVal, regs[srcRegIndex]); }

                        ramP->write16(memAddr, memVal.low(), *PID);

                    }

                    break;
                }

                case 0x1b: // sbb r32/16, m32/16, Bytes 3, [sbb, ModR/M, s32]
                {
                    uint8_t mode = (commands[1] >> 7) & 0b1;
                    uint8_t dstRegIndex = (commands[1] >> 3) & 0b0111;
                    uint8_t baseRegIndex = commands[1] & 0b111;
                    uint8_t segIndex = commands[2] & 0b111;
                    bool isHalf = (commands[1] & 0b1000) != 0;

                    uint32_t memAddr = regs[baseRegIndex].word() + segs[segIndex].word() * ramSegPer_maxize;
                    uint32_t memVal = 0;

                    if (mode == 0b0) // sbb r32/16 mem32
                    {
                        memVal |= ramP->read32(memAddr, *PID);

                        if (postProcessing_afterRamRW()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->sbb(&halfRegs[dstRegIndex], (uint16_t)memVal); }
                        // FullReg
                        else {cmdPcsr->sbb(&regs[dstRegIndex], memVal); }
                    }
                    else // sbb r32/16 mem16
                    {
                        memVal |= ramP->read16(memAddr, *PID);

                        if (postProcessing_afterRamRW()) {return; }

                        // HalfReg
                        if (isHalf) {cmdPcsr->sbb(&halfRegs[dstRegIndex], (uint16_t)memVal); }
                        // FullReg
                        else {cmdPcsr->sbb(&regs[dstRegIndex], memVal); }
                    }

                    break;
                }

                case 0x1c: // sbb AL imm16, Bytes 3, [sbb, immL, immH]
                {
                    uint16_t imm = ((uint16_t)commands[1] | ((uint16_t)commands[2] << 8));
                    cmdPcsr->sbb(&halfRegs[0], imm);

                    break;
                }

                case 0x1d: // sbb AX imm32, Bytes 5, [sbb, immLL, immLH, immHL, immHH]
                {
                    uint32_t imm = ((uint32_t)commands[1] | ((uint32_t)commands[2] << 8) 
                        | ((uint32_t)commands[3] << 16) | ((uint32_t)commands[4] << 24));
                    cmdPcsr->sbb(&regs[0], imm);

                    break;
                }

            }
        }
    
    };

    class commandAddressing
    {
    private:
        commandDecoder* cmDcdr;
        uint16_t* PID;
        reg32* IP;
        reg32* segs;
        reg32* regs;
        reg32* CS = &segs[0];
        Ram* ramP;
        uint8_t commands[commandsArr_maxize] = {0};
        uint8_t commandsBuffer[commandsArr_maxize] = {0};

    public:
        commandAddressing(reg32* IPt, reg32* segsT, reg32* regsT, Ram* ramG, commandDecoder* cmDcdrG, uint16_t* PIDt)
         : IP(IPt), segs(segsT), regs(regsT), ramP(ramG), cmDcdr(cmDcdrG), PID(PIDt) {}

        bool postProcessing_afterRamRW()
        {
            bool isDead = ramP->get_deadState();
            bool isDead2;
            bool isUA = ramP->get_UAstate(*PID);
            if (isUA)
            {
                // Unauthorized Access
                regs[8].stbit(30);
            }
            if (isDead)
            {
                isDead2 = ramP->get_deadState();
                if (isDead2)
                {
                    // Memory Overflow
                    regs[8].stbit(31);
                } else 
                {return true; }
            }
            return isDead || isUA;
        }

        void ___readRam(uint32_t addr, uint8_t* buffer)
        {
            *buffer = ramP->read(addr, *PID);
        }

        void mov_buffer() 
        {
            for (uint8_t index =0; index < commandsArr_maxize; ++index)
            {
                commandsBuffer[index] = commands[index]; 
                commands[index] = 0;
            }
        }

        void addressing()
        {
            std::unique_lock<std::mutex> ram_lock(ramP->raMutexC);
            uint32_t OPcode_addr = CS->word() * ramSegPer_maxize + IP->word();
            uint8_t opc = ramP->read(OPcode_addr, *PID);
            if (postProcessing_afterRamRW()) {return; }

            uint8_t commandLength = opCode_decodeTable[opc];
            for (uint8_t index =0; index < commandLength; ++index)
            {
                uint8_t temp = ramP->read(OPcode_addr + index, *PID);
                if (postProcessing_afterRamRW()) {return; }

                commands[index] = temp; 
            }
            mov_buffer();
            IP->word(IP->word() + commandLength);
            ram_lock.unlock();
            cmDcdr->decode(commandsBuffer);
        }
    
    };

    uint16_t PID;
    uint32_t IDt;
    uint32_t EID;
    uint32_t UD;
    uint32_t timE = 0;
    uint64_t runningTimes = 0;
    Ram* ramP;
    bool runningState = false;

    // state false means current IOdevice port is unvalid, next I/O command will select a IOdevice port;
    // state true means current IOdevice port is valid, next I/O command will send data to the IOdevice;
    bool IOstate = false;
    uint16_t port_IOdeviceC = 0;

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
    >>  UAF     30      1       Unauthorized Access Flag;
    >>  MOF     31      1       Memory Overflow Flag;
    */

    reg32 CS, DS, SS, ES, FS, GS, SS1, SS2;
    reg32* segs[segs_maxize] = {&CS, &DS, &SS, &ES, &FS, &GS, &SS1, &SS2};

    reg32 AX, CX, DX, BX;
    reg32 SP, BP, SI, DI;
    reg32* regs[regs_maxize] = {&AX, &CX, &DX, &BX, &SP, &BP, &SI, &DI, &FLAG};

    reg32 IP;
    
    commandProcesser* cmdPcsr = new commandProcesser(&FLAG, &AX, &DX, &timE, ramP);
    commandDecoder* cmdDcdr = new commandDecoder(cmdPcsr, *regs, *segs, &timE, ramP, &PID);
    commandAddressing* cmdAdr = new commandAddressing(&IP, *segs, *regs, ramP, cmdDcdr, &PID);
    
    std::map<uint16_t, devIO*> devsRegistry;

public:
    std::mutex centerMutexC;


    Center32x(uint32_t ID, Ram* ramG) : ramP(ramG)
    {
        IDt = ID;
        UD = Hasher::hash_uint32(IDt);
        uint8_t temp = UD & 0b0111;
        EID = Hasher::get_randNumHR2(UD, temp);
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
            dstArray[index].word(segs[index]->word());
        }
    }
    void getRegs(reg32* dstArray) const
    {
        for (int index =0; index < regs_maxize; ++index)
        {
            dstArray[index].word(regs[index]->word());
        }
    }

    void setPID(uint16_t PIDt) {PID = PIDt; }
    uint16_t getPID() const {return PID; }
    void setIP(uint32_t IPt) {IP.word(IPt); }
    uint32_t getIP() const {return IP.word(); }
    bool getRunningState() const {return runningState; }
    uint64_t getRunningTimes() const {return runningTimes; }

    int run_cycle()
    {
        std::lock_guard<std::mutex> centerLock(centerMutexC);
        if (!runningState) {return -1; }
        if (FLAG.ckbit(1) || FLAG.ckbit(31) || FLAG.ckbit(30)) 
        {stop(); return -1; }

        cmdAdr->addressing();
        runningTimes++;
        return 0;
    }
    void loop()
    {}

};

class rootOP
{};

class Center2OPtransferDev : public devIO
{
private:
    rootOP* rootGOP;

public:
    Center2OPtransferDev(uint32_t ID, Hasher* hashG, rootOP* rootG)
     : devIO(hashG->get_randNumHR2(ID), UKED_Center2OPtransferDev), rootGOP(rootG) {}

};

class Center2centerTransferDev : public devIO
{ 
private:
    Center32x* CenterOUTP;

public:
    Center2centerTransferDev(uint32_t ID, Hasher* hashG, Center32x* CenterOUT)
     : devIO(hashG->get_randNumHR2(ID), UKED_Center2centerTransferDev), CenterOUTP(CenterOUT) {}

};


class debugCMD
{
private:
    //static std::mutex regStateMutex;
    Center32x* centers[centers_maxize];
    reg32 regs_0f_centers[centers_maxize][regs_maxize];
    reg32 segs_0f_centers[centers_maxize][segs_maxize];
    std::atomic<bool> runState_0f_centers[centers_maxize];
    const int linesPerCenter = 5;
    const int linesT = centers_maxize * linesPerCenter;
    mutable uint16_t AL, AH, CL, CH, DL, DH, BL, BH, PID;
    mutable uint32_t SP, BP, SI, DI, CS, DS, SS, ES, FS, GS, SS1, SS2, IP, flags;
    mutable uint64_t runningTimes;

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
        //std::lock_guard<std::mutex> lockC(regStateMutex);
        for (int index =0; index < centers_maxize; ++index)
        {
            if (centers[index] == nullptr) {continue; }
            std::unique_lock<std::mutex> lockC(centers[index]->centerMutexC);
            centers[index]->getRegs(regs_0f_centers[index]);
            centers[index]->getSegs(segs_0f_centers[index]);
            runState_0f_centers[index] = centers[index]->getRunningState();
        }
    }

    void clearLine() const {std::cout << "\033[K"; }
    void movCursorUP() const {if (linesT >0) {std::cout << "\033[" << linesT << "A"; } }

    void get_datas(int centerIndex)
    {
        {// regs1
            AL = static_cast<uint16_t>(regs_0f_centers[centerIndex][0].low());
            AH = static_cast<uint16_t>(regs_0f_centers[centerIndex][0].high());
            CL = static_cast<uint16_t>(regs_0f_centers[centerIndex][1].low());
            CH = static_cast<uint16_t>(regs_0f_centers[centerIndex][1].high());
            DL = static_cast<uint16_t>(regs_0f_centers[centerIndex][2].low());
            DH = static_cast<uint16_t>(regs_0f_centers[centerIndex][2].high());
            BL = static_cast<uint16_t>(regs_0f_centers[centerIndex][3].low());
            BH = static_cast<uint16_t>(regs_0f_centers[centerIndex][3].high());
        }
        {// regs2
            SP = static_cast<uint32_t>(regs_0f_centers[centerIndex][4].word());
            BP = static_cast<uint32_t>(regs_0f_centers[centerIndex][5].word());
            SI = static_cast<uint32_t>(regs_0f_centers[centerIndex][6].word());
            DI = static_cast<uint32_t>(regs_0f_centers[centerIndex][7].word());
        }
        {// segs
            CS = static_cast<uint32_t>(segs_0f_centers[centerIndex][0].word());
            DS = static_cast<uint32_t>(segs_0f_centers[centerIndex][1].word());
            SS = static_cast<uint32_t>(segs_0f_centers[centerIndex][2].word());
            ES = static_cast<uint32_t>(segs_0f_centers[centerIndex][3].word());
            FS = static_cast<uint32_t>(segs_0f_centers[centerIndex][4].word());
            GS = static_cast<uint32_t>(segs_0f_centers[centerIndex][5].word());
            SS1 = static_cast<uint32_t>(segs_0f_centers[centerIndex][6].word());
            SS2 = static_cast<uint32_t>(segs_0f_centers[centerIndex][7].word());
        }
        flags = static_cast<uint32_t>(regs_0f_centers[centerIndex][8].word());
        std::unique_lock<std::mutex> lockC(centers[centerIndex]->centerMutexC);
        IP = centers[centerIndex]->getIP();
        PID = centers[centerIndex]->getPID();
        runningTimes = centers[centerIndex]->getRunningTimes();
    }

    int show_datas()
    {
        /* flags2FS_tabs = 3
            Center 0 ==----     PID : 0000          runTimes : 0000000000000000
                AX : FFFF FFFF  CX : FFFF FFFF      SP : FFFFFFFF   BP : FFFFFFFF       CS : FFFFFFFF   DS : FFFFFFFF
                DX : FFFF FFFF  BX : FFFF FFFF      SI : FFFFFFFF   DI : FFFFFFFF       SS : FFFFFFFF   ES : FFFFFFFF
                IP : FFFFFFFF   FLAGS : 11111111111111111111111111111111                FS : FFFFFFFF   GS : FFFFFFFF
            ----==end 0
        */
        for (int index =0; index < centers_maxize; ++index)
        {
            get_datas(index);
            clearLine();
            //std::cout << "Center " << std::dec << (int32_t)index << " ==----" << "\t\t\t\t--linEnd" << std::endl;
            printf("Center %02x ==----\tPID : %04x\t\trunTimes : %016x\t\t\n", index, PID, runningTimes);

            printf("\tAX : %04x %04x", AH, AL);
            printf("\tCX : %04x %04x", CH, CL);
            printf("\t\tSP : %08x", SP);
            printf("\tBP : %08x", BP);
            printf("\t\tCS : %08x", CS);
            printf("\tDS : %08x\n", DS);
            printf("\tDX : %04x %04x", DH, DL);
            printf("\tBX : %04x %04x", BH, BL);
            printf("\t\tSI : %08x", SI);
            printf("\tDI : %08x", DI);
            printf("\t\tSS : %08x", SS);
            printf("\tES : %08x\n", ES);
            printf("\tIP : %08x", IP);
            std::cout << "\tFLAGS : " << std::bitset<32>(flags) << "\t\t";
            printf("\tFS : %08x", FS);
            printf("\tGS : %08x\n", GS);

            //std::cout << "----==end " << index << "\t\t\t\t\t\t\t" << std::endl;
            printf("----==end %02x\t\t\t\t\t\t\t\n", index);
        }

        std::cout.flush();
        fflush(stdout);
        movCursorUP();
        return 0;
    }
    void loop()
    {
        bool selfRunning = true;
        clearLine();
        while (selfRunning)
        {
            data_update();
            int centerDeadCount = 0;
            for (int index =0; index < centers_maxize; ++index) {if (!runState_0f_centers[index]) {centerDeadCount++; continue; } }
            if (centerDeadCount >= centers_maxize) {selfRunning = false; break; }

            //clearLine();
            show_datas();
            std::this_thread::sleep_for(std::chrono::milliseconds(128));
        }
        for (int index =0; index <= linesT; ++index) {std::cout << std::endl; }
        return;
    }

};
//std::mutex debugCMD::regStateMutex;





static Hasher* ashor;

bool initG()
{
    // init the OPcode-Decode-Table
    if (opCode_decodeTable != nullptr)
    {
        memset(opCode_decodeTable, 0, opc_maxize +1);
        {// ADD
            opCode_decodeTable[0x00] = 2;
            opCode_decodeTable[0x01] = 7;
            opCode_decodeTable[0x02] = 3; 
            opCode_decodeTable[0x03] = 3;
            opCode_decodeTable[0x04] = 3;
            opCode_decodeTable[0x05] = 5;
        }
        {// OR
            opCode_decodeTable[0x08] = 2;
            opCode_decodeTable[0x09] = 7;
            opCode_decodeTable[0x0a] = 3;
            opCode_decodeTable[0x0b] = 3;
            opCode_decodeTable[0x0c] = 3;
            opCode_decodeTable[0x0d] = 5;
        }
        {// ADC
            opCode_decodeTable[0x10] = 2;
            opCode_decodeTable[0x11] = 7;
            opCode_decodeTable[0x12] = 3;
            opCode_decodeTable[0x13] = 3;
            opCode_decodeTable[0x14] = 3;
            opCode_decodeTable[0x15] = 5;
        }
        {// SBB
            opCode_decodeTable[0x18] = 2;
            opCode_decodeTable[0x19] = 7;
            opCode_decodeTable[0x1a] = 3;
            opCode_decodeTable[0x1b] = 3;
            opCode_decodeTable[0x1c] = 3;
            opCode_decodeTable[0x1d] = 5;
        }
    }
    
    // init the Hasher
    ashor = new Hasher();

    return true;
}

void test() 
{
    uint16_t rootPID = 0x00;
    Ram* ram1 = new Ram();
    Ram* ram2 = new Ram(0x3fffffff);// ~ 1 GiB
    Center32x center1{0x00, ram1}, center2{0x01, ram2};
    Center32x* centers[centers_maxize] = {&center1, &center2};
    debugCMD debuger{centers};
    center1.setPID(rootPID);
    center2.setPID(rootPID);
    ram1->write32(0x00, 0x05abcd04, rootPID);
    ram1->write32(0x04, 0xabcd0000, rootPID);
    {//*
        uint32_t addr = 0x20;
        ram1->write16(addr, 0x0001, rootPID);
        ram1->write(0x08, 0x01, rootPID);
        ram1->write(0x09, 0b11001000, rootPID);
        ram1->write(0x0a, 0x00, rootPID);
        ram1->write32(0x0b, addr, rootPID);
        // ADD CS:[addr] AL     ; test4 0x01 7B, ram->[addr] == 0xabce=171|206 in Idea t
    //*/
    }
    ram2->write(0x00, 0x1d, rootPID);
    ram2->write32(0x01, 0x00000001, rootPID);
    if (ram1->get_deadState()) {printf("Memory Overflow"); }
    center1.start();
    center2.start();
    std::thread debugPrintThread(&debugCMD::loop, &debuger);
    for (int times = 0; times < (uint32_t)0xffffffff; ++times)
    {
        std::thread centerThread2(&Center32x::run_cycle, &center2);
        center1.run_cycle();
        center1.run_cycle();
        center1.run_cycle();
        centerThread2.join();
        center1.setIP(0x00);
        center2.setIP(0x00);
    }
    center1.stop();
    center1.stop();
    debugPrintThread.join();
    return;
}


int main()
{
    initG();


    //* test
    test();
    //*/
    system("pause");
    return 0;
}


