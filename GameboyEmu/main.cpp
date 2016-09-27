//
//  main.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include <iostream>
#include "MemoryMap.hpp"
#include "Z80.hpp"
#include "instructions.hpp"

int main(int argc, const char * argv[]) {
    MemoryMap mem;
    mem.AddFile("GameBoyBios.gb", 0);
    Z80 proc(mem);
    
    while(1)
    {
        Step(proc);
    }
    
    return 0;
}
