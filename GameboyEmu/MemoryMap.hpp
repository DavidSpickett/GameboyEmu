//
//  Memory.hpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef MemoryMap_hpp
#define MemoryMap_hpp

#include <stdint.h>
#include "utils.hpp"
#include "RomHandler.hpp"
#include "LCD.hpp"
#include "HardwareIORegs.hpp"
#include "InputManager.hpp"
#include "SoundHandler.hpp"

class MemoryMap
{
public:
    MemoryMap(std::string& cartridge_name, bool bootstrap_skipped, int scale_factor):
    m_bootstrap_in_mem(true),
    m_rom_handler(cartridge_name),
    m_lcd_handler(scale_factor),
    m_hardware_regs_handler(),
    m_interrupt_handler(),
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
    }
    
    uint8_t read8(uint16_t addr);
    void write8(uint16_t addr, uint8_t value);
    
    uint16_t read16(uint16_t addr);
    void write16(uint16_t addr, uint16_t value);
    
    void tick(size_t curr_cycles);
    
    InputManager m_input_handler;
    
    LCD m_lcd_handler; //public for screenshots
    
    void set_int_callback(InterruptCallback callback)
    {
        m_rom_handler.post_int = callback;
        m_interrupt_handler.post_int = callback;
        m_lcd_handler.post_int = callback;
        m_input_handler.post_int = callback;
        m_sound_handler.post_int = callback;
        m_rom_handler.post_int = callback;
        m_null_handler.post_int = callback;
        m_default_handler.post_int = callback;
        m_hardware_regs_handler.post_int = callback;
    }
    
private:
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
    } m_dma_transfer;
    
    bool m_bootstrap_in_mem;
    MemoryManager& get_mm(uint16_t addr);
    InterruptManager m_interrupt_handler;
    ROMHandler m_rom_handler;
    HardwareIORegs m_hardware_regs_handler;
    DefaultMemoryManager m_default_handler;
    NullMemoryManager m_null_handler;
    SoundHandler m_sound_handler;
    size_t m_last_tick_cycles;
};


#endif /* MemoryMap_hpp */
