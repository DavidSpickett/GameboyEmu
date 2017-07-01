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
#include <vector>
#include "MemoryManager.hpp"

class ROMHandler: public MemoryManager
{
public:
    explicit ROMHandler(std::string file_path);
    
    void write8(uint16_t addr, uint8_t value);
    uint8_t read8(uint16_t addr);
    
    uint16_t read16(uint16_t addr);
    void write16(uint16_t addr, uint16_t value);
    
    bool is_cgb_only();
    std::string get_info();
    
    void tick(size_t curr_cycles) {}
    
private:
    std::vector<uint8_t> m_rom_contents;
    std::string get_string(const uint16_t start, size_t len);
    
    std::string cgb_support_to_str(uint8_t code);
    uint8_t get_cgb_support();
    std::string sgb_support_to_str(uint8_t value);
    uint8_t get_cartridge_type();
    std::string cartridge_type_to_str(uint8_t value);
    uint8_t get_rom_size();
    std::string rom_size_to_str(uint8_t value);
    uint8_t get_ram_size();
    std::string ram_size_to_str(uint8_t value);
    uint8_t get_dest_code();
    std::string dest_code_to_str(uint8_t value);
    uint8_t get_rom_version();
    uint8_t get_checksum();
    
    int m_rom_bank_no;
    std::string m_file_path;
    
    int m_rom_ram_mode;
    
    int m_ram_bank_no;
    bool m_ram_enable;
    std::vector<uint8_t> m_ram_bank;
};

#endif /* RomHandler_hpp */
