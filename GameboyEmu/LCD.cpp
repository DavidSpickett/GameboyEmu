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

void LCD::SDLDraw(uint8_t curr_scanline)
{
    if (m_window == NULL)
    {
        throw std::runtime_error("Cannot draw, the LCD window has not been init.");
    }
    
    int start_index = curr_scanline*LCD_WIDTH;
    for (int x=0; x != LCD_WIDTH; ++x)
    {
        colour c = m_pixel_data[start_index+x];
        SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
        
        SDL_Rect r;
        r.h = m_scale_factor;
        r.w = m_scale_factor;
        r.x = x*m_scale_factor;
        r.y = curr_scanline*m_scale_factor;
        
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

const LCDPallette LCD::get_pallete(uint16_t addr)
{
    LCDPallette ret;
    
    //Lowest bits first
    uint8_t regval = m_registers[addr];
    for (int i=0; i<4; ++i)
    {
        ret.push_back(regval & 0x3);
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
                                 formatted_string("SDL could not initialize! SDL_Error: %s\n", SDL_GetError()));
    }
    
    //Create window
    int width = LCD_WIDTH*m_scale_factor;
    int height = LCD_HEIGHT*m_scale_factor;
    m_window = SDL_CreateWindow("Gameboy Emulator",
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                width, height,
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
void LCD::tile_row_to_pixels(
    std::vector<uint8_t>::const_iterator data_b, //Pointer to pixel data
    int startx, //Top left x co-ord of the tile.
    int starty, //Top left y co-ord of the tile.
    int offsx,  //Srolling window x offset
    int offsy,  //Index of row within tile to get pixels from
    bool is_sprite, //Set to handle transparancy of colour 0
    const LCDPallette& pallette //Colour mapping
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
        
        int newx = startx + (7-shift) - offsx;
        if ((newx >= LCD_WIDTH) || (newx < 0))
        {
            continue;
        }
        
        int newy = starty + offsy;
        if ((newy >= LCD_HEIGHT) || (newy < 0))
        {
            continue;
        }
        
        m_pixel_data[(newy*LCD_WIDTH)+newx] = m_colours[pallette[c]];
    }
}

void LCD::draw_to_pixels()
{
    //Lines are drawn one at a time
    const uint8_t curr_scanline = get_curr_scanline();
    
    //Data describing the tiles themselves
    uint16_t background_tile_data_addr = m_control_reg.get_bgrnd_tile_data_addr();
    bool signed_tile_nos = background_tile_data_addr == 0x0800;
    
    if (m_control_reg.get_window_display())
    {
        printf("Skipping window display!\n");
    }
    
    if (m_control_reg.background_display())
    {
        //Address of table of tile indexes that form the background
        const uint16_t background_tile_addr_table = m_control_reg.get_bgrnd_tile_table_addr();
        
        //The point in the background at which pixel 0,0 on the screen is taken from.
        const uint8_t start_x = get_scroll_x();
        const uint8_t start_y = get_scroll_y();
        
        const uint8_t TILE_SIDE = 8;
        const uint8_t TILE_BYTES = 16;
        
        //There is only one line of tiles we are interested in since we're only doing one scanline
        //Note that Y wraps
        const uint16_t tile_row = (uint8_t(curr_scanline + start_y) / TILE_SIDE);
        
        //Then we must start somewhere in that row
        uint8_t tile_row_offset = (start_x / TILE_SIDE) % 32;
        
        //Which row of pixels within the tile
        const uint8_t tile_pixel_row = (curr_scanline + start_y) % TILE_SIDE;
        
        LCDPallette bgrnd_pal = get_bgrnd_pallette();
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
                x - (start_x % TILE_SIDE), curr_scanline-tile_pixel_row,
                0,//start_x % TILE_SIDE,
                tile_pixel_row,
                false,
                bgrnd_pal);
            
            //The idea being that these pixels are always the full row, that's why we
            //can incremement x by 8 each time. It gets the pixels before and ahead of x.
        }
    }
 
    //Sprites
    /*if (m_control_reg.get_sprite_size() != 8)
    {
        throw std::runtime_error("Sprite size is 8x16!!");
    }*/
    
    const uint64_t SPRITE_INFO_BYTES = 4;
    const uint16_t oam_size = LCD_OAM_END-LCD_OAM_START;
    
    const int SPRITE_HEIGHT = m_control_reg.get_sprite_size();
    const int SPRITE_WIDTH  = 8;
    const int SPRITE_BYTES  = 2*SPRITE_HEIGHT;

    for (uint16_t oam_addr=0; oam_addr < oam_size; oam_addr+=SPRITE_INFO_BYTES)
    //for (uint16_t oam_addr=0; oam_addr < SPRITE_INFO_BYTES; oam_addr+=SPRITE_INFO_BYTES)
    {
        Sprite sprite(m_oam_data.begin()+oam_addr);
        
        //Assume 8x8 mode for now
        int sprite_x = sprite.get_x()-SPRITE_WIDTH;
        //Note that this is offset by 16 even when sprites are 8x8 pixels
        int sprite_y = sprite.get_y()-16;
        
        int sprite_row_offset = int(curr_scanline) - sprite_y;
        LCDPallette pallette = sprite.get_pallette_number() ? get_obj_pal1() : get_obj_pal0();
        
        if ((curr_scanline >= sprite_y) &&
            (curr_scanline < (sprite_y+SPRITE_HEIGHT)) &&
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
            
            tile_row_to_pixels(m_data.begin()+tile_offset,
                        sprite_x, sprite_y,
                        0,
                        sprite_row_offset,
                        true,
                        pallette);
        }
    }

    SDLDraw(curr_scanline);
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
    uint8_t curr_scanline = get_curr_scanline();

    if (m_control_reg.get_lcd_operation() &&
        ((curr_cycles - m_last_scan_change_cycles) > per_scan_line))
    {
        //154 scan lines, 144 + 10 vblank period
        m_last_scan_change_cycles = curr_cycles;
        
        if (curr_scanline < 144)
        {
            draw_to_pixels();
        }
        
        if (curr_scanline != 153)
        {
            curr_scanline = inc_curr_scanline();
            
            if (curr_scanline == LCD_HEIGHT)
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
                        //printf("-----V-Blank interrupt-----\n");
                        
                        //Disable ints until the program enables them again
                        m_proc->interrupt_enable = false;
                        //Unhalt if need be
                        m_proc->halted = false;
                    }
                }
            }
        }
        else
        {
            set_curr_scanline(0);
        }
    }
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
        if ((addr-LCD_REGS_START) == SCROLLY)
        {
            //printf("scrolly set to 0x%02x\n", value);
        }
        if ((addr-LCD_REGS_START) == SCROLLX)
        {
            //printf("scrollx set to 0x%02x\n", value);
        }
        
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
            if (m_control_reg.get_lcd_operation() && (m_window == NULL))
            {
                SDLInit();
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
