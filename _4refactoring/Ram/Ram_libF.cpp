/*
 * Ram_libF.cpp —— Ram 模块的静态库实现文件
 *
 * 由 Ram.cpp 拷贝而来（_libF 后缀表示 "for library formatting"）。
 * 相对原件的改动仅为「库格式改造」：把原先写在类体内的成员函数实现
 * 全部移到类外，改为 Ram::xxx 的限定名定义；声明集中到 ../libs/Ram.h，
 * 以便编译成 C++ 风格的静态库（libRam.a）。
 *
 * 另有四处修正（均为原件缺陷，逐条列出以便取舍）：
 *
 *   [1] 成员初始化顺序（必须修）
 *       原件中 ramSegs_maxcount 的声明位于 ram_size 之前，而它由 ram_size
 *       推导。C++ 按「声明顺序」而非初始化列表顺序初始化成员，故原件会用
 *       尚未初始化的 ram_size 计算 ramSegs_maxcount，属未定义行为。
 *       已把 ram_size 声明前置（见 ../libs/Ram.h），并令初始化列表与声明顺序一致。
 *
 *   [2] 段权限表初始化（必须修）
 *       原件用 memset(ptr, publicPID, ramSegs_maxcount)：
 *         - 长度按「元素个数」而非「字节数」给出，只覆盖了表的一半；
 *         - memset 的填充值只取低 8 位，依赖 publicPID == 0xffff 才恰好正确。
 *       结果是表的后半段保持全 0（即 rootPID），使后半段内存被错误地锁成
 *       root-only。已改为 std::fill 逐元素填充整个表。
 *
 *   [3] 越界访问会踩踏段权限表（行为变更，请确认）
 *       原件的检查顺序是「先权限、后边界」，而段号由 index / ramSegs_size
 *       算出且 PIDcanRead/PIDcanWrite 内部不查边界。当 index 超出 ram_size 时，
 *       段号会越过 ramSgmtation_LocktPID 的范围，造成堆越界读；返回的标志位
 *       也因此变成 UAF 而非应有的 MOF。
 *       本副本把「边界检查」提前到「权限检查」之前，并在 PIDcanRead /
 *       PIDcanWrite 内补了一道边界防线。
 *       → 可见的行为变化：越界访问的返回由 UAF(0b0100) 变为 MOF(0b0010)。
 *
 *   [4] setSegPID 补边界检查（行为变更，请确认）
 *       原件不校验 index_ramSeg，越界即写坏堆内存。本副本改为越界时静默忽略。
 *
 * 除上述四点外，算法与行为语义（返回值、标志位、边界判定条件）与原件逐句一致。
 */

#include "../_libs/Ram.h"

#include <algorithm>
#include <cstring>


namespace CK
{


/* ==================================================================
 * 构造 / 析构
 * ================================================================== */

Ram::Ram(uint32_t ramSizeT)
    : ram_size(std::min(ram_maxize, std::max(ram_minSize, ramSizeT))),
    ramSegs_maxcount(ram_size / ramSegs_size),
    stackGeneral_endIndex(ram_size - stack_maxize),
    stackSegReg_endIndex(stackGeneral_endIndex - stack_maxize),
    stackFLAGS_endIndex(stackSegReg_endIndex - stack_maxize),
    ramD(new uint8_t[ram_size +1]()),
    ramSgmtation_LocktPID(new uint16_t[ramSegs_maxcount +1]())
{
    std::fill(ramSgmtation_LocktPID, ramSgmtation_LocktPID + ramSegs_maxcount +1, publicPID);
}

Ram::~Ram()
{
    delete[] ramD;
    delete[] ramSgmtation_LocktPID;
}


/* ==================================================================
 * 权限判定
 *
 * 前置边界防线：段号由 index / ramSegs_size 得出，若 index 超出 ram_size
 * 则段号可能越界，故此处先做范围判定再取表项（原件无此保护）。
 * ================================================================== */

bool Ram::PIDcanRead(uint32_t index, uint16_t PID) const
{
    if (index > ram_size) {return false; }
    uint32_t index_ramSeg = index / ramSegs_size;
    uint16_t ramPID = ramSgmtation_LocktPID[index_ramSeg];
    if (ramPID == rootPID) {return false; }
    if (ramPID == constPID) {return true; }
    if (ramPID == publicPID) {return true; }
    return PID == ramPID;
}

bool Ram::PIDcanWrite(uint32_t index, uint16_t PID) const
{
    if (index > ram_size) {return false; }
    uint32_t index_ramSeg = index / ramSegs_size;
    uint16_t ramPID = ramSgmtation_LocktPID[index_ramSeg];
    if (ramPID == rootPID) {return false; }
    if (ramPID == constPID) {return false; }
    if (ramPID == publicPID) {return true; }
    return PID == ramPID;
}


/* ==================================================================
 * 隐式转换 / 尺寸 getter
 * ================================================================== */

Ram::operator uint8_t() const {return *ramD; }

uint32_t Ram::get_generalEndIndex() const {return stackGeneral_endIndex; }
uint32_t Ram::get_segRegEndIndex() const {return stackSegReg_endIndex; }
uint32_t Ram::get_flagsEndIndex() const {return stackFLAGS_endIndex; }
uint32_t Ram::get_ramSize() const {return ram_size; }


/* ==================================================================
 * 读访问
 *
 * 注意：边界判定已提到权限判定之前（原件的顺序相反），
 *       故越界返回 MOF(0b0010)，不再返回 UAF(0b0100)。
 * ================================================================== */

resData Ram::read(uint32_t index, uint16_t PID) const
{
    resData res;
    if (index > ram_size) 
    {
        res.resFlag |=0b0010; // set MOF;
        return res;
    }
    bool canVisit = true; // root PID can pass
    if (!(PID == rootPID)) {canVisit = PIDcanRead(index, PID); }
    if (!canVisit) 
    {
        res.resFlag |= 0b0100; // set UAF
        return res;
    }
    read_times++;
    res.data = ramD[index];
    res.resFlag |= 0b0001;
    return res;
}

resData Ram::read16(uint32_t index, uint16_t PID) const
{
    resData res;
    if (index +1 > ram_size)
    {
        res.resFlag |=0b0010;
        return res;
    }
    bool canVisit = true; // root PID can pass
    if (!(PID == rootPID)) {canVisit = PIDcanRead(index, PID) && PIDcanRead(index +1, PID); }
    if (!canVisit) 
    {
        res.resFlag |= 0b0100;
        return res;
    }
    read_times++;
    res.data = ((uint16_t)ramD[index] | (uint16_t)ramD[index +1] << 8);
    res.resFlag |= 0b0001;
    return res;
}

resData Ram::read32(uint32_t index, uint16_t PID) const
{
    resData res;
    if (index +3 > ram_size)
    {
        res.resFlag |=0b0010;
        return res;
    }
    bool canVisit = true; // root PID can pass
    if (!(PID == rootPID)) {canVisit = PIDcanRead(index, PID) && PIDcanRead(index +3, PID); }
    if (!canVisit) 
    {
        res.resFlag |= 0b0100;
        return res;
    }
    read_times++;
    res.data = ((uint32_t)ramD[index] | (uint32_t)ramD[index +1] << 8 | (uint32_t)ramD[index +2] << 16 | (uint32_t)ramD[index +3] << 24);
    res.resFlag |= 0b0001;
    return res;
}


/* ==================================================================
 * 写访问
 *
 * 同上：边界判定已提到权限判定之前。
 * ================================================================== */

resData Ram::write(uint32_t index, uint8_t val, uint16_t PID)
{
    resData res;
    if (index > ram_size) 
    {
        res.resFlag |=0b0010; // set MOF;
        return res;
    }
    bool canVisit = true; // root PID can pass
    if (!(PID == rootPID)) {canVisit = PIDcanWrite(index, PID); }
    if (!canVisit) 
    {
        res.resFlag |= 0b0100; // set UAF
        return res;
    }
    write_times++;
    ramD[index] = val;
    res.resFlag |= 0b0001;
    return res;
}

resData Ram::write16(uint32_t index, uint16_t val, uint16_t PID)
{
    resData res;
    if (index +1 > ram_size)
    {
        res.resFlag |=0b0010;
        return res; 
    }
    bool canVisit = true; // root PID can pass
    if (!(PID == rootPID)) {canVisit = PIDcanWrite(index, PID) && PIDcanWrite(index +1, PID); }
    if (!canVisit) 
    {
        res.resFlag |= 0b0100;
        return res; 
    }
    write_times++;
    ramD[index] = val;
    ramD[index +1] = val >> 8;
    res.resFlag |= 0b0001;
    return res;
}

resData Ram::write32(uint32_t index, uint32_t val, uint16_t PID)
{
    resData res;
    if (index +3 > ram_size)
    {
        res.resFlag |=0b0010;
        return res;
    }
    bool canVisit = true; // root PID can pass
    if (!(PID == rootPID)) {canVisit = PIDcanWrite(index, PID) && PIDcanWrite(index +3, PID); }
    if (!canVisit) 
    {
        res.resFlag |= 0b0100;
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


/* ==================================================================
 * 状态查询 / 配置
 * ================================================================== */

bool Ram::___get_deadState() const {return deadState_temp; }

uint32_t Ram::get_readTimes() const {return read_times; }
uint32_t Ram::get_writeTimes() const {return write_times; }

void Ram::setSegPID(uint32_t index_ramSeg, uint16_t PIDt)
{
    if (index_ramSeg > ramSegs_maxcount) {return; }   // 边界防线，越界静默忽略
    ramSgmtation_LocktPID[index_ramSeg] = PIDt;
}


}
