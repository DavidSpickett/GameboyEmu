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
#include <fstream>
#include "MemoryMap.hpp"

class ROMHandler: public MemoryManager
{
public:
    ROMHandler(std::string file_path):
        MemoryManager(address_range(0x0100, 0x3fff)),
        m_file_path(file_path),
        file_str(std::ifstream(file_path.c_str(), std::ifstream::binary))
    {
    }
    
    //Not sure if inline is actually the right soloution here
    inline void write8(uint16_t addr, uint8_t value);
    inline uint8_t read8(uint16_t addr);
    
    inline bool is_cgb_only();
    inline std::string get_info();
    
private:
    inline uint8_t get_byte(uint16_t addr);
    
    std::string m_file_path;
    std::ifstream file_str;
};

#endif /* RomHandler_hpp */