//
//  LCD.hpp
//  GameboyEmu
//
//  Created by David Spickett on 28/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef LCD_hpp
#define LCD_hpp

#include <SDL2/SDL.h>
#include "MemoryManager.hpp"

const size_t LCD_WIDTH      = 160;
const size_t LCD_HEIGHT     = 144;
const int TILE_WIDTH        = 8;
const int SPRITE_INFO_BYTES = 4;

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

using LCDPalette = std::array<uint8_t, 4>;
using OAMData = std::array<uint8_t, LCD_OAM_END-LCD_OAM_START>;
using LCDData = std::array<uint8_t, LCD_MEM_END-LCD_BGRND_DATA>;

struct Sprite
{
    Sprite():
        x(-999), y(-999), pattern_number(0), priority(false),
        y_flip(false), x_flip(false), pallete_number(false)
    {}
    
    void update(uint16_t addr, uint8_t value)
    {
        switch (addr % SPRITE_INFO_BYTES)
        {
            case 0:
                y = int(value) - 16;
                break;
            case 1:
                x = int(value) - TILE_WIDTH;
                break;
            case 2:
                pattern_number = value;
                break;
            case 3:
                priority       = value & (1<<7);
                y_flip         = value & (1<<6);
                x_flip         = value & (1<<5);
                pallete_number = value & (1<<4);
                break;
        }
    }
    
    bool on_screen(uint8_t curr_scanline, int sprite_height) const
    {
        return (curr_scanline >= y) &&
            (curr_scanline < (y+sprite_height)) && (x > -TILE_WIDTH);
    }
    
    int x;
    int y;
    uint8_t pattern_number;
    bool priority;
    bool y_flip;
    bool x_flip;
    bool pallete_number;
    
    std::string to_str()
    {
        return formatted_string("Sprite at X:%d Y:%x priority:%d xflip:%d yflip:%d palettenum:%d",
                                x, y, priority, x_flip,
                                y_flip, pallete_number);
    }
};

using LCDSprites = std::array<Sprite, 40>;

struct TileRow
{
    TileRow():
        m_colours{0, 0, 0, 0, 0, 0, 0, 0},
        m_lsbs(0),
        m_msbs(0)
    {}
    std::array<uint8_t, 8> m_colours;
    
    void update(uint16_t addr, uint16_t value)
    {
        m_lsbs = value;
        m_msbs = value >> 8;
        _update();
    }
    
    void update(uint16_t addr, uint8_t value)
    {
        if (addr & 1)
        {
            m_msbs = value;
        }
        else
        {
            m_lsbs = value;
        }
        _update();
    }
private:
    void _update()
    {
        for (auto shift=0; shift<8; ++shift)
        {
            auto lsb = (m_lsbs >> shift) & 1;
            auto msb = (m_msbs >> shift) & 1;
            m_colours[shift] = (msb << 1) | lsb;
        }
    }
    
private:
    uint8_t m_lsbs;
    uint8_t m_msbs;
};

using TileRows = std::array<TileRow, (LCD_BGRND_DATA-LCD_MEM_START)/2>;

struct LCDControlReg
{
    LCDControlReg():
        m_value(0)
    {}
    
    uint8_t read()
    {
        return m_value;
    }
    
    void write(uint8_t value)
    {
        m_value=value;
        
        lcd_operation          = m_value & (1<<7);
        window_tile_table_addr = (m_value & (1<<6)) ? 0x9C00-LCD_BGRND_DATA : 0x9800-LCD_BGRND_DATA;
        window_display         = m_value & (1<<5);
        bgrnd_tile_data_addr   = (m_value & (1<<4)) ? 0x8000-LCD_MEM_START : 0x8800-LCD_MEM_START;
        bgrnd_tile_table_addr  = (m_value & (1<<3)) ? 0x9c00-LCD_BGRND_DATA : 0x9800-LCD_BGRND_DATA;
        sprite_size            = (m_value & (1<<2)) ? 16 : 8;
        colour_0_transparent   = (m_value & (1<<1)) == 0;
        background_display     = (m_value) & 1;
        signed_tile_nos        = (m_value & (1<<4)) == 0;
    }
    
    bool lcd_operation;
    uint16_t window_tile_table_addr;
    bool window_display;
    uint16_t bgrnd_tile_data_addr;
    uint16_t bgrnd_tile_table_addr;
    uint8_t sprite_size;
    bool colour_0_transparent;
    bool background_display;
    bool signed_tile_nos;
    
private:
    uint8_t m_value;
};

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
    
    private:
        SDL_Renderer* m_renderer;
        SDL_Window* m_window;
        std::array<colour, 4> m_colours;
        int m_scale_factor;
    
        int m_sdl_height;
        int m_sdl_width;
    
        LCDControlReg m_control_reg;
        LCDData m_data;
        LCDSprites m_sprites;
        TileRows m_tile_rows;
        std::array<colour, LCD_HEIGHT*LCD_WIDTH> m_pixel_data;
        size_t m_last_tick_cycles;
        size_t m_lcd_line_cycles;
        uint8_t m_curr_scanline;
    
        uint8_t m_lcd_stat;
        uint8_t m_scroll_y;
        uint8_t m_scroll_x;
        uint8_t m_cmpline;
        uint8_t m_winposy;
        uint8_t m_winposx;
    
        void SDLInit();
        void SDLDraw();
        void SDLClear();
    
        void draw_background();
        void draw_sprites();
        void draw_window();
    
        void tile_row_to_pixels(
            TileRow& tile_row,
            int startx, int starty,
            bool is_sprite,
            bool flip_x,
            const LCDPalette& palette);
    
        void update_sprite(uint16_t addr, uint8_t value);
        template <typename T>
        void update_tile_row(uint16_t addr, T value);
        void set_mode(uint8_t mode);

        LCDPalette make_palette(uint8_t addr);
        LCDPalette m_bgrd_pal;
        LCDPalette m_obj_pal_0;
        LCDPalette m_obj_pal_1;
};

#endif /* LCD_hpp */
