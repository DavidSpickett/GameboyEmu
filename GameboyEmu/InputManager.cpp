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

uint8_t InputManager::read8(uint16_t addr)
{
    if (addr != JOYPAD_REG)
    {
        throw std::runtime_error(formatted_string("Unknown read of address 0x%04x from input manager.", addr));
    }
    
    const uint8_t *state = SDL_GetKeyboardState(NULL);
    m_joypad = 0x0f;
    
    if (m_mode == MODE_DIR)
    {
        if (state[SDL_SCANCODE_DOWN])
        {
            m_joypad &= ~(1<<3);
            m_proc->stopped = false;
        }
        if (state[SDL_SCANCODE_UP])
        {
            m_joypad &= ~(1<<2);
        }
        if (state[SDL_SCANCODE_LEFT])
        {
            m_joypad &= ~(1<<1);
        }
        if (state[SDL_SCANCODE_RIGHT])
        {
            m_joypad &= ~1;
        }
    }
    else
    {
        if (state[SDL_SCANCODE_RETURN]) //Start
        {
            m_joypad &= ~(1<<3);
        }
        if (state[SDL_SCANCODE_RSHIFT]) //Select
        {
            m_joypad &= ~(1<<2);
        }
        if (state[SDL_SCANCODE_Z]) //B
        {
            m_joypad &= ~(1<<1);
        }
        if (state[SDL_SCANCODE_X]) //A
        {
            m_joypad &= ~1;
        }
    }
    
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
