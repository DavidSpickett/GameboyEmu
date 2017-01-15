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
    MemoryMap map("Tetris (World).gb");
    Z80 proc(map);
    //Icky
    map.m_interrupt_handler.m_proc = &proc;
    map.m_lcd_handler.m_proc = &proc;
    
    //skip_bootstrap(proc);

    while(1)
    {
        SDL_Event event;
        SDL_PollEvent(&event);
        
        Step(proc);
        
        if (proc.pc.read() == 0x0040)
        {
            //return 0;
            uint8_t foo = 1;
            (void)foo;
        }
    }
    
    return 0;
}
