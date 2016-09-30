//
//  LCD.cpp
//  GameboyEmu
//
//  Created by David Spickett on 28/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "LCD.hpp"

const size_t LCD_WIDTH  = 160;
const size_t LCD_HEIGHT = 144;

LCDWindow::LCDWindow()
{
    std::vector<uint8_t> row(LCD_WIDTH, 0);
    for (size_t i=0; i<LCD_HEIGHT; ++i)
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

void LCDWindow::draw()
{
    if (m_window == NULL)
    {
        throw std::runtime_error("Cannot draw, the LCD window has not been init.");
    }

    for (int y=0; y!=LCD_HEIGHT; ++y)
    {
        for (int x=0; x!=LCD_WIDTH; ++x)
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
    m_display.draw();
}

void LCD::show_display()
{
    m_display.init();
}

uint8_t LCD::read8(uint16_t addr)
{
    printf("8 bit read from addr: 0x%04x\n", addr);
    return 0;
}

void LCD::write8(uint16_t addr, uint8_t value)
{
    printf("8 bit write to addr: 0x%04x value: 0x%02x\n", addr, value);
}
