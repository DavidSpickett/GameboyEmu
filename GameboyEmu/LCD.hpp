//
//  LCD.hpp
//  GameboyEmu
//
//  Created by David Spickett on 28/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef LCD_hpp
#define LCD_hpp

#include <stdio.h>
#include "MemoryMap.hpp"

class LCD: public MemoryManager
{
    public:
        LCD():
            MemoryManager(address_range(0xff40, 0xffff))
        {
        }
    
        void write8(uint16_t addr, uint8_t value);
        uint8_t read8(uint16_t addr);
};

#endif /* LCD_hpp */
