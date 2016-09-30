//
//  HardwareIORegs.hpp
//  GameboyEmu
//
//  Created by David Spickett on 30/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef HardwareIORegs_hpp
#define HardwareIORegs_hpp

#include <stdio.h>
#include "MemoryMap.hpp"

class HardwareIORegs: public MemoryManager
{
public:
    HardwareIORegs():
        MemoryManager(address_range(0xff00, 0xff7f))
    {
    }
    
    void write8(uint16_t addr, uint8_t value);
    uint8_t read8(uint16_t addr);
};

#endif /* HardwareIORegs_hpp */
