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

void Step(Z80& proc)
{
    //Fetch first byte from PC
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
        default:
        {
            throw std::runtime_error(formatted_string("Unknown opcode byte: 0x%02x", b1));
        }
    }
    (void)cycles;
    //std::cout << formatted_string("Took %d cycles.\n", cycles);
    //std::cout << proc.status_string();
}

uint8_t cb_prefix_instr(Z80& proc)
{
    uint8_t temp8 = proc.fetch_byte();
    uint8_t cycles;
    
    if ((temp8 >= 0x40) && (temp8 <= 0x7f))
    {
        cycles = bit_b_r(proc, temp8);
    }
    else
    {
        throw std::runtime_error(
            formatted_string("Unknown byte after 0xCB prefix: 0x%02x", temp8));
    }
    
    return cycles;
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
            bit = 5;
            break;
        case 0x72:
            reg = &proc.d;
            bit = 5;
            break;
        case 0x73:
            reg = &proc.e;
            bit = 5;
            break;
        case 0x74:
            reg = &proc.h;
            bit = 5;
            break;
        case 0x75:
            reg = &proc.l;
            bit = 5;
            break;
        case 0x77:
            reg = &proc.a;
            bit = 5;
            break;
            
        case 0x78:
            reg = &proc.b;
            bit = 6;
            break;
        case 0x79:
            reg = &proc.c;
            bit = 6;
            break;
        case 0x7a:
            reg = &proc.d;
            bit = 6;
            break;
        case 0x7b:
            reg = &proc.e;
            bit = 6;
            break;
        case 0x7c:
            reg = &proc.h;
            bit = 6;
            break;
        case 0x7d:
            reg = &proc.l;
            bit = 6;
            break;
        case 0x7f:
            reg = &proc.a;
            bit = 6;
            break;
        //(HL) variants
        case 0x46:
            break;
        case 0x4e:
            break;
        case 0x56:
            break;
        case 0x5e:
            break;
        case 0x66:
            break;
        case 0x6e:
            break;
        case 0x76:
            break;
        case 0x7e:
            break;
        default:
            throw std::runtime_error(
                                     formatted_string("Unknown byte for bit b,r instruction: 0x%02x", temp8));
    }
    
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
    
    printf("%s\n", prt.c_str());

    return cycles;
}

uint8_t ld_nn_n(Z80& proc, uint8_t b1)
{
    uint8_t b2 = proc.fetch_byte();
    std::string fmt = "ld %s, 0x%02x";
    
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
    
    printf(fmt.c_str(), reg->name.c_str(), b2);
    
    return 8;
}

uint8_t ld_n_nn(Z80& proc, uint8_t b1)
{
    std::vector<uint8_t> bs = proc.fetch_bytes(2);
    uint16_t imm = (bs[1] << 8) | bs[0];
    
    std::string fmt = "ld %s, 0x%02x\n";
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
    
    printf(fmt.c_str(), r.c_str(), imm);

    return 12;
}

uint8_t ld_hl_dec_a(Z80& proc, uint8_t b1)
{
    std::string fmt = "ld (hl-), a(0x%04x, 0x%02x)\n";
    
    uint8_t temp8 = proc.a.read();
    uint16_t addr = proc.get_hl();
    proc.mem.write16(addr, temp8);
    
    printf(fmt.c_str(), addr, temp8);
    
    //Now decrement HL
    proc.set_hl(addr-1);
    return 8;
}
