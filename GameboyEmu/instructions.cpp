//
//  instructions.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "instructions.hpp"
#include <iostream>
#include "utils.hpp"

#define DEBUG_INSTR 0

namespace
{
    template< typename... Args >
    void debug_print(const char* format, Args... args)
    {
#if DEBUG_INSTR
        printf(format, args...);
#endif
    }
}

namespace
{
    void generic_add_a_n(Z80& proc, uint8_t value)
    {
        uint8_t original_value = proc.a.read();
        uint8_t new_value = original_value + value;
        proc.a.write(new_value);
        
        proc.f.set_z(new_value==0);
        proc.f.set_n(false);
        proc.f.set_h(((original_value&0xf) + (value&0xf)) & 0x10);
        proc.f.set_c((uint16_t(original_value)+uint16_t(value)) > 0xff);
    }
}

uint8_t add_a_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x87:
            reg = &proc.a;
            break;
        case 0x80:
            reg = &proc.b;
            break;
        case 0x81:
            reg = &proc.c;
            break;
        case 0x82:
            reg = &proc.d;
            break;
        case 0x83:
            reg = &proc.e;
            break;
        case 0x84:
            reg = &proc.h;
            break;
        case 0x85:
            reg = &proc.l;
            break;
        //Use (hl) as addr
        case 0x86:
        {
            uint16_t addr = proc.get_hl();
            generic_add_a_n(proc, proc.mem.read8(addr));
            
            debug_print("%s", "add a, (hl)\n");
            return 8;
        }
        //Use next byte as value
        case 0xC6:
        {
            uint8_t value = proc.fetch_byte();
            generic_add_a_n(proc, value);
            
            debug_print("add a, 0x%02x\n", value);
            return 8;
        }
    }
    
    generic_add_a_n(proc, reg->read());
    
    debug_print("add a, %s\n", reg->name.c_str());
    return 4;
}

namespace
{
    void generic_sub_n(Z80& proc, uint8_t value)
    {
        uint8_t orig_val = proc.a.read();
        uint8_t new_value = orig_val - value;
        
        proc.f.set_z(new_value==0);
        proc.f.set_n(true);
        proc.f.set_h((orig_val & 0xF) < (value & 0xF));
        //I think...acts as an underflow flag?
        proc.f.set_c(orig_val<value);
        
        proc.a.write(new_value);
    }
}

uint8_t sub_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x97:
            reg = &proc.a;
            break;
        case 0x90:
            reg = &proc.b;
            break;
        case 0x91:
            reg = &proc.c;
            break;
        case 0x92:
            reg = &proc.d;
            break;
        case 0x93:
            reg = &proc.e;
            break;
        case 0x94:
            reg = &proc.h;
            break;
        case 0x95:
            reg = &proc.l;
            break;
        //(hl) as an addr
        case 0x96:
        {
            uint16_t addr = proc.get_hl();
            uint8_t value = proc.mem.read8(addr);
            generic_sub_n(proc, value);
            
            debug_print("%s", "sub (hl)\n");
            return 8;
        }
        //Next byte is value
        case 0xd6:
        {
            uint8_t value = proc.fetch_byte();
            generic_sub_n(proc, value);
            
            debug_print("sub 0x%02x\n", value);
            return 8;
        }
    }
    
    generic_sub_n(proc, reg->read());
    
    debug_print("sub %s\n", reg->name.c_str());
    return 4;
}

uint8_t ldh_a_n(Z80& proc)
{
    uint8_t offs = proc.fetch_byte();
    uint16_t addr = 0xff00 + offs;
    proc.a.write(proc.mem.read8(addr));
    
    debug_print("ldh a, (0xff00+0x%02x)\n", offs);
    return 12;
}

uint8_t jr_n(Z80& proc)
{
    int8_t offs = proc.fetch_byte();
    
    //We jump from the address of the next instr, not the jr itself.
    proc.pc.write(proc.pc.read()+offs);
    
    debug_print("jr 0x%02x (%d)\n", uint8_t(offs), offs);
    return 8;
}

namespace
{
    void generic_cp_n(Z80& proc, uint8_t value)
    {
        uint8_t a = proc.a.read();
        uint8_t res = value - a;
        
        proc.f.set_z(res==0);
        proc.f.set_n(true);
        proc.f.set_h((a & 0xF) > (value & 0xF));
        proc.f.set_c(a<value);
    }
}

uint8_t cp_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0xbf:
            reg = &proc.a;
            break;
        case 0xb8:
            reg = &proc.b;
            break;
        case 0xb9:
            reg = &proc.c;
            break;
        case 0xba:
            reg = &proc.d;
            break;
        case 0xbb:
            reg = &proc.e;
            break;
        case 0xbc:
            reg = &proc.h;
            break;
        case 0xbd:
            reg = &proc.l;
            break;
        //Use (hl) as addr
        case 0xbe:
        {
            generic_cp_n(proc, proc.mem.read8(proc.get_hl()));
            debug_print("%s\n", "cp (hl)");
            return 8;
        }
        //Compare with immediate byte
        case 0xfe:
        {
            uint8_t value = proc.fetch_byte();
            generic_cp_n(proc, value);
            debug_print("cp 0x%02x\n", value);
            return 8;
        }
    }
    
    generic_cp_n(proc, reg->read());
    debug_print("cp %s\n", reg->name.c_str());
    
    return 4;
}

uint8_t ret(Z80& proc)
{
    //Get addr from stack
    uint16_t new_addr = proc.mem.read16(proc.sp.read());
    proc.sp.inc(2);
    proc.pc.write(new_addr);
    
    debug_print("%s", "ret\n");
    proc.add_ret(new_addr, false);
    return 8;
}

uint8_t inc_nn(Z80& proc, uint8_t b1)
{
    std::string pair = "?";
    
    switch (b1)
    {
        case 0x03:
            proc.set_bc(proc.get_bc()+1);
            pair = "bc";
            break;
        case 0x13:
            proc.set_de(proc.get_de()+1);
            pair = "de";
            break;
        case 0x23:
            proc.set_hl(proc.get_hl()+1);
            pair = "hl";
            break;
        case 0x33:
            proc.sp.inc(1);
            pair = "sp";
            break;
    }

    debug_print("inc %s\n", pair.c_str());
    return 8;
}

uint8_t ld_hl_plus_a(Z80& proc)
{
    //Write A to addr (hl)
    uint16_t hl = proc.get_hl();
    proc.mem.write8(hl, proc.a.read());
    //inc (hl)
    proc.set_hl(hl+1);
    
    debug_print("%s", "ld (hl+), a\n");
    return 8;
}

namespace
{
    uint8_t generic_dec_n(Z80& proc, uint8_t value)
    {
        uint8_t new_value = value-1;
        
        proc.f.set_z(new_value==0);
        proc.f.set_n(true);
        proc.f.set_h((value & 0x1f)==0x10);
        
        return new_value;
    }
}

uint8_t dec_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x3d:
            reg = &proc.a;
            break;
        case 0x05:
            reg = &proc.b;
            break;
        case 0x0d:
            reg = &proc.c;
            break;
        case 0x15:
            reg = &proc.d;
            break;
        case 0x1d:
            reg = &proc.e;
            break;
        case 0x25:
            reg = &proc.h;
            break;
        case 0x2d:
            reg = &proc.l;
            break;
        //Use (hl) as address of value
        case 0x35:
        {
            uint16_t addr = proc.get_hl();
            uint8_t new_val = generic_dec_n(proc, proc.mem.read8(addr));
            proc.mem.write8(addr, new_val);
            
            debug_print("%s", "dec (hl)\n");
            return 12;
        }
    }
    
    uint8_t new_val = generic_dec_n(proc, reg->read());
    reg->write(new_val);
    
    debug_print("dec %s\n", reg->name.c_str());
    return 4;
}

uint8_t pop_nn(Z80& proc, uint8_t b1)
{
    std::string pair = "?";
    uint16_t value = proc.mem.read16(proc.sp.read());
    proc.sp.inc(2);
    
    switch (b1)
    {
        case 0xf1:
            proc.set_af(value);
            pair = "af";
            break;
        case 0xc1:
            proc.set_bc(value);
            pair = "bc";
            break;
        case 0xd1:
            proc.set_de(value);
            pair = "de";
            break;
        case 0xe1:
            proc.set_hl(value);
            pair = "hl";
            break;
    }
    
    debug_print("pop (%s)\n", pair.c_str());
    return 12;
}

namespace {
    uint8_t generic_rl_n(Z80& proc, uint8_t value)
    {
        //Rotate, apply flags and return new value
        
        uint8_t new_val = value << 1;
        //Carry rotates into the bottom bit
        new_val |= proc.f.get_c();
        
        //Top bit of original goes into the carry
        proc.f.set_c(value >> 7);
        proc.f.set_z(new_val==0);
        proc.f.set_n(false);
        proc.f.set_h(false);
        
        return new_val;
    }
}

uint8_t rla(Z80& proc)
{
    uint8_t new_val = generic_rl_n(proc, proc.a.read());
    proc.a.write(new_val);
    
    debug_print("%s", "rla\n");
    return 4;
}

uint8_t push_nn(Z80& proc, uint8_t b1)
{
    uint16_t value=0;
    std::string pair = "?";
    
    switch(b1)
    {
        case 0xf5:
            value = proc.get_af();
            pair = "(af)";
            break;
        case 0xc5:
            value = proc.get_bc();
            pair = "(bc)";
            break;
        case 0xd5:
            value = proc.get_de();
            pair = "(de)";
            break;
        case 0xe5:
            value = proc.get_hl();
            pair = "(hl)";
            break;
    }
    
    //Decrement SP !!BEFORE!! we write the value.
    proc.sp.dec(2);
    proc.mem.write16(proc.sp.read(), value);
    
    debug_print("push %s\n", pair.c_str());
    return 16;
}

uint8_t call_nn(Z80& proc)
{
    //Equivalent of push pc, jump addr
    uint16_t j_addr = proc.fetch_short();
    
    //Decrement stack pointer !!BEFORE!! writing new value
    proc.sp.dec(2);
    proc.mem.write16(proc.sp.read(), proc.pc.read());
    
    uint16_t old_pc = proc.pc.read();
    proc.pc.write(j_addr);
    
    debug_print("call 0x%02x\n", j_addr);
    proc.add_call(old_pc, j_addr, proc.sp.read());
    return 12;
}

uint8_t ld_offs_n_a(Z80& proc)
{
    uint16_t addr = 0xff00 + proc.fetch_byte();
    proc.mem.write8(addr, proc.a.read());
    
    debug_print("ldh (0x%02x), a\n", addr);
    return 12;
}

uint8_t ld_n_a(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x7f:
            reg = &proc.a;
            break;
        case 0x47:
            reg = &proc.b;
            break;
        case 0x4f:
            reg = &proc.c;
            break;
        case 0x57:
            reg = &proc.d;
            break;
        case 0x5f:
            reg = &proc.e;
            break;
        case 0x67:
            reg = &proc.h;
            break;
        case 0x6f:
            reg = &proc.l;
            break;
        case 0x02:
        case 0x12:
        case 0x77:
        {
            //Use pair as memory address to write to
            uint16_t addr = 0;
            std::string r_name = "?";
            
            switch (b1)
            {
                case 0x02:
                    addr = proc.get_bc();
                    r_name = "(bc)";
                    break;
                case 0x12:
                    addr = proc.get_de();
                    r_name = "(de)";
                    break;
                case 0x77:
                    addr = proc.get_hl();
                    r_name = "(hl)";
                    break;
            }
            
            proc.mem.write8(addr, proc.a.read());
            
            debug_print("ld %s, a\n", r_name.c_str());
            return 8;
        }
        case 0xea:
        {
            //Use next 2 bytes as address to store to
            uint16_t addr = proc.fetch_short();
            proc.mem.write8(addr, proc.a.read());
            
            debug_print("ld (0x%02x), a\n", addr);
            return 16;
        }
    }
    
    reg->write(proc.a.read());
    
    debug_print("ld %s, a\n", reg->name.c_str());
    return 4;
}
                     
namespace
{
    uint8_t generic_inc_n(Z80& proc, uint8_t value)
    {
        uint8_t new_val = value+1;
        
        proc.f.set_z(new_val==0);
        proc.f.set_n(false);
        //Check for half carry
        //Because we only ever add one, the only time a carry happens is from 0xf to 0x10
        proc.f.set_h((value & 0xf) == 0xf);
        
        return new_val;
    }
}

uint8_t inc_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x3c:
            reg = &proc.a;
            break;
        case 0x04:
            reg = &proc.b;
            break;
        case 0x0c:
            reg = &proc.c;
            break;
        case 0x14:
            reg = &proc.d;
            break;
        case 0x1c:
            reg = &proc.e;
            break;
        case 0x24:
            reg = &proc.h;
            break;
        case 0x2c:
            reg = &proc.l;
            break;
        //Inc memory at address (hl)
        case 0x34:
        {
            uint16_t addr = proc.get_hl();
            uint8_t orig_val = proc.mem.read8(addr);
            
            uint8_t new_val = generic_inc_n(proc, orig_val);
            proc.mem.write8(addr, new_val);
            
            debug_print("%s", "inc (hl)\n");
            
            return 12;
        }
    }
    
    uint8_t orig_val = reg->read();
    uint8_t new_val = generic_inc_n(proc, orig_val);
    reg->write(new_val);
    
    debug_print("inc %s\n", reg->name.c_str());
    return 4;
}

uint8_t ld_offs_c_a(Z80& proc)
{
    uint16_t addr = 0xff00 + proc.c.read();
    proc.mem.write8(addr, proc.a.read());
    
    debug_print("%s", "ld (c), a\n");
    return 8;
}

uint8_t ld_a_n(Z80& proc, uint8_t b1)
{
    uint8_t cycles = 4;
    std::string reg_name;
    uint8_t temp8 = 0;
    
    switch (b1)
    {
        case 0x7f:
            temp8 = proc.a.read();
            reg_name = proc.a.name;
            break;
        case 0x78:
            temp8 = proc.b.read();
            reg_name = proc.b.name;
            break;
        case 0x79:
            temp8 = proc.c.read();
            reg_name = proc.c.name;
            break;
        case 0x7a:
            temp8 = proc.d.read();
            reg_name = proc.d.name;
            break;
        case 0x7b:
            temp8 = proc.e.read();
            reg_name = proc.e.name;
            break;
        case 0x7c:
            temp8 = proc.h.read();
            reg_name = proc.h.name;
            break;
        case 0x7d:
            temp8 = proc.l.read();
            reg_name = proc.l.name;
            break;
        case 0x0a:
        case 0x1a:
        case 0x7e:
        {
            //Use register pairs as mem address
            uint16_t addr = 0;
            switch (b1)
            {
                case 0xa:
                    addr = proc.get_bc();
                    reg_name = "(bc)";
                    break;
                case 0x1a:
                    addr = proc.get_de();
                    reg_name = "(de)";
                    break;
                case 0x7e:
                    addr = proc.get_hl();
                    reg_name = "(hl)";
                    break;
            }
            
            temp8 = proc.mem.read8(addr);
            break;
        }
        case 0xfa:
        {
            //Use next two instr bytes as address
            uint16_t addr = proc.fetch_short();
            temp8 = proc.mem.read8(addr);
            
            reg_name = formatted_string("(0x%02x)", addr);
            cycles = 16;
            
            break;
        }
        case 0x3e:
            //Load next instr byte into A
            temp8 = proc.fetch_byte();
            reg_name = formatted_string("0x%02x", temp8);
            break;
    }
    
    proc.a.write(temp8);
    
    debug_print("ld a, %s\n", reg_name.c_str());
    
    return cycles;
}

uint8_t jr_cc_n(Z80& proc, uint8_t b1)
{
    uint8_t cycles = 8;
    int8_t offset = proc.fetch_byte();
    bool jump = false;
    std::string type = "?";
    
    switch (b1)
    {
        case 0x20:
            jump = !proc.f.get_z();
            type = "nz";
            break;
        case 0x28:
            jump = proc.f.get_z();
            type = "z";
            break;
        case 0x30:
            jump = !proc.f.get_c();
            type = "nc";
            break;
        case 0x38:
            jump = proc.f.get_c();
            type = "c";
            break;
    }
    
    //Alway calculate it so we can show it in the dasm
    uint16_t new_pc = proc.pc.read();
    
    new_pc += offset;
    
    if (jump)
    {
        cycles = 12;
        proc.pc.write(new_pc);
    }
    
    debug_print("jr %s, %d (0x%04x)\n", type.c_str(), offset, new_pc);

    return cycles;
}

uint8_t rl_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x17:
            reg = &proc.a;
            break;
        case 0x10:
            reg = &proc.b;
            break;
        case 0x11:
            reg = &proc.c;
            break;
        case 0x12:
            reg = &proc.d;
            break;
        case 0x13:
            reg = &proc.e;
            break;
        case 0x14:
            reg = &proc.h;
            break;
        case 0x15:
            reg = &proc.l;
            break;
        //Use (hl) as address to rotate
        case 0x16:
        {
            uint16_t addr = proc.get_hl();
            uint8_t new_val = generic_rl_n(proc, proc.mem.read8(addr));
            proc.mem.write8(addr, new_val);
            
            debug_print("%s", "rl (hl)\n");
            return 16;
        }
    }
    
    uint8_t new_val = generic_rl_n(proc, reg->read());
    reg->write(new_val);
    
    debug_print("rl %s\n", reg->name.c_str());
    return 8;
}

uint8_t bit_b_hl(Z80& proc, uint8_t b1)
{
    uint16_t addr = proc.get_hl();
    uint8_t to_test = proc.mem.read8(addr);
    uint8_t bit;
    
    switch (b1)
    {
            //(HL) variants
        case 0x46:
            bit = 0;
            break;
        case 0x4e:
            bit = 1;
            break;
        case 0x56:
            bit = 2;
            break;
        case 0x5e:
            bit = 3;
            break;
        case 0x66:
            bit = 4;
            break;
        case 0x6e:
            bit = 5;
            break;
        case 0x76:
            bit = 6;
            break;
        case 0x7e:
            bit = 7;
            break;
        default:
            throw std::runtime_error(formatted_string("Unknown byte for bit r, (hl): 0x%02x", b1));
    }
    
    bool bit_set = to_test & (1<<bit);
    proc.f.set_z(!bit_set);
    proc.f.set_n(false);
    proc.f.set_h(true);
    
    debug_print("bit %d, (hl) (0x%04x, 0x%02x)\n", bit, addr, to_test);
    
    return 12;
}

uint8_t bit_b_r(Z80& proc, uint8_t b1)
{
    uint8_t cycles = 8;
    uint8_t bit;
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x40:
            reg = &proc.b;
            bit = 0;
            break;
        case 0x41:
            reg = &proc.c;
            bit = 0;
            break;
        case 0x42:
            reg = &proc.d;
            bit = 0;
            break;
        case 0x43:
            reg = &proc.e;
            bit = 0;
            break;
        case 0x44:
            reg = &proc.h;
            bit = 0;
            break;
        case 0x45:
            reg = &proc.l;
            bit = 0;
            break;
        case 0x47:
            reg = &proc.a;
            bit = 0;
            break;
            
        case 0x48:
            reg = &proc.b;
            bit = 1;
            break;
        case 0x49:
            reg = &proc.c;
            bit = 1;
            break;
        case 0x4a:
            reg = &proc.d;
            bit = 1;
            break;
        case 0x4b:
            reg = &proc.e;
            bit = 1;
            break;
        case 0x4c:
            reg = &proc.h;
            bit = 1;
            break;
        case 0x4d:
            reg = &proc.l;
            bit = 1;
            break;
        case 0x4f:
            reg = &proc.a;
            bit = 1;
            break;
            
        case 0x50:
            reg = &proc.b;
            bit = 2;
            break;
        case 0x51:
            reg = &proc.c;
            bit = 2;
            break;
        case 0x52:
            reg = &proc.d;
            bit = 2;
            break;
        case 0x53:
            reg = &proc.e;
            bit = 2;
            break;
        case 0x54:
            reg = &proc.h;
            bit = 2;
            break;
        case 0x55:
            reg = &proc.l;
            bit = 2;
            break;
        case 0x57:
            reg = &proc.a;
            bit = 2;
            break;
            
        case 0x58:
            reg = &proc.b;
            bit = 3;
            break;
        case 0x59:
            reg = &proc.c;
            bit = 3;
            break;
        case 0x5a:
            reg = &proc.d;
            bit = 3;
            break;
        case 0x5b:
            reg = &proc.e;
            bit = 3;
            break;
        case 0x5c:
            reg = &proc.h;
            bit = 3;
            break;
        case 0x5d:
            reg = &proc.l;
            bit = 3;
            break;
        case 0x5f:
            reg = &proc.a;
            bit = 3;
            break;
            
        case 0x60:
            reg = &proc.b;
            bit = 4;
            break;
        case 0x61:
            reg = &proc.c;
            bit = 4;
            break;
        case 0x62:
            reg = &proc.d;
            bit = 4;
            break;
        case 0x63:
            reg = &proc.e;
            bit = 4;
            break;
        case 0x64:
            reg = &proc.h;
            bit = 4;
            break;
        case 0x65:
            reg = &proc.l;
            bit = 4;
            break;
        case 0x67:
            reg = &proc.a;
            bit = 4;
            break;
            
        case 0x68:
            reg = &proc.b;
            bit = 5;
            break;
        case 0x69:
            reg = &proc.c;
            bit = 5;
            break;
        case 0x6a:
            reg = &proc.d;
            bit = 5;
            break;
        case 0x6b:
            reg = &proc.e;
            bit = 5;
            break;
        case 0x6c:
            reg = &proc.h;
            bit = 5;
            break;
        case 0x6d:
            reg = &proc.l;
            bit = 5;
            break;
        case 0x6f:
            reg = &proc.a;
            bit = 5;
            break;
            
        case 0x70:
            reg = &proc.b;
            bit = 6;
            break;
        case 0x71:
            reg = &proc.c;
            bit = 6;
            break;
        case 0x72:
            reg = &proc.d;
            bit = 6;
            break;
        case 0x73:
            reg = &proc.e;
            bit = 6;
            break;
        case 0x74:
            reg = &proc.h;
            bit = 6;
            break;
        case 0x75:
            reg = &proc.l;
            bit = 6;
            break;
        case 0x77:
            reg = &proc.a;
            bit = 6;
            break;
            
        case 0x78:
            reg = &proc.b;
            bit = 7;
            break;
        case 0x79:
            reg = &proc.c;
            bit = 7;
            break;
        case 0x7a:
            reg = &proc.d;
            bit = 7;
            break;
        case 0x7b:
            reg = &proc.e;
            bit = 7;
            break;
        case 0x7c:
            reg = &proc.h;
            bit = 7;
            break;
        case 0x7d:
            reg = &proc.l;
            bit = 7;
            break;
        case 0x7f:
            reg = &proc.a;
            bit = 7;
            break;
        //(HL) variants
        case 0x46:
        case 0x4e:
        case 0x56:
        case 0x5e:
        case 0x66:
        case 0x6e:
        case 0x76:
        case 0x7e:
            return bit_b_hl(proc, b1);
        default:
            throw std::runtime_error(
                formatted_string("Unknown byte for bit b,r instruction: 0x%02x",
                b1));
    }
    
    uint8_t to_test = reg->read();
    bool bit_set = to_test & (1<<bit);
    proc.f.set_z(!bit_set);
    proc.f.set_n(false);
    proc.f.set_h(true);
    
    debug_print("bit %d, %s\n", bit, reg->name.c_str());
    
    return cycles;
}

uint8_t xor_n(Z80& proc, uint8_t b1)
{
    uint8_t cycles;
    std::string prt = "xor %s";
    
    proc.f.set_n(false);
    proc.f.set_h(false);
    proc.f.set_c(false);
    
    uint8_t temp8;
    
    switch (b1)
    {
        case 0xAF:
            prt = formatted_string(prt.c_str(), "a");
            temp8 = proc.a.read();
            cycles = 4;
            break;
        case 0xA8:
            prt = formatted_string(prt.c_str(), "b");
            temp8 = proc.b.read();
            cycles = 4;
            break;
        case 0xA9:
            prt = formatted_string(prt.c_str(), "c");
            temp8 = proc.c.read();
            cycles = 4;
            break;
        case 0xAA:
            prt = formatted_string(prt.c_str(), "d");
            temp8 = proc.d.read();
            cycles = 4;
            break;
        case 0xAB:
            prt = formatted_string(prt.c_str(), "e");
            temp8 = proc.e.read();
            cycles = 4;
            break;
        case 0xAC:
            prt = formatted_string(prt.c_str(), "h");
            temp8 = proc.h.read();
            cycles = 4;
            break;
        case 0xAD:
            prt = formatted_string(prt.c_str(), "l");
            temp8 = proc.l.read();
            cycles = 4;
            break;
        case 0xAE:
            //Use value of HL as an address
            prt = formatted_string(prt.c_str(), "(hl)");
            temp8 = proc.mem.read8(proc.get_hl());
            prt += formatted_string(" (0x%02x)", temp8);
            cycles = 8;
            break;
        case 0xEE:
            temp8 = proc.fetch_byte();
            prt = formatted_string("xor 0x%02x", temp8);
            cycles = 8;
            break;
        default:
            throw std::runtime_error(formatted_string("Unknown byte 0x%02x for opcode xor_n", b1));
    }
    
    temp8 ^= proc.a.read();
    proc.f.set_z(temp8==0);
    proc.a.write(temp8);
    
    debug_print("%s\n", prt.c_str());

    return cycles;
}

uint8_t ld_nn_n(Z80& proc, uint8_t b1)
{
    uint8_t b2 = proc.fetch_byte();
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x06:
            reg = &proc.b;
            break;
        case 0x0E:
            reg = &proc.c;
            break;
        case 0x16:
            reg = &proc.d;
            break;
        case 0x1E:
            reg = &proc.e;
            break;
        case 0x26:
            reg = &proc.h;
            break;
        case 0x2E:
            reg = &proc.l;
            break;
        default:
            throw std::runtime_error(formatted_string("Unknown byte 0x%02x for opcode ld_nn_n", b1));
    }
    
    reg->write(b2);
    
    debug_print("ld %s, 0x%02x\n", reg->name.c_str(), b2);
    return 8;
}

uint8_t ld_n_nn(Z80& proc, uint8_t b1)
{
    uint16_t imm = proc.fetch_short();
    
    std::string fmt = "ld %s, 0x%04x\n";
    std::string r = "?";
    
    switch (b1)
    {
        case 0x01:
            proc.set_bc(imm);
            r = "bc";
            break;
        case 0x11:
            proc.set_de(imm);
            r = "de";
            break;
        case 0x21:
            proc.set_hl(imm);
            r = "hl";
            break;
        case 0x31:
            proc.sp.write(imm);
            r = "sp";
            break;
        default:
            throw std::runtime_error(formatted_string("Unknown byte 0x%02x for opcode ld_n_nn", b1));
    }
    
    debug_print(fmt.c_str(), r.c_str(), imm);
    return 12;
}

uint8_t ld_hl_dec_a(Z80& proc, uint8_t b1)
{
    uint8_t temp8 = proc.a.read();
    uint16_t addr = proc.get_hl();
    proc.mem.write8(addr, temp8);
    
    debug_print("ld (hl-), a (0x%04x, 0x%02x)\n", addr, temp8);
    
    //Now decrement HL
    proc.set_hl(addr-1);
    return 8;
}

uint8_t nop()
{
    debug_print("%s\n", "nop");
    return 4;
}

uint8_t jp_nn(Z80& proc)
{
    uint16_t addr = proc.fetch_short();
    proc.pc.write(addr);
    
    debug_print("jp 0x%04x\n", addr);
    return 12;
}
uint8_t di(Z80& proc)
{
    proc.interrupt_enable = false;
    debug_print("%s\n", "di");
    return 4;
}

uint8_t ei(Z80& proc)
{
    proc.interrupt_enable = true;
    debug_print("%s\n", "ei");
    return 4;
}

uint8_t ld_r1_r2(Z80& proc, uint8_t b1)
{
    uint8_t cycles = 4;
    Register<uint8_t>* lhs = nullptr;
    Register<uint8_t>* rhs = nullptr;
    
    switch (b1)
    {
        //B
        case 0x40:
            lhs = &proc.b;
            rhs = &proc.b;
            break;
        case 0x41:
            lhs = &proc.b;
            rhs = &proc.c;
            break;
        case 0x42:
            lhs = &proc.b;
            rhs = &proc.d;
            break;
        case 0x43:
            lhs = &proc.b;
            rhs = &proc.e;
            break;
        case 0x44:
            lhs = &proc.b;
            rhs = &proc.h;
            break;
        case 0x45:
            lhs = &proc.b;
            rhs = &proc.l;
            break;
        
        //C
        case 0x48:
            lhs = &proc.c;
            rhs = &proc.b;
            break;
        case 0x49:
            lhs = &proc.c;
            rhs = &proc.c;
            break;
        case 0x4a:
            lhs = &proc.c;
            rhs = &proc.d;
            break;
        case 0x4b:
            lhs = &proc.c;
            rhs = &proc.e;
            break;
        case 0x4c:
            lhs = &proc.c;
            rhs = &proc.h;
            break;
        case 0x4d:
            lhs = &proc.c;
            rhs = &proc.l;
            break;
            
        //D
        case 0x50:
            lhs = &proc.d;
            rhs = &proc.b;
            break;
        case 0x51:
            lhs = &proc.d;
            rhs = &proc.c;
            break;
        case 0x52:
            lhs = &proc.d;
            rhs = &proc.d;
            break;
        case 0x53:
            lhs = &proc.d;
            rhs = &proc.e;
            break;
        case 0x54:
            lhs = &proc.d;
            rhs = &proc.h;
            break;
        case 0x55:
            lhs = &proc.d;
            rhs = &proc.l;
            break;
        
        //E
        case 0x58:
            lhs = &proc.e;
            rhs = &proc.b;
            break;
        case 0x59:
            lhs = &proc.e;
            rhs = &proc.c;
            break;
        case 0x5a:
            lhs = &proc.e;
            rhs = &proc.d;
            break;
        case 0x5b:
            lhs = &proc.e;
            rhs = &proc.e;
            break;
        case 0x5c:
            lhs = &proc.e;
            rhs = &proc.h;
            break;
        case 0x5d:
            lhs = &proc.e;
            rhs = &proc.l;
            break;
        
        //H
        case 0x60:
            lhs = &proc.h;
            rhs = &proc.b;
            break;
        case 0x61:
            lhs = &proc.h;
            rhs = &proc.c;
            break;
        case 0x62:
            lhs = &proc.h;
            rhs = &proc.d;
            break;
        case 0x63:
            lhs = &proc.h;
            rhs = &proc.e;
            break;
        case 0x64:
            lhs = &proc.h;
            rhs = &proc.h;
            break;
        case 0x65:
            lhs = &proc.h;
            rhs = &proc.l;
            break;
        
        //L
        case 0x68:
            lhs = &proc.l;
            rhs = &proc.b;
            break;
        case 0x69:
            lhs = &proc.l;
            rhs = &proc.c;
            break;
        case 0x6a:
            lhs = &proc.l;
            rhs = &proc.d;
            break;
        case 0x6b:
            lhs = &proc.l;
            rhs = &proc.e;
            break;
        case 0x6c:
            lhs = &proc.l;
            rhs = &proc.h;
            break;
        case 0x6d:
            lhs = &proc.l;
            rhs = &proc.l;
            break;
        
        //ld (hl), reg
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        {
            uint16_t addr = proc.get_hl();
            Register<uint8_t>* reg = nullptr;
            
            switch (b1)
            {
                case 0x70:
                    reg = &proc.b;
                    break;
                case 0x71:
                    reg = &proc.c;
                    break;
                case 0x72:
                    reg = &proc.d;
                    break;
                case 0x73:
                    reg = &proc.e;
                    break;
                case 0x74:
                    reg = &proc.h;
                    break;
                case 0x75:
                    reg = &proc.l;
                    break;
            }
            
            proc.mem.write8(addr, reg->read());
            
            debug_print("ld (hl), %s\n", reg->name.c_str());
            return 8;
        }
         
        //ld reg, (HL)
        case 0x46:
        case 0x4e:
        case 0x56:
        case 0x5e:
        case 0x66:
        case 0x6e:
        {
            uint8_t value = proc.mem.read8(proc.get_hl());
            Register<uint8_t>* reg = nullptr;
            
            switch (b1)
            {
                case 0x46:
                    reg = &proc.b;
                    break;
                case 0x4e:
                    reg = &proc.c;
                    break;
                case 0x56:
                    reg = &proc.d;
                    break;
                case 0x5e:
                    reg = &proc.e;
                    break;
                case 0x66:
                    reg = &proc.h;
                    break;
                case 0x6e:
                    reg = &proc.l;
                    break;
            }
            
            reg->write(value);
            
            debug_print("ld %s, (hl)\n", reg->name.c_str());
            return 8;
        }
        //ld (hl), n
        case 0x36:
        {
            uint16_t addr = proc.get_hl();
            uint8_t value = proc.fetch_byte();
            proc.mem.write8(addr, value);
            
            debug_print("ld (hl), 0x%02x\n", value);
            return 12;
        }
        default:
            throw std::runtime_error(formatted_string("Unknown ld r1, r2 opcode 0x%02x", b1));
    }
    
    lhs->write(rhs->read());
    debug_print("ld %s, %s\n", lhs->name.c_str(), rhs->name.c_str());
    
    return cycles;
}

uint8_t ld_a_hl_plus(Z80& proc)
{
    uint16_t addr = proc.get_hl();
    proc.a.write(proc.mem.read8(addr));
    proc.set_hl(addr+1);
    
    debug_print("%s", "ld a, (hl+)\n");
    return 8;
}

uint8_t rst_n(Z80& proc, uint8_t b1)
{
    //Push present address on to stack
    proc.sp.dec(2);
    proc.mem.write16(proc.sp.read(), proc.pc.read());
    
    //Jump to addr
    uint16_t offset = 0;
    
    switch (b1)
    {
        case 0xc7:
            offset = 0x00;
            break;
        case 0xcf:
            offset = 0x08;
            break;
        case 0xd7:
            offset = 0x10;
            break;
        case 0xdf:
            offset = 0x18;
            break;
        case 0xe7:
            offset = 0x20;
            break;
        case 0xef:
            offset = 0x28;
            break;
        case 0xf7:
            offset = 0x30;
            break;
        case 0xff:
            offset = 0x38;
            break;
    }
    
    proc.pc.write(offset);
    
    debug_print("rst 0x%04x\n", offset);
    return 32;
}

namespace
{
    void generic_and_n(Z80& proc, uint8_t value)
    {
        uint8_t res = proc.a.read() & value;
        proc.a.write(res);
        
        proc.f.set_z(res==0);
        proc.f.set_n(false);
        proc.f.set_h(true);
        proc.f.set_c(false);
    }
}

uint8_t and_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0xa7:
            reg = &proc.a;
            break;
        case 0xa0:
            reg = &proc.b;
            break;
        case 0xa1:
            reg = &proc.c;
            break;
        case 0xa2:
            reg = &proc.d;
            break;
        case 0xa3:
            reg = &proc.e;
            break;
        case 0xa4:
            reg = &proc.h;
            break;
        case 0xa5:
            reg = &proc.l;
            break;
        //hl as addr
        case 0xa6:
        {
            uint8_t value = proc.mem.read8(proc.get_hl());
            generic_and_n(proc, value);
            
            debug_print("%s\n", "and (hl)");
            return 8;
        }
        case 0xe6:
        {
            uint8_t value = proc.fetch_byte();
            generic_and_n(proc, value);
            
            debug_print("and 0x%02x\n", value);
            return 8;
        }
    }
    
    generic_and_n(proc, reg->read());
    
    debug_print("and %s\n", reg->name.c_str());
    return 4;
}

uint8_t dec_nn(Z80& proc, uint8_t b1)
{
    std::string reg;
    
    switch (b1)
    {
        case 0x0b:
            proc.set_bc(proc.get_bc()-1);
            reg = "bc";
            break;
        case 0x1b:
            proc.set_de(proc.get_de()-1);
            reg = "de";
            break;
        case 0x2b:
            proc.set_hl(proc.get_hl()-1);
            reg = "hl";
            break;
        case 0x3b:
            proc.sp.inc(1);
            reg = "sp";
            break;
    }
    
    debug_print("dec %s\n", reg.c_str());
    return 8;
}

namespace {
    void generic_or_n(Z80& proc, uint8_t value)
    {
        uint8_t res = proc.a.read() | value;
        proc.a.write(res);
        
        proc.f.set_z(res==0);
        proc.f.set_c(false);
        proc.f.set_h(false);
        proc.f.set_n(false);
    }
}

uint8_t or_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0xb7:
            reg = &proc.a;
            break;
        case 0xb0:
            reg = &proc.b;
            break;
        case 0xb1:
            reg = &proc.c;
            break;
        case 0xb2:
            reg = &proc.d;
            break;
        case 0xb3:
            reg = &proc.e;
            break;
        case 0xb4:
            reg = &proc.h;
            break;
        case 0xb5:
            reg = &proc.l;
            break;
        //Use hl as addr
        case 0xb6:
        {
            uint8_t value = proc.mem.read8(proc.get_hl());
            generic_or_n(proc, value);
            
            debug_print("%s\n", "or (hl)");
            return 8;
        }
        //Use next byte
        case 0xf6:
        {
            uint8_t value = proc.fetch_byte();
            generic_or_n(proc, value);
            
            debug_print("or 0x%02x\n", value);
            return 8;
        }
    }
    
    generic_or_n(proc, reg->read());
    
    debug_print("or %s\n", reg->name.c_str());
    return 4;
}

uint8_t cpl(Z80& proc)
{
    uint8_t new_val = ~proc.a.read();
    proc.a.write(new_val);
    
    proc.f.set_n(true);
    proc.f.set_h(true);
    
    debug_print("%s\n", "cpl");
    return 4;
}

namespace {
    void generic_adc_a(Z80& proc, uint8_t value)
    {
        uint8_t orig_a = proc.a.read();
        uint8_t carry = proc.f.get_c();
        uint8_t new_value = orig_a + value + carry;
        proc.a.write(new_value);
        
        proc.f.set_z(new_value==0);
        proc.f.set_n(false);
        
        bool half_carry = ((value & 0xf) + (orig_a & 0xf) + carry) > 0xf;
        proc.f.set_h(half_carry);
        
        bool did_carry = (uint16_t(value) + uint16_t(orig_a) + uint16_t(carry)) > 0xff;
        proc.f.set_c(did_carry);
    }
}

uint8_t adc_a_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x8f:
            reg = &proc.a;
            break;
        case 0x88:
            reg = &proc.b;
            break;
        case 0x89:
            reg = &proc.c;
            break;
        case 0x8a:
            reg = &proc.d;
            break;
        case 0x8b:
            reg = &proc.e;
            break;
        case 0x8c:
            reg = &proc.h;
            break;
        case 0x8d:
            reg = &proc.l;
            break;
        //Hl as addr
        case 0x8e:
        {
            uint16_t addr = proc.get_hl();
            generic_adc_a(proc, proc.mem.read8(addr));
            
            debug_print("%s", "adc a, (hl)\n");
            return 8;
        }
        case 0xce:
        {
            uint8_t value = proc.fetch_byte();
            generic_adc_a(proc, value);
            
            debug_print("adc a, 0x%02x\n", value);
            return 8;
        }
    }
    
    generic_adc_a(proc, reg->read());
    
    debug_print("adc a, %s\n", reg->name.c_str());
    return 4;
}

namespace {
    uint8_t swap(uint8_t value)
    {
        return ((value & 0xf) << 4) | (value >> 4);
    }
}

uint8_t swap_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    proc.f.set_c(false);
    proc.f.set_h(false);
    proc.f.set_c(false);
    
    switch (b1)
    {
        case 0x37:
            reg = &proc.a;
            break;
        case 0x30:
            reg = &proc.b;
            break;
        case 0x31:
            reg = &proc.c;
            break;
        case 0x32:
            reg = &proc.d;
            break;
        case 0x33:
            reg = &proc.e;
            break;
        case 0x34:
            reg = &proc.h;
            break;
        case 0x35:
            reg = &proc.l;
            break;
        //Use hl as addr
        case 0x36:
        {
            uint16_t addr = proc.get_hl();
            uint8_t value = proc.mem.read8(addr);
            value = swap(value);
            proc.mem.write8(addr, value);
            proc.f.set_z(value==0);
            
            debug_print("%s\n", "swap (hl)");
            return 16;
        }
    }
    
    uint8_t value = swap(reg->read());
    reg->write(value);
    proc.f.set_z(value==0);
    
    debug_print("swap %s\n", reg->name.c_str());
    return 8;
}

uint8_t add_hl_n(Z80& proc, uint8_t b1)
{
    uint16_t original = proc.get_hl();
    uint16_t new_val  = original;
    std::string pair;
    proc.f.set_n(false);
    
    switch (b1)
    {
        case 0x09:
            new_val += proc.get_bc();
            pair = "bc";
            break;
        case 0x19:
            new_val += proc.get_de();
            pair = "de";
            break;
        case 0x29:
            new_val += proc.get_hl();
            pair = "hl";
            break;
        case 0x39:
            new_val += proc.sp.read();
            pair = "sp";
            break;
    }
    
    proc.set_hl(new_val);
    
    //Carry from bit 11
    proc.f.set_h(((original & 0xfff)+(new_val & 0xfff)) > 0xfff);
    
    //Carry from bit 15
    proc.f.set_c((uint32_t(original) + uint32_t(new_val)) > 0xffff);
    
    debug_print("add hl, %s\n", pair.c_str());
    return 8;
}

uint8_t jp_hl(Z80& proc)
{
    proc.pc.write(proc.get_hl());
    
    debug_print("%s\n", "jp (hl)");
    return 4;
}

namespace {
    uint8_t reset_bit(uint8_t value, uint8_t bit)
    {
        return value & ~(1<<bit);
    }
}

uint8_t res_b_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    uint8_t bit;
    
    if ((b1 >= 0x80) && (b1 < 0x88))
    {
        bit = 0;
    }
    else if ((b1 >= 0x88) && (b1 < 0x90))
    {
        bit = 1;
    }
    else if ((b1 >= 0x90) && (b1 < 0x98))
    {
        bit = 2;
    }
    else if ((b1 >= 0x98) && (b1 < 0xa0))
    {
        bit = 3;
    }
    else if ((b1 >= 0xa0) && (b1 < 0xa8))
    {
        bit = 4;
    }
    else if ((b1 >= 0xa8) && (b1 < 0xb0))
    {
        bit = 5;
    }
    else
    {
        throw std::runtime_error("Couldn't get bit no. for res instr!");
    }
    
    switch (b1 & 0xf)
    {
        case 0x0:
            reg = &proc.b;
            break;
        case 0x1:
            reg = &proc.c;
            break;
        case 0x2:
            reg = &proc.d;
            break;
        case 0x3:
            reg = &proc.e;
            break;
        case 0x4:
            reg = &proc.h;
            break;
        case 0x5:
            reg = &proc.l;
            break;
        case 0x7:
            reg = &proc.a;
            break;
        //use hl
        case 0x6:
        {
            uint16_t addr = proc.get_hl();
            uint8_t value = reset_bit(proc.mem.read8(addr), bit);
            proc.mem.write8(addr, value);
            
            debug_print("res %d, (hl)\n", bit);
            return 16;
        }
        default:
            throw std::runtime_error("Couldn't decode res instr!");
    }
    
    reg->write(reset_bit(reg->read(), bit));
    
    debug_print("res %d, %s\n", bit, reg->name.c_str());
    return 8;
}

uint8_t jp_cc_nn(Z80& proc, uint8_t b1)
{
    uint16_t jump_addr = proc.fetch_short();
    bool jump = false;
    std::string jtype;
    
    switch (b1)
    {
        case 0xc2:
            jtype = "nz";
            jump = !proc.f.get_z();
            break;
        case 0xca:
            jtype = "z";
            jump = proc.f.get_z();
            break;
        case 0xd2:
            jtype = "nc";
            jump = !proc.f.get_c();
            break;
        case 0xda:
            jtype = "c";
            jump = proc.f.get_c();
            break;
    }
    
    if (jump)
    {
        proc.pc.write(jump_addr);
    }
    
    debug_print("jp %s, 0x%02x\n", jtype.c_str(), jump_addr);
    return 12;
}

uint8_t ret_cc(Z80& proc, uint8_t b1)
{
    bool jump = false;
    std::string ret_type;
    
    switch (b1)
    {
        case 0xc0:
            ret_type = "nz";
            jump = !proc.f.get_z();
            break;
        case 0xc8:
            ret_type = "z";
            jump = proc.f.get_z();
            break;
        case 0xd0:
            ret_type = "nc";
            jump = !proc.f.get_c();
            break;
        case 0xd8:
            ret_type = "c";
            jump = proc.f.get_c();
            break;
    }
    
    uint16_t new_addr = proc.mem.read16(proc.sp.read());
    if (jump)
    {
        proc.pc.write(new_addr);
        proc.sp.inc(2);
    }
    
    debug_print("ret %s\n", ret_type.c_str());
    if (jump)
    {
        proc.add_ret(new_addr, false);
    }
    return 8;
}

namespace {
    uint8_t generic_sla(Z80& proc, uint8_t value)
    {
        uint8_t new_val = value << 1;
        
        proc.f.set_z(new_val==0);
        proc.f.set_n(false);
        proc.f.set_h(false);
        proc.f.set_c(value>>7);
        
        return new_val;
    }
}

uint8_t sla_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x27:
            reg = &proc.a;
            break;
        case 0x20:
            reg = &proc.b;
            break;
        case 0x21:
            reg = &proc.c;
            break;
        case 0x22:
            reg = &proc.d;
            break;
        case 0x23:
            reg = &proc.e;
            break;
        case 0x24:
            reg = &proc.h;
            break;
        case 0x25:
            reg = &proc.l;
            break;
        //Hl as addr
        case 0x26:
        {
            uint16_t addr = proc.get_hl();
            uint8_t new_val = generic_sla(proc, proc.mem.read8(addr));
            proc.mem.write8(addr, new_val);
            return 16;
        }
    }
    
    reg->write(generic_sla(proc, reg->read()));
    
    debug_print("sla %s\n", reg->name.c_str());
    return 8;
}

uint8_t call_cc_nn(Z80& proc, uint8_t b1)
{
    std::string ctype;
    bool jump = false;
    uint16_t addr = proc.fetch_short();
    
    switch (b1)
    {
        case 0xc4:
            ctype = "nz";
            jump = !proc.f.get_z();
            break;
        case 0xcc:
            ctype = "z";
            jump = proc.f.get_z();
            break;
        case 0xd4:
            ctype = "nc";
            jump = !proc.f.get_n();
            break;
        case 0xdc:
            ctype = "c";
            jump = proc.f.get_c();
            break;
    }
    
    uint16_t old_pc = proc.pc.read();
    if (jump)
    {
        proc.sp.dec(2);
        proc.mem.write16(proc.sp.read(), proc.pc.read());
        proc.pc.write(addr);
    }
    
    debug_print("call %s, 0x%04x\n", ctype.c_str(), addr);
    if (jump)
    {
        proc.add_call(old_pc, addr, proc.sp.read());
    }
    return 12;
}

uint8_t rlca(Z80& proc)
{
    uint8_t a = proc.a.read();
    uint8_t new_a = a<<1;
    //The 'rotate' part
    new_a |= (a>>7);
    proc.a.write(new_a);
    
    proc.f.set_z(new_a==0);
    proc.f.set_n(false);
    proc.f.set_h(false);
    proc.f.set_c(a>>7);
    
    debug_print("%s\n", "rlca");
    return 4;
}

uint8_t add_sp_n(Z80& proc)
{
    int8_t offs = proc.fetch_byte();
    
    if (offs < 0)
    {
        proc.sp.dec(offs*-1);
    }
    else
    {
        proc.sp.inc(offs);
    }
    
    proc.f.set_n(false);
    proc.f.set_z(false);
    
    //H and C?
    
    debug_print("add sp, %d\n", offs);
    return 16;
}

namespace {
    uint8_t generic_rlc(Z80& proc, uint8_t value)
    {
        uint8_t new_value = value << 1;
        
        proc.f.set_z(new_value==0);
        proc.f.set_n(false);
        proc.f.set_h(false);
        proc.f.set_c(value & (1<<7));
        
        return new_value;
    }
}

uint8_t rlc_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x07:
            reg = &proc.a;
            break;
        case 0x00:
            reg = &proc.b;
            break;
        case 0x01:
            reg = &proc.c;
            break;
        case 0x02:
            reg = &proc.d;
            break;
        case 0x03:
            reg = &proc.e;
            break;
        case 0x04:
            reg = &proc.h;
            break;
        case 0x05:
            reg = &proc.l;
            break;
        //hl as addr
        case 0x06:
        {
            uint16_t addr = proc.get_hl();
            uint8_t new_value = generic_rlc(proc, proc.mem.read8(addr));
            proc.mem.write8(addr, new_value);
            
            debug_print("%s\n", "rlc (hl)");
            return 16;
        }
    }
    
    uint8_t new_value = generic_rlc(proc, reg->read());
    reg->write(new_value);
    
    debug_print("rlc %s\n", reg->name.c_str());
    return 8;
}

uint8_t reti(Z80& proc)
{
    uint16_t addr = proc.mem.read16(proc.sp.read());
    proc.pc.write(addr);
    proc.sp.inc(2);
    proc.interrupt_enable = true;
    
    debug_print("%s\n", "reti");
    proc.add_ret(addr, true);
    return 16;
}

namespace
{
    void generic_sbc_n(Z80& proc, uint8_t value)
    {
        uint8_t orig_val = proc.a.read();
        uint8_t new_value = orig_val - value - uint8_t(proc.f.get_c());
        
        proc.f.set_z(new_value==0);
        proc.f.set_n(true);
        proc.f.set_h((orig_val & 0xF) < (value & 0xF));
        //I think...acts as an underflow flag?
        proc.f.set_c(orig_val<value);
        
        proc.a.write(new_value);
    }
}

uint8_t sbc_a_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x9f:
            reg = &proc.a;
            break;
        case 0x98:
            reg = &proc.b;
            break;
        case 0x99:
            reg = &proc.c;
            break;
        case 0x9a:
            reg = &proc.d;
            break;
        case 0x9b:
            reg = &proc.e;
            break;
        case 0x9c:
            reg = &proc.h;
            break;
        case 0x9d:
            reg = &proc.l;
            break;
        //hl as addr
        case 0x9e:
        {
            uint16_t addr = proc.get_hl();
            uint8_t val = proc.mem.read8(addr);
            generic_sbc_n(proc, val);
            
            debug_print("%s\n", "sbc a, (hl)");
            return 8;
        }
    }
    
    generic_sbc_n(proc, reg->read());
    
    debug_print("sbc a, %s\n", reg->name.c_str());
    return 4;
}

uint8_t ld_a_c(Z80& proc)
{
    uint16_t addr = 0xff00 + uint16_t(proc.c.read());
    proc.a.write(proc.mem.read8(addr));
    
    debug_print("%s\n", "ld a, (c)");
    return 8;
}

uint8_t scf(Z80& proc)
{
    proc.f.set_c(true);
    
    debug_print("%s\n", "scf");
    return 4;
}

uint8_t ld_nn_sp(Z80& proc)
{
    uint16_t addr = proc.fetch_short();
    proc.mem.write16(addr, proc.sp.read());
    
    debug_print("ld 0x%04x, sp\n", addr);
    return 20;
}

uint8_t ld_sp_hl(Z80& proc)
{
    proc.sp.write(proc.get_hl());
    debug_print("%s", "ld sp hl\n");
    return 8;
}

namespace {
    uint8_t generic_srl_n(Z80& proc, uint8_t value)
    {
        uint8_t new_value = value >> 1;
        
        proc.f.set_z(new_value==0);
        proc.f.set_n(false);
        proc.f.set_h(false);
        proc.f.set_c(value & 0x1);
        
        return new_value;
    }
}

uint8_t srl_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = nullptr;
    
    switch (b1)
    {
        case 0x3f:
            reg = &proc.a;
            break;
        case 0x38:
            reg = &proc.b;
            break;
        case 0x39:
            reg = &proc.c;
            break;
        case 0x3a:
            reg = &proc.d;
            break;
        case 0x3b:
            reg = &proc.e;
            break;
        case 0x3c:
            reg = &proc.h;
            break;
        case 0x3d:
            reg = &proc.l;
            break;
        //HL as addr
        case 0x3e:
        {
            uint16_t addr = proc.get_hl();
            uint8_t new_value = generic_srl_n(proc, proc.mem.read8(addr));
            proc.mem.write8(addr, new_value);
            
            debug_print("%s\n", "srl (hl)");
            return 16;
        }
    }
    
    reg->write(generic_srl_n(proc, reg->read()));
    
    debug_print("srl %s\n", reg->name.c_str());
    return 8;
}

uint8_t ldhl_sp_n(Z80& proc)
{
    uint8_t offs = proc.fetch_byte();
    uint16_t addr = proc.sp.read() + offs;
    proc.set_hl(proc.mem.read16(addr));
    
    debug_print("ldhl sp, 0x%02x\n", offs);
    return 12;
}

uint8_t rcca(Z80& proc)
{
    uint8_t a = proc.a.read();
    proc.f.set_n(false);
    proc.f.set_h(false);
    proc.f.set_c(a & 0x1);
    
    a = a >> 1;
    proc.f.set_z(a==0);
    proc.a.write(a);
    
    debug_print("rcca\n");
    return 4;
}

uint8_t ld_a_hl_minus(Z80& proc)
{
    uint16_t addr = proc.get_hl();
    uint8_t value = proc.mem.read8(addr);
    proc.a.write(value);
    proc.set_hl(addr-1);
    
    debug_print("ld a (hl-)\n");
    return 8;
}

uint8_t stop(Z80& proc)
{
    proc.fetch_byte(); //2 byte opcode for some reason
    
    throw std::runtime_error("Impliment me!");
    return 4;
}

namespace
{
    uint8_t generic_rr_n(Z80& proc, uint8_t value)
    {
        proc.f.set_c(value & 0x1);
        value >>= 1;
        
        proc.f.set_z(value==0);
        proc.f.set_n(false);
        proc.f.set_h(false);
        
        return value;
    }
}

uint8_t rr_n(Z80& proc, uint8_t b1)
{
    Register<uint8_t>* reg = NULL;
    
    switch (b1)
    {
        case 0x1f:
            reg = &proc.a;
            break;
        case 0x18:
            reg = &proc.b;
            break;
        case 0x19:
            reg = &proc.c;
            break;
        case 0x1a:
            reg = &proc.d;
            break;
        case 0x1b:
            reg = &proc.e;
            break;
        case 0x1c:
            reg = &proc.h;
            break;
        case 0x1d:
            reg = &proc.l;
            break;
        case 0x1e:
        {
            //HL as addr
            uint16_t addr = proc.get_hl();
            uint8_t new_value = generic_rr_n(proc, proc.mem.read8(addr));
            proc.mem.write8(addr, new_value);
            
            debug_print("rr (hl)\n");
            return 16;
        }
    }
    
    reg->write(generic_rr_n(proc, reg->read()));
    debug_print("rr %s\n", reg->name.c_str());
    return 8;
}

uint8_t halt(Z80& proc)
{
    proc.halted = true;
    
    debug_print("halt\n");
    return 4;
}

uint8_t cb_prefix_instr(Z80& proc)
{
    uint8_t temp8 = proc.fetch_byte();
    uint8_t cycles;
    
    if ((temp8 >= 0x40) && (temp8 <= 0x7f))
    {
        cycles = bit_b_r(proc, temp8);
    }
    else if ((temp8 >= 0x10) && (temp8 <= 0x17))
    {
        cycles = rl_n(proc, temp8);
    }
    else if ((temp8 >= 0x30) && (temp8 <= 0x37))
    {
        cycles = swap_n(proc, temp8);
    }
    else if ((temp8 >= 0x80) && (temp8 <= 0xBf))
    {
        cycles = res_b_n(proc, temp8);
    }
    else if ((temp8 >= 0x20) && (temp8 <= 0x27))
    {
        cycles = sla_n(proc, temp8);
    }
    else if ((temp8 >= 0x00) && (temp8 <= 0x07))
    {
        cycles = rlc_n(proc, temp8);
    }
    else if ((temp8 >= 0x38) && (temp8 <= 0x3f))
    {
        cycles = srl_n(proc, temp8);
    }
    else if ((temp8 >= 0x18) && (temp8 <= 0x1f))
    {
        cycles = rr_n(proc, temp8);
    }
    else
    {
        throw std::runtime_error(
                                 formatted_string("Unknown byte after 0xCB prefix: 0x%02x", temp8));
    }
    
    return cycles;
}

void Step(Z80& proc)
{
    //Fetch first byte from PC
    debug_print("PC: 0x%04x - ", proc.pc.read());
    
    uint8_t b1 = proc.fetch_byte();
    uint8_t cycles = 0;
    
    switch (b1)
    {
        case 0x06:
        case 0x0E:
        case 0x16:
        case 0x1E:
        case 0x26:
        case 0x2E:
            cycles = ld_nn_n(proc, b1);
            break;
        case 0x01:
        case 0x11:
        case 0x21:
        case 0x31:
            cycles = ld_n_nn(proc, b1);
            break;
        case 0xAF:
        case 0xA8:
        case 0xA9:
        case 0xAA:
        case 0xAB:
        case 0xAC:
        case 0xAD:
        case 0xAE:
        case 0xEE:
            cycles = xor_n(proc, b1);
            break;
        case 0x32:
            cycles = ld_hl_dec_a(proc, b1);
            break;
        case 0xCB:
            cycles = cb_prefix_instr(proc);
            break;
        case 0x20:
        case 0x28:
        case 0x30:
        case 0x38:
            cycles = jr_cc_n(proc, b1);
            break;
        //case 0x7f: this is ld a,a so we'll just pick one of the handlers to use
        case 0x78:
        case 0x79:
        case 0x7a:
        case 0x7b:
        case 0x7c:
        case 0x7d:
        case 0x0a:
        case 0x1a:
        case 0x7e:
        case 0xfa:
        case 0x3e:
            cycles = ld_a_n(proc, b1);
            break;
        case 0xe2:
            cycles = ld_offs_c_a(proc);
            break;
        case 0x3c:
        case 0x04:
        case 0x0c:
        case 0x14:
        case 0x1c:
        case 0x24:
        case 0x2c:
        case 0x34:
            cycles = inc_n(proc, b1);
            break;
        case 0x7f:
        case 0x47:
        case 0x4f:
        case 0x57:
        case 0x5f:
        case 0x67:
        case 0x6f:
        case 0x02:
        case 0x12:
        case 0x77:
        case 0xea:
            cycles = ld_n_a(proc, b1);
            break;
        case 0xe0:
            cycles = ld_offs_n_a(proc);
            break;
        case 0xcd:
            cycles = call_nn(proc);
            break;
        case 0xf5:
        case 0xc5:
        case 0xd5:
        case 0xe5:
            cycles = push_nn(proc, b1);
            break;
        case 0x17:
            cycles = rla(proc);
            break;
        case 0xf1:
        case 0xc1:
        case 0xd1:
        case 0xe1:
            cycles = pop_nn(proc, b1);
            break;
        case 0x3d:
        case 0x05:
        case 0x0d:
        case 0x15:
        case 0x1d:
        case 0x25:
        case 0x2d:
        case 0x35:
            cycles = dec_n(proc, b1);
            break;
        case 0x22:
            cycles = ld_hl_plus_a(proc);
            break;
        case 0x03:
        case 0x13:
        case 0x23:
        case 0x33:
            cycles = inc_nn(proc, b1);
            break;
        case 0xc9:
            cycles = ret(proc);
            break;
        case 0xbf:
        case 0xb8:
        case 0xb9:
        case 0xba:
        case 0xbb:
        case 0xbc:
        case 0xbd:
        case 0xbe:
        case 0xfe:
            cycles = cp_n(proc, b1);
            break;
        case 0x18:
            cycles = jr_n(proc);
            break;
        case 0xf0:
            cycles = ldh_a_n(proc);
            break;
        case 0x97:
        case 0x90:
        case 0x91:
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x96:
        case 0xd6:
            cycles = sub_n(proc, b1);
            break;
        case 0x87:
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
        case 0x86:
        case 0xC6:
            cycles = add_a_n(proc, b1);
            break;
        case 0x00:
            cycles = nop();
            break;
        case 0xF3:
            cycles = di(proc);
            break;
        case 0xFB:
            cycles = ei(proc);
            break;
        case 0xc3:
            cycles = jp_nn(proc);
            break;
        //case 0x7f: These are all covered by ld a, n
        //case 0x78:  |
        //case 0x79:  |
        //case 0x7a:  |
        //case 0x7b:  |
        //case 0x7c:  |
        //case 0x7d:  |
        //case 0x7e:<-|
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46: //Note: no 0x47/ 0x57 etc.
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x58:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5e:
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x68:
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6e:
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x36:
            cycles = ld_r1_r2(proc, b1);
            break;
        case 0x2a:
            cycles = ld_a_hl_plus(proc);
            break;
        case 0x0b:
        case 0x1b:
        case 0x2b:
        case 0x3b:
            cycles = dec_nn(proc, b1);
            break;
        case 0xb7:
        case 0xb0:
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xb5:
        case 0xb6:
        case 0xf6:
            cycles = or_n(proc, b1);
            break;
        case 0x2f:
            cycles = cpl(proc);
            break;
        case 0xa7:
        case 0xa0:
        case 0xa1:
        case 0xa2:
        case 0xa3:
        case 0xa4:
        case 0xa5:
        case 0xa6:
        case 0xe6:
            cycles = and_n(proc, b1);
            break;
        case 0xc7:
        case 0xcf:
        case 0xd7:
        case 0xdf:
        case 0xe7:
        case 0xef:
        case 0xf7:
        case 0xff:
            cycles = rst_n(proc, b1);
            break;
        case 0x09:
        case 0x19:
        case 0x29:
        case 0x39:
            cycles = add_hl_n(proc, b1);
            break;
        case 0xe9:
            cycles = jp_hl(proc);
            break;
        case 0xc2:
        case 0xca:
        case 0xd2:
        case 0xda:
            cycles = jp_cc_nn(proc, b1);
            break;
        case 0xc0:
        case 0xc8:
        case 0xd0:
        case 0xd8:
            cycles = ret_cc(proc, b1);
            break;
        case 0xc4:
        case 0xcc:
        case 0xd4:
        case 0xdc:
            cycles = call_cc_nn(proc, b1);
            break;
        case 0x07:
            cycles = rlca(proc);
            break;
        case 0x8f:
        case 0x88:
        case 0x89:
        case 0x8a:
        case 0x8b:
        case 0x8c:
        case 0x8d:
        case 0x8e:
        case 0xce:
            cycles = adc_a_n(proc, b1);
            break;
        case 0xe8:
            cycles = add_sp_n(proc);
            break;
        case 0xd9:
            cycles = reti(proc);
            break;
        case 0x9f:
        case 0x98:
        case 0x99:
        case 0x9a:
        case 0x9b:
        case 0x9c:
        case 0x9d:
        case 0x9e:
            cycles = sbc_a_n(proc, b1);
            break;
        case 0xf2:
            cycles = ld_a_c(proc);
            break;
        case 0x37:
            cycles = scf(proc);
            break;
        case 0x08:
            cycles = ld_nn_sp(proc);
            break;
        case 0x10:
            cycles = stop(proc);
            break;
        case 0x3a:
            cycles = ld_a_hl_minus(proc);
            break;
        case 0x0f:
        case 0x1f: //rra appears to be the same thing
            cycles = rcca(proc);
            break;
        case 0xf8:
            cycles = ldhl_sp_n(proc);
            break;
        case 0xf9:
            cycles = ld_sp_hl(proc);
            break;
        case 0x76:
            cycles = halt(proc);
            break;
        default:
        {
            throw std::runtime_error(formatted_string("Unknown opcode byte: 0x%02x", b1));
        }
    }
    
    //For future timing use/interrupt ei/di handling
    proc.tick(cycles);
}
