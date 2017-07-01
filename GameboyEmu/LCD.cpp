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

const int SPRITE_INFO_BYTES = 4;
const int TILE_BYTES        = 16;
const int TILES_PER_LINE    = 32;
const int TILE_WIDTH        = 8;

struct Pixel
{
    Pixel(int x, int y, uint8_t c):
    x(x), y(y), c(c)
    {
    }
    
    int x;
    int y;
    uint8_t c;
};

class Sprite
{
public:
    explicit Sprite(OAMData::const_iterator start):
    m_data(start)
    {}
    
    Sprite& operator++()
    {
        m_data += SPRITE_INFO_BYTES;
        return *this;
    }
    
    int get_y() { return int(*m_data) - 16; }
    int get_x() { return int(*(m_data+1)) - TILE_WIDTH; }
    uint8_t get_pattern_number() { return *(m_data+2); }
    
    bool get_priority() { return get_flag(7); }
    bool get_y_flip() { return get_flag(6); }
    bool get_x_flip() { return get_flag(5); }
    bool get_palette_number() { return get_flag(4); }
    
    std::string to_str()
    {
        return formatted_string("Sprite at X:%d Y:%x priority:%d xflip:%d yflip:%d palettenum:%d",
                                get_x(), get_y(), get_priority(), get_x_flip(),
                                get_y_flip(), get_palette_number());
    }
    
private:
    bool get_flag(uint8_t pos) { return *(m_data+3) & (1<<pos); }
    OAMData::const_iterator m_data;
};


LCD::LCD(int scale_factor):
m_last_tick_cycles(0),
m_lcd_line_cycles(0),
m_scale_factor(scale_factor),
m_curr_scanline(145),
m_colours{colour(0xff, 0xff, 0xff), colour(0xb9, 0xb9, 0xb9),
          colour(0x6b, 0x6b, 0x6b), colour(0x00, 0x00, 0x00)},
m_control_reg(0)
{
    m_sdl_width = LCD_WIDTH*m_scale_factor;
    m_sdl_height = LCD_HEIGHT*m_scale_factor;
    
    init_array(m_bgrd_pal);
    init_array(m_obj_pal_0);
    init_array(m_obj_pal_1);
    init_array(m_pixel_data);
    init_array(m_registers);
    init_array(m_oam_data);
    init_array(m_data);
    
    set_mode(LCD_MODE_VBLANK);
}

void LCD::set_mode(uint8_t mode)
{
    set_reg8(LCDSTAT, (get_reg8(LCDSTAT) & ~3) | mode);
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
    //Note: we need to clear the screen here to remove the after
    //effect of the previous render. (once we can run fast enough for that to work)
    
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

//Note that tile also means sprite here, colour data is in the same format.
void LCD::tile_row_to_pixels(
    LCDData::const_iterator data_b, //Pointer to pixel data (must already point to correct row)
    int startx, //Top left x co-ord of the tile.
    int starty, //Top left y co-ord of the tile.
    bool is_sprite, //Set to handle transparancy of colour 0
    bool flip_x, //Mirror X co-ords
    const LCDPalette& palette //Colour mapping
    )
{
    uint8_t b1 = *data_b++;
    uint8_t b2 = *data_b;
    
    //Signed int!!
    for (int shift=7; shift>= 0; --shift)
    {
        int shift_diff = flip_x ? shift : (7-shift);
        int newx = startx + shift_diff;
        if ((newx >= LCD_WIDTH) || (newx < 0))
        {
            continue;
        }
        
        uint8_t lsb = (b1 >> shift) & 0x1;
        uint8_t msb = (b2 >> shift) & 0x1;
        uint8_t c = (msb << 1) | lsb;
        
        if (is_sprite && c==0)
        {
            //Colour 0 is always 'transparent' for sprites
            continue;
        }
        
        m_pixel_data[(starty*LCD_WIDTH)+newx] = m_colours[palette[c]];
    }
}

void LCD::draw_sprites()
{
    const int sprite_height = m_control_reg.get_sprite_size();
    const int sprite_bytes  = 2*sprite_height;
    
    Sprite sprite(m_oam_data.begin());
    for (auto oam_addr=0; oam_addr < LCD_OAM_SIZE; oam_addr+=SPRITE_INFO_BYTES, ++sprite)
    {
        auto sprite_x = sprite.get_x();
        auto sprite_y = sprite.get_y();
        
        int sprite_row_offset = int(m_curr_scanline) - sprite_y;
        LCDPalette& palette = sprite.get_palette_number() ? m_obj_pal_1 : m_obj_pal_0;
        
        if ((m_curr_scanline >= sprite_y) &&
            (m_curr_scanline < (sprite_y+sprite_height)) &&
            (sprite_x > - TILE_WIDTH)
            )
        {
            //Sprite pixels are stored in the same place as backgound tiles
            uint16_t tile_offset = sprite.get_pattern_number();
            if (sprite_height == 16)
            {
                tile_offset &= ~1;
            }
            tile_offset *= sprite_bytes;
            
            LCDData::const_iterator norm_sprite(m_data.begin()+tile_offset);
            if (sprite.get_y_flip())
            {
                norm_sprite += (sprite_height-sprite_row_offset-1)*2;
            }
            else
            {
                norm_sprite += sprite_row_offset*2;
            }
            
            tile_row_to_pixels(norm_sprite,
                               sprite_x, sprite_y+sprite_row_offset,
                               true,
                               sprite.get_x_flip(),
                               palette);
            
        }
    }

}

void LCD::draw_window()
{
    if (m_control_reg.get_window_display())
    {
        auto winy = get_reg8(WINPOSY);
        
        if (m_curr_scanline >= winy)
        {
            auto tile_table_address = m_control_reg.get_window_tile_table_addr();
            auto winx = get_reg8(WINPOSX)-7;
            
            auto tile_data_addr = m_control_reg.get_bgrnd_tile_data_addr();
            auto signed_tile_nos = tile_data_addr == 0x0800;
            auto transparancy = m_control_reg.get_colour_0_transparent();
            
            auto tile_index_row = (m_curr_scanline-winy) / 8;
            auto tile_row_offset = (m_curr_scanline-winy) % 8;
            
            for (auto x=winx, tile_no=0; x<(LCD_WIDTH+8); x+=TILE_WIDTH, ++tile_no)
            {
                auto tile_index = m_data[tile_table_address+(tile_index_row*TILES_PER_LINE)+tile_no];
                if (signed_tile_nos)
                {
                    tile_index += 128;
                }
                
                uint16_t tile_addr = (tile_index*TILE_BYTES);
                
                tile_row_to_pixels(
                   m_data.begin() + tile_addr + tile_data_addr + (tile_row_offset*2),
                   x, m_curr_scanline,
                   transparancy,
                   false,
                   m_bgrd_pal);
            }
        }
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
        const uint8_t start_x = get_reg8(SCROLLX);
        const uint8_t start_y = get_reg8(SCROLLY);
        
        //printf("scanline %d scrollx %d\n", m_curr_scanline, get_reg8(SCROLLX));
        
        //There is only one line of tiles we are interested in since we're only doing one scanline
        //Note that Y wraps
        const uint16_t tile_row = (uint8_t(m_curr_scanline + start_y) / TILE_WIDTH);
        
        //Then we must start somewhere in that row
        uint8_t tile_row_offset = (start_x / TILE_WIDTH) % TILES_PER_LINE;
        
        //Which row of pixels within the tile
        const uint8_t tile_pixel_row = (m_curr_scanline + start_y) % TILE_WIDTH;
        
        for (uint8_t x=0; x<(LCD_WIDTH+8); x+=TILE_WIDTH, ++tile_row_offset)
        {
            if (tile_row_offset >= TILES_PER_LINE)
            {
                tile_row_offset %= TILES_PER_LINE;
            }
            
            //Get actual value of index from the table
            uint8_t tile_index = m_data[background_tile_addr_table+(TILES_PER_LINE*tile_row)+tile_row_offset];
            
            if (signed_tile_nos)
            {
                tile_index += 128;
            }
            
            //The final address to read pixel data from
            uint16_t tile_addr = tile_index*TILE_BYTES;
            
            tile_row_to_pixels(
               m_data.begin() + tile_addr + background_tile_data_addr + (tile_pixel_row*2),
               x - (start_x % TILE_WIDTH), m_curr_scanline,
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
    auto new_mode = old_mode;
    auto old_scanline = m_curr_scanline;
    
    size_t cycle_diff = curr_cycles - m_last_tick_cycles;
    m_lcd_line_cycles += cycle_diff;
    
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
                if (m_curr_scanline == VBLANK_SCANLINE)
                {
                    new_mode = LCD_MODE_VBLANK;
                    /*This interrupt type has a higher priority so it's
                     ok that the post_interrupt further down will be ignored.*/
                    post_int(LCD_VBLANK_INT);
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
    
    auto cmpline = get_reg8(CMPLINE);
    if ((m_curr_scanline != old_scanline) &&
        (m_curr_scanline == cmpline) &&
        (lcd_stat & (1<<6)))
    {
        post_int(LCD_STAT_INT);
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
                post_int(LCD_STAT_INT);
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
        switch (addr)
        {
            case CURLINE:
                return m_curr_scanline;
            case LCDCONTROL:
                return m_control_reg.read();
            default:
                return get_reg8(addr);
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
    else if ((addr >= LCD_OAM_START) && (addr < LCD_OAM_END))
    {
        m_oam_data[addr-LCD_OAM_START] = value;
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
                if (m_control_reg.get_lcd_operation() && (m_window == NULL))
                {
                    SDLInit();
                }
                else if (!m_control_reg.get_lcd_operation())
                {
                    SDLClear();
                }
                break;
            case LCDSTAT:
                //Mode bits are read only
                set_reg8(LCDSTAT, get_reg8(LCDSTAT) | (value & ~3));
                break;
            case BGRDPAL:
                m_bgrd_pal = make_palette(value);
            case OBJPAL0:
                m_obj_pal_0 = make_palette(value);
            case OBJPAL1:
                m_obj_pal_1 = make_palette(value);
            default:
                set_reg8(addr, value);
                break;
        }
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
