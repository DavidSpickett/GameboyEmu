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

InputManager::InputManager():
    m_mode(INVALID),
    m_joypad_keycodes{SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN},
    m_button_keycodes{SDL_SCANCODE_X, SDL_SCANCODE_Z, SDL_SCANCODE_RSHIFT, SDL_SCANCODE_RETURN}
{
}

uint8_t InputManager::get_joy_vaue(InputMode mode, const uint8_t* state)
{
    auto new_pad_value = 0;
    if (mode != INVALID)
    {
        auto button_codes = mode == DIR ? m_joypad_keycodes.begin() : m_button_keycodes.begin();
        for (auto i=0; i != 4; ++i, ++button_codes)
        {
            //Bit is set if the button is *NOT* held down
            if (!state[*button_codes])
            {
                new_pad_value |= 1<<i;
            }
        }
    }
    
    return new_pad_value;
}

bool InputManager::read_inputs()
{
    //Used to get the console out of a stopped state
    auto state = SDL_GetKeyboardState(NULL);
    return (get_joy_vaue(DIR, state) != 0x0f) || (get_joy_vaue(BUTTON, state) != 0x0f);
}

uint8_t InputManager::read8(uint16_t addr)
{
    return get_joy_vaue(m_mode, SDL_GetKeyboardState(NULL));;
}

void InputManager::write8(uint16_t addr, uint8_t value)
{
    m_mode = static_cast<InputMode>((value >> 4) & 3);
}
