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
    
    std::vector<Pixel> to_pixels(const std::vector<uint8_t>& pallette) const;
    std::string to_string(const std::vector<uint8_t>& pallette) const;
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

const uint16_t LCDCONTROL = 0xff40;
const uint16_t LCDSTAT    = 0xff41;
const uint16_t SCROLLY    = 0xff42;
const uint16_t SCROLLX    = 0xff43;
const uint16_t CURLINE    = 0xff44;
const uint16_t BGRDPAL    = 0xff47;
const uint16_t OBJPAL0    = 0xff48;
const uint16_t OBJPAL1    = 0xff49;
const uint16_t WINPOSY    = 0xff4a; //Yes, Y is first.
const uint16_t WINPOSX    = 0xff4b;

class LCDControlReg
{
public:
    LCDControlReg(uint8_t value):
        m_value(value)
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
typedef std::vector<uint8_t> LCDPallette;

class LCD: public MemoryManager
{
    public:
        LCD():
            m_display(),  m_proc(nullptr)
        {
            m_data.resize(LCD_MEM_END-LCD_MEM_START, 0);
            m_oam_data.resize(LCD_OAM_END-LCD_OAM_START, 0);
            m_registers.resize(LCD_REGS_END-LCD_REGS_START, 0);
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
        std::vector<uint8_t> m_oam_data;
        std::vector<uint8_t> m_registers;
        uint8_t m_last_scan_change_cycles;

        uint16_t get_regs_addr(uint16_t addr) { return addr-LCD_REGS_START; }
    
        LCDControlReg get_control_reg()
        {
            return LCDControlReg(m_registers[get_regs_addr(LCDCONTROL)]);
        }
        uint8_t get_scroll_x()
        {
            return m_registers[get_regs_addr(SCROLLX)];
        }
        uint8_t get_scroll_y()
        {
            return m_registers[get_regs_addr(SCROLLY)];
        }
        uint8_t get_winpos_x()
        {
            return m_registers[get_regs_addr(WINPOSX)];
        }
        uint8_t get_winpos_y()
        {
            return m_registers[get_regs_addr(WINPOSY)];
        }
        uint8_t get_curr_scanline()
        {
            return m_registers[get_regs_addr(CURLINE)];
        }
        void set_curr_scanline(uint8_t value)
        {
            m_registers[get_regs_addr(CURLINE)] = value;
        }
        uint8_t inc_curr_scanline()
        {
            uint16_t addr = get_regs_addr(CURLINE);
            uint8_t val = m_registers[addr];
            val++;
            m_registers[addr] = val;
            return val;
        }
    
    
        const LCDPallette get_pallete(uint16_t addr);
        const LCDPallette get_bgrnd_pallette() { return get_pallete(BGRDPAL); }
        const LCDPallette get_obj_pal0() { return get_pallete(OBJPAL0); }
        const LCDPallette get_obj_pal1() { return get_pallete(OBJPAL1); }
    
        void do_after_reg_write(uint16_t addr);
        void do_after_reg_write16(uint16_t addr);
    
        uint8_t get_reg8(uint16_t addr) { return m_registers[get_regs_addr(addr)]; }
        void set_reg8(uint16_t addr, uint8_t value)
        {
            m_registers[get_regs_addr(addr)] = value;
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
