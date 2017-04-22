//
//  main.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include <iostream>
#include "Z80.hpp"
#include "instructions.hpp"
#include "LCD.hpp"
#include "HardwareIORegs.hpp"
#include "RomHandler.hpp"
#include <SDL2/SDL.h>
#include "GDBHandler.hpp"

void skip_bootstrap(Z80& proc)
{
    //Start of cartridge
    proc.pc.write(0x100);
    //Setup stack
    proc.sp.write(0xfffe);
    //Turn of bootstrap program
    proc.mem.write8(0xff50, 0x1);
    //Turn on LCD
    proc.mem.write8(0xff40, 0x91);
}

struct emu_args
{
    emu_args():
        skip_boot(false), scale_factor(1), rom_name("")
    {}
    
    std::string to_str()
    {
        return formatted_string(
                "skip_boot=%d scale_fcator=%d rom_name=%s\n",
                skip_boot,
                scale_factor,
                rom_name.c_str());
    }
    
    bool skip_boot;
    int scale_factor;
    std::string rom_name;
};

emu_args process_args(int argc, const char* argv[])
{
    emu_args a;
    
    for (int i=1; i<argc; ++i)
    {
        const char* arg_ptr = argv[i];
        
        char skip_boot_arg[] = "skipboot";
        if (strncmp(arg_ptr, skip_boot_arg, strlen(skip_boot_arg)) == 0)
        {
            a.skip_boot = true;
        }
        
        char scale_arg[] = "--scale=";
        if (strncmp(arg_ptr, scale_arg, strlen(scale_arg)) == 0)
        {
            const char* factor = argv[i]+strlen(scale_arg);
            a.scale_factor = int(strtol(factor, NULL, 10));
        }
        
        char rom_arg[] = "--rom=";
        if (strncmp(arg_ptr, rom_arg, strlen(rom_arg)) == 0)
        {
            a.rom_name = std::string(arg_ptr+strlen(rom_arg), strlen(arg_ptr));
        }
    }
    
    if (a.rom_name.empty())
    {
        throw std::runtime_error("Rom name is required. (--rom=<path>)");
    }
    
    return a;
}

int main(int argc, const char * argv[]) {
    emu_args a = process_args(argc, argv);
    printf("%s", a.to_str().c_str());
    
    MemoryMap map(a.rom_name, a.skip_boot, a.scale_factor);
    Z80 proc(map);
    map.set_proc_pointers(&proc);
    
    if (a.skip_boot)
    {
        skip_bootstrap(proc);
    }

    SDL_Event event;
    while(1)
    {
        SDL_PollEvent(&event);
        if(event.type == SDL_QUIT)
        {
            break;
        }
        
        /*if (proc.pc.read() == 0x03f2)
        {
            //Bodge to speed up tic tac toe rom when it's playing sound
            printf("Skipped sound loop.\n");
            proc.f.set_z(true);
        }*/
        
        Step(proc);
        
        if (proc.pc.read() == 0x256f)
        {
            //return 0;
            uint8_t foo = 1;
            (void)foo;
        }
    }
    
    return 0;
}
