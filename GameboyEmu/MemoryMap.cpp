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

bool MemoryManager::contains(uint16_t addr) const
{
    std::vector<address_range>::const_iterator it = m_address_ranges.begin();
    for ( ; it!= m_address_ranges.end(); ++it)
    {
        if (it->contains_addr(addr))
        {
            return true;
        }
    }
    
    return false;
}

std::vector<address_range> to_vector(address_range rng)
{
    std::vector<address_range> ret(1, rng);
    return ret;
}
std::vector<address_range> to_vector(address_range rng, address_range rng2)
{
    std::vector<address_range> ret;
    ret.push_back(rng);
    ret.push_back(rng2);
    return ret;
}

void MemoryMap::AddMemoryManager(MemoryManager& manager)
{
    m_memory_managers.push_back(std::reference_wrapper<MemoryManager>(manager));
}

void MemoryMap::AddFile(std::string path, uint16_t addr)
{
    //Copy file into memory at given location
    std::ifstream in(path.c_str(), std::ifstream::ate | std::ifstream::binary);
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
    std::vector<std::reference_wrapper<MemoryManager>>::const_iterator it = m_memory_managers.begin();
    for (; it != m_memory_managers.end(); ++it)
    {
        if (it->get().contains(addr))
        {
            return it->get().read8(addr);
        }
    }
    
    return m_mem[addr];
}

void MemoryMap::write8(uint16_t addr, uint8_t value)
{
    //Bodge to get the bootstrap out of memory
    if ((addr == 0xff50) && (value == 1))
    {
        //Find memory manager that starts at 0x100 currently
        std::vector<std::reference_wrapper<MemoryManager>>::iterator it = m_memory_managers.begin();
        for (; it != m_memory_managers.end(); ++it)
        {
            if (it->get().contains(0x100))
            {
                it->get().m_address_ranges.push_back(address_range(0, 0xff));
            }
        }
    }
    
    std::vector<std::reference_wrapper<MemoryManager>>::const_iterator it = m_memory_managers.begin();
    for (; it != m_memory_managers.end(); ++it)
    {
        std::vector<address_range>::const_iterator ar = it->get().m_address_ranges.begin();
        for (; ar != it->get().m_address_ranges.end(); ++ar)
        {
            if (it->get().contains(addr))
            {
                return it->get().write8(addr, value);
            }
        }
    }
    
    m_mem[addr] = value;
}

//Check bounds!!
uint16_t MemoryMap::read16(uint16_t addr)
{
    std::vector<std::reference_wrapper<MemoryManager>>::const_iterator it = m_memory_managers.begin();
    for (; it != m_memory_managers.end(); ++it)
    {
        if (it->get().contains(addr))
        {
            return it->get().read16(addr);
        }
    }
    
    return (m_mem[addr+1] << 8) | m_mem[addr];
}

//Check bounds!
void MemoryMap::write16(uint16_t addr, uint16_t value)
{
    std::vector<std::reference_wrapper<MemoryManager>>::const_iterator it = m_memory_managers.begin();
    for (; it != m_memory_managers.end(); ++it)
    {
        if (it->get().contains(addr))
        {
            return it->get().write16(addr, value);
        }
    }
    
    m_mem[addr] = value&0xff;
    m_mem[addr+1] = value>>8;
}

void MemoryMap::tick(size_t curr_cycles) const
{
    std::vector<std::reference_wrapper<MemoryManager>>::const_iterator it = m_memory_managers.begin();
    for( ; it != m_memory_managers.end(); ++it)
    {
        it->get().tick(curr_cycles);
    }
}
