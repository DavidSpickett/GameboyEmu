//
//  LCD.cpp
//  GameboyEmu
//
//  Created by David Spickett on 28/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "LCD.hpp"
#include "utils.hpp"
#include "Z80.hpp"

//Video memory is 256x256 but the viewport is smaller and moves around
const size_t LCD_WIDTH  = 160;
const size_t LCD_HEIGHT = 144;
//const size_t LCD_WIDTH  = 256;
//const size_t LCD_HEIGHT = 256;

std::string Tile::to_string(const std::vector<uint8_t>& pallette) const
{
    std::vector<Pixel> pixels = to_pixels(pallette);
    std::string ret;
    
    //Using an index to make splitting the lines easier
    for (size_t i=0; i != 56; ++i)
    {
        if ((i != 0) && ((i % 8) == 0))
        {
            ret += "\n";
        }
        
        ret += formatted_string("%d", pixels[i].c);
    }
    
    return ret;
}

namespace {
    bool non_zero(uint8_t val)
    {
        return val != 0;
    }
}

bool Tile::has_some_colour() const
{
    return std::find_if(data_b, data_e, non_zero) != data_e;
}

std::vector<Pixel> Tile::to_pixels(const std::vector<uint8_t>& pallette) const
{
    std::vector<Pixel> pixels;
    pixels.reserve(8*h);
    
    for (size_t row=0; row!=h; ++row)
    {
        uint8_t b1 = *(data_b + (row*2));
        uint8_t b2 = *(data_b + (row*2)+1);
        
        //Signed int!!
        for (int shift=7; shift>= 0; --shift)
        {
            uint8_t lsb = (b1 >> shift) & 0x1;
            uint8_t msb = (b2 >> shift) & 0x1;
            uint8_t c = (msb << 1) | lsb;
            
            //Pallette remaps colours
            pixels.push_back(Pixel(x+(7-shift), y+row, pallette[c]));
        }
    }
    
    return pixels;
}

LCDWindow::LCDWindow()
{
    std::vector<uint8_t> row(256, 0);
    for (size_t i=0; i<256; ++i)
    {
        m_pixels.push_back(row);
    }
}

void LCDWindow::init()
{
    //Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        throw std::runtime_error(
            formatted_string("SDL could not initialize! SDL_Error: %s\n", SDL_GetError()));
    }
    
    //Create window
    m_window = SDL_CreateWindow("Gameboy Emulator",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        LCD_WIDTH, LCD_HEIGHT,
        SDL_WINDOW_SHOWN);
    
    if(m_window == NULL)
    {
        throw std::runtime_error(formatted_string(
            "Window could not be created! SDL_Error: %s\n", SDL_GetError()));
    }
    
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);

    m_colours.push_back(colour(0xff, 0xff, 0xff));
    m_colours.push_back(colour(0xb9, 0xb9, 0xb9));
    m_colours.push_back(colour(0x6b, 0x6b, 0x6b));
    m_colours.push_back(colour(0x00, 0x00, 0x00));
    
    SDL_SetRenderDrawColor(m_renderer, m_colours[0].r, m_colours[0].g, m_colours[0].b, m_colours[0].a);
    SDL_RenderClear(m_renderer);
    SDL_RenderPresent(m_renderer);
}

void LCDWindow::draw(std::vector<Pixel>& pixels, uint8_t win_pos_x, uint8_t win_pos_y)
{
    if (m_window == NULL)
    {
        throw std::runtime_error("Cannot draw, the LCD window has not been init.");
    }
    
    SDL_SetRenderDrawColor(m_renderer, m_colours[0].r, m_colours[0].g, m_colours[0].b, m_colours[0].a);
    SDL_RenderClear(m_renderer);
    
    //Update what has changed
    std::vector<Pixel>::const_iterator it = pixels.begin();
    for (; it!=pixels.end(); ++it)
    {
        uint16_t x = it->x;
        uint16_t y = it->y;
        
        if ((x >= 256) || (x<0) || (y>=256) || (y<0))
        {
            //Skip off memory pixels
            continue;
        }

        m_pixels[y][x] = it->c;
            
        //TODO: window scrolling
//      if((x>=win_pos_x) && (x<(LCD_WIDTH+win_pos_x)) && (y>=win_pos_y) && ((y<(LCD_HEIGHT+win_pos_y))))
        if ((x < LCD_WIDTH) && (y < LCD_HEIGHT))
        {
            colour c = m_colours[it->c];
            SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
            SDL_RenderDrawPoint(m_renderer, x, y);
        }
    }
    
    //Draw
    SDL_RenderPresent(m_renderer);
}

LCDWindow::~LCDWindow()
{
    if (m_window != NULL)
    {
        SDL_DestroyWindow(m_window);
        SDL_DestroyRenderer(m_renderer);
        SDL_Quit();
    }
}

const LCDPallette LCD::get_pallete(uint16_t addr)
{
    LCDPallette ret;
    addr = get_regs_addr(addr);
    
    //Lowest bits first
    uint8_t regval = m_registers[addr];
    for (int i=0; i<4; ++i)
    {
        ret.push_back(regval & 0x3);
        regval >>= 2;
    }
    
    return ret;
}

void LCD::draw()
{
    LCDControlReg control_reg = get_control_reg();
    uint8_t scrollx = get_scroll_x();
    uint8_t scrolly = get_scroll_y();
    uint8_t winposx = get_winpos_x();
    uint8_t winposy = get_winpos_y();
    
    if (!control_reg.get_lcd_operation())
    {
        return;
    }
    
    //Assume bg map 1 for the time being
    uint16_t bg_map_start = control_reg.get_bgrnd_tile_table_addr();
    uint16_t char_ram_start = control_reg.get_tile_patt_table_addr();
    
    //1024 bytes where each byte represents an index into the character RAM
    //So there are 32x32 indexes, each pointing to an 8x8 character
    //256*256 == (32*32)*(8*8)
    
    //The sprite height can be doubled, halving the number of sprites
    uint8_t sprite_size = control_reg.get_sprite_size();
    uint8_t bytes_per_sprite = sprite_size * 2;
    
    //Generate background pixels
    std::vector<Pixel> pixels;
    pixels.reserve(1024*8*sprite_size);
    
    if (control_reg.background_display())
    {
        for (size_t index=0; index != 1024; ++index)
        {
            uint8_t ptr_val = m_data[bg_map_start+index-LCD_MEM_START];
            
            //In 16 height mode pointing to an even sprite no.
            //just points to the previous odd sprite no.
            if (sprite_size == 16)
            {
                ptr_val -= ptr_val % 2;
            }
            
            //TODO: should - addr depending on bg being used
            uint16_t char_addr = char_ram_start + (ptr_val*bytes_per_sprite) - LCD_MEM_START;
            
            //Note that the y scroll is minus because the y co-ordinite is inverted
            Tile t(((index % 32)*8)+scrollx,
                   ((index/32)*sprite_size)-scrolly,
                   sprite_size,
                   m_data.begin()+char_addr,
                   m_data.begin()+char_addr+bytes_per_sprite);
            
            //Renderer clears to white so don't bother with those tiles
            if (t.has_some_colour())
            {
                std::vector<Pixel> ps = t.to_pixels(get_bgrnd_pallette());
                //pixels.reserve(pixels.size() + distance(ps.begin(), ps.end()));
                pixels.insert(pixels.end(), ps.begin(), ps.end());
            }
        }
    }
    
    m_display.draw(pixels, winposx, winposy);
}

void LCD::tick(size_t curr_cycles)
{
    /*
    To explain the math for future reference:
    0.954uS per instruction cycle
    15.66m + 1.09m seconds to draw the whole frame (1.09m for vblank)
    That makes it 0.1087mS per scan line.
    Which is 113.941 instruction cycles, aka 114 cycles per scan line.
    */
    const size_t per_scan_line = 114;
    LCDControlReg control_reg = get_control_reg();
    uint8_t curr_scanline = get_curr_scanline();

    if (control_reg.get_lcd_operation() &&
        ((curr_cycles - m_last_scan_change_cycles) > per_scan_line))
    {
        //154 scan lines, 144 + 10 vblank period
        m_last_scan_change_cycles = curr_cycles;
        
        if (curr_scanline != 153)
        {
            curr_scanline = inc_curr_scanline();
            
            if (curr_scanline == 0x90)
            {
                if (m_proc != nullptr)
                {
                    //TODO: do all this in the proc itself?
                    if ((m_proc->mem.read8(0xffff) & 1) && m_proc->interrupt_enable)
                    {
                        //Save PC to stack
                        m_proc->sp.dec(2);
                        m_proc->mem.write16(m_proc->sp.read(), m_proc->pc.read());
                        //Then jump there.
                        m_proc->pc.write(0x0040);
                        
                        //Set occurred flag (bit 0)
                        m_proc->mem.write8(0xff0f, m_proc->mem.read8(0xff0f) | 1);
                        printf("-----V-Blank interrupt-----\n");
                        
                        //Disable ints until the program enables them again
                        m_proc->interrupt_enable = false;
                    }
                }

            }
        }
        else
        {
            set_curr_scanline(0);
            draw();
        }
        
        //TODO: tell window to draw that line
    }
}

void LCD::show_display()
{
    m_display.init();
}

uint8_t LCD::read8(uint16_t addr)
{
    //printf("8 bit read from LCD addr: 0x%04x\n", addr);
    
    if ((addr >= LCD_MEM_START) && (addr < LCD_MEM_END))
    {
        return m_data[addr-LCD_MEM_START];
    }
    else if ((addr >= LCD_REGS_START) && (addr < LCD_REGS_END))
    {
        return get_reg8(addr);
    }
    else if ((addr >= LCD_OAM_START) && (addr < LCD_OAM_END))
    {
        return m_oam_data[addr-LCD_OAM_START];
    }
    else
    {
        throw std::runtime_error(formatted_string("8 bit read of LCD addr 0x%04x", addr));
    }
}

void LCD::write8(uint16_t addr, uint8_t value)
{
    //printf("8 bit write to LCD addr: 0x%04x value: 0x%02x\n", addr, value);
    
    if ((addr >= LCD_MEM_START) && (addr < LCD_MEM_END))
    {
        m_data[addr-LCD_MEM_START] = value;
    }
    else if ((addr >= LCD_REGS_START) && (addr < LCD_REGS_END))
    {
        set_reg8(addr, value);
    }
    else if ((addr >= LCD_OAM_START) && (addr < LCD_OAM_END))
    {
        m_oam_data[addr-LCD_OAM_START] = value;
    }
    else
    {
        throw std::runtime_error(formatted_string("8 bit write to LCD addr 0x%04x of value 0x%02x", addr, value));
    }
}

uint16_t LCD::read16(uint16_t addr)
{
    if ((addr >= LCD_MEM_START) && (addr < LCD_MEM_END))
    {
        uint16_t offset = addr-LCD_MEM_START;
        uint16_t ret = (uint16_t(m_data[offset+1]) << 8) | uint16_t(m_data[offset]);
        return ret;
    }
    else if ((addr >= LCD_OAM_START) && (addr < LCD_OAM_END))
    {
        uint16_t offset = addr-LCD_OAM_START;
        uint16_t ret = (uint16_t(m_oam_data[offset+1]) << 8) | uint16_t(m_oam_data[offset]);
        return ret;
    }
    else if ((addr >= LCD_REGS_START) && (addr < LCD_REGS_END))
    {
        return get_reg16(addr);
    }
    else
    {
        throw std::runtime_error(formatted_string("16 bit read of LCD addr 0x%04x", addr));
    }
}

void LCD::write16(uint16_t addr, uint16_t value)
{
    if ((addr >= LCD_MEM_START) && (addr < LCD_MEM_END))
    {
        uint16_t offset = addr-LCD_MEM_START;
        m_data[offset] = value & 0xff;
        m_data[offset+1] = (value >> 8) & 0xff;
        //printf("16 bit write to LCD addr 0x%04x of value 0x%04x\n", addr, value);
    }
    else if ((addr >= LCD_OAM_START) && (addr < LCD_OAM_END))
    {
        uint16_t offset = addr-LCD_OAM_START;
        m_oam_data[offset] = value & 0xff;
        m_oam_data[offset+1] = (value >> 8) & 0xff;
    }
    else if ((addr >= LCD_REGS_START) && (addr < LCD_REGS_END))
    {
        set_reg16(addr, value);
    }
    else
    {
        throw std::runtime_error(formatted_string("16 bit write to LCD addr 0x%04x of value 0x%04x", addr, value));
    }
}

void LCD::do_after_reg_write(uint16_t addr)
{
    //React to settings being changed.
    
    switch (addr)
    {
        case LCDCONTROL:
        {
            LCDControlReg ctrl(get_control_reg());
            if (ctrl.get_lcd_operation() && (m_display.m_window == NULL))
            {
                show_display();
            }
            break;
        }
        case CURLINE:
            //Writing anything sets the register to 0
            set_curr_scanline(0);
            break;
    }
}

void LCD::do_after_reg_write16(uint16_t addr)
{
    do_after_reg_write(addr);
    do_after_reg_write(addr+1);
}
