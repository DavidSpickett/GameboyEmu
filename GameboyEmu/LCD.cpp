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

namespace
{
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

    const uint8_t VBLANK_SCANLINE = 144;

    const int TILE_BYTES        = 16;
    const int TILES_PER_LINE    = 32;
}

LCD::LCD(int scale_factor):
m_last_tick_cycles(0),
m_lcd_line_cycles(0),
m_scale_factor(scale_factor),
m_curr_scanline(145),
m_colours{colour(0xff, 0xff, 0xff), colour(0xb9, 0xb9, 0xb9),
          colour(0x6b, 0x6b, 0x6b), colour(0x00, 0x00, 0x00)},
m_lcd_stat(0),
m_scroll_y(0),
m_scroll_x(0),
m_cmpline(0),
m_winposy(0),
m_winposx(0)
{
    m_sdl_width = LCD_WIDTH*m_scale_factor;
    m_sdl_height = LCD_HEIGHT*m_scale_factor;
    
    init_array(m_bgrd_pal);
    init_array(m_obj_pal_0);
    init_array(m_obj_pal_1);
    init_array(m_pixel_data);
    init_array(m_data);
    
    set_mode(LCD_MODE_VBLANK);
}

void LCD::set_mode(uint8_t mode)
{
    m_lcd_stat = (m_lcd_stat & ~3) | mode;
}

void LCD::SDLSaveImage(std::string filename)
{
    SDL_Surface *temp_sur = SDL_CreateRGBSurface(0, m_sdl_width, m_sdl_height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    SDL_RenderReadPixels(m_renderer, NULL, SDL_PIXELFORMAT_ARGB8888,
                         temp_sur->pixels, temp_sur->pitch);
    
    SDL_SaveBMP(temp_sur, filename.c_str());
    SDL_FreeSurface(temp_sur);
}

void LCD::SDLClear()
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

void LCD::SDLDraw()
{
    SDL_Rect r;
    r.h = m_scale_factor;
    r.w = m_scale_factor;
    r.x = 0;
    r.y = m_curr_scanline*m_scale_factor;
    
    auto p_start(m_pixel_data.cbegin()+(m_curr_scanline*LCD_WIDTH));
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

LCDPalette LCD::make_palette(uint8_t value)
{
    LCDPalette ret;
    for (auto i=0; i<4; ++i)
    {
        ret[i] = value & 0x3;
        value >>= 2;
    }
    
    return ret;
}

void LCD::SDLInit()
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
    
    SDLClear();
}

void LCD::update_sprite(uint16_t addr, uint8_t value)
{
    auto index = (addr-LCD_OAM_START) / SPRITE_INFO_BYTES;
    m_sprites[index].update(addr, value);
}

template <typename T>
void LCD::update_tile_row(uint16_t addr, T value)
{
    m_tile_rows[tile_index(addr)].update(addr, value);
}

//Note that tile also means sprite here, colour data is in the same format.
void LCD::tile_row_to_pixels(
    TileRow& tile_row, //Precalculated pixel unmapped colour values
    int startx, //Top left x co-ord of the tile.
    int starty, //Top left y co-ord of the tile.
    bool is_sprite, //Set to handle transparancy of colour 0
    bool flip_x, //Mirror X co-ords
    const LCDPalette& palette //Colour mapping
    )
{
    auto row_start = starty*LCD_WIDTH;
    
    for (auto shift=7; shift>= 0; --shift)
    {
        auto shift_diff = flip_x ? shift : (7-shift);
        auto newx = startx + shift_diff;
        if ((newx >= LCD_WIDTH) || (newx < 0))
        {
            continue;
        }
        
        auto c = tile_row.m_colours[shift];
        
        if (is_sprite && c==0)
        {
            //Colour 0 is always 'transparent' for sprites
            continue;
        }
        
        m_pixel_data[row_start+newx] = m_colours[palette[c]];
    }
}

void LCD::draw_sprites()
{
    const int sprite_bytes = 2*m_control_reg.sprite_size;
    
    std::for_each(m_sprites.begin(), m_sprites.end(), [=](const Sprite& sprite)
    {
        int sprite_row_offset = int(m_curr_scanline) - sprite.y;
        LCDPalette& palette = sprite.pallete_number ? m_obj_pal_1 : m_obj_pal_0;
        
        if (sprite.on_screen(m_curr_scanline, m_control_reg.sprite_size))
        {
            //Sprite pixels are stored in the same place as backgound tiles
            uint16_t tile_offset = sprite.pattern_number;
            if (m_control_reg.sprite_size == 16)
            {
                tile_offset &= ~1;
            }
            tile_offset *= sprite_bytes;
            
            auto sprite_offset = tile_offset;
            if (sprite.y_flip)
            {
                sprite_offset += (m_control_reg.sprite_size-sprite_row_offset-1)*2;
            }
            else
            {
                sprite_offset += sprite_row_offset*2;
            }
            
            tile_row_to_pixels(m_tile_rows[sprite_offset/2],
                               sprite.x, sprite.y + sprite_row_offset,
                               true,
                               sprite.x_flip,
                               palette);
        }
    });
}

void LCD::draw_window()
{
    if (m_control_reg.window_display)
    {
        if (m_curr_scanline >= m_winposy)
        {
            auto x = m_winposx-7;
            auto tile_index_row = (m_curr_scanline-m_winposy) / 8;
            auto tile_row_offset = (m_curr_scanline-m_winposy) % 8;
            
            for (auto tile_no=0; x<(LCD_WIDTH+8); x+=TILE_WIDTH, ++tile_no)
            {
                auto offset = m_control_reg.window_tile_table_addr+(tile_index_row*TILES_PER_LINE)+tile_no;
                auto tile_index = m_data[offset];
                
                if (m_control_reg.signed_tile_nos)
                {
                    tile_index += 128;
                }
                
                uint16_t tile_addr = tile_index*TILE_BYTES;
                
                tile_row_to_pixels(
                   m_tile_rows[(tile_addr + m_control_reg.bgrnd_tile_data_addr + (tile_row_offset*2))/2],
                   x, m_curr_scanline,
                   m_control_reg.colour_0_transparent,
                   false,
                   m_bgrd_pal);
            }
        }
    }
}

void LCD::draw_background()
{
    if (m_control_reg.background_display)
    {
        //There is only one line of tiles we are interested in since we're only doing one scanline
        //Note that Y wraps
        const uint16_t tile_row = (uint8_t(m_curr_scanline + m_scroll_y) / TILE_WIDTH);
        
        //Then we must start somewhere in that row
        uint8_t tile_row_offset = (m_scroll_x / TILE_WIDTH) % TILES_PER_LINE;
        
        //Which row of pixels within the tile
        const uint8_t tile_pixel_row = (m_curr_scanline + m_scroll_y) % TILE_WIDTH;
        
        for (uint8_t x=0; x<(LCD_WIDTH+8); x+=TILE_WIDTH, ++tile_row_offset)
        {
            if (tile_row_offset >= TILES_PER_LINE)
            {
                tile_row_offset %= TILES_PER_LINE;
            }
            
            //Get actual value of index from the table
            uint8_t tile_index = m_data[m_control_reg.bgrnd_tile_table_addr+(TILES_PER_LINE*tile_row)+tile_row_offset];
            
            if (m_control_reg.signed_tile_nos)
            {
                tile_index += 128;
            }
            
            //The final address to read pixel data from
            uint16_t tile_addr = tile_index*TILE_BYTES;
            
            auto tile_row_index = (tile_addr + m_control_reg.bgrnd_tile_data_addr + (tile_pixel_row*2))/2;
            tile_row_to_pixels(
               m_tile_rows[tile_row_index],
               x - (m_scroll_x % TILE_WIDTH), m_curr_scanline,
               false,
               false,
               m_bgrd_pal);
            
            //The idea being that these pixels are always the full row, that's why we
            //can incremement x by 8 each time. It gets the pixels before and ahead of x.
        }
    }
}

void LCD::tick(size_t curr_cycles)
{
    /*
     See:
     http://gameboy.mongenel.com/dmg/gbc_lcdc_timing.txt
     http://imrannazar.com/GameBoy-Emulation-in-JavaScript:-GPU-Timings
     
     4 clocks tick at 4MHz is one 'cycle'
     
     Mode 2 = 80  clocks = 20  cycles
     Mode 3 = 172 clocks = 43  cycles
     Mode 0 = 204 clocks = 51  cycles
     Total  = 456 clocks = 114 cycles per line.
     
     Mode 1 = 10*456 = 4560 clocks = 1140 cycles
     
     */
    const size_t CYCLES_PER_SCAN_LINE = 114;
    
    const size_t CYCLES_MODE_2_OAM_ACCESS  = 20;
    const size_t CYCLES_MODE_3_BOTH_ACCESS = 43 + CYCLES_MODE_2_OAM_ACCESS;
    const size_t CYCLES_MODE_0_HBLANK      = 51 + CYCLES_MODE_3_BOTH_ACCESS;
    
    uint8_t old_mode = m_lcd_stat & 3;
    auto new_mode = old_mode;
    auto old_scanline = m_curr_scanline;
    
    m_lcd_line_cycles += curr_cycles - m_last_tick_cycles;
    
    switch (old_mode)
    {
        case LCD_MODE_OAM_ACCESS:
        {
            if (m_lcd_line_cycles >= CYCLES_MODE_2_OAM_ACCESS)
            {
                new_mode = LCD_MODE_BOTH_ACCESS;
                //No interrupt for entering both accessed mode
            }
            break;
        }
        case LCD_MODE_BOTH_ACCESS:
            if (m_lcd_line_cycles >= CYCLES_MODE_3_BOTH_ACCESS)
            {
                new_mode = LCD_MODE_HBLANK;
                draw_background();
                draw_window();
                //TOOD: sprite priority
                draw_sprites();
                
                /*
                 State machine continues if LCD is off, games like Dr. Mario
                 disable it during transitions.
                */
                if (m_control_reg.lcd_operation)
                {
                    SDLDraw();
                }
            }
            break;
        case LCD_MODE_HBLANK:
            if (m_lcd_line_cycles >= CYCLES_MODE_0_HBLANK)
            {
                m_curr_scanline++;
                if (m_curr_scanline == VBLANK_SCANLINE)
                {
                    new_mode = LCD_MODE_VBLANK;
                    /*This interrupt type has a higher priority so it's
                     ok that the post_interrupt further down will be ignored.*/
                    post_int(LCD_VBLANK);
                }
                else
                {
                    new_mode = LCD_MODE_OAM_ACCESS;
                    //Take away cycles per line so we don't loose any overflow cycles
                    m_lcd_line_cycles = 0;
                }
            }
            break;
        case LCD_MODE_VBLANK:
            if (m_lcd_line_cycles >= CYCLES_PER_SCAN_LINE)
            {
                //Note that this is not 255! Backgrounds go to
                //that but the LCD only has 10 lines of VBLANK.
                if (m_curr_scanline == 153)
                {
                    new_mode = LCD_MODE_OAM_ACCESS;
                    m_curr_scanline = 0;
                }
                else
                {
                    m_curr_scanline++;
                }
                m_lcd_line_cycles = 0;
            }
            break;
    }
    
    if ((m_curr_scanline != old_scanline) &&
        (m_lcd_stat & (1<<6)))
    {
        if (m_curr_scanline == m_cmpline)
        {
            post_int(LCD_STAT);
        }
    }
    
    if (old_mode != new_mode)
    {
        set_mode(new_mode);
        if (new_mode != LCD_MODE_BOTH_ACCESS)
        {
            int bit = 0;
            switch (new_mode)
            {
                case LCD_MODE_OAM_ACCESS:
                    bit = 5;
                    break;
                case LCD_MODE_HBLANK:
                    bit = 3;
                    break;
                case LCD_MODE_VBLANK:
                    bit = 4;
            }
            
            if (m_lcd_stat & (1<<bit))
            {
                post_int(LCD_STAT);
            }
        }
    }
    
    m_last_tick_cycles = curr_cycles;
}

uint8_t LCD::read8(uint16_t addr)
{
    if ((addr >= LCD_MEM_START) && (addr < LCD_MEM_END))
    {
        /*Race Drivin' reads character RAM. I *think* it's waiting for
         an interrupt routine to fill in the data.*/
        return m_tile_rows[tile_index(addr)].get(addr);
    }
    else if ((addr >= LCD_BGRND_DATA) && (addr < LCD_MEM_END))
    {
        return m_data[addr-LCD_BGRND_DATA];
    }
    else if ((addr >= LCD_REGS_START) && (addr < LCD_REGS_END))
    {
        switch (addr)
        {
            case CURLINE:
                return m_curr_scanline;
            case LCDCONTROL:
                return m_control_reg.read();
            case LCDSTAT:
                return m_lcd_stat;
            case SCROLLY:
                return m_scroll_y;
            case SCROLLX:
                return m_scroll_x;
            case CMPLINE:
                return m_cmpline;
            case WINPOSY:
                return m_winposy;
            case WINPOSX:
                return m_winposx;
            default:
                throw std::runtime_error("Unknown LCD register read!");
        }
    }
    else
    {
        throw std::runtime_error(formatted_string("8 bit read of LCD addr 0x%04x", addr));
    }
}

void LCD::write8(uint16_t addr, uint8_t value)
{
    if ((addr >= LCD_MEM_START) && (addr < LCD_BGRND_DATA))
    {
        update_tile_row(addr, value);
    }
    else if ((addr >= LCD_BGRND_DATA) && (addr < LCD_MEM_END))
    {
        m_data[addr-LCD_BGRND_DATA] = value;
    }
    else if ((addr >= LCD_OAM_START) && (addr < LCD_OAM_END))
    {
        update_sprite(addr, value);
    }
    else if ((addr >= LCD_REGS_START) && (addr < LCD_REGS_END))
    {
        switch (addr)
        {
            case CURLINE:
                m_curr_scanline = 0;
                set_mode(LCD_MODE_OAM_ACCESS);
                break;
            case LCDCONTROL:
                m_control_reg.write(value);
                if (m_control_reg.lcd_operation && (m_window == NULL))
                {
                    SDLInit();
                }
                else if (!m_control_reg.lcd_operation)
                {
                    SDLClear();
                }
                break;
            case LCDSTAT:
                //Mode bits are read only
                m_lcd_stat = m_lcd_stat | (value & ~3);
                break;
            case BGRDPAL:
                m_bgrd_pal = make_palette(value);
                break;
            case OBJPAL0:
                m_obj_pal_0 = make_palette(value);
                break;
            case OBJPAL1:
                m_obj_pal_1 = make_palette(value);
                break;
            case SCROLLY:
                m_scroll_y = value;
                break;
            case SCROLLX:
                m_scroll_x = value;
                break;
            case CMPLINE:
                m_cmpline = value;
                break;
            case WINPOSY:
                m_winposy = value;
                break;
            case WINPOSX:
                m_winposx = value;
                break;
            default:
                throw std::runtime_error("Unknown LCD register write!");
        }
    }
    else
    {
        throw std::runtime_error(formatted_string("8 bit write to LCD addr 0x%04x of value 0x%02x", addr, value));
    }
}

uint16_t LCD::read16(uint16_t addr)
{
    //Don't allow reads of OAM or character RMAM
    if ((addr >= LCD_BGRND_DATA) && (addr < LCD_MEM_END))
    {
        uint16_t offset = addr-LCD_BGRND_DATA;
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
    if ((addr >= LCD_MEM_START) && (addr < LCD_BGRND_DATA))
    {
        update_tile_row(addr, value);
    }
    else if ((addr >= LCD_BGRND_DATA) && (addr < LCD_MEM_END))
    {
        uint16_t offset = addr-LCD_BGRND_DATA;
        m_data[offset] = value & 0xff;
        m_data[offset+1] = (value >> 8) & 0xff;
    }
    else if ((addr >= LCD_OAM_START) && (addr < LCD_OAM_END))
    {
        update_sprite(addr, value & 0xff);
        update_sprite(addr+1, (value >> 8) & 0xff);
    }
    else
    {
        throw std::runtime_error(formatted_string("16 bit write to LCD addr 0x%04x of value 0x%04x", addr, value));
    }
}
