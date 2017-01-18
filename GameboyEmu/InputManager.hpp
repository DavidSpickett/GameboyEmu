//
//  InputManager.hpp
//  GameboyEmu
//
//  Created by David Spickett on 07/10/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef InputManager_hpp
#define InputManager_hpp

#include <stdio.h>
#include "MemoryManager.hpp"

class Z80;

class InputManager: public MemoryManager
{
public:
    InputManager():
        m_proc(NULL)
    {
        m_joypad = 0xff;
    }
    
    uint8_t read8(uint16_t addr);
    void write8(uint16_t addr, uint8_t value);
    
    uint16_t read16(uint16_t addr) {throw std::runtime_error("?");}
    void write16(uint16_t addr, uint16_t value) {throw std::runtime_error("?");}
    
    void tick(size_t curr_cycles);
    Z80* m_proc;
    
private:
    uint8_t m_joypad;
    uint8_t m_mode;
};

#endif /* InputManager_hpp */
