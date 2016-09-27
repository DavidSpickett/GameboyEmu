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

const uint16_t MAX_MEM = 0xffff;

class MemoryMap
{
    //NOTE: little endian!!
public:
    MemoryMap()
    {
        m_mem.resize(MAX_MEM);
        std::string f = formatted_string_("%d", 1);
        (void)f;
    }
    
    uint8_t read8(uint16_t addr);
    void write8(uint16_t addr, uint8_t value);
    
    uint16_t read16(uint16_t addr);
    void write16(uint16_t addr, uint16_t value);
    
    std::vector<uint8_t> read_bytes(uint16_t addr, uint16_t num);
    
    void AddFile(std::string path, uint16_t addr);
    void AddBlock(std::vector<uint8_t>&, uint16_t addr);
    
private:
    std::vector<uint8_t> m_mem;
};


#endif /* MemoryMap_hpp */
