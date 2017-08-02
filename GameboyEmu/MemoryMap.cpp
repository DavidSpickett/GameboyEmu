//
//  Memory.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "MemoryMap.hpp"
#include <fstream>
#include <string>
#include "utils.hpp"

MemoryRange& MemoryRange::operator=(const MemoryRange& other)
{
    if (&other == this)
    {
        return *this;
    }
    
    manager = other.manager;
    start = other.start;
    end = other.end;
    
    return *this;
}

bool MemoryRange::contains(uint32_t addr) const
{
    return (addr >= start) && (addr < end);
}

bool MemoryRange::operator<(const MemoryRange& rhs) const
{
    return start < rhs.start;
}

bool MemoryRange::operator<(int rhs) const
{
    return rhs >= end;
}

void swap(MemoryRange& lhs, MemoryRange& rhs)
{
    using std::swap;
    swap(lhs.manager, rhs.manager);
    swap(lhs.start, rhs.start);
    swap(lhs.end, rhs.end);
}

void DefaultMemoryManager::AddFile(std::string path)
{
    //Copy file into memory at given location
    std::ifstream in(path.c_str(), std::ifstream::ate | std::ifstream::binary);
    
    if (!in.is_open())
    {
        throw std::runtime_error(formatted_string("File %s does not exist.", path.c_str()));
    }
    
    //Go back to beginning and copy to end
    in.seekg(0);
    std::copy(std::istreambuf_iterator<char>(in),
              std::istreambuf_iterator<char>(),
              m_mem.begin());
}

MemoryMap::MemoryMap(std::string& cartridge_name, bool bootstrap_skipped, int scale_factor):
    m_bootstrap_in_mem(true),
    m_rom_handler(cartridge_name),
    m_lcd_handler(scale_factor),
    m_hardware_regs_handler(),
    m_input_handler(),
    m_default_handler(),
    m_null_handler(),
    m_sound_handler(),
    m_last_tick_cycles(0)
{
    if (!bootstrap_skipped)
    {
        m_default_handler.AddFile("GameBoyBios.gb");
    }
    
    AddMemoryRange(ROM_START, ROM_END, m_rom_handler);
    AddMemoryRange(SWITCHABLE_ROM_START, SWITCHABLE_ROM_END, m_rom_handler);
    AddMemoryRange(CART_RAM_START, CART_RAM_END, m_rom_handler);
    
    AddMemoryRange(LCD_MEM_START, LCD_MEM_END, m_lcd_handler);
    AddMemoryRange(LCD_REGS_START, LCD_REGS_END, m_lcd_handler);
    AddMemoryRange(LCD_OAM_START, LCD_OAM_END, m_lcd_handler);
    
    AddMemoryRange(GB_RAM_START, GB_RAM_END, m_default_handler);
    AddMemoryRange(ECHO_RAM_START, ECHO_RAM_END, m_default_handler);
    AddMemoryRange(GB_HIGH_RAM_START, GB_HIGH_RAM_END, m_default_handler);
    
    AddMemoryRange(UNUSED_START, UNUSED_END, m_null_handler);
    
    AddMemoryRange(JOYPAD_REG, m_input_handler);
    
    //I assume these are Colour/Super GB regs
    AddMemoryRange(UNUSED_IO_REGS_START, UNUSED_IO_REGS_END, m_null_handler);
    
    AddMemoryRange(SOUND_BEGIN, SOUND_END, m_sound_handler);
    
    AddMemoryRange(HARDWARE_REGS_START, HARDWARE_REGS_END, m_hardware_regs_handler);
    
    //Sort so we can lower bound later
    std::sort(m_mem_ranges.begin(), m_mem_ranges.end());
}

void MemoryMap::AddMemoryRange(uint32_t start, uint32_t end, MemoryManager& manager)
{
    for (auto range : m_mem_ranges)
    {
        if ((range.start >= start) && (range.start < end))
        {
            throw std::runtime_error(formatted_string(
              "Cannot add range from 0x%04x to 0x%04x, overlaps with range from 0x%04x to 0x%04x",
              start, end, range.start, range.end));
        }
    }
    
    m_mem_ranges.push_back(MemoryRange(start, end, manager));
}

MemoryManager& MemoryMap::get_mm(uint16_t addr)
{
    //TODO: make this work with memory ranges
    if ((addr >= 0x0000) && (addr < 0x0100))
    {
        if (m_bootstrap_in_mem)
        {
            return m_default_handler;
        }
        else
        {
            return m_rom_handler;
        }
    }
    else
    {
        auto range = std::lower_bound(m_mem_ranges.begin(), m_mem_ranges.end(), addr);
        if (range != m_mem_ranges.end())
        {
            return range->manager;
        }
        throw std::runtime_error(formatted_string("Couldn't find memory manager for address 0x%04x", addr));
    }
}

uint8_t MemoryMap::read8(uint16_t addr)
{
    MemoryManager& m = get_mm(addr);
    return m.read8(addr);
}

void MemoryMap::write8(uint16_t addr, uint8_t value)
{
    //Bodge to get the bootstrap out of memory
    if ((addr == 0xff50) && (value == 1))
    {
        m_bootstrap_in_mem = false;
    }
    //Start a DMA transfer
    else if (addr == 0xff46)
    {
        /*This potentially is wrong because we will count the cycles
         of this instruction against the timing of the DMA.*/
        m_dma_transfer = DMATransfer(uint16_t(value) << 8);
    }
    else
    {
        MemoryManager& m = get_mm(addr);
        m.write8(addr, value);
    }
}

//Check bounds!!
uint16_t MemoryMap::read16(uint16_t addr)
{
    return read8(addr) | (read8(addr+1) << 8);
}

//Check bounds!
void MemoryMap::write16(uint16_t addr, uint16_t value)
{
    write8(addr, value);
    write8(addr+1, value >> 8);
}

void MemoryMap::tick(size_t curr_cycles)
{
    if (m_dma_transfer.cycles_remaining > 0)
    {
        m_dma_transfer.cycles_remaining -= (curr_cycles-m_last_tick_cycles);
        
        if (m_dma_transfer.cycles_remaining <= 0)
        {
            m_dma_transfer.cycles_remaining = -1;
            
            MemoryManager& source_m = get_mm(m_dma_transfer.source_addr);
            uint16_t read_addr = m_dma_transfer.source_addr;
            for (uint16_t write_addr=LCD_OAM_START; write_addr<LCD_OAM_END; ++read_addr,++write_addr)
            {
                m_lcd_handler.write8(write_addr, source_m.read8(read_addr));
            }
        }
    }
    
    m_input_handler.tick(curr_cycles);
    m_lcd_handler.tick(curr_cycles);
    m_hardware_regs_handler.tick(curr_cycles);
    
    m_last_tick_cycles = curr_cycles;
}
