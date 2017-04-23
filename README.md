A Gameboy (monochrome) emulator written in C++. Very limited compatibility,
Tetris is working fine without sound. Builds on Mac OSX El Capitan with XCode and 
requires the SDL2 library. 

Command line options
--------------------
    * --rom=<path rom>     Load the ROM file from the given path.
    * —-numcycles=<number> Number of CPU cycles to run before saving a screenshot
                         and quitting. Mainly for testing.
    * skipboot             Skip the boot ROM. If not given then a BIOS file called
                         "GameBoyBios.gb" is required in the same folder as the
                         emulator.
    * --scale=<number>     Set the dimension of each pixel on screen. 1 means 1 Gameboy
                         pixel is one pixel on screen. 2 means each pixel is a 2x2 square
                         and so on. Note that screenshots are taken at the window size
                         not the Gameboy's resolution.

Input keys
----------

|Gameboy Button | Keyboard key|
|---------------|-------------|
|UP             |up arrow     |
|DOWN           |down arrow   |
|LEFT           |left arrow   |
|RIGHT          |right arrow  |
|B              |z            |
|A              |x            |
|START          |enter        |
|SELECT         |right shift  |

You can also press 's' to take a screenshot and then exit (printing the no. of cycles ran) or
press 'esc' to quit directly.

Things to do/known issues
-------------------------
    - Upon loosing a round of Tetris the screen fills with blocks apart from the last row.
    - Window display is not working. (pause menu in Super Mario Land)
    - Sound is completely non functional.
    - LCD doesn't report modes, so Super Mario Land's scrolling doesn't work.
    - 2 player serial over a socket (Tetris).
    - Test framework, which is what the screenshot functions are for eventually.
    - Background scrolling has some corruption (opus5.gb).
    - Get the GDB Handler working with the GDB Z80 port.