#include <cstdint>
#include <cmath>
#include <thread>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <cstring>




namespace CK
{
    
    static const uint32_t ram_maxize = 0x7fffffff;     // uint8 -> 1 Byte * 0x7fffffff => ~ 2 GiB
    static const uint32_t ram_minSize = 0x7ffffff;     // uint8 -> 1 Byte * 0xfffffff => ~ 128 MiB
    static const uint32_t stack_maxize = 0xfffff;      // uint8 -> 1 Byte * 0xfffff => ~ 1 MiB
    static const uint32_t gram_maxize = 0xfffffff;     // uint8 -> 1 Byte * 0xfffffff => ~ 256 MiB
    static const uint32_t ramSegPer_size = 262144;     // 256 KiB per Segment in RAM
    
    static const uint8_t dead_magicNum = 0xcc;


    typedef struct
    {
        uint32_t data = 0xcccccccc; // truncate as needed
        uint8_t resFlag = 0x00; // [safeData(4)|iSafe(1)|UAF(1)|MOF(1)|isValid(1)]
        /* safeData is reserved for future interactions with Center16g */
    } resData;


    class Ram
    {
    private:
        uint32_t ramSegs_maxcount;
        uint32_t ram_size;
        uint32_t stackGeneral_endIndex, stackSegReg_endIndex, stackFLAGS_endIndex;
        const static uint32_t ramSegs_size = ramSegPer_size;
        const static uint32_t PID_maxize = 0xffff;
        const static uint16_t rootPID = 0x00, constPID = 0x01, publicPID = 0xffff;
        mutable uint32_t read_times = 0;
        mutable uint32_t write_times = 0;
        mutable bool deadState_temp = false;
        mutable uint8_t* ramD;
        
        // 0x00 is root PID, same means it s locked for other Processes tha cant visit;
        // 0x01 is const PID, same means every Process can visit tha ram segmentation locked by it but read only except root PID;
        // 0xffff is public PID, same means every Process can visit the ram segmentation locked by it;
        mutable uint16_t* ramSgmtation_LocktPID;

    public:
        std::mutex raMutexC;


        Ram(uint32_t ramSizeT=ram_maxize)
            : ram_size(std::min(ram_maxize, std::max(ram_minSize, ramSizeT))), ramD(new uint8_t[ram_size +1]()),
            ramSegs_maxcount(ram_size / ramSegs_size), ramSgmtation_LocktPID(new uint16_t[ramSegs_maxcount +1]()),
            stackGeneral_endIndex(ram_size - stack_maxize),
            stackSegReg_endIndex(stackGeneral_endIndex - stack_maxize),
            stackFLAGS_endIndex(stackSegReg_endIndex - stack_maxize) 
        {
            memset(ramSgmtation_LocktPID, publicPID, ramSegs_maxcount);
        }

        bool PIDcanRead(uint32_t index, uint16_t PID) const
        {
            uint32_t index_ramSeg = index / ramSegs_size;
            uint16_t ramPID = ramSgmtation_LocktPID[index_ramSeg];
            if (ramPID == rootPID) {return false; }
            if (ramPID == constPID) {return true; }
            if (ramPID == publicPID) {return true; }
            return PID == ramPID;
        }
        bool PIDcanWrite(uint32_t index, uint16_t PID) const
        {
            uint32_t index_ramSeg = index / ramSegs_size;
            uint16_t ramPID = ramSgmtation_LocktPID[index_ramSeg];
            if (ramPID == rootPID) {return false; }
            if (ramPID == constPID) {return false; }
            if (ramPID == publicPID) {return true; }
            return PID == ramPID;
        }

        operator uint8_t() const {return *ramD; }

        uint32_t get_generalEndIndex() const {return stackGeneral_endIndex; }
        uint32_t get_segRegEndIndex() const {return stackSegReg_endIndex; }
        uint32_t get_flagsEndIndex() const {return stackFLAGS_endIndex; }
        uint32_t get_ramSize() const {return ram_size; }

        resData read(uint32_t index, uint16_t PID=publicPID) const
        {
            resData res;
            bool canVisit = true; // root PID can pass
            if (!(PID == rootPID)) {canVisit = PIDcanRead(index, PID); }
            if (!canVisit) 
            {
                res.resFlag |= 0b0100; // set UAF
                return res;
            }
            if (index > ram_size) 
            {
                res.resFlag |=0b0010; // set MOF;
                return res;
            }
            read_times++;
            res.data = ramD[index];
            res.resFlag |= 0b0001;
            return res;
        }
        resData write(uint32_t index, uint8_t val, uint16_t PID=publicPID)
        {
            resData res;
            bool canVisit = true; // root PID can pass
            if (!(PID == rootPID)) {canVisit = PIDcanWrite(index, PID); }
            if (!canVisit) 
            {
                res.resFlag |= 0b0100; // set UAF
                return res;
            }
            if (index > ram_size) 
            {
                res.resFlag |=0b0010; // set MOF;
                return res;
            }
            write_times++;
            ramD[index] = val;
            res.resFlag |= 0b0001;
            return res;
        }
        resData read16(uint32_t index, uint16_t PID=publicPID) const
        {
            resData res;
            bool canVisit = true; // root PID can pass
            if (!(PID == rootPID)) {canVisit = PIDcanRead(index, PID) && PIDcanRead(index +1, PID); }
            if (!canVisit) 
            {
                res.resFlag |= 0b0100;
                return res;
            }
            if (index +1 > ram_size)
            {
                res.resFlag |=0b0010;
                return res;
            }
            read_times++;
            res.data = ((uint16_t)ramD[index] | (uint16_t)ramD[index +1] << 8);
            res.resFlag |= 0b0001;
            return res;
        }
        resData read32(uint32_t index, uint16_t PID=publicPID) const
        {
            resData res;
            bool canVisit = true; // root PID can pass
            if (!(PID == rootPID)) {canVisit = PIDcanRead(index, PID) && PIDcanRead(index +3, PID); }
            if (!canVisit) 
            {
                res.resFlag |= 0b0100;
                return res;
            }
            if (index +3 > ram_size)
            {
                res.resFlag |=0b0010;
                return res;
            }
            read_times++;
            res.data = ((uint32_t)ramD[index] | (uint32_t)ramD[index +1] << 8 | (uint32_t)ramD[index +2] << 16 | (uint32_t)ramD[index +3] << 24);
            res.resFlag |= 0b0001;
            return res;
        }
        resData write16(uint32_t index, uint16_t val, uint16_t PID=publicPID)
        {
            resData res;
            bool canVisit = true; // root PID can pass
            if (!(PID == rootPID)) {canVisit = PIDcanWrite(index, PID) && PIDcanWrite(index +1, PID); }
            if (!canVisit) 
            {
                res.resFlag |= 0b0100;
                return res; 
            }
            if (index +1 > ram_size)
            {
                res.resFlag |=0b0010;
                return res; 
            }
            write_times++;
            ramD[index] = val;
            ramD[index +1] = val >> 8;
            res.resFlag |= 0b0001;
            return res;
        }
        resData write32(uint32_t index, uint32_t val, uint16_t PID=publicPID)
        {
            resData res;
            bool canVisit = true; // root PID can pass
            if (!(PID == rootPID)) {canVisit = PIDcanWrite(index, PID) && PIDcanWrite(index +3, PID); }
            if (!canVisit) 
            {
                res.resFlag |= 0b0100;
                return res;
            }
            if (index +3 > ram_size)
            {
                res.resFlag |=0b0010;
                return res;
            }
            write_times++;
            ramD[index] = val;
            ramD[index +1] = val >> 8;
            ramD[index +2] = val >> 16;
            ramD[index +3] = val >> 24;
            res.resFlag |= 0b0001;
            return res;
        }

        bool ___get_deadState() const {return deadState_temp; }

        uint32_t get_readTimes() const {return read_times; }
        uint32_t get_writeTimes() const {return write_times; }

        void setSegPID(uint32_t index_ramSeg, uint16_t PIDt) {ramSgmtation_LocktPID[index_ramSeg] = PIDt; }


        ~Ram()
        {
            delete[] ramD;
            delete[] ramSgmtation_LocktPID;
        }

    };

};



