//
//  SDLApp.cpp
//  GameboyEmu
//
//  Created by David Spickett on 01/08/2017.
//  Copyright © 2017 David Spickett. All rights reserved.
//

#include "SDLApp.hpp"

void SDLApp::SaveImage(std::string filename)
{
    SDL_Surface *temp_sur = SDL_CreateRGBSurface(0, m_sdl_width, m_sdl_height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    SDL_RenderReadPixels(m_renderer, NULL, SDL_PIXELFORMAT_ARGB8888,
                         temp_sur->pixels, temp_sur->pitch);
    
    SDL_SaveBMP(temp_sur, filename.c_str());
    SDL_FreeSurface(temp_sur);
}

void SDLApp::Clear()
{
    //Clear whole screen when LCD is disabled.
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    
    SDL_Rect r;
    r.h = m_sdl_height;
    r.w = m_sdl_width;
    r.x = 0;
    r.y = 0;
    
    SDL_RenderFillRect(m_renderer, &r);
    SDL_RenderPresent(m_renderer);
}

void SDLApp::Draw(uint8_t curr_scanline)
{
    SDL_Rect r;
    r.h = m_scale_factor;
    r.w = m_scale_factor;
    r.x = 0;
    r.y = curr_scanline*m_scale_factor;
    
    auto p_start(m_pixel_data.cbegin()+(curr_scanline*LCD_WIDTH));
    auto p_end(p_start+LCD_WIDTH);
    for ( ; p_start != p_end; ++p_start, r.x+=m_scale_factor)
    {
        SDL_SetRenderDrawColor(m_renderer, p_start->r, p_start->g, p_start->b, p_start->a);
        SDL_RenderFillRect(m_renderer, &r);
    }
    
    //Draw
    //SDL_RenderPresent(m_renderer);
    static auto delay = 0;
    if (delay == 200)
    {
        SDL_RenderPresent(m_renderer);
        delay = 0;
    }
    delay++;
}

void SDLApp::Init()
{
    if (m_window == NULL)
    {
        //Initialize SDL
        if(SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            throw std::runtime_error(
                 formatted_string("SDL could not initialize! SDL_Error: %s\n",
                                  SDL_GetError()));
        }
        
        //Create window
        m_window = SDL_CreateWindow("Gameboy Emulator",
                                    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                    m_sdl_width, m_sdl_height,
                                    SDL_WINDOW_SHOWN);
        
        if(m_window == NULL)
        {
            throw std::runtime_error(formatted_string(
              "Window could not be created! SDL_Error: %s\n", SDL_GetError()));
        }
        
        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
        
        Clear();
    }
}
