//
//  Z80.hpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef Z80_hpp
#define Z80_hpp

#include "MemoryMap.hpp"

template <class int_type> class Register
{
public:
    explicit Register(std::string name_str):
        m_value(0), logging(false)
    {
        strncpy(name, name_str.c_str(), 3);
    }
    
    Register(int_type value, std::string name_str):
        m_value(value), logging(false)
    {
        strncpy(name, name_str.c_str(), 3);
    }
    
    int_type read() const {return m_value;}
    virtual void write(int_type val)
    {
        m_value = val;
        
        if (logging)
        {
            printf("Reg: %s new value: 0x%x\n", name, val);
        }
    }
    
    void inc(int_type val)
    {
        m_value+=val;
        
        if (logging)
        {
            printf("Reg: %s inc by %d to: 0x%x\n", name, val, m_value);
        }
    }
    
    void dec(int_type val)
    {
        m_value-=val;
        
        if (logging)
        {
            printf("Reg: %s dec by %d to: 0x%x\n", name, val, m_value);
        }
    }
    
    char name[3];
    bool logging;
    
protected:
    int_type m_value;
};

class FlagRegister: public Register<uint8_t>
{
public:
    explicit FlagRegister(std::string name):
        Register(name)
    {}
    
    void write(uint8_t val)
    {
        //You can't write the bottom 4 bits of the flag register!
        m_value = val & 0xf0;
        
        if (logging)
        {
            printf("Reg: %s new value: 0x%x\n", name, val);
        }
    }
    
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

enum InterruptSignal { LCD_VBLANK, LCD_STAT, TIMER_OVERFLOW, END_SERIAL, PIN };

class Z80
{
public:
    explicit Z80(MemoryMap& mem):
        pc(0, "pc"),
        sp(0xFFFE, "sp"),
        mem(mem),
        a("a"),
        b("b"),
        c("c"),
        d("d"),
        e("e"),
        f("f"),
        h("h"),
        l("l"),
        m_total_cycles(0),
        interrupt_enable(false),
        halted(false),
        stopped(false),
        m_interrupt_addrs{0x0040, 0x0048, 0x0050, 0x0058, 0x0060}
    {}
    
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
    
    MemoryMap& mem;
    
    uint8_t fetch_byte();
    uint16_t fetch_short();
    
    uint16_t get_af() const { return get_pair(a, f); }
    uint16_t get_bc() const { return get_pair(b, c); }
    uint16_t get_de() const { return get_pair(d, e); }
    uint16_t get_hl() const { return get_pair(h, l); }
    
    void set_af(uint16_t value) { return set_pair(a, f, value); }
    void set_bc(uint16_t value) { return set_pair(b, c, value); }
    void set_de(uint16_t value) { return set_pair(d, e, value); }
    void set_hl(uint16_t value) { return set_pair(h, l, value); }
    
    std::string status_string();
    
    void tick(uint8_t cycles);
    
    void post_interrupt(uint8_t num);
    void skip_bootstrap();
    
    bool interrupt_enable;
    bool halted;
    bool stopped;
    
    size_t m_total_cycles;
    
private:
    std::array<uint16_t, 5> m_interrupt_addrs;
    
    uint16_t get_pair(Register<uint8_t> high, Register<uint8_t> low) const
    {
        return (high.read() << 8) | low.read();
    }
    
    void set_pair(Register<uint8_t>& high, Register<uint8_t>& low, uint16_t value)
    {
        high.write(value >> 8);
        low.write(uint8_t(value));
    }
};

#endif /* Z80_hpp */
