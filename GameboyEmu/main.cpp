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

int main(int argc, const char * argv[]) {
    bool skip_boot = true;
    std::string rom_name = "Tetris (World).gb";
    
    MemoryMap map(rom_name, skip_boot);
    Z80 proc(map);
    map.set_proc_pointers(&proc);
    
    if (skip_boot)
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
