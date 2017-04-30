//
//  SoundHandler.hpp
//  GameboyEmu
//
//  Created by David Spickett on 15/01/2017.
//  Copyright © 2017 David Spickett. All rights reserved.
//

#ifndef SoundHandler_h
#define SoundHandler_h

#include "MemoryManager.hpp"

class SoundHandler : public MemoryManager
{
public:
    SoundHandler(MemoryMap& map):
        MemoryManager(map)
    {}
    
    uint8_t read8(uint16_t addr) { return 0; }
    void write8(uint16_t addr, uint8_t value) {}
    
    uint16_t read16(uint16_t addr) { return 0; }
    void write16(uint16_t addr, uint16_t value) {}
    
    void tick(size_t curr_cycles) {}
};

#endif /* SoundHandler_h */
