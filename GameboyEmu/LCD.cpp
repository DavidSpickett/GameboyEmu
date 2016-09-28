//
//  LCD.cpp
//  GameboyEmu
//
//  Created by David Spickett on 28/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "LCD.hpp"

uint8_t LCD::read8(uint16_t addr)
{
    printf("8 bit read from addr: 0x%04x\n", addr);
    if (addr == 0xff44)
    {
        //Bodge, pretend to be in blank timeslot
        return 144;
    }
    return 0;
}

void LCD::write8(uint16_t addr, uint8_t value)
{
    printf("8 bit write to addr: 0x%04x value: 0x%02x\n", addr, value);
}
