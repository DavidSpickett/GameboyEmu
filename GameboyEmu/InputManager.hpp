//
//  InputManager.hpp
//  GameboyEmu
//
//  Created by David Spickett on 07/10/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef InputManager_hpp
#define InputManager_hpp

#include "MemoryManager.hpp"

class InputManager: public MemoryManager
{
public:
    InputManager();
    
    uint8_t read8(uint16_t addr);
    void write8(uint16_t addr, uint8_t value);
    
    uint16_t read16(uint16_t addr) {throw std::runtime_error("?");}
    void write16(uint16_t addr, uint16_t value) {throw std::runtime_error("?");}
    
    bool read_inputs();
    
    void tick(size_t curr_cycles) {}
    
private:
    enum InputMode
    {
        DIR,
        BUTTON,
        INVALID
    };
    
    uint8_t get_joy_vaue(InputMode mode, const uint8_t* state);
    
    uint8_t m_joypad;
    const std::array<int, 4> m_joypad_keycodes;
    const std::array<int, 4> m_button_keycodes;
    InputMode m_mode;
};

#endif /* InputManager_hpp */
