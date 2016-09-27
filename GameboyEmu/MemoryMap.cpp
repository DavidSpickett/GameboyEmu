//
//  Memory.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "MemoryMap.hpp"

uint8_t MemoryMap::read8(uint16_t addr)
{
    return m_mem[addr];
}

void MemoryMap::write8(uint16_t addr, uint8_t value)
{
    m_mem[addr] = value;
}

//Check bounds!!
uint16_t MemoryMap::read16(uint16_t addr)
{
    return (m_mem[addr+1] << 8) | m_mem[addr];
}

//Check bounds!
void MemoryMap::write16(uint16_t addr, uint16_t value)
{
    m_mem[addr] = value>>8; m_mem[addr+1] = value&0xff;
}
