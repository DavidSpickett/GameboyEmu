//
//  LCD.cpp
//  GameboyEmu
//
//  Created by David Spickett on 28/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "LCD.hpp"
#include "utils.hpp"

//Video memory is 256x256 but the viewport is smaller and moves around
//const size_t LCD_WIDTH  = 160;
//const size_t LCD_HEIGHT = 144;
const size_t LCD_WIDTH  = 256;
const size_t LCD_HEIGHT = 256;

std::string Tile::to_string(std::vector<uint8_t>& pallette) const
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

std::vector<Pixel> Tile::to_pixels(std::vector<uint8_t>& pallette) const
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

void LCD::draw()
{
    if (!m_control_reg.get_lcd_operation())
    {
        return;
    }
    
    //Assume bg map 1 for the time being
    uint16_t bg_map_start = m_control_reg.get_bgrnd_tile_table_addr();
    uint16_t char_ram_start = m_control_reg.get_tile_patt_table_addr();
    
    //1024 bytes where each byte represents an index into the character RAM
    //So there are 32x32 indexes, each pointing to an 8x8 character
    //256*256 == (32*32)*(8*8)
    
    //The sprite height can be doubled, halving the number of sprites
    uint8_t sprite_size = m_control_reg.get_sprite_size();
    uint8_t bytes_per_sprite = sprite_size * 2;
    
    //Generate background pixels
    std::vector<Pixel> pixels;
    pixels.reserve(1024*8*sprite_size);
    
    if (m_control_reg.background_display())
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
            Tile t(((index % 32)*8)+m_scroll_x,
                   ((index/32)*sprite_size)-m_scroll_y,
                   sprite_size,
                   m_data.begin()+char_addr,
                   m_data.begin()+char_addr+bytes_per_sprite);
            
            //Renderer clears to white so don't bother with those tiles
            if (t.has_some_colour())
            {
                std::vector<Pixel> ps = t.to_pixels(m_pallette);
                //pixels.reserve(pixels.size() + distance(ps.begin(), ps.end()));
                pixels.insert(pixels.end(), ps.begin(), ps.end());
            }
        }
    }
    
    m_display.draw(pixels, m_win_pos_x, m_win_pos_y);
}

void LCD::tick(size_t curr_cycles)
{
    /*
    To explain the math ofr future reference:
    0.954uS per instruction cycle
    15.66m + 1.09m seconds to draw the whole frame (1.09m for vblank)
    That makes it 0.1087mS per scan line.
    Which is 113.941 instruction cycles, aka 114 cycles per scan line.
    */
    const size_t per_scan_line = 114;

    if ((curr_cycles - m_last_scan_change_cycles) > per_scan_line)
    {
        //154 scan lines, 144 + 10 vblank period
        m_last_scan_change_cycles = curr_cycles;
        
        if (m_curr_scanline != 153)
        {
            m_curr_scanline++;
        }
        else
        {
            m_curr_scanline = 0;
            draw();
        }
        
        //TODO: tell window to draw that line
    }
    
    //Uncomment to speed through the bootstrap
    //m_curr_scanline = 0x90;
}

void LCD::show_display()
{
    m_display.init();
}

const uint16_t LCDCONTROL = 0xff40;
const uint16_t SCROLLY    = 0xff42;
const uint16_t SCROLLX    = 0xff43;
const uint16_t CURLINE    = 0xff44;
const uint16_t BGRDPAL    = 0xff47;
const uint16_t WINPOSX    = 0xff4a;
const uint16_t WINPOSY    = 0xff4b;

uint8_t LCD::read8(uint16_t addr)
{
    //printf("8 bit read from LCD addr: 0x%04x\n", addr);
    
    switch (addr)
    {
        case WINPOSX:
            return m_win_pos_x;
        case WINPOSY:
            return m_win_pos_y;
        case LCDCONTROL:
            return m_control_reg.read();
        case SCROLLX:
            return m_scroll_x;
        case SCROLLY:
            return m_scroll_y;
        case CURLINE:
            //draw();
            //return 0x90; //Bodge, pretend we're in vblank area
            return m_curr_scanline;
        case BGRDPAL:
        {
            //Not sure that anything will actually read this reg though...
            uint8_t val = 0;
            for (int i=0; i<4; ++i)
            {
                val |= m_pallette[0] << (i*8);
            }
            return val;
        }
        default:
            return m_data[addr-LCD_MEM_START];
    }
}

void LCD::write8(uint16_t addr, uint8_t value)
{
    //printf("8 bit write to LCD addr: 0x%04x value: 0x%02x\n", addr, value);
    
    switch (addr)
    {
        case WINPOSX:
            m_win_pos_x = value;
            break;
        case WINPOSY:
            m_win_pos_y = value;
            break;
        case LCDCONTROL:
            m_control_reg.write(value);
            if (m_control_reg.get_lcd_operation())
            {
                show_display();
            }
            break;
        case SCROLLX:
            m_scroll_x = value;
            break;
        case SCROLLY:
            m_scroll_y = value;
            break;
        case CURLINE:
            //Writing resets the register
            m_curr_scanline = 0;
            break;
        case BGRDPAL:
        {
            //Pallette
            for (int i=0; i<4; ++i)
            {
                m_pallette[i] = value & 0x3;
                value = value >> 2;
            }
            break;
        }
        default:
            m_data[addr-LCD_MEM_START] = value;
            break;
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
    else
    {
        throw std::runtime_error(formatted_string("16 bit write to LCD addr 0x%04x of value 0x%04x", addr, value));
    }
}
