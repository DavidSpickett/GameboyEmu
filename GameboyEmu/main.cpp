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
    Z80 proc;
    
    proc.mem.AddFile("GameBoyBios.gb", 0);
    
    HardwareIORegs io_regs;
    proc.mem.AddMemoryManager(io_regs);
    
    LCD lcd;
    proc.mem.AddMemoryManager(lcd);
    
    ROMHandler rhandler("Legend of Zelda, The - Link's Awakening (USA, Europe).gb");
    printf("%s\n", rhandler.get_info().c_str());
    if (rhandler.is_cgb_only())
    {
        throw std::runtime_error("ROM is CGB only.");
    }
    proc.mem.AddMemoryManager(rhandler);
    
    while(1)
    {
        SDL_Event event;
        SDL_PollEvent(&event);
        
        Step(proc);
        
        if (proc.pc.read() == 0x100)
        {
            lcd.draw();
            return 0;
            uint8_t foo = 1;
            (void)foo;
        }
    }
    
    return 0;
}
