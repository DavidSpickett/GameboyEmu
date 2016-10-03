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

int main(int argc, const char * argv[]) {
    MemoryMap map = MemoryMap("Tetris (World).gb");
    Z80 proc(map);
   
    while(1)
    {
        SDL_Event event;
        SDL_PollEvent(&event);
        
        Step(proc);
        
        if (proc.pc.read() == 0x100)
        {
            return 0;
            uint8_t foo = 1;
            (void)foo;
        }
    }
    
    return 0;
}
