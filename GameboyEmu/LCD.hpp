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
#include <array>
#include <SDL2/SDL.h>
#include "MemoryManager.hpp"

const size_t LCD_WIDTH  = 160;
const size_t LCD_HEIGHT = 144;

const int SPRITE_INFO_BYTES = 4;
const int TILE_WIDTH  = 8;

using LCDPalette = std::array<uint8_t, 4>;
using OAMData = std::array<uint8_t, LCD_OAM_END-LCD_OAM_START>;
using LCDData = std::array<uint8_t, LCD_MEM_END-LCD_MEM_START>;

class Sprite
{
public:
    explicit Sprite(OAMData::const_iterator start):
        m_data(start)
    {}
    
    Sprite& operator++()
    {
        m_data += SPRITE_INFO_BYTES;
        return *this;
    }
    
    int get_y() { return int(*m_data) - 16; }
    int get_x() { return int(*(m_data+1)) - TILE_WIDTH; }
    uint8_t get_pattern_number() { return *(m_data+2); }
    
    bool get_priority() { return get_flag(7); }
    bool get_y_flip() { return get_flag(6); }
    bool get_x_flip() { return get_flag(5); }
    bool get_palette_number() { return get_flag(4); }
    
    std::string to_str()
    {
        return formatted_string("Sprite at X:%d Y:%x priority:%d xflip:%d yflip:%d palettenum:%d",
                get_x(), get_y(), get_priority(), get_x_flip(),
                get_y_flip(), get_palette_number());
    }
    
private:
    bool get_flag(uint8_t pos) { return *(m_data+3) & (1<<pos); }
    OAMData::const_iterator m_data;
};

struct Pixel
{
    Pixel(int x, int y, uint8_t c):
    x(x), y(y), c(c)
    {
    }
    
    int x;
    int y;
    uint8_t c;
};

struct colour
{
    colour(uint8_t r, uint8_t g, uint8_t b):
    a(SDL_ALPHA_OPAQUE), r(r), g(g), b(b)
    {
    }
    
    colour():
    a(SDL_ALPHA_OPAQUE), r(255), g(255), b(255)
    {
    }
    
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

const uint16_t LCDCONTROL = 0xff40;
const uint16_t LCDSTAT    = 0xff41;
const uint16_t SCROLLY    = 0xff42;
const uint16_t SCROLLX    = 0xff43;
const uint16_t CURLINE    = 0xff44;
const uint16_t CMPLINE    = 0xff45;
const uint16_t BGRDPAL    = 0xff47;
const uint16_t OBJPAL0    = 0xff48;
const uint16_t OBJPAL1    = 0xff49;
const uint16_t WINPOSY    = 0xff4a; //Yes, Y is first.
const uint16_t WINPOSX    = 0xff4b;

const uint8_t LCD_MODE_HBLANK      = 0;
const uint8_t LCD_MODE_VBLANK      = 1;
const uint8_t LCD_MODE_OAM_ACCESS  = 2;
const uint8_t LCD_MODE_BOTH_ACCESS = 3;

class LCDControlReg
{
public:
    LCDControlReg(uint8_t value):
        m_value(value)
    {}
    
    void write(uint8_t value) { m_value=value; }
    uint8_t read()            { return m_value; }
    
    bool get_lcd_operation()              { return m_value & (1<<7); }
    uint16_t get_window_tile_table_addr() { return (m_value & (1<<6)) ? 0x9C00-LCD_MEM_START : 0x9800-LCD_MEM_START; }
    bool get_window_display()             { return m_value & (1<<5); }
    uint16_t get_bgrnd_tile_data_addr()   { return (m_value & (1<<4)) ? 0x8000-LCD_MEM_START : 0x8800-LCD_MEM_START; }
    uint16_t get_bgrnd_tile_table_addr()  { return (m_value & (1<<3)) ? 0x9c00-LCD_MEM_START : 0x9800-LCD_MEM_START; }
    uint8_t get_sprite_size()             { return (m_value & (1<<2)) ? 16 : 8; }
    uint8_t get_colour_0_transp()         { return (m_value & (1<<1)) ? 1 : 0; }
    bool background_display()             { return (m_value) & 1; }
    
private:
    uint8_t m_value;
};

class Z80;

class LCD: public MemoryManager
{
    public:
        explicit LCD(int scale_factor);
        ~LCD()
        {
            if (m_window != NULL)
            {
                SDL_DestroyRenderer(m_renderer);
                SDL_DestroyWindow(m_window);
                SDL_Quit();
            }
        }
    
        void write8(uint16_t addr, uint8_t value);
        uint8_t read8(uint16_t addr);
    
        uint16_t read16(uint16_t addr);
        void write16(uint16_t addr, uint16_t value);
    
        void SDLSaveImage(std::string filename);
    
        void tick(size_t curr_cycles);
    
        Z80* m_proc; //Probably not ideal
    
    private:
        SDL_Renderer* m_renderer;
        SDL_Window* m_window;
        std::array<colour, 4> m_colours;
        int m_scale_factor;
    
        int m_sdl_height;
        int m_sdl_width;
    
        LCDControlReg m_control_reg;
        LCDData m_data;
        OAMData m_oam_data;
        std::array<uint8_t, LCD_REGS_END-LCD_REGS_START> m_registers;
        std::array<colour, LCD_HEIGHT*LCD_WIDTH> m_pixel_data;
        size_t m_last_tick_cycles;
        size_t m_lcd_line_cycles;
        uint8_t m_curr_scanline;
    
        void SDLInit();
        void SDLDraw();
        void SDLClear();
    
        void draw_background();
        void draw_sprites();
        void draw_window();
    
        void tile_row_to_pixels(
            LCDData::const_iterator data_b,
            int startx, int starty,
            bool is_sprite,
            bool flip_x,
            const LCDPalette& palette);
    
        void set_mode(uint8_t mode)
        {
            set_reg8(LCDSTAT, (get_reg8(LCDSTAT) & ~3) | mode);
            //printf("Set LCD mode to %d\n", mode);
        }

        LCDPalette get_palette(uint8_t addr);
        LCDPalette m_bgrd_pal;
        LCDPalette m_obj_pal_0;
        LCDPalette m_obj_pal_1;
    
        uint8_t get_reg8(uint16_t addr)
        {
            return m_registers[addr-LCD_REGS_START];
        }
    
        void set_reg8(uint16_t addr, uint8_t value)
        {
            m_registers[addr-LCD_REGS_START] = value;
        }
    
        uint16_t get_reg16(uint16_t addr)
        {
            return get_reg8(addr) | (get_reg8(addr+1) << 8);
        }
        void set_reg16(uint16_t addr, uint16_t value)
        {
            set_reg8(addr, value);
            set_reg8(addr+1, value >> 8);
        }
};

#endif /* LCD_hpp */
