#ifndef CK_RAM_H
#define CK_RAM_H

#include <cstdint>
#include <mutex>

/*
 * Ram 模块 —— 静态库公共头文件（libRam.a）
 *
 * 对外提供一个带 PID 权限隔离的分段内存模型：
 *   - 内存按 ramSegPer_size（256 KiB）分段，每段记录一个锁定 PID；
 *   - 读 / 写 / 16 位 / 32 位访问统一返回 resData，把「数据」与「错误标志」
 *     分离，调用方据此判断访问是否越权（UAF）或越界（MOF）；
 *   - 构造函数把 RAM 尾部依次划出三段栈区：通用栈、段寄存器栈、FLAGS 栈。
 *
 * 权限语义（见下）：
 *   0x00 rootPID   —— 仅 root 可访问，其他 PID 一律拒绝；
 *   0x01 constPID  —— 所有 PID 可读，仅 root 可写；
 *   0xffff publicPID —— 所有 PID 可读可写。
 *
 * 注意：Ram 持有裸指针并负责释放，已显式禁用拷贝构造与拷贝赋值，
 * 以免默认浅拷贝导致重复释放。需要传递时请使用指针或引用。
 *
 * 所有符号位于 namespace CK，采用 C++ 链接（无 extern "C"）。
 *
 * Ram module — public header for the static library (libRam.a).
 * A segmented memory model with PID-based access isolation. All accesses
 * return resData, which separates the payload from the error flags (UAF =
 * unauthorized access, MOF = memory out of range). Copy construction and
 * copy assignment are explicitly deleted because the class owns raw pointers.
 * All symbols live in namespace CK with C++ linkage (no extern "C").
 */

namespace CK
{

inline constexpr uint32_t ram_maxize = 0x7fffffff;  // uint8 -> 1 Byte * 0x7fffffff => ~ 2 GiB
inline constexpr uint32_t ram_minSize = 0x7ffffff;  // uint8 -> 1 Byte * 0xfffffff => ~ 128 MiB
inline constexpr uint32_t stack_maxize = 0xfffff;   // uint8 -> 1 Byte * 0xfffff => ~ 1 MiB
inline constexpr uint32_t gram_maxize = 0xfffffff;  // uint8 -> 1 Byte * 0xfffffff => ~ 256 MiB
inline constexpr uint32_t ramSegPer_size = 262144;  // 256 KiB per Segment in RAM

inline constexpr uint8_t dead_magicNum = 0xcc;


typedef struct resData
{
    uint32_t data = 0xcccccccc; // truncate as needed
    uint8_t resFlag = 0x00;     // [safeData(4)|iSafe(1)|UAF(1)|MOF(1)|isValid(1)]
    /* safeData is reserved for future interactions with Center16g */
} resData;


class Ram
{
private:
    // NOTE: 成员初始化按「声明顺序」而非初始化列表顺序执行。
    //       ramSegs_maxcount 由 ram_size 推导，故 ram_size 必须声明在前，
    //       否则 ramSegs_maxcount 会用尚未初始化的 ram_size 计算（原实现即如此）。
    uint32_t ram_size;
    uint32_t ramSegs_maxcount;
    uint32_t stackGeneral_endIndex, stackSegReg_endIndex, stackFLAGS_endIndex;

    const static uint32_t ramSegs_size = ramSegPer_size;
    const static uint32_t PID_maxize = 0xffff;

    mutable uint32_t read_times = 0;
    mutable uint32_t write_times = 0;
    mutable bool deadState_temp = false;
    mutable uint8_t* ramD;

    // 0x00 is root PID, same means it s locked for other Processes tha cant visit;
    // 0x01 is const PID, same means every Process can visit tha ram segmentation locked by it but read only except root PID;
    // 0xffff is public PID, same means every Process can visit the ram segmentation locked by it;
    mutable uint16_t* ramSgmtation_LocktPID;

public:
    static constexpr uint16_t rootPID = 0x00;
    static constexpr uint16_t constPID = 0x01;
    static constexpr uint16_t publicPID = 0xffff;

    std::mutex raMutexC;

    Ram(uint32_t ramSizeT=ram_maxize);
    ~Ram();

    Ram(const Ram&) = delete;
    Ram& operator=(const Ram&) = delete;

    bool PIDcanRead(uint32_t index, uint16_t PID) const;
    bool PIDcanWrite(uint32_t index, uint16_t PID) const;

    operator uint8_t() const;

    uint32_t get_generalEndIndex() const;
    uint32_t get_segRegEndIndex() const;
    uint32_t get_flagsEndIndex() const;
    uint32_t get_ramSize() const;

    resData read(uint32_t index, uint16_t PID=publicPID) const;
    resData write(uint32_t index, uint8_t val, uint16_t PID=publicPID);
    resData read16(uint32_t index, uint16_t PID=publicPID) const;
    resData read32(uint32_t index, uint16_t PID=publicPID) const;
    resData write16(uint32_t index, uint16_t val, uint16_t PID=publicPID);
    resData write32(uint32_t index, uint32_t val, uint16_t PID=publicPID);

    bool ___get_deadState() const;

    uint32_t get_readTimes() const;
    uint32_t get_writeTimes() const;

    void setSegPID(uint32_t index_ramSeg, uint16_t PIDt);
};

}

#endif /* CK_RAM_H */
