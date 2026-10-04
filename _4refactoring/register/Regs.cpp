#include <cstdint>
#include <cmath>
#include <algorithm>





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

    };

    class reg16
    {
    private:
        uint16_t _value;

    public:
        class HalfRef
        {
        public:
            HalfRef(uint16_t& whole, int shift) : _whole(whole), _shift(shift) {}
            HalfRef(reg16& reg, int shift) : _whole(reg._value), _shift(shift) {}

            operator uint8_t() const {return static_cast<uint8_t>((_whole >> _shift) & 0xff); }

            bool ckbit(int bit_index) const
            {
                bit_index = std::min(7, std::max(0, bit_index));
                return ((_whole >> _shift) & (0b01u << bit_index)) ==1;
            }

            HalfRef& operator=(uint8_t val)
            {
                uint16_t mask = 0xffu << _shift;
                _whole = (_whole & ~mask) | ((uint16_t)val << _shift); 

                return *this;
            }
            HalfRef& operator=(const HalfRef& otherHalfReg) 
            {
                uint16_t mask = 0xffu << _shift;
                _whole = (_whole & ~mask) | (otherHalfReg._whole & mask); 

                return *this;
            }
            HalfRef& operator^=(uint8_t mask)
            {
                uint16_t fullMask = (uint16_t)mask << _shift;
                _whole ^= fullMask;
                return *this;
            }
            HalfRef& operator|=(uint8_t mask)
            {
                uint16_t fullMask = (uint16_t)mask << _shift;
                _whole |= fullMask;
                return *this;
            }
            HalfRef& operator&=(uint8_t mask)
            {
                uint8_t current = static_cast<uint8_t>((_whole >> _shift) & 0xffu);
                current &= mask;
                *this = current; 
                return *this;
            }
            HalfRef& operator+=(uint8_t val)
            {
                uint8_t current = static_cast<uint8_t>((_whole >> _shift) & 0xffu);
                current += val;
                *this = current; 
                return *this;
            }
            HalfRef& operator-=(uint8_t val)
            {
                uint8_t current = static_cast<uint8_t>((_whole >> _shift) & 0xffu);
                current -= val;
                *this = current;
                return *this;
            }

        private:
            uint16_t& _whole;
            int _shift;
        
        };

        reg16(uint16_t init =0) : _value(init) {}

        HalfRef low()   {return HalfRef(_value, 0); }
        HalfRef high()  {return HalfRef(_value, 8); }

        uint16_t word() const   {return _value; }
        void word(uint16_t Val) {_value = Val; }

        reg16& operator=(uint16_t Val) {_value = Val; return *this; }
        operator uint16_t() const {return _value; }

        void chbit(int bit_index)
        {
            bit_index = std::min(15, std::max(0, bit_index));
            _value ^= (0b01u << bit_index);
        }
        bool ckbit(int bit_index) const
        {
            bit_index = std::min(15, std::max(0, bit_index));
            return ((_value >> bit_index) & 0b01u) ==1;
        }
        void stbit(int bit_index) 
        {
            bit_index = std::min(15, std::max(0, bit_index));
            _value |= (0b01u << bit_index);
        }
        void clbit(int bit_index) 
        {
            bit_index = std::min(15, std::max(0, bit_index));
            _value &= ~(0b01u << bit_index);
        }
        bool isParity() const
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
            bit_index = std::min(7, std::max(0, bit_index));
            _value ^= (0b01u << bit_index);
        }
        bool ckbitL(int bit_index) const
        {
            bit_index = std::min(7, std::max(0, bit_index));
            return ((_value >> bit_index) & 0b01u) ==1;
        }
        void stbitL(int bit_index) 
        {
            bit_index = std::min(7, std::max(0, bit_index));
            _value |= (0b01u << bit_index);
        }
        void clbitL(int bit_index)
        {
            bit_index = std::min(7, std::max(0, bit_index));
            _value &= ~(0b01u << bit_index);
        }

        // High halfReg
        void chbitH(int bit_index)    
        {
            bit_index = std::min(7, std::max(0, bit_index));
            _value ^= (0b01u << (bit_index + 8));
        }
        bool ckbitH(int bit_index) const
        {
            bit_index = std::min(7, std::max(0, bit_index));
            return ((_value >> (bit_index + 8)) & 0b01u) ==1;
        }
        void stbitH(int bit_index) 
        {
            bit_index = std::min(7, std::max(0, bit_index));
            _value |= (0b01u << (bit_index + 8));
        }
        void clbitH(int bit_index)
        {
            bit_index = std::min(7, std::max(0, bit_index));
            _value &= ~(0b01u << (bit_index + 8));
        }

    };    

    class regF32
    {
    private:
        float _value;

    public:
        regF32(float init =0) : _value(init) {}

        regF32& operator=(float Val) {_value = Val; return *this; }
        operator float() const {return _value; }
        float word() const  {return _value; }
        void word(float Val) {_value = Val; }

    };

};








