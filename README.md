# RyChip8
A CHIP-8 emulator that primarily supports the original CHIP-8 specification
for the COSMAC VIP computer, the SUPER-CHIP 1.1 specification, and the
XO-CHIP specification from John Earnest.


## Building
This project relies on the following external build dependencies:
- CMake >=3.16 
- Any C compiler that supports C11

It also includes the following external projects as Git submodules:
- SDL (>=3.2)

When you clone this repository, it is recommended that you also include the previously listed Git submodules using the following command: 
```
git clone --recurse-submodules :URL:
```

After you clone the repository (including submodules), you just need to run the following to build the project:
```
cmake -S . -B build
cmake --build build
```

The built application will be inside the build folder.


## Current Goals
- Add support for more Chip8 variations:
  - The original COSMAC VIP CHIP-8 (NOW SUPPORTED)
  - SCHIP 1.1 (NOW SUPPORTED).
  - XO-Chip 
- Add a GUI that allows users to...
  - Switch between using the COSMAC VIP CHIP-8, SCHIP 1.1, and XO-CHIP emulators.
  - Individually toggle all of the supported CHIP-8 quirks for improved compatibility.
  - Change key bindings
  - Change the color of displayed pixels in emulator.


## Specification
For SCHIP 1.1, I primarily used the following 2 documents
http://devernay.free.fr/hacks/chip8/schip.txt

http://devernay.free.fr/hacks/chip8/C8TECH10.HTM

Note that the 2nd source states that it's for the original CHIP-8, but that
is only partially correct. All of the instructions are from the original 
CHIP-8 specification, but they have been altered with SUPER-CHIP8 quirks.

For more information about CHIP-8 quirks as well as the many
extensions for CHIP-8, view the web pages below:

https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Extensions-Reference#super-chip-erik-bryntse-1991

https://github.com/Chromatophore/HP48-Superchip

https://chip8.gulrak.net/

https://johnearnest.github.io/Octo/docs/SuperChip.html

https://chip-8.github.io/extensions/


## RyChip8 Specifications
Because there is a lot of "quirks" between different CHIP-8 variations and implementations, I'm listing all of the specifications and instructions that this emulator conforms to:

TODO: Include full list of supported instructions and quirks.



## Test Suite Used
For easier debugging, I've used a test suite found at:

https://github.com/Timendus/chip8-test-suite

It contains 8 CHIP-8 test ROMs (with 7 being compatible with CHIP-8, and 1 being for SUPER-CHIP8).

I'm listing it here since it was a major help in finding multiple weird bugs I encountered while playing certain ROM games on RyChip8.


## Supported Quirks.
Since there never was a formal specification for CHIP-8, many minor quirks
and variations sprouted between implementations of CHIP-8. 


Below I will list:
  - The instruction with a quirk
  - How I handle that instruction
  - A description of the quirk and how different interpreters handle the instruction.


- 8xy6 (Shift Right)
  - RyChip8's version:
    - Vx = Vx >> 1, VF = LSB of Vx before shift.
  - Quirk Description
    - On the original COSMAC VIP computers that first CHIP-8 interpreter was written on, this instruction was handled like so: 
      ```
      Vx = Vy >> 1, VF = LSB of Vx before shift. 
      ```
    - CHIP48 and more modern CHIP-8 emulators, however, have changed the original COSMAC VIP version of the instruction to the version RyChip8 uses.
    

- 8xyE (Shift Left)
  - RyChip8's version:
    - Vx = Vx << 1, VF = MSB of Vx before shift.
  - Quirk Description
    - On the original COSMAC VIP computers that first CHIP-8 interpreter was written on, this instruction was handled like so: 
      ```
      Vx = Vy << 1, VF = MSB of Vx before shift. 
      ```
    - CHIP48 and more modern CHIP-8 emulators, however, have changed the original COSMAC VIP version of the instruction to the version RyChip8 uses.

- Fx55 (Save)
  - RyChip8's version:
    - Copy value of registers V0 to Vx (inclusive) into memory starting at `I`. Value of register `I` is not changed.
  - Quirk Description
    - The COSMAC VIP computers that CHIP-8 was first written on incremented the value of register `I` after each V register was placed into memory. CHIP48 and modern CHIP8 emulators no longer increment I.

- Fx65 (Load)
  - RyChip8's version:
    - Starting at memory location specified by register `I`, each byte was copied to V0 to Vx, inclusive.
  - Quirk Description
    - The COSMAC VIP computers that CHIP-8 was first written on incremented the value of register `I` after a byte from memory was written to a V register. CHIP48 and modern CHIP8 emulators no longer increment I.

- Dxyn (Draw) 
  - RyChip8's version
    - If sprite moves to edge of screen, the sprite will wrap around to the other side of the screen. The sprite will not be clipped when at the edge of a screen.
  - Quirk Description
    - Most CHIP8 interpreters will clip sprites if they reach the bottom or right side of the screen rather than wrap them to the other end of the screen. However, the starting location of the sprite (at (Vx, Vy)) IS WRAPPED. So if X = 68 in the framebuffer, it will wrap around to X=5.
    - So the initial location of where to paint the sprite should wrap, but when drawing the sprite, the edge of the screen will clip off the rest of the sprite.


- Bnnn (Jump with Offset)
  - RyChip8's version
    - Jump to value of nnn + V0
  - Quirk Description
    - The original COSMAC VIP CHIP-8 interpreter used the same method as my version for jumps with offsets. 
    - However, the CHIP48 and SUPER-CHIP emulators changed it to work as Bxnn, where:
      ```
      Jump to address XNN + Vx.
      ```



