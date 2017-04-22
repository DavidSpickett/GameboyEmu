//
//  utils.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "utils.hpp"
#include <stdlib.h>

bool find_arg(std::string name, const char* arg)
{
    return strncmp(arg, name.c_str(), name.size()) == 0;
}

emu_args process_args(int argc, const char* argv[])
{
    emu_args a;
    
    for (int i=1; i<argc; ++i)
    {
        const char* arg_ptr = argv[i];
        
        if (find_arg("skipboot", arg_ptr))
        {
            a.skip_boot = true;
        }
        
        std::string scale_factor = "--scale=";
        if (find_arg(scale_factor, arg_ptr))
        {
            const char* factor = argv[i]+scale_factor.size();
            a.scale_factor = int(strtol(factor, NULL, 10));
        }
        
        std::string num_cycles_arg = "--numcycles=";
        if (find_arg(num_cycles_arg, arg_ptr))
        {
            const char* num = argv[i]+num_cycles_arg.size();
            a.num_cycles = strtol(num, NULL, 10);
        }
        
        std::string rom_arg = "--rom=";
        if (find_arg(rom_arg, arg_ptr))
        {
            a.rom_name = std::string(arg_ptr+rom_arg.size(), strlen(arg_ptr));
        }
    }
    
    if (a.rom_name.empty())
    {
        throw std::runtime_error("Rom name is required. (--rom=<path>)");
    }
    
    return a;
}
