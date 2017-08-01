//
//  main.cpp
//  GameboyEmu
//
//  Created by David Spickett on 27/09/2016.
//  Copyright © 2016 David Spickett. All rights reserved.
//

#include <SDL2/SDL.h>
#include "Z80.hpp"
#include "instructions.hpp"
#include "utils.hpp"

void screenshot_and_exit(Z80& proc, const std::string& rom_name, bool& _continue)
{
    std::string file_name = rom_name;
    std::replace(file_name.begin(), file_name.end(), '.', '_');
    file_name += "_screenshot.bmp";
    proc.mem.m_lcd_handler.SDLSaveImage(file_name);
    printf("Exiting and saving screenshot to %s after running for %zu cycles.\n", file_name.c_str(), proc.m_total_cycles);
    _continue = false;
}

class InputPollTimer
{
public:
    InputPollTimer():
        m_last_polled(SDL_GetTicks())
    {}
    
    bool ShouldPoll()
    {
        auto current = SDL_GetTicks();
        if ((current - m_last_polled) >= (1000 / 60))
        {
            m_last_polled = current;
            return true;
        }
        return false;
    }
    
private:
    uint32_t m_last_polled;
};

int main(int argc, const char * argv[]) {
    emu_args a = process_args(argc, argv);
    printf("%s", a.to_str().c_str());
    
    MemoryMap map(a.rom_name, a.skip_boot, a.scale_factor);
    Z80 proc(map);
    auto callback = [&proc](uint8_t num) { proc.post_interrupt(num); };
    map.set_int_callback(callback);
    
    if (a.skip_boot)
    {
        proc.skip_bootstrap();
    }

    SDL_Event event;
    bool run = true;
    InputPollTimer input_timer;
    while(run)
    {
        if (input_timer.ShouldPoll())
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
        }
        
        /*if ((a.rom_name == "ttt.gb") && (proc.pc.read() == 0x03f2))
        {
            //Bodge to speed up tic tac toe rom when it's playing sound
            //printf("Skipped sound loop.\n");
            proc.f.set_z(true);
        }*/
        
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
