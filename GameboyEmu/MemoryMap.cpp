//
//  Memory.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "MemoryMap.hpp"
#include <fstream>
#include "utils.hpp"

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

MemoryManager& MemoryMap::get_mm(uint16_t addr)
{
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
    else if (
        ((addr >= ROM_START) && (addr < ROM_END)) ||
        ((addr >= SWITCHABLE_ROM_START) && (addr < SWITCHABLE_ROM_END)) ||
        ((addr >= CART_RAM_START) && (addr < CART_RAM_END)))
    {
        return m_rom_handler;
    }
    else if (
      ((addr >= LCD_MEM_START) && (addr < LCD_MEM_END)) ||
      ((addr >= LCD_REGS_START) && (addr < LCD_REGS_END)) ||
      ((addr >= LCD_OAM_START) && (addr < LCD_OAM_END))
    )
    {
        return m_lcd_handler;
    }
    else if (((addr >= HARDWARE_REGS_START) && (addr < HARDWARE_REGS_END)) &&
             (addr != JOYPAD_REG))
    {
        return m_hardware_regs_handler;
    }
    else if (
        ((addr >= GB_RAM_START) && (addr < GB_RAM_END)) ||
        ((addr >= ECHO_RAM_START) && (addr < ECHO_RAM_END))
        )
    {
        return m_default_handler;
    }
    else if ((addr >= GB_HIGH_RAM_START) && (addr < GB_HIGH_RAM_END))
    {
        return m_default_handler;
    }
    else if ((addr >= UNUSED_START) && (addr < UNUSED_END))
    {
        return m_null_handler;
    }
    else if ((addr == INTERRUPT_FLAGS) || (addr == INTERRUPT_SWITCH))
    {
        return m_interrupt_handler;
    }
    else if (addr == JOYPAD_REG)
    {
        return m_input_handler;
    }
    else if ((addr >= UNUSED_IO_REGS_START) && (addr < UNUSED_IO_REGS_END))
    {
        //I assume these are Colour/Super GB regs
        return m_null_handler;
    }
    else if ((addr >= SOUND_BEGIN) && (addr < SOUND_END))
    {
        return m_sound_handler;
    }
    else
    {
        throw std::runtime_error(formatted_string("Don't have a handler for addr 0x%04x!", addr));
    }
}

uint8_t MemoryMap::read8(uint16_t addr)
{
    MemoryManager& m = get_mm(addr);
    return m.read8(addr);
}

void MemoryMap::write8(uint16_t addr, uint8_t value)
{
    //Output for instruction tests
    if (addr == 0xff01)
    {
        printf("%c", value);
    }
    //Bodge to get the bootstrap out of memory
    else if ((addr == 0xff50) && (value == 1))
    {
        m_bootstrap_in_mem = false;
    }
    //Start a DMA transfer
    else if (addr == 0xff46)
    {
        /*This potentially is wrong because we will count the cycles
         of this instruction against the timing of the DMA.*/
        m_dma_transfer = DMATransfer(uint16_t(value) << 8);
        //printf("Setup DMA transfer from 0x%04x\n", m_dma_transfer.source_addr);
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
    MemoryManager& m = get_mm(addr);
    return m.read16(addr);
}

//Check bounds!
void MemoryMap::write16(uint16_t addr, uint16_t value)
{
    MemoryManager& m = get_mm(addr);
    m.write16(addr, value);
}

void MemoryMap::tick(size_t curr_cycles)
{
    if (m_dma_transfer.cycles_remaining > 0)
    {
        m_dma_transfer.cycles_remaining -= curr_cycles;
        
        if (m_dma_transfer.cycles_remaining <= 0)
        {
            //printf("Doing DMA transfer from 0x%04x\n", m_dma_transfer.source_addr);
            m_dma_transfer.cycles_remaining = -1;
            
            MemoryManager& source_m = get_mm(m_dma_transfer.source_addr);
            uint16_t read_addr = m_dma_transfer.source_addr;
            //Hex 0x100!!
            for (uint16_t write_addr=LCD_OAM_START; write_addr<LCD_OAM_END; ++read_addr,++write_addr)
            {
                m_lcd_handler.write8(write_addr, source_m.read8(read_addr));
            }
        }
    }
    
    m_interrupt_handler.tick(curr_cycles);
    m_input_handler.tick(curr_cycles);
    m_lcd_handler.tick(curr_cycles);
    m_hardware_regs_handler.tick(curr_cycles);
}
