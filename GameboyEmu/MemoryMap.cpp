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

//Again, bounds
void MemoryMap::AddFile(std::string path, uint16_t addr)
{
    //Copy file into memory at given location
    std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
    if ((addr + in.tellg()) > m_mem.size())
    {
        throw std::runtime_error("Cannot add file to memory, overflows end.");
    }
    
    //Go back to beginning and copy to end
    in.seekg(0);
    std::copy(std::istreambuf_iterator<char>(in),
              std::istreambuf_iterator<char>(),
              &m_mem[addr]);
}

void MemoryMap::AddBlock(std::vector<uint8_t>& block, uint16_t addr)
{
    if ((addr + block.size()) > m_mem.size())
    {
        throw std::runtime_error("Cannot add block to memory, overflows end.");
    }
    std:copy(block.begin(), block.end(), m_mem.begin()+addr);
}

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

std::vector<uint8_t> MemoryMap::read_bytes(uint16_t addr, uint16_t num)
{
    if ((addr + num) > m_mem.size())
    {
        throw std::runtime_error(formatted_string("Read of 0x%04x, %d bytes would go off the end of memory.", addr, num));
    }
    
    std::vector<uint8_t> ret(num);
    
    std::vector<uint8_t>::const_iterator begin(m_mem.begin()+addr);
    std::copy(begin, begin+num, ret.begin());
    
    return ret;
}
