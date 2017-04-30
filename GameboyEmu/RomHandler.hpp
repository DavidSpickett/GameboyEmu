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

const uint16_t ROM_MODE = 0;
const uint16_t RAM_MODE = 1;
const uint16_t ROM_FIXED_BLOCK_SIZE = 0x4000;

class ROMHandler: public MemoryManager
{
public:
    explicit ROMHandler(MemoryMap& map, std::string file_path):
        MemoryManager(map),
        m_file_path(file_path),
        m_rom_bank_no(1), //Starts at 1 because bank 0 is permemnantley mapped
        m_ram_bank_no(0),
        m_ram_enable(false),
        m_rom_ram_mode(RAM_MODE) //Zelda seems to expect RAM mode to start with
    {
        //std::streampos file_size = 0;
        std::ifstream file_str = std::ifstream(file_path.c_str(), std::ifstream::binary);
        if (!file_str.is_open())
        {
            throw std::runtime_error(formatted_string("File %s does not exist.", file_path.c_str()));
        }
        
        m_rom_contents = std::vector<uint8_t>(std::istreambuf_iterator<char>(file_str), std::istreambuf_iterator<char>());
        
        printf("%s\n", get_info().c_str());
        if (is_cgb_only())
        {
            throw std::runtime_error("ROM is CGB only.");
        }
        
        switch (get_ram_size())
        {
            case 0x00:
                break;
            case 0x01: //2K
                m_ram_bank = std::vector<uint8_t>(2*1024, 0);
                break;
            case 0x02: //8k
                m_ram_bank = std::vector<uint8_t>(8*1024, 0);
                break;
            default:
                printf("Unsupported RAM size!!!");
                break;
        }
    }
    
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
