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

const size_t LCD_WIDTH  = 160;
const size_t LCD_HEIGHT = 144;

typedef std::vector<uint8_t> LCDPallette;

class Sprite
{
public:
    Sprite(std::vector<uint8_t>::const_iterator start):
        m_data(start)
    {
    }
    
    uint8_t get_y() { return *m_data; }
    uint8_t get_x() { return *(m_data+1); }
    uint8_t get_pattern_number() { return *(m_data+2); }
    
    bool get_priority() { return get_flag(7); }
    bool get_y_flip() { return get_flag(6); }
    bool get_x_flip() { return get_flag(5); }
    bool get_pallette_number() { return get_flag(4); }
    
    std::string to_str()
    {
        return formatted_string("Sprite at X:%d Y:%x priority:%d xflip:%d yflip:%d pallettenum:%d",
                get_x(), get_y(), get_priority(), get_x_flip(),
                get_y_flip(), get_pallette_number());
    }
    
private:
    uint8_t get_flag(uint8_t pos) { return (*(m_data+3)) & (1<<pos); }
    std::vector<uint8_t>::const_iterator m_data;
};

struct Pixel
{
    Pixel(int x, int y, uint8_t c):
    x(x), y(y), c(c)
    {
    }
    
    bool operator < (const Pixel& p) const
    {
        return (c < p.c);
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

const uint16_t LCDCONTROL = 0xff40-LCD_REGS_START;
const uint16_t LCDSTAT    = 0xff41-LCD_REGS_START;
const uint16_t SCROLLY    = 0xff42-LCD_REGS_START;
const uint16_t SCROLLX    = 0xff43-LCD_REGS_START;
const uint16_t CURLINE    = 0xff44-LCD_REGS_START;
const uint16_t BGRDPAL    = 0xff47-LCD_REGS_START;
const uint16_t OBJPAL0    = 0xff48-LCD_REGS_START;
const uint16_t OBJPAL1    = 0xff49-LCD_REGS_START;
const uint16_t WINPOSY    = 0xff4a-LCD_REGS_START; //Yes, Y is first.
const uint16_t WINPOSX    = 0xff4b-LCD_REGS_START;

class LCDControlReg
{
public:
    LCDControlReg(uint8_t* value):
        m_value(value)
    {
    }
    
    LCDControlReg():
        m_value(NULL)
    {}
    
    void write(uint8_t value) { *m_value=value; }
    uint8_t read()            { return *m_value; }
    
    bool get_lcd_operation()              { return *m_value & (1<<7); }
    uint16_t get_window_tile_table_addr() { return *m_value & (1<<6) ? 0x9C00-LCD_MEM_START : 0x9800-LCD_MEM_START; }
    bool get_window_display()             { return *m_value & (1<<5); }
    uint16_t get_bgrnd_tile_data_addr()   { return *m_value & (1<<4) ? 0x8000-LCD_MEM_START : 0x8800-LCD_MEM_START; }
    uint16_t get_bgrnd_tile_table_addr()  { return *m_value & (1<<3) ? 0x9c00-LCD_MEM_START : 0x9800-LCD_MEM_START; }
    uint8_t get_sprite_size()             { return *m_value & (1<<2) ? 16 : 8; }
    uint8_t get_colour_0_transp()         { return *m_value & (1<<1) ? 1 : 0; }
    bool background_display()             { return *m_value & 1; }
    
private:
    uint8_t* m_value;
};

class Z80;

class LCD: public MemoryManager
{
    public:
        LCD(int scale_factor):
            m_proc(nullptr), m_last_scan_change_cycles(0), m_scale_factor(scale_factor)
        {
            m_data.resize(LCD_MEM_END-LCD_MEM_START, 0);
            m_oam_data.resize(LCD_OAM_END-LCD_OAM_START, 0);
            m_registers.resize(LCD_REGS_END-LCD_REGS_START, 0);
            m_pixel_data.resize(LCD_WIDTH*LCD_HEIGHT);
            
            m_control_reg = LCDControlReg(&m_registers[LCDCONTROL]);
            
            m_colours.push_back(colour(0xff, 0xff, 0xff));
            m_colours.push_back(colour(0xb9, 0xb9, 0xb9));
            m_colours.push_back(colour(0x6b, 0x6b, 0x6b));
            m_colours.push_back(colour(0x00, 0x00, 0x00));
            
            m_sdl_width = LCD_WIDTH*m_scale_factor;
            m_sdl_height = LCD_HEIGHT*m_scale_factor;
        }
    
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
    
        Z80* m_proc; /////HACK HACK HACK
    
    private:
        SDL_Renderer* m_renderer;
        SDL_Window* m_window;
        std::vector<colour> m_colours;
        int m_scale_factor;
    
        int m_sdl_height;
        int m_sdl_width;
    
        LCDControlReg m_control_reg;
        std::vector<uint8_t> m_data;
        std::vector<uint8_t> m_oam_data;
        std::vector<uint8_t> m_registers;
        std::vector<colour> m_pixel_data;
        size_t m_last_scan_change_cycles;
    
        void SDLInit();
        void SDLDraw(uint8_t curr_scanline);
        void draw_to_pixels();
    
        void tile_row_to_pixels(
            std::vector<uint8_t>::const_iterator data_b,
            int startx, int starty,
            int offsx, int offsy,
            bool is_sprite,
            const LCDPallette& pallette);

        uint8_t get_scroll_x()
        {
            return m_registers[SCROLLX];
        }
    
        uint8_t get_scroll_y()
        {
            return m_registers[SCROLLY];
        }
    
        uint8_t get_winpos_x()
        {
            return m_registers[WINPOSX];
        }
    
        uint8_t get_winpos_y()
        {
            return m_registers[WINPOSY];
        }
    
        uint8_t get_curr_scanline()
        {
            return m_registers[CURLINE];
        }
    
        void set_curr_scanline(uint8_t value)
        {
            m_registers[CURLINE] = value;
        }
    
        uint8_t inc_curr_scanline()
        {
            return ++m_registers[CURLINE];
        }
    
        const LCDPallette get_pallete(uint16_t addr);
        const LCDPallette get_bgrnd_pallette() { return get_pallete(BGRDPAL); }
        const LCDPallette get_obj_pal0() { return get_pallete(OBJPAL0); }
        const LCDPallette get_obj_pal1() { return get_pallete(OBJPAL1); }
    
        void do_after_reg_write(uint16_t addr);
        void do_after_reg_write16(uint16_t addr);
    
        uint16_t get_regs_addr(uint16_t addr) { return addr - LCD_REGS_START; }
    
        uint8_t get_reg8(uint16_t addr) { return m_registers[get_regs_addr(addr)]; }
        void set_reg8(uint16_t addr, uint8_t value)
        {
            addr = get_regs_addr(addr);
            m_registers[addr] = value;
            do_after_reg_write(addr);
        }
    
        //Might want to move these into a register block class
        // and do them in terms of write 8, so that later we can get callbacks
        // when a register changes.
        uint16_t get_reg16(uint16_t addr)
        {
            addr = get_regs_addr(addr);
            return m_registers[addr] | (m_registers[addr+1] << 8);
        }
        void set_reg16(uint16_t addr, uint16_t value)
        {
            addr = get_regs_addr(addr);
            m_registers[addr] = value;
            m_registers[addr+1] = value >> 8;
            do_after_reg_write16(addr);
        }
};

#endif /* LCD_hpp */
