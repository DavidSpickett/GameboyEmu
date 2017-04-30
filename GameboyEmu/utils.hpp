//
//  utils.hpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#ifndef utils_hpp
#define utils_hpp

#include <stdio.h>
#include <string>

template <typename T>
void init_array(T& container)
{
    std::fill(container.begin(), container.end(), typename T::value_type());
}

//For reasons unknown to me, if this is in a cpp file, the linker complains.
template< typename... Args >
std::string formatted_string(const char* format, Args... args)
{
    //Nullptr doesn't write anything, just returns length needed
    int length = std::snprintf(nullptr, 0, format, args...);
 
    if (length < 0)
    {
        throw "Error formatting string.";
    }
 
    char* buf = new char[length + 1];
    std::snprintf( buf, length + 1, format, args... );
 
    std::string str(buf);
    delete[] buf;
    return str;
}

struct emu_args
{
    emu_args():
    skip_boot(false),
    scale_factor(1),
    rom_name(""),
    num_cycles(0)
    {}
    
    std::string to_str()
    {
        return formatted_string(
                                "skipboot=%d scale=%d rom=\"%s\" numcycles=%zu\n",
                                skip_boot,
                                scale_factor,
                                rom_name.c_str(),
                                num_cycles);
    }
    
    bool skip_boot;
    int scale_factor;
    std::string rom_name;
    size_t num_cycles;
};

emu_args process_args(int argc, const char* argv[]);
 
#endif /* utils_hpp */
