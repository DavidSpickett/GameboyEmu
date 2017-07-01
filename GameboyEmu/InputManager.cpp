//
//  InputManager.cpp
//  GameboyEmu
//
//  Created by David Spickett on 07/10/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "InputManager.hpp"
#include <SDL2/SDL.h>
#include "Z80.hpp"

namespace
{
    const uint8_t MODE_DIR     = 0;
    const uint8_t MODE_BUTTON  = 1;
    const uint8_t MODE_INVALID = 2;
    
    const int joypad_keycodes[] = {SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN};
    const int button_keycodes[] = {SDL_SCANCODE_X, SDL_SCANCODE_Z, SDL_SCANCODE_RSHIFT, SDL_SCANCODE_RETURN};
    
    uint8_t get_joy_vaue(uint8_t mode, const uint8_t* state)
    {
        uint8_t new_pad_value = 0x0f;
        
        if (mode != MODE_INVALID)
        {
            const int* button_codes = mode == MODE_DIR ? joypad_keycodes : button_keycodes;
            
            for (auto i=0; i != 4; ++i)
            {
                if (state[*(button_codes+i)])
                {
                    new_pad_value = new_pad_value & ~(1<<i);
                }
            }
        }
        
        return new_pad_value;
    }
}

InputManager::InputManager():
    m_joypad(0xff), m_mode(MODE_INVALID)
{
}

bool InputManager::read_inputs()
{
    //Used to get the console out of a halted state.
    const uint8_t *state = SDL_GetKeyboardState(NULL);
    uint8_t dir = get_joy_vaue(MODE_DIR, state);
    uint8_t but = get_joy_vaue(MODE_BUTTON, state);
    return (dir == 0x0f) && (but == 0x0f);
}

uint8_t InputManager::read8(uint16_t addr)
{
    if (addr != JOYPAD_REG)
    {
        throw std::runtime_error(formatted_string("Unknown read of address 0x%04x from input manager.", addr));
    }
    
    m_joypad = get_joy_vaue(m_mode, SDL_GetKeyboardState(NULL));
    
    //printf("Joypad: 0x%02x\n", m_joypad);
    return m_joypad;
}

void InputManager::write8(uint16_t addr, uint8_t value)
{
    if ((value & 0x30) == 0x30)
    {
        m_mode = MODE_INVALID;
    }
    else if ((value & (1<<4)) == 0)
    {
        m_mode = MODE_DIR;
    }
    else if ((value & (1<<5)) == 0)
    {
        m_mode = MODE_BUTTON;
    }
}
