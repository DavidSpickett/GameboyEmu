//
//  HardwareIORegs.cpp
//  GameboyEmu
//
//  Created by David Spickett on 30/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "HardwareIORegs.hpp"
#include "Z80.hpp"

namespace
{
    const uint16_t DIVCOUNT = 0xff04;
    const uint16_t TIMECNT  = 0xff05;
    const uint16_t TIMEMOD  = 0xff06;
    const uint16_t TIMECONT = 0xff07;
}

uint8_t HardwareIORegs::read8(uint16_t addr)
{
    switch (addr)
    {
        case SERIAL_CONTROL:
            //Not sure if we need to remove the transfer bit, probably won't get read anyway
            return m_serial_control;
        case SERIAL_DATA:
            return m_serial_data_recieved;
        case TIMEMOD:
            return m_time_mod;
        case TIMECNT:
            return m_time_cnt;
        case TIMECONT:
            return m_time_cont;
        case DIVCOUNT:
            return m_divider_cnt;
        case INTERRUPT_FLAGS:
            return m_interrupt_flags;
        case INTERRUPT_SWITCH:
            return m_interrupt_switch;
        default:
            return 0;
    }
}

void HardwareIORegs::write8(uint16_t addr, uint8_t value)
{
    switch (addr)
    {
        case SERIAL_CONTROL:
            //Starting a transfer
            if (value & (1<<7))
            {
                //With internal clock
                if (value & 1)
                {
                    if (m_serial_transfer.valid())
                    {
                        throw std::runtime_error("Tried to start a serial transfer with one already in progress!");
                    }
                    
                    //Nothing connected returns 0xff
                    m_serial_transfer = SerialTransfer(0xff);
                }
                else
                {
                    m_serial_data_recieved = 0x0;
                }
            }
            m_serial_control = value;
            break;
        case SERIAL_DATA:
            //Print for instruction tests
            printf("%c", value);
            break;
        case TIMEMOD:
            m_time_mod = value;
            break;
        case TIMECNT:
            m_time_cnt = value;
            break;
        case TIMECONT:
            m_time_cont = value;
            
            m_clock_enable = value & (1 << 2);
            
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
            m_divider_cnt = 0;
            break;
        case INTERRUPT_FLAGS:
            m_interrupt_flags = value;
            return;
        case INTERRUPT_SWITCH:
            m_interrupt_switch = value;
            return;
        default:
            return;
    }
}

void HardwareIORegs::tick(size_t curr_cycles)
{
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
    if (m_clock_enable)
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
            
            //Inc the actual timer register
            if (m_time_cnt == 0xff)
            {
                //When it overflows at 255 we set it back to the time mod value.
                //time mod is NOT a limit, it's a starting point.
                m_time_cnt = m_time_mod;
                post_int(TIMER_OVERFLOW);
            }
            else
            {
                m_time_cnt += 1;
            }
            
            //Apply the remainder to the new value, setting it to the start count first
            m_timer_countdown = m_timer_countdown_start - spare_cycles;
        }
    }
    
    //Serial is lowest priority int so it's at the end
    if (m_serial_transfer.valid())
    {
        m_serial_transfer.cycles -= cycles_passed;
        if (m_serial_transfer.cycles <= 0)
        {
            m_serial_transfer.cycles = -1;
            m_serial_data_recieved = m_serial_transfer.value;
            post_int(END_SERIAL);
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
