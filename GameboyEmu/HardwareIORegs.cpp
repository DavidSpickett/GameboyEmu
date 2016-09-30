//
//  HardwareIORegs.cpp
//  GameboyEmu
//
//  Created by David Spickett on 30/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "HardwareIORegs.hpp"

uint8_t HardwareIORegs::read8(uint16_t addr)
{
    if (addr == 0xff44)
    {
        //Bodge, pretend to be in blank timeslot
        return 144;
    }
    return 0;
}

void HardwareIORegs::write8(uint16_t addr, uint8_t value)
{
}
