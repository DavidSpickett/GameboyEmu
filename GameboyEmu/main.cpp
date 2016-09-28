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
#include "RomHandler.cpp"

int main(int argc, const char * argv[]) {
    Z80 proc;
    proc.mem.AddFile("GameBoyBios.gb", 0);
    LCD lcd;
    proc.mem.AddMemoryManager(lcd);
    ROMHandler rhandler("Super Mario Land (World).gb");
    proc.mem.AddMemoryManager(rhandler);
    
    while(1)
    {
        Step(proc);
        
        //Uncomment to break on a particular PC
        if (proc.pc.read() == 0xe7)
        {
            uint8_t foo = 1;
            (void)foo;
        }
    }
    
    return 0;
}
