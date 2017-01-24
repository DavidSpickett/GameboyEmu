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

namespace {
    const uint8_t MODE_DIR = 0;
    const uint8_t MODE_BUTTON = 1;
}

namespace {
    uint8_t get_dir_joypad(const uint8_t* state)
    {
        uint8_t new_joypad = 0x0f;
        
        if (state[SDL_SCANCODE_DOWN])
        {
            new_joypad &= ~(1<<3);
        }
        if (state[SDL_SCANCODE_UP])
        {
            new_joypad &= ~(1<<2);
        }
        if (state[SDL_SCANCODE_LEFT])
        {
            new_joypad &= ~(1<<1);
        }
        if (state[SDL_SCANCODE_RIGHT])
        {
            new_joypad &= ~1;
        }
        
        return new_joypad;
    }
    
    uint8_t get_button_joypad(const uint8_t* state)
    {
        uint8_t new_joypad = 0x0f;
        
        if (state[SDL_SCANCODE_RETURN]) //Start
        {
            new_joypad &= ~(1<<3);
        }
        if (state[SDL_SCANCODE_RSHIFT]) //Select
        {
            new_joypad &= ~(1<<2);
        }
        if (state[SDL_SCANCODE_Z]) //B
        {
            new_joypad &= ~(1<<1);
        }
        if (state[SDL_SCANCODE_X]) //A
        {
            new_joypad &= ~1;
        }
        
        return new_joypad;
    }
}

bool InputManager::read_inputs()
{
    const uint8_t *state = SDL_GetKeyboardState(NULL);
    uint8_t dir = get_dir_joypad(state);
    uint8_t but = get_button_joypad(state);
    return (dir == 0x0f) && (but == 0x0f);
}

uint8_t InputManager::read8(uint16_t addr)
{
    if (addr != JOYPAD_REG)
    {
        throw std::runtime_error(formatted_string("Unknown read of address 0x%04x from input manager.", addr));
    }
    
    const uint8_t *state = SDL_GetKeyboardState(NULL);
    m_joypad = m_mode == MODE_DIR ? get_dir_joypad(state) : get_button_joypad(state);
    
    //printf("Joypad: 0x%02x\n", m_joypad);
    return m_joypad;
}

void InputManager::write8(uint16_t addr, uint8_t value)
{
    if ((value & (1<<5)) == 0)
    {
        m_mode = MODE_DIR;
    }
    else if ((value & (1<<4)) == 0)
    {
        m_mode = MODE_BUTTON;
    }
}

void InputManager::tick(size_t curr_cycles)
{
}
