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

class Pixel
{
public:
    Pixel(uint16_t x, uint16_t y, uint8_t c):
    x(x), y(y), c(c)
    {
    }
    
    uint16_t x;
    uint16_t y;
    uint8_t c;
};

class Tile
{
public:
    Tile(uint16_t x, uint16_t y, uint16_t h, std::vector<uint8_t> data):
    x(x), y(y), h(h), data(data)
    {
    }
    
    std::vector<Pixel> to_pixels() const;
    std::string to_string() const;
    
    uint16_t x;
    uint16_t y;
    uint16_t h; //For potential double height mode
    std::vector<uint8_t> data;
};

class LCDWindow
{
public:
    LCDWindow();
    ~LCDWindow();
    
    void init();
    void draw(std::vector<Pixel>& pixels);
    void set_viewport_pos(uint16_t x, uint16_t y) { m_x_origin=x; m_y_origin=y; }

private:
    SDL_Window* m_window;
    SDL_Surface* m_surface;
    std::vector<std::vector<uint8_t>> m_pixels;
    std::vector<uint32_t> m_colours;
    uint16_t m_x_origin;
    uint16_t m_y_origin;
};

class LCD: public MemoryManager
{
    public:
        LCD():
            MemoryManager(to_vector(address_range(0x8000, 0x9fff))),
            m_display()
        {
            m_data = std::vector<uint8_t>(0x2000, 0);
        }
    
        void write8(uint16_t addr, uint8_t value);
        uint8_t read8(uint16_t addr);
    
        void show_display();
        void draw();
    
    private:
        LCDWindow m_display;
        std::vector<uint8_t> m_data;
};

#endif /* LCD_hpp */
