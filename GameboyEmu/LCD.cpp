//
//  LCD.cpp
//  GameboyEmu
//
//  Created by David Spickett on 28/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "LCD.hpp"

//Video memory is 256x256 but the viewport is smaller and moves around
const size_t LCD_WIDTH  = 160;
const size_t LCD_HEIGHT = 144;

std::string Tile::to_string() const
{
    std::vector<Pixel> pixels = to_pixels();
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

std::vector<Pixel> Tile::to_pixels() const
{
    std::vector<Pixel> pixels;
    
    for (size_t row=0; row!=8; ++row)
    {
        uint8_t b1 = data[row*2];
        uint8_t b2 = data[(row*2)+1];
        
        for (uint8_t shift=7; shift>0; --shift)
        {
            uint8_t lsb = (b1 >> shift) & 0x1;
            uint8_t msb = (b2 >> shift) & 0x1;
            uint8_t c = (msb << 1) | lsb;
            
            pixels.push_back(Pixel(x+(7-shift), y+row, c));
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
    
    //Get window surface
    m_surface = SDL_GetWindowSurface(m_window);
    
    m_colours.push_back(SDL_MapRGB(m_surface->format, 0x00, 0x00, 0x00)); //black
    m_colours.push_back(SDL_MapRGB(m_surface->format, 0x6B, 0x6B, 0x6B)); //darker grey
    m_colours.push_back(SDL_MapRGB(m_surface->format, 0xB9, 0xB9, 0xB9)); //lighter grey
    m_colours.push_back(SDL_MapRGB(m_surface->format, 0xFF, 0xFF, 0xFF)); //white
}

namespace
{
    SDL_Rect make_pixel_rect(int x, int y)
    {
        SDL_Rect r;
        r.h = 1;
        r.w = 1;
        r.x = x;
        r.y = y;
        return r;
    }
}

void LCDWindow::draw(std::vector<Pixel>& pixels)
{
    if (m_window == NULL)
    {
        throw std::runtime_error("Cannot draw, the LCD window has not been init.");
    }
    
    //Set new pixel values in array
    std::vector<Pixel>::const_iterator it = pixels.begin();
    for (; it != pixels.end(); ++it)
    {
        m_pixels[it->y][it->x] = it->c;
    }

    for (int y=m_y_origin; y!=(m_y_origin+LCD_HEIGHT); ++y)
    {
        for (int x=m_x_origin; x!=(m_x_origin+LCD_WIDTH); ++x)
        {
            SDL_Rect r = make_pixel_rect(x, y);
            if (SDL_FillRect(m_surface, &r, m_colours[m_pixels[y][x]]) != 0)
            {
                throw std::runtime_error("Something went wrong drawing a pixel");
            }
        }
    }
    
    //Draw
    if (SDL_UpdateWindowSurface(m_window) != 0)
    {
        throw std::runtime_error("Something went wrong updating screen.");
    }
    
    SDL_Delay(2000);
}

LCDWindow::~LCDWindow()
{
    if (m_window != NULL)
    {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
    }
}

void LCD::draw()
{
    std::vector<Tile> tiles;
    
    //Assume bg map 1 for the time being
    uint16_t bg_map_1_start = 0x9800;
    uint16_t char_ram_start = 0x8000;
    
    //1024 bytes where each byte represents an index into the character RAM
    //So there are 32x32 indexes, each pointing to an 8x8 character
    //256*256 == (32*32)*(8*8)
    
    for (size_t index=0; index != 1024; ++index)
    {
        uint8_t ptr_val = m_data[bg_map_1_start+index];
        uint16_t char_addr = char_ram_start + ptr_val;
        
        std::vector<uint8_t> tile_data;
        for (size_t i=0; i!=16; ++i)
        {
            tile_data.push_back(m_data[char_addr+i]);
        }
        
        //TODO: double height mode
        tiles.push_back(Tile(index%32, index/32, 8, tile_data));
    }
    
    //Now we have all the tiles, convert them into pixel co-ords
    std::vector<Pixel> pixels;
    
    std::vector<Tile>::const_iterator it = tiles.begin();
    for (; it != tiles.end(); ++it)
    {
        printf("%s", it->to_string().c_str());
        printf("%s", "\n\n");
        std::vector<Pixel> ps = it->to_pixels();
        pixels.reserve(pixels.size() + distance(ps.begin(), ps.end()));
        pixels.insert(pixels.end(), ps.begin(), ps.end());
    }
    
    m_display.draw(pixels);
}

void LCD::show_display()
{
    m_display.init();
}

uint8_t LCD::read8(uint16_t addr)
{
    printf("8 bit read from addr: 0x%04x\n", addr);
    return m_data[addr-m_address_range.start];
}

void LCD::write8(uint16_t addr, uint8_t value)
{
    printf("8 bit write to addr: 0x%04x value: 0x%02x\n", addr, value);
    m_data[addr-m_address_range.start] = value;
}
