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

uint8_t Z80::fetch_byte()
{
    uint8_t ret = mem.read8(pc.read());
    pc.inc(1);
    return ret;
}

uint16_t Z80::fetch_short()
{
    std::vector<uint8_t> bs = fetch_bytes(2);
    return (bs[1] << 8) | bs[0];
}

std::vector<uint8_t> Z80::fetch_bytes(uint16_t num)
{
    std::vector<uint8_t> ret = mem.read_bytes(pc.read(), num);
    pc.inc(num);
    return ret;
}
