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
#include "RomHandler.hpp"
#include "LCD.hpp"
#include "HardwareIORegs.hpp"
#include "InputManager.hpp"
#include "SoundHandler.hpp"

struct DMATransfer
{
    explicit DMATransfer(uint16_t source_addr):
        cycles_remaining(640), source_addr(source_addr)
    {}
    
    DMATransfer():
        cycles_remaining(-1), source_addr(0)
    {}
    
    int cycles_remaining;
    uint16_t source_addr;
};

class MemoryMap
{
public:
    MemoryMap(std::string& cartridge_name, bool bootstrap_skipped, int scale_factor):
    m_bootstrap_in_mem(true),
    m_rom_handler(*this, cartridge_name),
    m_lcd_handler(*this, scale_factor),
    m_hardware_regs_handler(*this),
    m_interrupt_handler(*this),
    m_input_handler(*this),
    m_default_handler(*this),
    m_null_handler(*this),
    m_sound_handler(*this)
    {
        if (!bootstrap_skipped)
        {
            m_default_handler.AddFile("GameBoyBios.gb");
        }
    }
    
    uint8_t read8(uint16_t addr);
    void write8(uint16_t addr, uint8_t value);
    
    uint16_t read16(uint16_t addr);
    void write16(uint16_t addr, uint16_t value);
    
    void tick(size_t curr_cycles);
    
    InputManager m_input_handler;
    
    void post_interrupt(uint8_t num)
    {
        m_post_int(num);
    }
    
    LCD m_lcd_handler; //public for screenshots
    std::function<void(uint8_t)> m_post_int;
    
private:
    DMATransfer m_dma_transfer;
    bool m_bootstrap_in_mem;
    MemoryManager& get_mm(uint16_t addr);
    InterruptManager m_interrupt_handler;
    ROMHandler m_rom_handler;
    HardwareIORegs m_hardware_regs_handler;
    DefaultMemoryManager m_default_handler;
    NullMemoryManager m_null_handler;
    SoundHandler m_sound_handler;
};


#endif /* MemoryMap_hpp */
