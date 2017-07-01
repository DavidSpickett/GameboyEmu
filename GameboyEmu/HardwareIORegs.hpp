//
//  HardwareIORegs.hpp
//  GameboyEmu
//
//  Created by David Spickett on 30/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef HardwareIORegs_hpp
#define HardwareIORegs_hpp

#include <stdio.h>
#include "MemoryManager.hpp"

const uint16_t DIVCOUNT = 0xff04;
const uint16_t TIMECNT  = 0xff05;
const uint16_t TIMEMOD  = 0xff06;
const uint16_t TIMECONT = 0xff07;

class HardwareIORegs: public MemoryManager
{
public:
    HardwareIORegs():
        m_clock_enable(false), m_timer_countdown_start(1024),
        m_timer_countdown(1024), m_divider_countdown(256), m_cycles(0),
        m_divider_cnt(0), m_time_cont(0), m_time_mod(0), m_time_cnt(0),
        m_serial_data_recieved(0), m_serial_control(0)
    {
    }
    
    void write8(uint16_t addr, uint8_t value);
    uint8_t read8(uint16_t addr);
    
    uint16_t read16(uint16_t addr);
    void write16(uint16_t addr, uint16_t value);
    
    void tick(size_t curr_cycles);

private:
    /*
     Serial clock for original Gameboy is 8192Hz ~= 1Kbyte per second
     1 bit = 1/8192 = 0.0001220703125s = 122uS
     1 main clock cycle  = 1 / 4194304 = 0.238uS
     
     So 8 bits = 8 * 122uS = 976uS
     In main clock cycles = 976uS / 0.238uS = 4100 clock cycles
     4 clock cycles per instruction cycle so 4100/4 = 1025 instr cycles
    */
    struct SerialTransfer
    {
        explicit SerialTransfer(uint8_t v):
            value(v), cycles(1025)
        {}
        
        SerialTransfer():
            value(0), cycles(-1)
        {}
        
        bool valid() { return cycles != -1; }
        
        int cycles;
        uint8_t value;
    } m_serial_transfer;
    
    size_t m_cycles;
    bool m_clock_enable;
    uint8_t m_divider_cnt;
    uint8_t m_time_cont;
    uint32_t m_timer_countdown_start;
    uint32_t m_timer_countdown;
    uint32_t m_divider_countdown;
    uint8_t m_time_mod;
    uint8_t m_time_cnt;
    
    uint8_t m_serial_data_recieved;
    uint8_t m_serial_control;
};

#endif /* HardwareIORegs_hpp */
