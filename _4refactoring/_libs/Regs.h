#ifndef CK_REGS_H
#define CK_REGS_H

#include <cstdint>

/*
 * register 模块 —— 静态库公共头文件（libRegs.a）
 *
 * 对外提供三个寄存器封装类，均为值语义，可直接在栈上或容器中持有：
 *   reg32   —— 32 位寄存器，low() / high() 返回 16 位 HalfRef 半字视图
 *   reg16   —— 16 位寄存器，low() / high() 返回 8 位  HalfRef 字节视图
 *   regF32  —— 32 位浮点寄存器
 *
 * HalfRef 是对宿主寄存器内部存储的「引用视图」，自身不持有数据：
 *   - 对它的赋值与复合赋值会就地回写到宿主寄存器的对应半字，
 *     宿主寄存器的其余位保持不变；
 *   - 因含引用成员，HalfRef 不可默认构造，且拷贝赋值由用户定义。
 *
 * 所有符号位于 namespace CK，采用 C++ 链接（无 extern "C"），
 * 故只能由 C++ 翻译单元链接使用。
 *
 * register module — public header for the static library (libRegs.a).
 * Three register wrapper classes with value semantics.
 * HalfRef is a reference view into the host register's storage; writes
 * are applied in place to the corresponding half-word, other bits intact.
 * All symbols live in namespace CK with C++ linkage (no extern "C").
 */

namespace CK
{

class reg32
{
private:
    uint32_t _value;

public:
    class HalfRef
    {
    public:
        HalfRef(uint32_t& whole, int shift);
        HalfRef(reg32& reg, int shift);

        operator uint16_t() const;

        bool isParity() const;
        bool ckbit(int bit_index) const;

        HalfRef& operator=(uint16_t val);
        HalfRef& operator=(const HalfRef& otherHalfReg);
        HalfRef& operator^=(uint16_t mask);
        HalfRef& operator|=(uint16_t mask);
        HalfRef& operator&=(uint16_t mask);
        HalfRef& operator+=(uint16_t val);
        HalfRef& operator-=(uint16_t val);

    private:
        uint32_t& _whole;
        int _shift;
    };

    reg32(uint32_t init =0);

    HalfRef low();
    HalfRef high();

    uint32_t word() const;
    void word(uint32_t Val);

    reg32& operator=(uint32_t Val);
    operator uint32_t() const;

    void chbit(int bit_index);          // change bit
    bool ckbit(int bit_index) const;    // check bit
    void stbit(int bit_index);          // set bit
    void clbit(int bit_index);          // clear bit
    bool isParityL8() const;            // parity of the low 8 bits

    // Low halfReg
    void chbitL(int bit_index);
    bool ckbitL(int bit_index) const;
    void stbitL(int bit_index);
    void clbitL(int bit_index);

    // High halfReg
    void chbitH(int bit_index);
    bool ckbitH(int bit_index) const;
    void stbitH(int bit_index);
    void clbitH(int bit_index);

};

class reg16
{
private:
    uint16_t _value;

public:
    class HalfRef
    {
    public:
        HalfRef(uint16_t& whole, int shift);
        HalfRef(reg16& reg, int shift);

        operator uint8_t() const;

        bool ckbit(int bit_index) const;

        HalfRef& operator=(uint8_t val);
        HalfRef& operator=(const HalfRef& otherHalfReg);
        HalfRef& operator^=(uint8_t mask);
        HalfRef& operator|=(uint8_t mask);
        HalfRef& operator&=(uint8_t mask);
        HalfRef& operator+=(uint8_t val);
        HalfRef& operator-=(uint8_t val);

    private:
        uint16_t& _whole;
        int _shift;
    };

    reg16(uint16_t init =0);

    HalfRef low();
    HalfRef high();

    uint16_t word() const;
    void word(uint16_t Val);

    reg16& operator=(uint16_t Val);
    operator uint16_t() const;

    void chbit(int bit_index);
    bool ckbit(int bit_index) const;
    void stbit(int bit_index);
    void clbit(int bit_index);
    bool isParity() const;

    // Low halfReg
    void chbitL(int bit_index);
    bool ckbitL(int bit_index) const;
    void stbitL(int bit_index);
    void clbitL(int bit_index);

    // High halfReg
    void chbitH(int bit_index);
    bool ckbitH(int bit_index) const;
    void stbitH(int bit_index);
    void clbitH(int bit_index);

};

class regF32
{
private:
    float _value;

public:
    regF32(float init =0);

    regF32& operator=(float Val);
    operator float() const;

    float word() const;
    void word(float Val);

};

}

#endif /* CK_REGS_H */
