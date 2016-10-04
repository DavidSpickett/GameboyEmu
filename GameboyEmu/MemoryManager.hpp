//
//  MemoryManager.hpp
//  GameboyEmu
//
//  Created by David Spickett on 03/10/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef MemoryManager_hpp
#define MemoryManager_hpp

#include <stdio.h>
#include <stdint.h>
#include "utils.hpp"

class Z80;

const uint16_t LCD_MEM_START  = 0x8000;
const uint16_t LCD_MEM_END    = 0xa000;
const uint16_t LCD_REGS_START = 0xff40;
const uint16_t LCD_REGS_END   = 0xff48;

const uint16_t ROM_START = 0x0100; //Before bootstrap turns off that is
const uint16_t ROM_END   = 0x4000;

const uint16_t CART_RAM_START = 0xa000;
const uint16_t CART_RAM_END   = 0xc000;

const uint16_t HARDWARE_REGS_START = 0xff00;
const uint16_t HARDWARE_REGS_END   = 0xff27;

const uint16_t GB_RAM_START = 0xc000;
const uint16_t GB_RAM_END   = 0xe000;

const uint16_t GB_HIGH_RAM_START = 0xFF80;
const uint16_t GB_HIGH_RAM_END   = 0xFFFF;

const uint16_t ECHO_RAM_START = 0xe000;
const uint16_t ECHO_RAM_END   = 0xffe0;

const uint16_t UNUSED_START = 0xFEA0;
const uint16_t UNUSED_END   = 0xFEFF;

const uint16_t INTERRUPT_FLAGS  = 0xff0f;
const uint16_t INTERRUPT_SWITCH = 0xffff;

const uint16_t SWITCHABLE_ROM_START = 0x4000;
const uint16_t SWITCHABLE_ROM_END   = 0x8000;

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
};

class NullMemoryManager: public MemoryManager
{
    uint8_t read8(uint16_t addr) { return 0; }
    void write8(uint16_t addr, uint8_t value) {}
    uint16_t read16(uint16_t addr) { return 0; };
    void write16(uint16_t addr, uint16_t value) {}
    void tick(size_t curr_cycles) {}
};

class InterruptManager: public MemoryManager
{
public:
    InterruptManager():
    m_proc(nullptr)
    {
    }
    
    uint8_t read8(uint16_t addr)
    {
        switch (addr)
        {
            case INTERRUPT_FLAGS:
                return m_interrupt_flags;
            case INTERRUPT_SWITCH:
                return m_interrupt_switch;
        }
        
        throw std::runtime_error(formatted_string("Addr 0x%04x read from interrupt handler!", addr));
    }
    
    void write8(uint16_t addr, uint8_t value)
    {
        switch (addr)
        {
            case INTERRUPT_FLAGS:
                m_interrupt_flags = value;
                return;
            case INTERRUPT_SWITCH:
                m_interrupt_switch = value;
                return;
        }
    }
    
    uint16_t read16(uint16_t addr) { throw std::runtime_error("Read 16 from interrupt handler!"); }
    void write16(uint16_t addr, uint16_t value) { throw std::runtime_error("Write 16 to interrupt handler!"); }
        
    void tick(size_t curr_cylces) {} //TOOD: actually generate the interrupts
    
    Z80* m_proc; //Could be reduced to a callback function, to be neater.

private:
    uint8_t m_interrupt_flags;
    uint8_t m_interrupt_switch;
};

#endif /* MemoryManager_hpp */
