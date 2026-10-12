/*
 * Regs_libF.cpp —— register 模块的静态库实现文件
 *
 * 由 Regs.cpp 拷贝而来（_libF 后缀表示 "for library formatting"）。
 * 相对原件的改动仅为「库格式改造」：把原先写在类体内的成员函数实现
 * 全部移到类外，改为 reg32::xxx / reg16::xxx / regF32::xxx 的限定名定义；
 * 类声明集中到 ../libs/Regs.h，以便编译成 C++ 风格的静态库（libRegs.a）。
 *
 * 算法与行为语义与原件逐句一致，未做任何逻辑修改。
 *
 * 说明：HalfRef 是嵌套类，其成员函数用 reg32::HalfRef::xxx 形式定义。
 *       C++11 起嵌套类可访问外层类的私有成员，故 HalfRef(reg32&, int)
 *       可以直接读取 reg._value。
 */

#include "../_libs/Regs.h"

#include <algorithm>


namespace CK
{


/* ==================================================================
 * reg32
 * ================================================================== */

reg32::reg32(uint32_t init) : _value(init) {}

reg32::HalfRef reg32::low()  {return HalfRef(_value, 0); }
reg32::HalfRef reg32::high() {return HalfRef(_value, 16); }

uint32_t reg32::word() const     {return _value; }
void reg32::word(uint32_t Val)   {_value = Val; }

reg32& reg32::operator=(uint32_t Val) {_value = Val; return *this; }
reg32::operator uint32_t() const {return _value; }

void reg32::chbit(int bit_index)       // change bit
{
    bit_index = std::min(31, std::max(0, bit_index));
    _value ^= (0b01u << bit_index);
}
bool reg32::ckbit(int bit_index) const // check bit
{
    bit_index = std::min(31, std::max(0, bit_index));
    return ((_value >> bit_index) & 0b01u) ==1;
}
void reg32::stbit(int bit_index)       // set bit
{
    bit_index = std::min(31, std::max(0, bit_index));
    _value |= (0b01u << bit_index);
}
void reg32::clbit(int bit_index)       // clear bit
{
    bit_index = std::min(31, std::max(0, bit_index));
    _value &= ~(0b01u << bit_index);
}
bool reg32::isParityL8() const
{
    uint8_t valueT = _value & 0xffu;
    valueT ^= (valueT >> 4);
    valueT ^= (valueT >> 2);
    valueT ^= (valueT >> 1);
    return (valueT & 0b01u) ==0;
}

// Low halfReg
void reg32::chbitL(int bit_index)
{
    bit_index = std::min(15, std::max(0, bit_index));
    low() ^= (0b01u << bit_index);
}
bool reg32::ckbitL(int bit_index) const
{
    bit_index = std::min(15, std::max(0, bit_index));
    return ((_value >> bit_index) & 0b01u) ==1;
}
void reg32::stbitL(int bit_index)
{
    bit_index = std::min(15, std::max(0, bit_index));
    low() |= (0b01u << bit_index);
}
void reg32::clbitL(int bit_index)
{
    bit_index = std::min(15, std::max(0, bit_index));
    low() &= ~(0b01u << bit_index);
}

// High halfReg
void reg32::chbitH(int bit_index)
{
    bit_index = std::min(15, std::max(0, bit_index));
    high() ^= (0b01u << bit_index);
}
bool reg32::ckbitH(int bit_index) const
{
    bit_index = std::min(15, std::max(0, bit_index));
    return ((_value >> (bit_index + 16)) & 0b01u) ==1;
}
void reg32::stbitH(int bit_index)
{
    bit_index = std::min(15, std::max(0, bit_index));
    high() |= (0b01u << bit_index);
}
void reg32::clbitH(int bit_index)
{
    bit_index = std::min(15, std::max(0, bit_index));
    high() &= ~(0b01u << bit_index);
}


/* --------------------------------------------------------------
 * reg32::HalfRef
 * -------------------------------------------------------------- */

reg32::HalfRef::HalfRef(uint32_t& whole, int shift) : _whole(whole), _shift(shift) {}
reg32::HalfRef::HalfRef(reg32& reg, int shift) : _whole(reg._value), _shift(shift) {}

reg32::HalfRef::operator uint16_t() const {return static_cast<uint16_t>((_whole >> _shift) & 0xffff); }

bool reg32::HalfRef::isParity() const
{
    uint8_t valueT = static_cast<uint16_t>((_whole >> _shift) & 0xffu);
    valueT ^= (valueT >> 4);
    valueT ^= (valueT >> 2);
    valueT ^= (valueT >> 1);
    return (valueT & 0b01u) ==0;
}
bool reg32::HalfRef::ckbit(int bit_index) const
{
    bit_index = std::min(15, std::max(0, bit_index));
    return ((_whole >> _shift) & (0b01u << bit_index)) ==1;
}

reg32::HalfRef& reg32::HalfRef::operator=(uint16_t val)
{
    uint32_t mask = 0xffffu << _shift;
    _whole = (_whole & ~mask) | ((uint32_t)val << _shift); 

    return *this;
}
reg32::HalfRef& reg32::HalfRef::operator=(const HalfRef& otherHalfReg)
{
    uint32_t mask = 0xffffu << _shift;
    _whole = (_whole & ~mask) | (otherHalfReg._whole & mask); 

    return *this;
}
reg32::HalfRef& reg32::HalfRef::operator^=(uint16_t mask)
{
    uint32_t fullMask = (uint32_t)mask << _shift;
    _whole ^= fullMask;
    return *this;
}
reg32::HalfRef& reg32::HalfRef::operator|=(uint16_t mask)
{
    uint32_t fullMask = (uint32_t)mask << _shift;
    _whole |= fullMask;
    return *this;
}
reg32::HalfRef& reg32::HalfRef::operator&=(uint16_t mask)
{
    uint16_t current = static_cast<uint16_t>((_whole >> _shift) & 0xffffu);
    current &= mask;
    *this = current; 
    return *this;
}
reg32::HalfRef& reg32::HalfRef::operator+=(uint16_t val)
{
    uint16_t current = static_cast<uint16_t>((_whole >> _shift) & 0xffffu);
    current += val;
    *this = current; 
    return *this;
}
reg32::HalfRef& reg32::HalfRef::operator-=(uint16_t val)
{
    uint16_t current = static_cast<uint16_t>((_whole >> _shift) & 0xffffu);
    current -= val;
    *this = current;
    return *this;
}


/* ==================================================================
 * reg16
 * ================================================================== */

reg16::reg16(uint16_t init) : _value(init) {}

reg16::HalfRef reg16::low()  {return HalfRef(_value, 0); }
reg16::HalfRef reg16::high() {return HalfRef(_value, 8); }

uint16_t reg16::word() const     {return _value; }
void reg16::word(uint16_t Val)   {_value = Val; }

reg16& reg16::operator=(uint16_t Val) {_value = Val; return *this; }
reg16::operator uint16_t() const {return _value; }

void reg16::chbit(int bit_index)
{
    bit_index = std::min(15, std::max(0, bit_index));
    _value ^= (0b01u << bit_index);
}
bool reg16::ckbit(int bit_index) const
{
    bit_index = std::min(15, std::max(0, bit_index));
    return ((_value >> bit_index) & 0b01u) ==1;
}
void reg16::stbit(int bit_index)
{
    bit_index = std::min(15, std::max(0, bit_index));
    _value |= (0b01u << bit_index);
}
void reg16::clbit(int bit_index)
{
    bit_index = std::min(15, std::max(0, bit_index));
    _value &= ~(0b01u << bit_index);
}
bool reg16::isParity() const
{
    uint8_t valueT = _value & 0xffu;
    valueT ^= (valueT >> 4);
    valueT ^= (valueT >> 2);
    valueT ^= (valueT >> 1);
    return (valueT & 0b01u) ==0;
}

// Low halfReg
void reg16::chbitL(int bit_index)
{
    bit_index = std::min(7, std::max(0, bit_index));
    _value ^= (0b01u << bit_index);
}
bool reg16::ckbitL(int bit_index) const
{
    bit_index = std::min(7, std::max(0, bit_index));
    return ((_value >> bit_index) & 0b01u) ==1;
}
void reg16::stbitL(int bit_index)
{
    bit_index = std::min(7, std::max(0, bit_index));
    _value |= (0b01u << bit_index);
}
void reg16::clbitL(int bit_index)
{
    bit_index = std::min(7, std::max(0, bit_index));
    _value &= ~(0b01u << bit_index);
}

// High halfReg
void reg16::chbitH(int bit_index)
{
    bit_index = std::min(7, std::max(0, bit_index));
    _value ^= (0b01u << (bit_index + 8));
}
bool reg16::ckbitH(int bit_index) const
{
    bit_index = std::min(7, std::max(0, bit_index));
    return ((_value >> (bit_index + 8)) & 0b01u) ==1;
}
void reg16::stbitH(int bit_index)
{
    bit_index = std::min(7, std::max(0, bit_index));
    _value |= (0b01u << (bit_index + 8));
}
void reg16::clbitH(int bit_index)
{
    bit_index = std::min(7, std::max(0, bit_index));
    _value &= ~(0b01u << (bit_index + 8));
}


/* --------------------------------------------------------------
 * reg16::HalfRef
 * -------------------------------------------------------------- */

reg16::HalfRef::HalfRef(uint16_t& whole, int shift) : _whole(whole), _shift(shift) {}
reg16::HalfRef::HalfRef(reg16& reg, int shift) : _whole(reg._value), _shift(shift) {}

reg16::HalfRef::operator uint8_t() const {return static_cast<uint8_t>((_whole >> _shift) & 0xff); }

bool reg16::HalfRef::ckbit(int bit_index) const
{
    bit_index = std::min(7, std::max(0, bit_index));
    return ((_whole >> _shift) & (0b01u << bit_index)) ==1;
}

reg16::HalfRef& reg16::HalfRef::operator=(uint8_t val)
{
    uint16_t mask = 0xffu << _shift;
    _whole = (_whole & ~mask) | ((uint16_t)val << _shift); 

    return *this;
}
reg16::HalfRef& reg16::HalfRef::operator=(const HalfRef& otherHalfReg)
{
    uint16_t mask = 0xffu << _shift;
    _whole = (_whole & ~mask) | (otherHalfReg._whole & mask); 

    return *this;
}
reg16::HalfRef& reg16::HalfRef::operator^=(uint8_t mask)
{
    uint16_t fullMask = (uint16_t)mask << _shift;
    _whole ^= fullMask;
    return *this;
}
reg16::HalfRef& reg16::HalfRef::operator|=(uint8_t mask)
{
    uint16_t fullMask = (uint16_t)mask << _shift;
    _whole |= fullMask;
    return *this;
}
reg16::HalfRef& reg16::HalfRef::operator&=(uint8_t mask)
{
    uint8_t current = static_cast<uint8_t>((_whole >> _shift) & 0xffu);
    current &= mask;
    *this = current; 
    return *this;
}
reg16::HalfRef& reg16::HalfRef::operator+=(uint8_t val)
{
    uint8_t current = static_cast<uint8_t>((_whole >> _shift) & 0xffu);
    current += val;
    *this = current; 
    return *this;
}
reg16::HalfRef& reg16::HalfRef::operator-=(uint8_t val)
{
    uint8_t current = static_cast<uint8_t>((_whole >> _shift) & 0xffu);
    current -= val;
    *this = current;
    return *this;
}


/* ==================================================================
 * regF32
 * ================================================================== */

regF32::regF32(float init) : _value(init) {}

regF32& regF32::operator=(float Val) {_value = Val; return *this; }
regF32::operator float() const {return _value; }

float regF32::word() const  {return _value; }
void regF32::word(float Val) {_value = Val; }


}
