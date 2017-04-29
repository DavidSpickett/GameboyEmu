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
    template <typename T>
    void init_array(T& container)
    {
        std::fill(container.begin(), container.end(), typename T::value_type());
    }
}

LCD::LCD(int scale_factor):
m_proc(nullptr),
m_last_tick_cycles(0),
m_lcd_line_cycles(0),
m_scale_factor(scale_factor),
m_curr_scanline(145),
m_data(LCD_MEM_END-LCD_MEM_START, 0),
m_oam_data(LCD_OAM_END-LCD_OAM_START, 0),
m_registers(LCD_REGS_END-LCD_REGS_START, 0),
m_colours{colour(0xff, 0xff, 0xff), colour(0xb9, 0xb9, 0xb9), colour(0x6b, 0x6b, 0x6b), colour(0x00, 0x00, 0x00)}
{
    m_control_reg = LCDControlReg(&m_registers[LCDCONTROL]);
    
    m_sdl_width = LCD_WIDTH*m_scale_factor;
    m_sdl_height = LCD_HEIGHT*m_scale_factor;
    
    init_array(m_bgrd_pal);
    init_array(m_obj_pal_0);
    init_array(m_obj_pal_1);
    init_array(m_pixel_data);
    
    set_mode(LCD_MODE_VBLANK);
}

void LCD::SDLSaveImage(std::string filename)
{
    SDL_Surface *temp_sur = SDL_CreateRGBSurface(0, m_sdl_width, m_sdl_height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    SDL_RenderReadPixels(m_renderer, NULL, SDL_PIXELFORMAT_ARGB8888, temp_sur->pixels, temp_sur->pitch);
    
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
    
    auto p_it(m_pixel_data.cbegin()+(m_curr_scanline*LCD_WIDTH));
    for (auto x=0; x != LCD_WIDTH; ++x, r.x+=m_scale_factor, ++p_it)
    {
        SDL_SetRenderDrawColor(m_renderer, p_it->r, p_it->g, p_it->b, p_it->a);
        SDL_RenderFillRect(m_renderer, &r);
    }

    //Draw
    //SDL_RenderPresent(m_renderer);
    static int delay = 0;
    if (delay == 200)
    {
        SDL_RenderPresent(m_renderer);
        delay = 0;
    }
    delay++;
}

LCDPalette LCD::get_palette(uint16_t addr)
{
    LCDPalette ret;
    
    //Lowest bits first
    uint8_t regval = m_registers[addr];
    for (int i=0; i<4; ++i)
    {
        ret[i] = regval & 0x3;
        regval >>= 2;
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
    
    SDL_SetRenderDrawColor(m_renderer, m_colours[0].r, m_colours[0].g, m_colours[0].b, m_colours[0].a);
    SDL_RenderClear(m_renderer);
    SDL_RenderPresent(m_renderer);
}

//Note that tile also means sprite here, colour data is in the same format.
template <typename Iterator>
void LCD::tile_row_to_pixels(
    Iterator data_b, //Pointer to pixel data
    int startx, //Top left x co-ord of the tile.
    int starty, //Top left y co-ord of the tile.
    int offsx,  //Srolling window x offset
    int offsy,  //Index of row within tile to get pixels from
    bool is_sprite, //Set to handle transparancy of colour 0
    bool flip_x, //Mirror X co-ords
    const LCDPalette& palette //Colour mapping
    )
{
    uint8_t b1 = *(data_b + (offsy*2));
    uint8_t b2 = *(data_b + (offsy*2)+1);
    
    //Signed int!!
    for (int shift=7; shift>= 0; --shift)
    {
        uint8_t lsb = (b1 >> shift) & 0x1;
        uint8_t msb = (b2 >> shift) & 0x1;
        uint8_t c = (msb << 1) | lsb;
        
        if (is_sprite && c==0)
        {
            //Colour 0 is always 'transparent' for sprites
            continue;
        }
        
        int shift_diff = flip_x ? shift : (7-shift);
        int newx = startx + shift_diff - offsx;
        if ((newx >= LCD_WIDTH) || (newx < 0))
        {
            continue;
        }
        
        int newy = starty + offsy;
        if ((newy >= LCD_HEIGHT) || (newy < 0))
        {
            continue;
        }
        
        m_pixel_data[(newy*LCD_WIDTH)+newx] = m_colours[palette[c]];
    }
}

void LCD::draw_sprites()
{
    const uint64_t SPRITE_INFO_BYTES = 4;
    const uint16_t oam_size = LCD_OAM_END-LCD_OAM_START;
    
    const int SPRITE_HEIGHT = m_control_reg.get_sprite_size();
    const int SPRITE_WIDTH  = 8;
    const int SPRITE_BYTES  = 2*SPRITE_HEIGHT;
    
    for (uint16_t oam_addr=0; oam_addr < oam_size; oam_addr+=SPRITE_INFO_BYTES)
    {
        Sprite sprite(m_oam_data.begin()+oam_addr);
        
        //Assume 8x8 mode for now
        int sprite_x = sprite.get_x()-SPRITE_WIDTH;
        //Note that this is offset by 16 even when sprites are 8x8 pixels
        int sprite_y = sprite.get_y()-16;
        
        int sprite_row_offset = int(m_curr_scanline) - sprite_y;
        LCDPalette& palette = sprite.get_palette_number() ? m_obj_pal_1 : m_obj_pal_0;
        
        if ((m_curr_scanline >= sprite_y) &&
            (m_curr_scanline < (sprite_y+SPRITE_HEIGHT)) &&
            (sprite_x > -SPRITE_WIDTH)
            )
        {
            //Sprite pixels are stored in the same place as backgound tiles
            uint16_t tile_offset = sprite.get_pattern_number();
            if (SPRITE_HEIGHT == 16)
            {
                tile_offset &= ~1;
            }
            tile_offset *= SPRITE_BYTES;
            
            std::vector<uint8_t>::const_iterator norm_sprite(m_data.begin()+tile_offset);
            if (sprite.get_y_flip())
            {
                /*Because we give it a pointer to the last byte and go back
                 the lines are reversed so colour 0b01 becomes 0b10.
                 This is a temp fix for that until I can think of something more elegant.*/
                std::swap(palette[1], palette[2]);
                
                //You'd think this would need a -1 but it doesn't.
                std::vector<uint8_t>::const_reverse_iterator inv_sprite(norm_sprite+SPRITE_BYTES);
                tile_row_to_pixels(inv_sprite,
                                   sprite_x, sprite_y,
                                   0,
                                   sprite_row_offset,
                                   true,
                                   sprite.get_x_flip(),
                                   palette);
                
                std::swap(palette[1], palette[2]);
            }
            else
            {
                tile_row_to_pixels(norm_sprite,
                                   sprite_x, sprite_y,
                                   0,
                                   sprite_row_offset,
                                   true,
                                   sprite.get_x_flip(),
                                   palette);
            }
            
        }
    }

}

void LCD::draw_window()
{
    if (m_control_reg.get_window_display())
    {
        printf("Skipping window display!\n");
    }
}

void LCD::draw_background()
{
    if (m_control_reg.background_display())
    {
        //Data describing the tiles themselves
        uint16_t background_tile_data_addr = m_control_reg.get_bgrnd_tile_data_addr();
        bool signed_tile_nos = background_tile_data_addr == 0x0800;
        
        //Address of table of tile indexes that form the background
        const uint16_t background_tile_addr_table = m_control_reg.get_bgrnd_tile_table_addr();
        
        //The point in the background at which pixel 0,0 on the screen is taken from.
        const uint8_t start_x = get_scroll_x();
        const uint8_t start_y = get_scroll_y();
        
        const uint8_t TILE_SIDE = 8;
        const uint8_t TILE_BYTES = 16;
        
        //There is only one line of tiles we are interested in since we're only doing one scanline
        //Note that Y wraps
        const uint16_t tile_row = (uint8_t(m_curr_scanline + start_y) / TILE_SIDE);
        
        //Then we must start somewhere in that row
        uint8_t tile_row_offset = (start_x / TILE_SIDE) % 32;
        
        //Which row of pixels within the tile
        const uint8_t tile_pixel_row = (m_curr_scanline + start_y) % TILE_SIDE;
        
        for (uint8_t x=0; x<168; x+=TILE_SIDE, ++tile_row_offset)
        {
            if (tile_row_offset >= 32)
            {
                tile_row_offset %= 32;
            }
            
            //Get actual value of index from the table
            uint8_t tile_index = m_data[background_tile_addr_table+(32*tile_row)+tile_row_offset];
            
            if (signed_tile_nos)
            {
                tile_index += 128;
            }
            
            //The final address to read pixel data from
            uint16_t tile_addr = tile_index*TILE_BYTES;
            
            tile_row_to_pixels(m_data.begin() + tile_addr + background_tile_data_addr,
                               x - (start_x % TILE_SIDE), m_curr_scanline-tile_pixel_row,
                               0,//start_x % TILE_SIDE,
                               tile_pixel_row,
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
    
    uint8_t lcd_stat = get_reg8(LCDSTAT);
    uint8_t old_mode = lcd_stat & 3;
    uint8_t new_mode = old_mode;
    
    size_t cycle_diff = curr_cycles - m_last_tick_cycles;
    m_lcd_line_cycles += cycle_diff;
    
    //printf("Curr scanline %d\n", curr_scanline);
    
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
                draw_sprites();
                draw_window();
                
                /*
                 State machine continues if LCD is off, games like Dr. Mario
                 disable it during transitions.
                */
                if (m_control_reg.get_lcd_operation())
                {
                    SDLDraw();
                }
            }
            break;
        case LCD_MODE_HBLANK:
            if (m_lcd_line_cycles >= CYCLES_MODE_0_HBLANK)
            {
                m_curr_scanline++;
                if (m_curr_scanline == 144)
                {
                    new_mode = LCD_MODE_VBLANK;
                    /*This interrupt type has a higher priority so it's
                     ok that the post_interrupt further down will be ignored.*/
                    m_proc->post_interrupt(LCD_VBLANK_INT);
                }
                else
                {
                    new_mode = LCD_MODE_OAM_ACCESS;
                    //Take away cycles per line so we don't loose any overflow cycles
                    m_lcd_line_cycles = m_lcd_line_cycles-CYCLES_PER_SCAN_LINE;
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
                m_lcd_line_cycles = m_lcd_line_cycles - CYCLES_PER_SCAN_LINE;
            }
            break;
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
            
            if (lcd_stat & (1<<bit))
            {
                m_proc->post_interrupt(LCD_STAT_INT);
            }
        }
    }
    
    m_last_tick_cycles = curr_cycles;
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
        if (addr == (CURLINE+LCD_REGS_START))
        {
            return m_curr_scanline;
        }
        else
        {
            return get_reg8(get_regs_addr(addr));
        }
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
        if (addr == (CURLINE+LCD_REGS_START))
        {
            m_curr_scanline = 0;
            set_mode(LCD_MODE_OAM_ACCESS);
        }
        else
        {
            set_reg8(get_regs_addr(addr), value);
        }
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
        return get_reg16(get_regs_addr(addr));
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
        set_reg16(get_regs_addr(addr), value);
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
            if (m_control_reg.get_lcd_operation() && (m_window == NULL))
            {
                SDLInit();
            }
            else if (!m_control_reg.get_lcd_operation())
            {
                SDLClear();
            }
            break;
        }
        case BGRDPAL:
            m_bgrd_pal = get_palette(BGRDPAL);
            break;
        case OBJPAL0:
            m_obj_pal_0 = get_palette(OBJPAL0);
            break;
        case OBJPAL1:
            m_obj_pal_1 = get_palette(OBJPAL1);
            break;
    }
}

void LCD::do_after_reg_write16(uint16_t addr)
{
    do_after_reg_write(addr);
    do_after_reg_write(addr+1);
}
