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

const uint8_t MODE_DIR = 0;
const uint8_t MODE_BUTTON = 1;
const uint8_t MODE_INVALID = 2;

class Z80;

class InputManager: public MemoryManager
{
public:
    InputManager():
        m_joypad(0xff), m_mode(MODE_INVALID)
    {}
    
    uint8_t read8(uint16_t addr);
    void write8(uint16_t addr, uint8_t value);
    
    uint16_t read16(uint16_t addr) {throw std::runtime_error("?");}
    void write16(uint16_t addr, uint16_t value) {throw std::runtime_error("?");}
    
    bool read_inputs();
    
    void tick(size_t curr_cycles);
    
private:
    uint8_t m_joypad;
    uint8_t m_mode;
};

#endif /* InputManager_hpp */
