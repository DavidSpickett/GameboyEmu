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

struct address_range {
    address_range(uint16_t start, uint16_t end):
        start(start), end(end)
    {}
    
    uint16_t start;
    uint16_t end;
    
    uint16_t size() const
    {
        return end-start;
    }
    
    bool contains_addr(uint16_t addr) const
    {
        //Inclusive, so I don't have to deal with the end of memory
        return ((addr >= start) && (addr <= end));
    }
};

class MemoryManager
{
public:
    MemoryManager(address_range rng):
        m_address_range(rng)
    {
    }
    
    virtual uint8_t read8(uint16_t addr) = 0;
    virtual void write8(uint16_t addr, uint8_t value) = 0;
    
    address_range m_address_range;
};

struct address_range_entry
{
    address_range_entry(address_range rng, MemoryManager& manager):
        rng(rng), manager(manager)
    {}
    
    address_range rng;
    MemoryManager& manager;
};

class MemoryMap
{
public:
    MemoryMap()
    {
        m_mem.resize(0xffff);
    }
    
    uint8_t read8(uint16_t addr);
    void write8(uint16_t addr, uint8_t value);
    
    uint16_t read16(uint16_t addr);
    void write16(uint16_t addr, uint16_t value);
    
    std::vector<uint8_t> read_bytes(uint16_t addr, uint16_t num);
    
    void AddFile(std::string path, uint16_t addr);
    void AddBlock(std::vector<uint8_t>&, uint16_t addr);
    
    //Note: only affects 8 bit accesses for now.
    void AddMemoryManager(MemoryManager& manager);
    
private:
    std::vector<uint8_t> m_mem;
    std::vector<std::reference_wrapper<MemoryManager> > m_memory_managers;
};


#endif /* MemoryMap_hpp */
