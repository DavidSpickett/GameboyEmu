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
#include "MemoryManager.hpp"
#include "RomHandler.hpp"
#include "LCD.hpp"
#include "HardwareIORegs.hpp"
#include "InputManager.hpp"
#include "SoundHandler.hpp"

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
    
    void AddFile(std::string path);
    
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
    MemoryMap(std::string cartridge_name):
    m_bootstrap_in_mem(true), m_rom_handler(cartridge_name)
    {
        m_default_handler.AddFile("GameBoyBios.gb");
    }
    
    uint8_t read8(uint16_t addr);
    void write8(uint16_t addr, uint8_t value);
    
    uint16_t read16(uint16_t addr);
    void write16(uint16_t addr, uint16_t value);
    
    void tick(size_t curr_cycles);
    
    void set_proc_pointers(Z80* proc)
    {
        m_input_handler.m_proc = proc;
        m_interrupt_handler.m_proc = proc;
        m_lcd_handler.m_proc = proc;
    }
    
private:
    bool m_bootstrap_in_mem;
    MemoryManager& get_mm(uint16_t addr);
    InterruptManager m_interrupt_handler;
    LCD m_lcd_handler;
    ROMHandler m_rom_handler;
    HardwareIORegs m_hardware_regs_handler;
    DefaultMemoryManager m_default_handler;
    NullMemoryManager m_null_handler;
    InputManager m_input_handler;
    SoundHandler m_sound_handler;
};


#endif /* MemoryMap_hpp */
