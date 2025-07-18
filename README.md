# RyChip8
A CHIP-8 emulator. Can run most modern CHIP-8 ROMs.


## Supported Quirks.
Since there never was a format specification for CHIP-8, many minor quirks
and variations sprouted between interpreters. Certain instructions were slightly altered thoughout the years while still claiming to be valid CHIP-8.

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



## Current Goals
- Continue to fix any emulator behaviors.
- Add option to select a ROM to load for the emulator:
  - https://wiki.libsdl.org/SDL3/SDL_ShowOpenFileDialog
- Add option to modify what instruction quirks to use within the emulator, allowing the emulator to run more types of CHIP-8 programs that rely on those quirks.
- Add menu to change key bindings.