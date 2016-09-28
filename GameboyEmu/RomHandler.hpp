//
//  RomHandler.hpp
//  GameboyEmu
//
//  Created by David Spickett on 28/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef RomHandler_hpp
#define RomHandler_hpp

#include <stdio.h>
#include <string>
#include "MemoryMap.hpp"
#include <fstream>

class ROMHandler: public MemoryManager
{
public:
    ROMHandler(std::string file_path):
        MemoryManager(address_range(0x0100, 0x3fff)),
        m_file_path(file_path),
    file_str(std::ifstream(file_path.c_str(), std::ifstream::binary))
    {
    }
    
    //For some reason I can't define these in a cpp file
    //even though LCD does the same thing AFAIK.
    void write8(uint16_t addr, uint8_t value)
    {
    }
    
    uint8_t read8(uint16_t addr)
    {
        printf("Read addr: 0x%04x from ROM\n", addr);
        file_str.seekg(addr);
        return file_str.get();
    }
    
private:
    std::string m_file_path;
    std::ifstream file_str;
};

#endif /* RomHandler_hpp */
