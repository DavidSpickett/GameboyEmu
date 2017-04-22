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
#include "utils.hpp"

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

void screenshot_and_exit(Z80& proc, std::string rom_name, bool& _continue)
{
    std::string file_name = rom_name;
    std::replace(file_name.begin(), file_name.end(), '.', '_');
    file_name += "_screenshot.bmp";
    proc.mem.m_lcd_handler.SDLSaveImage(file_name);
    printf("Exiting and saving screenshot to %s after running for %zu cycles.\n", file_name.c_str(), proc.m_total_cycles);
    _continue = false;
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
    bool run = true;
    while(run)
    {
        SDL_PollEvent(&event);
        switch (event.type)
        {
            case SDL_QUIT:
                run = false;
                break;
            case SDL_KEYDOWN:
            {
                const uint8_t *state = SDL_GetKeyboardState(NULL);
                if (state[SDL_SCANCODE_S])
                {
                    screenshot_and_exit(proc, a.rom_name, run);
                    break;
                }
                else if (state[SDL_SCANCODE_ESCAPE])
                {
                    run = false;
                    break;
                }
            }
        }
        
        if ((a.rom_name == "ttt.gb") && (proc.pc.read() == 0x03f2))
        {
            //Bodge to speed up tic tac toe rom when it's playing sound
            //printf("Skipped sound loop.\n");
            proc.f.set_z(true);
        }
        
        Step(proc);
        
        if ((a.num_cycles != 0) && (proc.m_total_cycles >= a.num_cycles))
        {
            screenshot_and_exit(proc, a.rom_name, run);
        }
        
        if (proc.pc.read() == 0x256f)
        {
            //return 0;
            uint8_t foo = 1;
            (void)foo;
        }
    }
    
    return 0;
}
