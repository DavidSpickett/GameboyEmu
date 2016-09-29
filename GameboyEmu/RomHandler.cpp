//
//  RomHandler.cpp
//  GameboyEmu
//
//  Created by David Spickett on 28/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "RomHandler.hpp"

namespace
{
    std::string get_string(std::ifstream& file, const uint16_t start, size_t len)
    {
        file.seekg(start);
        std::vector<char> temp_str;
        temp_str.resize(len+1);
        
        for (uint16_t addr = start; addr < (start+len); ++addr)
        {
            uint8_t b = file.get();
            temp_str[addr-start] = b;
            
            //Assume unused are all on the end
            if (b == 0)
            {
                break;
            }
        }
        
        return std::string(&temp_str[0]);
    }
    
    std::string cgb_support_to_str(uint8_t code)
    {
        switch (code)
        {
            case 0x00:
                return "CGB Incompatible (0x00)";
            case 0x80:
                return "CGB Compatible (0x80)";
            case 0xC0:
                return "CGB Exclusive (0xc0)";
            default:
                return formatted_string("Unknown (0x%02x)", code);
        }
    }
    
    uint8_t get_cgb_support(std::ifstream& file)
    {
        file.seekg(0x0143);
        return file.get();
    }
    
    std::string sgb_support_to_str(uint8_t value)
    {
        switch (value)
        {
            case 0x00:
                return "Game Boy (will also run on Super Game Boy) (0x00)";
            case 0x03:
                return "Uses Super Game Boy Functions (0x03)";
            default:
                return formatted_string("Unknown (0x%02x)", value);
        }
    }
    
    uint8_t get_cartridge_type(std::ifstream& file)
    {
        file.seekg(0x0147);
        return file.get();
    }
    
    std::string cartridge_type_to_str(uint8_t value)
    {
        std::string type_str;
        switch(value)
        {
            case 0x00:
                type_str = "ROM ONLY";
                break;
            case 0x01:
                type_str = "MBC1";
                break;
            case 0x02:
                type_str = "MBC1+RAM";
                break;
            case 0x03:
                type_str = "MBC1+RAM+BATTERY";
                break;
            case 0x05:
                type_str = "MBC2";
                break;
            case 0x06:
                type_str = "MBC2+BATTERY";
                break;
            case 0x08:
                type_str = "ROM+RAM";
                break;
            case 0x09:
                type_str = "ROM+RAM+BATTERY";
                break;
            case 0x0B:
                type_str = "MMM01";
                break;
            case 0x0C:
                type_str = "MMM01+RAM";
                break;
            case 0x0D:
                type_str = "MMM01+RAM+BATTERY";
                break;
            case 0x0F:
                type_str = "MBC3+TIMER+BATTERY";
                break;
            case 0x10:
                type_str = "MBC3+TIMER+RAM+BATTERY";
                break;
            case 0x11:
                type_str = "MBC3";
                break;
            case 0x12:
                type_str = "MBC3+RAM";
                break;
            case 0x13:
                type_str = "MBC3+RAM+BATTERY";
                break;
            case 0x15:
                type_str = "MBC4";
                break;
            case 0x16:
                type_str = "MBC4+RAM";
                break;
            case 0x17:
                type_str = "MBC4+RAM+BATTERY";
                break;
            case 0x19:
                type_str = "MBC5";
                break;
            case 0x1A:
                type_str = "MBC5+RAM";
                break;
            case 0x1B:
                type_str = "MBC5+RAM+BATTERY";
                break;
            case 0x1C:
                type_str = "MBC5+RUMBLE";
                break;
            case 0x1D:
                type_str = "MBC5+RUMBLE+RAM";
                break;
            case 0x1E:
                type_str = "MBC5+RUMBLE+RAM+BATTERY";
                break;
            case 0x20:
                type_str = "MBC6";
                break;
            case 0x22:
                type_str = "MBC7+SENSOR+RUMBLE+RAM+BATTERY";
                break;
            case 0xFC:
                type_str = "POCKET CAMERA";
                break;
            case 0xFD:
                type_str = "BANDAI TAMA5";
                break;
            case 0xFE:
                type_str = "HuC3";
                break;
            case 0xFF:
                type_str = "HuC1+RAM+BATTERY";
                break;
            default:
                type_str = "Unknown";
                break;
        }
        
        return formatted_string("%s (0x%02x)", type_str.c_str(), value);
    }
    
    uint8_t get_rom_size(std::ifstream& file)
    {
        file.seekg(0x0148);
        return file.get();
    }
    
    std::string rom_size_to_str(uint8_t value)
    {
        std::string rom_size;
        switch (value)
        {
            case 0x00:
                rom_size = "32KByte (no ROM banking)";
                break;
            case 0x01:
                rom_size = "64KByte (4 banks)";
                break;
            case 0x02:
                rom_size = "128KByte (8 banks)";
                break;
            case 0x03:
                rom_size = "256KByte (16 banks)";
                break;
            case 0x04:
                rom_size = "512KByte (32 banks)";
                break;
            case 0x05:
                rom_size = "1MByte (64 banks)  - only 63 banks used by MBC1";
                break;
            case 0x06:
                rom_size = "2MByte (128 banks) - only 125 banks used by MBC1";
                break;
            case 0x07:
                rom_size = "4MByte (256 banks)";
                break;
            case 0x52:
                rom_size = "1.1MByte (72 banks)";
                break;
            case 0x53:
                rom_size = "1.2MByte (80 banks)";
                break;
            case 0x54:
                rom_size = "1.5MByte (96 banks)";
                break;
            default:
                rom_size = "Unknown";
                break;
        }
        
        return formatted_string("%s (0x%02x)", rom_size.c_str(), value);
    }
    
    uint8_t get_ram_size(std::ifstream& file)
    {
        file.seekg(0x0149);
        return file.get();
    }
    
    std::string ram_size_to_str(uint8_t value)
    {
        std::string ram_size;
        
        switch (value)
        {
            case 0x00:
                ram_size = "None";
                break;
            case 0x01:
                ram_size = "2 KBytes";
                break;
            case 0x02:
                ram_size = "8 Kbytes";
                break;
            case 0x03:
                ram_size = "32 KBytes (4 banks of 8KBytes each)";
                break;
            case 0x04:
                ram_size = "128 KBytes (16 banks of 8KBytes each)";
                break;
            case 0x05:
                ram_size = "64 KBytes (8 banks of 8KBytes each)";
                break;
            default:
                ram_size = "Unknown";
                break;
        }
        
        return formatted_string("%s (0x%02x)", ram_size.c_str(), value);
    }
    
    uint8_t get_dest_code(std::ifstream& file)
    {
        file.seekg(0x014a);
        return file.get();
    }
    
    std::string dest_code_to_str(uint8_t value)
    {
        switch (value)
        {
            case 0x00:
                return "Japanese (0x00)";
            case 0x01:
                return "Non-Japanese (0x01)";
            default:
                return formatted_string("Unknown (0x%02x)", value);
        }
    }
    
    uint8_t get_rom_version(std::ifstream& file)
    {
        file.seekg(0x014C);
        return file.get();
    }
    
    uint8_t get_checksum(std::ifstream& file)
    {
        //Sum of all header bytes, plus the number of header bytes must equal 0.
        uint8_t chksum = 0;
        file.seekg(0x0134);
        
        for (uint16_t i=0; i<26; ++i)
        {
            uint8_t val = file.get();
            chksum += val;
        }
        
        return chksum+0x19;
    }
}

bool ROMHandler::is_cgb_only()
{
    return get_cgb_support(file_str) == 0xC0;
}

std::string ROMHandler::get_info()
{
    std::string title = get_string(file_str, 0x0134, 11);
    std::string game_code = get_string(file_str, 0x013f, 4);
    std::string cgb_supp = cgb_support_to_str(get_cgb_support(file_str));
    std::string maker_code = get_string(file_str, 0x0144, 2);
    std::string sgb_supp = sgb_support_to_str(get_byte(0x0146));
    std::string cartridge_type = cartridge_type_to_str(get_cartridge_type(file_str));
    std::string rom_size = rom_size_to_str(get_rom_size(file_str));
    std::string ram_size = ram_size_to_str(get_ram_size(file_str));
    std::string dest_code = dest_code_to_str(get_dest_code(file_str));
    std::string rom_version = formatted_string("%d", get_rom_version(file_str));
    std::string header_chksm = formatted_string("0x%02x", get_checksum(file_str));
    
    return formatted_string(
        "           Title: %s\n"
        "       Game Code: %s\n"
        "CGB Support Code: %s\n"
        "      Maker Code: %s\n"
        "SGB Support Code: %s\n"
        "  Cartridge Type: %s\n"
        "        ROM Size: %s\n"
        "        RAM Size: %s\n"
        "       Dest Code: %s\n"
        "     ROM Version: %s\n"
        " Header Checksum: %s\n",
        title.c_str(),
        game_code.c_str(),
        cgb_supp.c_str(),
        maker_code.c_str(),
        sgb_supp.c_str(),
        cartridge_type.c_str(),
        rom_size.c_str(),
        ram_size.c_str(),
        dest_code.c_str(),
        rom_version.c_str(),
        header_chksm.c_str()
        );
}

uint8_t ROMHandler::read8(uint16_t addr)
{
    printf("Read addr: 0x%04x from ROM\n", addr);
    return get_byte(addr);
}

void ROMHandler::write8(uint16_t addr, uint8_t value)
{
    
}

uint8_t ROMHandler::get_byte(uint16_t addr)
{
    file_str.seekg(addr);
    return file_str.get();
}