//
//  instructions.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "instructions.hpp"
#include "utils.hpp"

void Step(Z80& proc)
{
    //Fetch first byte from PC
    uint8_t b1 = proc.mem.read8(proc.pc.read());
    uint8_t bytes_consumed;
    
    switch (b1)
    {
            //LD nn, n
        case 0x06:
        case 0x0E:
        case 0x16:
        case 0x1E:
        case 0x26:
        case 0x2E:
            bytes_consumed = ld_nn_n(proc);
            break;
        default:
        {
            throw formatted_string("Unknown opcode byte: 0x%02x", b1);
        }
    }
    
    proc.pc.inc(bytes_consumed);
}

uint8_t ld_nn_n(Z80& proc)
{
    //uint8_t b1;
    return 0;
}
