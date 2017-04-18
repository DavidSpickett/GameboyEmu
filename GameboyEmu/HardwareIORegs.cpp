//
//  HardwareIORegs.cpp
//  GameboyEmu
//
//  Created by David Spickett on 30/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "HardwareIORegs.hpp"

uint8_t HardwareIORegs::read8(uint16_t addr)
{
    switch (addr)
    {
        case TIMEMOD:
            return m_time_mod;
        case TIMECNT:
            return m_time_cnt;
        case TIMECONT:
            return m_time_cont;
        case DIVCOUNT:
            return m_divider_cnt;
        default:
            return 0;
    }
}

void HardwareIORegs::write8(uint16_t addr, uint8_t value)
{
    switch (addr)
    {
        case TIMEMOD:
            m_time_mod = value;
            break;
        case TIMECNT:
            m_time_cnt = value;
            break;
        case TIMECONT:
            m_time_cont = value;
            
            /*Based on clock speed of 4194304Hz / frequency setting to give no. of
            clock cycles.*/
            switch (m_time_cont & 3)
            {
                case 0: //4096
                    m_timer_countdown_start = 1024;
                    break;
                case 1: //262144
                    m_timer_countdown_start = 16;
                    break;
                case 2: //65536
                    m_timer_countdown_start = 64;
                    break;
                case 3: //16384
                    m_timer_countdown_start = 256;
                    break;
                    
            }
            m_timer_countdown = m_timer_countdown_start;
            
            break;
        case DIVCOUNT:
            break;
        default:
            return;
    }
}

void HardwareIORegs::tick(size_t curr_cycles)
{
    if ((curr_cycles - m_cycles) > 16)
    {
        throw std::runtime_error("Timers will not be correct!");
    }
    
    size_t cycles_passed = curr_cycles - m_cycles;
    m_cycles = curr_cycles;
    
    
    //Divider register
    if (m_divider_countdown > cycles_passed)
    {
        m_divider_countdown -= cycles_passed;
    }
    else
    {
        uint32_t spare_cycles = uint32_t(cycles_passed) - m_divider_countdown;
        m_divider_cnt += 1; //Note overflow at 255 is done for us
        m_divider_countdown = 256 - spare_cycles;
    }
    
    //Timer
    if (m_time_cont & 1)
    {
        //Note that we're assuming nothing is going to take more than 16 cycles!
        if (m_timer_countdown > cycles_passed)
        {
            m_timer_countdown -= cycles_passed;
        }
        else
        {
            //We would overflow if we tried to deduct the cycles
            uint32_t spare_cycles = uint32_t(cycles_passed) - m_timer_countdown;
            
            //Inc the actual timer
            m_time_cnt += 1;
            
            //Apply the remainder to the new value, setting it to the start count first
            m_timer_countdown = m_timer_countdown_start - spare_cycles;
        }
        
        //Timer register resets when we hit the
        if (m_time_cnt >= m_time_mod)
        {
            m_time_cnt = 0;
        }
    }
}

uint16_t HardwareIORegs::read16(uint16_t addr)
{
    return 0;
}

void HardwareIORegs::write16(uint16_t addr, uint16_t value)
{
}
