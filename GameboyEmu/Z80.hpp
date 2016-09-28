//
//  Z80.hpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef Z80_hpp
#define Z80_hpp

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include "MemoryMap.hpp"

template <class int_type> class Register
{
public:
    Register(std::string name):
        m_value(0), name(name), logging(false)
    {}
    
    Register(int_type value, std::string name):
        m_value(value), name(name), logging(false)
    {}
    
    int_type read() {return m_value;}
    void write(int_type val)
    {
        m_value = val;
        
        if (logging)
        {
            printf("Reg: %s new value: 0x%x\n", name.c_str(), val);
        }
    }
    
    void inc(int_type val)
    {
        m_value+=val;
        
        if (logging)
        {
            printf("Reg: %s inc by %d to: 0x%x\n", name.c_str(), val, m_value);
        }
    }
    
    void dec(int_type val)
    {
        m_value-=val;
        
        if (logging)
        {
            printf("Reg: %s dec by %d to: 0x%x\n", name.c_str(), val, m_value);
        }
    }
    
    const std::string name;
    bool logging;
    
protected:
    int_type m_value;
};

class FlagRegister: public Register<uint8_t>
{
public:
    FlagRegister(std::string name):
        Register(name)
    {}
    
    bool get_z() { return get_bit(7); }
    bool get_n() { return get_bit(6); }
    bool get_h() { return get_bit(5); }
    bool get_c() { return get_bit(4); }
    
    void set_z(bool val) { return set_bit(7, val); }
    void set_n(bool val) { return set_bit(6, val); }
    void set_h(bool val) { return set_bit(5, val); }
    void set_c(bool val) { return set_bit(4, val); }
    
    std::string to_string();
    
private:
    bool get_bit(uint8_t bit) { return m_value & (1<<bit); }
    void set_bit(uint8_t bit, bool val) { m_value &= ~(1<<bit); m_value |= uint8_t(val) << bit; }
};

class Z80
{
public:
    Z80():
        pc(0, "pc"),
        sp(0xFFFE, "sp"),
        mem(MemoryMap()),
        a("a"),
        b("b"),
        c("c"),
        d("d"),
        e("e"),
        f("f"),
        h("h"),
        l("l")
    {
    }
    
    Register <uint16_t> pc;
    Register <uint16_t> sp;
    FlagRegister f;
    
    Register <uint8_t> a;
    Register <uint8_t> b;
    Register <uint8_t> c;
    Register <uint8_t> d;
    Register <uint8_t> e;
    Register <uint8_t> h;
    Register <uint8_t> l;
    
    MemoryMap mem;
    
    uint8_t fetch_byte();
    uint16_t fetch_short();
    std::vector<uint8_t> fetch_bytes(uint16_t num);
    
    uint16_t get_af() { return get_pair(a, f); }
    uint16_t get_bc() { return get_pair(b, c); }
    uint16_t get_de() { return get_pair(d, e); }
    uint16_t get_hl() { return get_pair(h, l); }
    
    void set_af(uint16_t value) { return set_pair(a, f, value); }
    void set_bc(uint16_t value) { return set_pair(b, c, value); }
    void set_de(uint16_t value) { return set_pair(d, e, value); }
    void set_hl(uint16_t value) { return set_pair(h, l, value); }
    
    std::string status_string();
    
private:
    uint16_t get_pair(Register<uint8_t> high, Register<uint8_t> low)
    {
        return (high.read() << 8) | low.read();
    }
    
    void set_pair(Register<uint8_t>& high, Register<uint8_t>& low, uint16_t value)
    {
        high.write(value >> 8);
        low.write(value & 0xff);
    }
};

#endif /* Z80_hpp */
