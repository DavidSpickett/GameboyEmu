//
//  utils.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include "utils.hpp"

namespace
{
    bool find_arg(std::string name, std::string arg)
    {
        return arg.find(name) != std::string::npos;
    }
}

emu_args process_args(int argc, const char* argv[])
{
    emu_args a;
    
    for (auto i=1; i<argc; ++i)
    {
        std::string arg(argv[i]);
        
        if (find_arg("skipboot", arg))
        {
            a.skip_boot = true;
        }
        
        std::string scale_factor = "--scale=";
        if (find_arg(scale_factor, arg))
        {
            a.scale_factor = std::stoi(arg.substr(scale_factor.size(), std::string::npos), NULL, 10);
        }
        
        std::string num_cycles_arg = "--numcycles=";
        if (find_arg(num_cycles_arg, arg))
        {
            a.num_cycles = std::stol(arg.substr(num_cycles_arg.size(), std::string::npos), NULL, 10);
        }
        
        std::string rom_arg = "--rom=";
        if (find_arg(rom_arg, arg))
        {
            a.rom_name = arg.substr(rom_arg.size(), std::string::npos);
        }
    }
    
    if (a.rom_name.empty())
    {
        throw std::runtime_error("Rom name is required. (--rom=<path>)");
    }
    
    return a;
}
