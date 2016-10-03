//
//  Memory.hpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef MemoryMap_hpp
#define MemoryMap_hpp

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include "utils.hpp"

const uint16_t LCD_MEM_START  = 0x8000;
const uint16_t LCD_MEM_END    = 0xa000;
const uint16_t LCD_REGS_START = 0xff40;
const uint16_t LCD_REGS_END   = 0xff48;

const uint16_t ROM_START = 0x0100; //Before bootstrap turns off that is
const uint16_t ROM_END   = 0x4000;

const uint16_t HARDWARE_REGS_START = 0xff00;
const uint16_t HARDWARE_REGS_END   = 0xff27;

const uint16_t GB_RAM_START = 0xc000;
const uint16_t GB_RAM_END   = 0xe000;

const uint16_t GB_HIGH_RAM_START = 0xFF80;
const uint16_t GB_HIGH_RAM_END   = 0xFFFF;

const uint16_t ECHO_RAM_START = 0xe000;
const uint16_t ECHO_RAM_END   = 0xffe0;

class MemoryManager
{
public:
    MemoryManager()
    {
    }
    
    virtual uint8_t read8(uint16_t addr) = 0;
    virtual void write8(uint16_t addr, uint8_t value) = 0;
    
    virtual uint16_t read16(uint16_t addr) = 0;
    virtual void write16(uint16_t addr, uint16_t value) = 0;
    
    virtual void tick(size_t curr_cycles) = 0;
    bool contains(uint16_t addr) const;
};

class DefaultMemoryManager: public MemoryManager
{
public:
    DefaultMemoryManager()
    {
        m_mem.resize(0x10000);
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
        m_mem[addr] = uint8_t(value);
        m_mem[addr+1] = value >> 8;
    }
    
    void tick(size_t curr_cycles) {}
    
    void AddFile(std::string path, uint16_t addr);
    
private:
    std::vector<uint8_t> m_mem;
    
    uint16_t normalise_addr(uint16_t addr)
    {
        if ((addr >= ECHO_RAM_START) && (addr < ECHO_RAM_END))
        {
            addr -= 0x2000;
        }
        return addr;
    }
};

class MemoryMap
{
public:
    MemoryMap(MemoryManager& rom_handler, MemoryManager& lcd_handler, MemoryManager& hardware_regs_handler):
    m_bootstrap_in_mem(true), m_rom_handler(rom_handler), m_lcd_handler(lcd_handler), m_hardware_regs_handler(hardware_regs_handler)
    {
        m_default_handler.AddFile("GameBoyBios.gb", 0x0000);
    }
    
    uint8_t read8(uint16_t addr);
    void write8(uint16_t addr, uint8_t value);
    
    uint16_t read16(uint16_t addr);
    void write16(uint16_t addr, uint16_t value);
    
    void tick(size_t curr_cycles) const;
    
private:
    bool m_bootstrap_in_mem;
    MemoryManager& get_mm(uint16_t addr);
    MemoryManager& m_rom_handler;
    MemoryManager& m_lcd_handler;
    MemoryManager& m_hardware_regs_handler;
    DefaultMemoryManager m_default_handler;
};


#endif /* MemoryMap_hpp */
