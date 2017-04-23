A Gameboy (monochrome) emulator written in C++. Very limited compatibility, Tetris is the most playable so far. 
Builds on Mac OSX El Capitan with XCode and requires the SDL2 library. 

![Tetris](/screenshots/Tetris_example_0.bmp) ![Tetris](/screenshots/Tetris_example_1.bmp)

![Dr. Mario](“/screenshots/Dr_Mario_example_0.bmp”)![Dr. Mario](“/screenshots/Dr_Mario_example_1.bmp”)

![Super Mario Land](“/screenshots/Super_Mario_Land_example_0.bmp”)  ![Super Mario Land](“/screenshots/Super_Mario_Land_example_1.bmp”)


Command Line Options
--------------------

| Option               | Meaning                                                                                                                                 |
|----------------------|-----------------------------------------------------------------------------------------------------------------------------------------|
| --rom=<path to file> | Load ROM file from given path. (required)                                                                                               |
| --numcycles=<number> | Number of cycles to run before taking and screenshot then quitting. (for testing, default of 0 meaning run forever)                     |
| --scale=<number>     | Set the dimension of each pixel. default of 1 means 1 Gameboy pixel is 1 pixel on screen, 2 means each pixel is a 2x2 square and so on. |
| skipboot             | Skip the boot ROM. If not set a BIOS file in the same folder called “GameboyBios<i></i>.gb” is required.                                       |

Usage
-----

    ./GameboyEmu --rom=“Tetris (world).gb” --scale=2 skipboot
    ./GameboyEmu --numcycles=100000 --rom=“opus5.gb”

(note that argument order is not important)

Input
-----

|Gameboy Button | Keyboard Key|
|---------------|-------------|
|UP             |up arrow     |
|DOWN           |down arrow   |
|LEFT           |left arrow   |
|RIGHT          |right arrow  |
|B              |z            |
|A              |x            |
|START          |enter        |
|SELECT         |right shift  |

You can also press 's' to take a screenshot and then exit (printing the number of cycles ran) or
press 'esc' to quit directly.

To Do and Known Issues
----------------------
- Upon loosing a round of Tetris the screen fills with blocks apart from the last row.
- Window display is not working (pause menu in Super Mario Land).
- Sound is completely non functional.
- LCD doesn't report modes, so Super Mario Land's scrolling doesn't work.
- 2 player serial over a socket (Tetris).
- Test framework, which is what the screenshot functions are for eventually.
- Background scrolling has some corruption (opus5<i></i>.gb).
- Get the GDB Handler working with the GDB Z80 port.