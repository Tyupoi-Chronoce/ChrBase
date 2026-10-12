#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <thread>
#include <bitset>
#include <mutex>
#include <atomic>
#include "../_libs/Ram.h"
#include "../_libs/Regs.h"


//#define OMPopen

#if defined(OMPopen)
#  if !defined(_OPENMP)
#    error "定义了 OMPopen 却没有加 -fopenmp：此时 pragma 会被静默忽略，实际仍是串行。请补上 -fopenmp。"
#  endif
#  define OMP_FOR _Pragma("omp parallel for")
#else
#  define OMP_FOR
#endif




namespace CK
{

    uint32_t _float_2uint32(float data)
    {
        uint32_t res;
        memcpy(&res, &data, sizeof(uint32_t));
        return res;
    }
    float _uint32_2float(uint32_t data)
    {
        float res;
        memcpy(&res, &data, sizeof(float));
        return res;
    }
    
    static const uint8_t corePer_Gcenter_maxize = 0xff;
    static const uint8_t corePer_Gcenter_minSize = 0xc0;
    static const uint32_t gRamSegPer_size = 131072; // 128 KiB per Segment in GRAM
    static const uint8_t gCenter_commandsMaxize = 10;

    static const uint8_t gCenter_opc_maxize = 0xff;


    class Center32g
    {
    private:
        class Core
        { // ALU Core of GraphicDev
        private:
            reg32 copy1, copy2;
            reg16 copy3, copy4;
            regF32 copy5, copy6;

        public:
            Core() {}

            // if the src opr is reg, it degenerates into the data with the bits same or more;
            void add(reg32* dst, uint32_t src) {dst->word(dst->word() + src); }
            void add(reg16* dst, uint16_t src) {dst->word(dst->word() + src); }
            void sub(reg32* dst, uint32_t src) {dst->word(dst->word() - src); }
            void sub(reg16* dst, uint16_t src) {dst->word(dst->word() - src); }
            void andL(reg32* dst, uint32_t src) {dst->word(dst->word() & src); }
            void andL(reg16* dst, uint16_t src) {dst->word(dst->word() & src); }
            void orL(reg32* dst, uint32_t src) {dst->word(dst->word() | src); }
            void orL(reg16* dst, uint16_t src) {dst->word(dst->word() | src); }
            void xorL(reg32* dst, uint32_t src) {dst->word(dst->word() ^ src); }
            void xorL(reg16* dst, uint16_t src) {dst->word(dst->word() ^ src); }
            void mul(reg32* dst, uint16_t src) {dst->word(dst->low() * src); }
            void mul(reg16* dst, uint8_t src) {dst->word(dst->low() * src); }
            void mul(regF32* dst, float src) {dst->word(dst->word() * src); }
            void div(reg32* dst, uint16_t src) 
            {
                if (src == 0) {return; }
                copy1.word(dst->word());
                dst->low() = (uint16_t)(copy1.word() / src); 
                dst->high() = (uint16_t)(copy1.word() % src);
                copy1.word(0);
            }
            void div(reg16* dst, uint8_t src) 
            {
                if (src == 0) {return; }
                copy3.word(dst->word());
                dst->low() = (uint8_t)(copy3.word() / src); 
                dst->high() = (uint8_t)(copy3.word() % src);
                copy3.word(0);
            }
            void div(regF32* dst, float src) 
            {
                if (src == 0) {return; }
                dst->word(dst->word() / src); 
            }
            void inc(reg32* dst) {dst->word(dst->word() + 1); }
            void inc(reg16* dst) {dst->word(dst->word() + 1); }
            void dec(reg32* dst) {dst->word(dst->word() - 1); }
            void dec(reg16* dst) {dst->word(dst->word() - 1); }
            void sin(regF32* dst, uint16_t src) {dst->word(sinf((float)src)); }
            void sin(regF32* dst, float src) {dst->word(sinf(src)); }
            void cos(regF32* dst, uint16_t src) {dst->word(cosf((float)src)); }
            void cos(regF32* dst, float src) {dst->word(cosf(src)); }
            void tan(regF32* dst, uint16_t src) {dst->word(tanf((float)src)); }
            void tan(regF32* dst, float src) {dst->word(tanf(src)); }
            void asin(regF32* dst, uint16_t src) {dst->word(asinf((float)src)); }
            void asin(regF32* dst, float src) {dst->word(asinf(src)); }
            void acos(regF32* dst, uint16_t src) {dst->word(acosf((float)src)); }
            void acos(regF32* dst, float src) {dst->word(acosf(src)); }
            void atan(regF32* dst, uint16_t src) {dst->word(atanf((float)src)); }
            void atan(regF32* dst, float src) {dst->word(atanf(src)); }
            void mov(reg32* dst, uint32_t src) {dst->word(src); }
            void mov(reg16* dst, uint16_t src) {dst->word(src); }
            void mov(regF32* dst, float src) {dst->word(src); }
            void mov(regF32* dst, uint32_t src) {dst->word(_uint32_2float(src)); }
            void mov(reg32* dst, float src) {dst->word(_float_2uint32(src)); }

        };


        uint16_t Core_count;
        uint16_t DramPID = Ram::rootPID;
        mutable bool runningState = false;

        // use registers as cache
        reg16* reg16sP;
        reg32* reg32sP;
        regF32* regF32sP;
        Core* coresP;
        Ram* DramP;
        uint8_t* GramP;
        uint32_t Gram_size;
        uint8_t commands[gCenter_commandsMaxize] = {};
        uint8_t commandsBuffer[gCenter_commandsMaxize] = {};
        
        static bool decodeTable_inited;
        static std::mutex decodeTable_mutex;
        static uint8_t* gCenter_opCode_decodeTable;

        reg32 DGP, DDS;         // Dram General Pointer,        Dram Data Segment
        reg32 GIP, GCS, GDS;    // Gram Instruction Pointer,    Gram Code Segment,  Gram Data Segment


        void _mov_buffer()
        {
            for (int index =0; index < gCenter_commandsMaxize; ++index)
            {
                commandsBuffer[index] = commands[index];
                commands[index] = 0;
            }
        }

    public:
        Center32g(uint8_t CoreCount, Ram* Dram, uint8_t* Gram, uint32_t GramSize)
            : Core_count(std::min(corePer_Gcenter_maxize, std::max(CoreCount, corePer_Gcenter_minSize)) +1u), 
            reg16sP(new reg16[Core_count]), reg32sP(new reg32[Core_count]), regF32sP(new regF32[Core_count]),
            coresP(new Core[Core_count]), DramP(Dram), GramP(Gram), Gram_size(GramSize)
        { /*coresP[0].add(&reg16sP[0], reg16sP[1]);
                example ^   without error message;;
            */
            std::unique_lock<std::mutex> staticLock(decodeTable_mutex);
            if (!decodeTable_inited and gCenter_opCode_decodeTable != nullptr)
            {
                memset(gCenter_opCode_decodeTable, 0, gCenter_opc_maxize +1);
                {// ADD
                    gCenter_opCode_decodeTable[0x00] = 3;
                    gCenter_opCode_decodeTable[0x01] = 3;
                    gCenter_opCode_decodeTable[0x02] = 4;
                    gCenter_opCode_decodeTable[0x03] = 6;
                }
                {// SUB
                    gCenter_opCode_decodeTable[0x05] = 3;
                    gCenter_opCode_decodeTable[0x06] = 3;
                    gCenter_opCode_decodeTable[0x07] = 4;
                    gCenter_opCode_decodeTable[0x08] = 6;
                }
                {// AND
                    gCenter_opCode_decodeTable[0x0a] = 3;
                }
                decodeTable_inited = true;
            }
            memset(commands, 0, gCenter_commandsMaxize);
            memset(commandsBuffer, 0, gCenter_commandsMaxize);
        }

        ~Center32g()
        {
            delete[] reg16sP;
            delete[] reg32sP;
            delete[] regF32sP;
            delete[] coresP;
        }

        void start() const {runningState = true; }
        void stop() const {runningState = false; }

        void decode()
        {
            uint8_t opCode = commandsBuffer[0];

            switch (opCode)
            {
                default:
                {return; }

                case 0x00: // ADD _dst32/16 _src32/16 dstEndIndex srcIndex, Bytes 3, [add, dstEndIndex, dstMod32/16(1)|srcMod32/16(1)|srcOffset(6)(0xC0|~)]
                {
                    uint8_t dstEndIndex = commandsBuffer[1];
                    uint8_t srcOffset = (commandsBuffer[2] & 0x3f) | 0xC0;
                    if (dstEndIndex >= Core_count || srcOffset >= Core_count) {return; }
                    bool dstIsMod32 = (commandsBuffer[2] & 0x80) != 0;
                    bool srcIsMod32 = (commandsBuffer[2] & 0x40) != 0;
                    if (srcIsMod32)
                    {
                        reg32 srcData(reg32sP[srcOffset].word());

                        if (dstIsMod32) // ADD _dst32 _src32 dstEndIndex srcIndex
                        {
                            OMP_FOR
                            for (int index =0; index <= dstEndIndex; ++index)
                            {coresP[index].add(&reg32sP[index], srcData); }
                        }
                        else // ADD _dst16 _src32 dstEndIndex srcIndex (srcData -> low of~)
                        {
                            OMP_FOR
                            for (int index =0; index <= dstEndIndex; ++index)
                            {coresP[index].add(&reg16sP[index], srcData.low()); }
                        }
                    }
                    else
                    {
                        reg16 srcData(reg16sP[srcOffset].word());

                        if (dstIsMod32) // ADD _dst32 _src16 dstEndIndex srcIndex
                        {
                            OMP_FOR
                            for (int index =0; index <= dstEndIndex; ++index)
                            {coresP[index].add(&reg32sP[index], srcData); }
                        }
                        else // ADD _dst16 _src16 dstEndIndex srcIndex
                        {
                            OMP_FOR
                            for (int index =0; index <= dstEndIndex; ++index)
                            {coresP[index].add(&reg16sP[index], srcData); }
                        }
                    }
                    return;
                }
                case 0x01: // ADD _X _dst32/16 _src32/16 begIndex endIndex, Bytes 3, [add, dstMod32/16(1)|srcMod32/16(1)|begIndex(6), endIndex]
                {
                    uint8_t begIndex = commandsBuffer[1] & 0x3F;
                    uint8_t endIndex = commandsBuffer[2];
                    if (begIndex >= Core_count || endIndex >= Core_count || begIndex > endIndex) {return; }
                    bool dstIsMod32 = (commandsBuffer[1] & 0x80) != 0;
                    bool srcIsMod32 = (commandsBuffer[1] & 0x40) != 0;
                    if (dstIsMod32)
                    {
                        if (srcIsMod32) // ADD _X _dst32 _src32 begIndex endIndex
                        {
                            OMP_FOR
                            for (int index = begIndex; index <= endIndex; ++index)
                            {coresP[index].add(&reg32sP[index], reg32sP[index]); }
                        }
                        else // ADD _X _dst32 _src16 begIndex endIndex
                        {
                            OMP_FOR
                            for (int index = begIndex; index <= endIndex; ++index)
                            {coresP[index].add(&reg32sP[index], reg16sP[index]); }
                        }
                    }
                    else 
                    {
                        if (srcIsMod32) // ADD _X _dst16 _src32 begIndex endIndex (srcData -> low of~)
                        {
                            OMP_FOR
                            for (int index = begIndex; index <= endIndex; ++index)
                            {coresP[index].add(&reg16sP[index], reg32sP[index].low()); }
                        }
                        else // ADD _X _dst16 _src16 begIndex endIndex
                        {
                            OMP_FOR
                            for (int index = begIndex; index <= endIndex; ++index)
                            {coresP[index].add(&reg16sP[index], reg16sP[index]); }
                        }
                    }
                    return;
                }
                case 0x02: // ADD _dst16 _srcImm dstEndIndex imm16, Bytes 4, [add, dstEndIndex, immL, immH]
                {
                    uint8_t dstEndIndex = commandsBuffer[1];
                    if (dstEndIndex >= Core_count) {return; }
                    uint16_t imm16 = (uint16_t)commandsBuffer[2] | ((uint16_t)commandsBuffer[3] << 8);
                    OMP_FOR
                    for (int index =0; index <= dstEndIndex; ++index)
                    {coresP[index].add(&reg16sP[index], imm16); }
                    return;
                }
                case 0x03: // ADD _dst32 _srcImm dstEndIndex imm32, Bytes 6, [add, dstEndIndex, immLL, immLH, immHL, immHH]
                {
                    uint8_t dstEndIndex = commandsBuffer[1];
                    if (dstEndIndex >= Core_count) {return; }
                    uint32_t imm32 = (uint32_t)commandsBuffer[2] | ((uint32_t)commandsBuffer[3] << 8) |
                        ((uint32_t)commandsBuffer[4] << 16) | ((uint32_t)commandsBuffer[5] << 24);
                    OMP_FOR
                    for (int index =0; index <= dstEndIndex; ++index)
                    {coresP[index].add(&reg32sP[index], imm32); }
                    return;
                }
            
                case 0x05: // SUB _dst32/16 _src32/16 dstEndIndex srcIndex, Bytes 3, [sub, dstEndIndex, dstMod32/16(1)|srcMod32/16(1)|srcOffset(6)(0xC0|~)]
                {
                    uint8_t dstEndIndex = commandsBuffer[1];
                    uint8_t srcOffset = (commandsBuffer[2] & 0x3f) | 0xC0;
                    if (dstEndIndex >= Core_count || srcOffset >= Core_count) {return; }
                    bool dstIsMod32 = (commandsBuffer[2] & 0x80) != 0;
                    bool srcIsMod32 = (commandsBuffer[2] & 0x40) != 0;
                    if (srcIsMod32)
                    {
                        reg32 srcData(reg32sP[srcOffset].word());

                        if (dstIsMod32) // SUB _dst32 _src32 dstEndIndex srcIndex
                        {
                            OMP_FOR
                            for (int index =0; index <= dstEndIndex; ++index)
                            {coresP[index].sub(&reg32sP[index], srcData); }
                        }
                        else // SUB _dst16 _src32 dstEndIndex srcIndex (srcData -> low of~)
                        {
                            OMP_FOR
                            for (int index =0; index <= dstEndIndex; ++index)
                            {coresP[index].sub(&reg16sP[index], srcData.low()); }
                        }
                    }
                    else
                    {
                        reg16 srcData(reg16sP[srcOffset].word());

                        if (dstIsMod32) // SUB _dst32 _src16 dstEndIndex srcIndex
                        {
                            OMP_FOR
                            for (int index =0; index <= dstEndIndex; ++index)
                            {coresP[index].sub(&reg32sP[index], srcData); }
                        }
                        else // SUB _dst16 _src16 dstEndIndex srcIndex
                        {
                            OMP_FOR
                            for (int index =0; index <= dstEndIndex; ++index)
                            {coresP[index].sub(&reg16sP[index], srcData); }
                        }
                    }
                    return;
                }
                case 0x06: // SUB _X _dst32/16 _src32/16 begIndex endIndex, Bytes 3, [sub, dstMod32/16(1)|srcMod32/16(1)|begIndex(6), endIndex]
                {
                    uint8_t begIndex = commandsBuffer[1] & 0x3F;
                    uint8_t endIndex = commandsBuffer[2];
                    if (begIndex >= Core_count || endIndex >= Core_count || begIndex > endIndex) {return; }
                    bool dstIsMod32 = (commandsBuffer[1] & 0x80) != 0;
                    bool srcIsMod32 = (commandsBuffer[1] & 0x40) != 0;
                    if (dstIsMod32)
                    {
                        if (srcIsMod32) // SUB _X _dst32 _src32 begIndex endIndex
                        {
                            OMP_FOR
                            for (int index = begIndex; index <= endIndex; ++index)
                            {coresP[index].sub(&reg32sP[index], reg32sP[index]); }
                        }
                        else // SUB _X _dst32 _src16 begIndex endIndex
                        {
                            OMP_FOR
                            for (int index = begIndex; index <= endIndex; ++index)
                            {coresP[index].sub(&reg32sP[index], reg16sP[index]); }
                        }
                    }
                    else 
                    {
                        if (srcIsMod32) // SUB _X _dst16 _src32 begIndex endIndex (srcData -> low of~)
                        {
                            OMP_FOR
                            for (int index = begIndex; index <= endIndex; ++index)
                            {coresP[index].sub(&reg16sP[index], reg32sP[index].low()); }
                        }
                        else // SUB _X _dst16 _src16 begIndex endIndex
                        {
                            OMP_FOR
                            for (int index = begIndex; index <= endIndex; ++index)
                            {coresP[index].sub(&reg16sP[index], reg16sP[index]); }
                        }
                    }
                    return;
                }
                case 0x07: // SUB _dst16 _srcImm dstEndIndex imm16, Bytes 4, [sub, dstEndIndex, immL, immH]
                {
                    uint8_t dstEndIndex = commandsBuffer[1];
                    if (dstEndIndex >= Core_count) {return; }
                    uint16_t imm16 = (uint16_t)commandsBuffer[2] | ((uint16_t)commandsBuffer[3] << 8);
                    OMP_FOR
                    for (int index =0; index <= dstEndIndex; ++index)
                    {coresP[index].sub(&reg16sP[index], imm16); }
                    return;
                }
                case 0x08: // SUB _dst32 _srcImm dstEndIndex imm32, Bytes 6, [sub, dstEndIndex, immLL, immLH, immHL, immHH]
                {
                    uint8_t dstEndIndex = commandsBuffer[1];
                    if (dstEndIndex >= Core_count) {return; }
                    uint32_t imm32 = (uint32_t)commandsBuffer[2] | ((uint32_t)commandsBuffer[3] << 8) |
                        ((uint32_t)commandsBuffer[4] << 16) | ((uint32_t)commandsBuffer[5] << 24);
                    OMP_FOR
                    for (int index =0; index <= dstEndIndex; ++index)
                    {coresP[index].sub(&reg32sP[index], imm32); }
                    return;
                }

                case 0x0a: // AND _dst32/16 _src32/16 dstEndIndex srcIndex, Bytes 3, [and, dstEndIndex, dstMod32/16(1)|srcMod32/16(1)|srcOffset(6)(0xC0|~)]
                {
                    uint8_t dstEndIndex = commandsBuffer[1];
                    uint8_t srcOffset = (commandsBuffer[2] & 0x3f) | 0xC0;
                    if (dstEndIndex >= Core_count || srcOffset >= Core_count) {return; }
                    bool dstIsMod32 = (commandsBuffer[2] & 0x80) != 0;
                    bool srcIsMod32 = (commandsBuffer[2] & 0x40) != 0;
                    if (srcIsMod32)
                    {
                        reg32 srcData(reg32sP[srcOffset].word());

                        if (dstIsMod32) // AND _dst32 _src32 dstEndIndex srcIndex
                        {
                            OMP_FOR
                            for (int index =0; index <= dstEndIndex; ++index)
                            {coresP[index].andL(&reg32sP[index], srcData); }
                        }
                        else // AND _dst16 _src32 dstEndIndex srcIndex (srcData -> low of~)
                        {
                            OMP_FOR
                            for (int index =0; index <= dstEndIndex; ++index)
                            {coresP[index].andL(&reg16sP[index], srcData.low()); }
                        }
                    }
                    else
                    {
                        reg16 srcData(reg16sP[srcOffset].word());

                        if (dstIsMod32) // AND _dst32 _src16 dstEndIndex srcIndex
                        {
                            OMP_FOR
                            for (int index =0; index <= dstEndIndex; ++index)
                            {coresP[index].andL(&reg32sP[index], srcData); }
                        }
                        else // AND _dst16 _src16 dstEndIndex srcIndex
                        {
                            OMP_FOR
                            for (int index =0; index <= dstEndIndex; ++index)
                            {coresP[index].andL(&reg16sP[index], srcData); }
                        }
                    }
                    return;
                }

            }
        }
        void addressing()
        {
            uint32_t OPcode_addr = GCS.word() * gRamSegPer_size + GIP.word();
            if (OPcode_addr >= Gram_size) {return; }
            uint8_t opc = GramP[OPcode_addr];
            //if (opc > gCenter_opc_maxize) {return; }
            uint8_t commandLength = gCenter_opCode_decodeTable[opc];
            if (commandLength == 0 || OPcode_addr + commandLength -1 >= Gram_size) {return; }

            for (uint8_t index =0; index < commandLength; ++index)
            {commands[index] = GramP[OPcode_addr + index]; }

            _mov_buffer();
            GIP.word(GIP.word() + commandLength);
            decode();
        }

        int run_cycle() 
        {
            if (!runningState) {return -1; }
            return 0;
        }
        void loop()
        {}

    };

    bool Center32g::decodeTable_inited = false;
    uint8_t* Center32g::gCenter_opCode_decodeTable = new uint8_t[gCenter_opc_maxize +1]();

};







