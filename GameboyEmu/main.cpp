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

int main(int argc, const char * argv[]) {
    MemoryMap mem;
    Z80 proc(mem);
    
    std::cout << "out\n";
    return 0;
}
