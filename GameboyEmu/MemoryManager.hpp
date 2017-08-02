//
//  MemoryManager.hpp
//  GameboyEmu
//
//  Created by David Spickett on 03/10/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef MemoryManager_hpp
#define MemoryManager_hpp

#include <stdint.h>
#include <array>
#include <functional>
#include "utils.hpp"

const uint16_t LCD_MEM_START  = 0x8000;
const uint16_t LCD_MEM_END    = 0xa000;
const uint16_t LCD_BGRND_DATA = 0x9800;

const uint16_t LCD_REGS_START = 0xff40;
const uint16_t LCD_REGS_END   = 0xff4c;

const uint16_t LCD_OAM_START = 0xfe00;
const uint16_t LCD_OAM_SIZE  = 0xa0;
const uint16_t LCD_OAM_END   = LCD_OAM_START+LCD_OAM_SIZE;

const uint16_t ROM_START = 0x0100; //Before bootstrap turns off that is
const uint16_t ROM_END   = 0x4000;

const uint16_t CART_RAM_START = 0xa000;
const uint16_t CART_RAM_END   = 0xc000;

//Joypad assigned to a seperate handler
const uint16_t JOYPAD_REG          = 0xff00;
const uint16_t HARDWARE_REGS_START = 0xff01;
const uint16_t HARDWARE_REGS_END   = 0xff10;

const uint16_t SERIAL_DATA    = 0xff01;
const uint16_t SERIAL_CONTROL = 0xff02;

const uint16_t GB_RAM_START = 0xc000;
const uint16_t GB_RAM_END   = 0xe000;

const uint16_t GB_HIGH_RAM_START = 0xFF80;
//Since we need 1 more bit
const uint32_t GB_HIGH_RAM_END = 0xF0000;

const uint16_t ECHO_RAM_START = 0xe000;
const uint16_t ECHO_RAM_END   = 0xfe00;

const uint16_t UNUSED_START = 0xfea0;
const uint16_t UNUSED_END   = 0xff00;

const uint16_t INTERRUPT_FLAGS  = 0xff0f;
const uint16_t INTERRUPT_SWITCH = 0xffff;

const uint16_t SWITCHABLE_ROM_START = 0x4000;
const uint16_t SWITCHABLE_ROM_END   = 0x8000;

const uint16_t UNUSED_IO_REGS_START = 0xff4c;
const uint16_t UNUSED_IO_REGS_END   = 0xff80;

const uint16_t SOUND_BEGIN = 0xff10;
const uint16_t SOUND_END   = 0xff40;

using InterruptCallback = std::function<void(uint8_t)>;

class MemoryMap;

class MemoryManager
{
public:
    MemoryManager():
        post_int([](uint8_t num){})
    {}
    
    virtual uint8_t read8(uint16_t addr) = 0;
    virtual void write8(uint16_t addr, uint8_t value) = 0;
    
    virtual uint16_t read16(uint16_t addr) = 0;
    virtual void write16(uint16_t addr, uint16_t value) = 0;
    
    virtual void tick(size_t curr_cycles) = 0;
    
    InterruptCallback post_int;
};

class NullMemoryManager: public MemoryManager
{
    using MemoryManager::MemoryManager;
    
    uint8_t read8(uint16_t addr) { return 0; }
    void write8(uint16_t addr, uint8_t value) {}
    uint16_t read16(uint16_t addr) { return 0; };
    void write16(uint16_t addr, uint16_t value) {}
    void tick(size_t curr_cycles) {}
};

class DefaultMemoryManager: public MemoryManager
{
public:
    DefaultMemoryManager()
    {
        init_array(m_mem);
    }
    
    uint8_t read8(uint16_t addr)
    {
        addr = normalise_addr(addr);
        return m_mem[addr];
    }
    
    void write8(uint16_t addr, uint8_t value)
    {
        addr = normalise_addr(addr);
        m_mem[addr] = value;
    }
    
    uint16_t read16(uint16_t addr)
    {
        addr = normalise_addr(addr);
        return m_mem[addr] | (m_mem[addr+1] << 8);
    }
    
    void write16(uint16_t addr, uint16_t value)
    {
        addr = normalise_addr(addr);
        m_mem[addr] = value;
        m_mem[addr+1] = value >> 8;
    }
    
    void tick(size_t curr_cycles) {}
    
    void AddFile(std::string path);
    
private:
    //Note that this is a full 16 bits of memory, so we don't
    //have to offset it in any way.
    std::array<uint8_t, 0x10000> m_mem;
    
    uint16_t normalise_addr(uint16_t addr)
    {
        if ((addr >= ECHO_RAM_START) && (addr < ECHO_RAM_END))
        {
            addr -= 0x2000;
        }
        return addr;
    }
};

#endif /* MemoryManager_hpp */
