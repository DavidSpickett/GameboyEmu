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
#include <SDL2/SDL.h>

class LCDWindow
{
public:
    LCDWindow();
    ~LCDWindow();
    
    void init();
    void draw();

private:
    SDL_Window* m_window;
    SDL_Surface* m_surface;
    std::vector<std::vector<uint8_t>> m_pixels;
    std::vector<uint32_t> m_colours;
};

class LCD: public MemoryManager
{
    public:
        LCD():
            MemoryManager(address_range(0x8000, 0x9fff)),
            m_display()
        {
        }
    
        void write8(uint16_t addr, uint8_t value);
        uint8_t read8(uint16_t addr);
    
        void show_display();
        void draw();
    
    private:
        LCDWindow m_display;
};

#endif /* LCD_hpp */
