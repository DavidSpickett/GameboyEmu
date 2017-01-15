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
#include <vector>
#include <string>
#include <SDL2/SDL.h>
#include "MemoryManager.hpp"

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
    Tile(uint16_t x, uint16_t y, uint16_t h, std::vector<uint8_t>::const_iterator data_b, std::vector<uint8_t>::const_iterator data_e):
    x(x), y(y), h(h), data_b(data_b), data_e(data_e)
    {
    }
    
    std::vector<Pixel> to_pixels(std::vector<uint8_t>& pallette) const;
    std::string to_string(std::vector<uint8_t>& pallette) const;
    bool has_some_colour() const;
    
    uint16_t x;
    uint16_t y;
    uint16_t h; //For potential double height mode
    std::vector<uint8_t>::const_iterator data_b;
    std::vector<uint8_t>::const_iterator data_e;
};

struct colour
{
    colour(uint8_t r, uint8_t g, uint8_t b):
    a(SDL_ALPHA_OPAQUE), r(r), g(g), b(b)
    {
    }
    
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

class LCDWindow
{
public:
    LCDWindow();
    ~LCDWindow();
    
    void init();
    void draw(std::vector<Pixel>& pixels, uint8_t win_pos_x, uint8_t win_pos_y);
    void set_viewport_pos(uint16_t x, uint16_t y) { m_x_origin=x; m_y_origin=y; }
    SDL_Window* m_window;
    
private:
    SDL_Renderer* m_renderer;
    std::vector<std::vector<uint8_t>> m_pixels;
    std::vector<colour> m_colours;
    uint16_t m_x_origin;
    uint16_t m_y_origin;
};

class LCDControlReg
{
public:
    LCDControlReg():
        m_value(0)
    {
    }
    
    void write(uint8_t value) { m_value=value; }
    uint8_t read() { return m_value; }
    
    bool get_lcd_operation() { return m_value & (1<<7); }
    uint16_t get_window_tile_table_addr() { return m_value & (1<<6) ? 0x9C00 : 0x9800; }
    bool get_window_display() { return m_value & (1<<5); }
    uint16_t get_tile_patt_table_addr() { return m_value & (1<<4) ? 0x8000 : 0x8800; }
    uint16_t get_bgrnd_tile_table_addr() { return m_value & (1<<3) ? 0x9c00 : 0x9800; }
    uint8_t get_sprite_size() { return m_value & (1<<2) ? 16 : 8; }
    uint8_t get_colour_0_transp() { return m_value & (1<<1) ? 1 : 0; }
    bool background_display() { return m_value & 1; }
    
private:
    uint8_t m_value;
};

class LCDStatReg
{
public:
    LCDStatReg():
    m_value(0)
    {
    }
    
    void write(uint8_t value) { m_value=value; }
    uint8_t read() { return m_value; }
    
private:
    uint8_t m_value;
};

class Z80;

class LCD: public MemoryManager
{
    public:
        LCD():
            m_display(), m_curr_scanline(0), m_scroll_x(0), m_scroll_y(0), m_win_pos_x(0), m_win_pos_y(0), m_last_scan_change_cycles(0), m_proc(nullptr), m_cmp_line(0)
        {
            m_data = std::vector<uint8_t>(0x2000, 0);
            
            for (size_t i=0; i<4; ++i)
            {
                m_bgrnd_pallette.push_back(i);
                m_obj_pallette_0.push_back(i);
                m_obj_pallette_1.push_back(i);
            }
        }
    
        void write8(uint16_t addr, uint8_t value);
        uint8_t read8(uint16_t addr);
    
        uint16_t read16(uint16_t addr);
        void write16(uint16_t addr, uint16_t value);
    
        void show_display();
        void draw();
        void tick(size_t curr_cycles);
    
        Z80* m_proc; /////HACK HACK HACK
    
    private:
        LCDWindow m_display;
        std::vector<uint8_t> m_data;
        std::vector<uint8_t> m_bgrnd_pallette;
        std::vector<uint8_t> m_obj_pallette_0;
        std::vector<uint8_t> m_obj_pallette_1;
        uint8_t m_curr_scanline;
        uint8_t m_cmp_line;
        uint8_t m_scroll_x;
        uint8_t m_scroll_y;
        LCDControlReg m_control_reg;
        LCDStatReg m_lcd_stat_reg;
        uint8_t m_win_pos_x;
        uint8_t m_win_pos_y;
        size_t m_last_scan_change_cycles;
};

#endif /* LCD_hpp */
