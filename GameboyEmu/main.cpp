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
    MemoryMap map("ttt.gb");
    //MemoryMap map("Tetris (World).gb");
    //MemoryMap map("gb-test-roms-master/cpu_instrs/individual/04-op r,imm.gb");
    Z80 proc(map);
    //Icky
    map.set_proc_pointers(&proc);
    
    skip_bootstrap(proc);

    SDL_Event event;
    while(1)
    {
        SDL_PollEvent(&event);
        
        if(event.type == SDL_QUIT)
        {
            break;
        }
        
        Step(proc);
        
        /*if (proc.pc.read() == 0x2400)
        {
            //return 0;
            uint8_t foo = 1;
            (void)foo;
        }*/
    }
    
    return 0;
}
