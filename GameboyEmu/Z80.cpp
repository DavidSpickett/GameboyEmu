//
//  Z80.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "Z80.hpp"
#include <string>

std::string FlagRegister::to_string()
{
    return formatted_string("z:%d n:%d h:%d c:%d\n", get_z(), get_n(), get_h(), get_c());
}

std::string Z80::status_string()
{
    std::string pc_ = formatted_string("PC: 0x%04x\n", pc.read());
    std::string sp_ = formatted_string("SP: 0x%04x\n", sp.read());
    
    std::string _1 = formatted_string("a: 0x%02x f: 0x%02x\n", a.read(), f.read());
    std::string _2 = formatted_string("b: 0x%02x c: 0x%02x\n", b.read(), c.read());
    std::string _3 = formatted_string("d: 0x%02x e: 0x%02x\n", d.read(), e.read());
    std::string _4 = formatted_string("h: 0x%02x l: 0x%02x\n", h.read(), l.read());
    
    //fancy formatter for flag reg bits
    
    return pc_+f.to_string()+sp_+_1+_2+_3+_4;
}

void Z80::skip_bootstrap()
{
    //Start of cartridge
    pc.write(0x100);
    //Setup stack
    sp.write(0xfffe);
    //Turn of bootstrap program
    mem.write8(0xff50, 0x1);
    //Turn on LCD
    mem.write8(0xff40, 0x91);
}

void Z80::post_interrupt(uint8_t num)
{
    if (num > 5)
    {
        throw std::runtime_error("Can't raise interrupt > 5!");
    }
    
    //We leave halted even if we don't jump to the addr.
    //May also wait some number of cycles before doing so, not sure.
    halted = false;
    
    if (interrupt_enable)
    {
        bool enabled = mem.read8(INTERRUPT_SWITCH) & (1<<num);
        if (enabled)
        {
            //Save PC to stack
            sp.dec(2);
            mem.write16(sp.read(), pc.read());
            //Then jump to new addr
            pc.write(m_interrupt_addrs[num]);
            
            //Set occurred bit
            mem.write8(INTERRUPT_FLAGS, mem.read8(INTERRUPT_FLAGS) | (1 << num));
            
            /*Disable ints until the program enables them again (and make sure
             we don't push to the stack twice in one step.*/
            interrupt_enable = false;
        }
    }
    
}

void Z80::tick(uint8_t cycles)
{
    m_total_cycles += cycles;
    mem.tick(m_total_cycles);
}

uint8_t Z80::fetch_byte()
{
    uint8_t ret = mem.read8(pc.read());
    pc.inc(1);
    return ret;
}

uint16_t Z80::fetch_short()
{
    auto b1 = fetch_byte();
    auto b2 = fetch_byte();
    return (b2 << 8) | b1;
}
