//
//  SDLApp.hpp
//  GameboyEmu
//
//  Created by David Spickett on 01/08/2017.
//  Copyright © 2017 David Spickett. All rights reserved.
//

#ifndef SDLApp_hpp
#define SDLApp_hpp

#include <SDL2/SDL.h>
#include <string>
#include <array>
#include "utils.hpp"

const size_t LCD_WIDTH      = 160;
const size_t LCD_HEIGHT     = 144;

struct colour
{
    colour(uint8_t r, uint8_t g, uint8_t b):
    a(SDL_ALPHA_OPAQUE), r(r), g(g), b(b)
    {}
    
    colour():
    a(SDL_ALPHA_OPAQUE), r(255), g(255), b(255)
    {}
    
    uint8_t r, g, b, a;
};

class SDLApp
{
public:
    explicit SDLApp(int scale_factor):
        m_scale_factor(scale_factor),
        m_sdl_width(LCD_WIDTH*scale_factor),
        m_sdl_height(LCD_HEIGHT*scale_factor)
    {
        init_array(m_pixel_data);
    }
    
    ~SDLApp()
    {
        if (m_window != NULL)
        {
            SDL_DestroyRenderer(m_renderer);
            SDL_DestroyWindow(m_window);
            SDL_Quit();
        }
    }
    void SaveImage(std::string filename);
    void Init();
    void Draw(uint8_t curr_scanline);
    void Clear();
    
    std::array<colour, LCD_HEIGHT*LCD_WIDTH> m_pixel_data;
    
private:
    SDL_Renderer* m_renderer;
    SDL_Window* m_window;
    
    int m_scale_factor;
    int m_sdl_height;
    int m_sdl_width;
};

#endif /* SDLApp_hpp */
