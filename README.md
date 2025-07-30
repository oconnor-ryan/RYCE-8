# RyChip8
A CHIP-8 emulator that primarily supports the original CHIP-8 specification
for the COSMAC VIP computer, the SUPER-CHIP 1.1 specification, and the
XO-CHIP specification.

## Supported Platforms
This has been primary tested on MacOS, though I did not include any compiler-specific
or platform-specific code, and SDL is a cross-platform library, so I assume this
will work on Linux and Windows.

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

## Usage
`rychip8 --type <VIP | SUPER | XO> <ROM_FILE_PATH>`

After generating the executable, you are required to provide the following 
command line arguments:

* `--type` - Select which CHIP-8 variant to use. The supported types include:
  * VIP - The original CHIP-8 specification for the COSMAC VIP computers by Joseph Weisbecker.
  * SUPER - The SUPER-CHIP 1.1 specification by Erik Bryntse.
  * XO - The XO-CHIP specification by John Earnest

* `<ROM_FILE_PATH>` - The file path of the CHIP-8 ROM you want to run.



## Current Goals
- Add support for more Chip8 variations:
  - The original COSMAC VIP CHIP-8 (NOW SUPPORTED)
  - SCHIP 1.1 (NOW SUPPORTED).
  - XO-Chip 

- Add command line options to configure the emulator...
  - Toggle all supported quirks.
  - Include a `man` page or a --help option to document the command line options.
  - Change the color of the displayed colors.

### Quality of Life Goals
> Note that these features would be great for the users of this emulator, but since
> the focus of this project is emulation and not user experience, these features
> are low priority and will likely not be implemented.

- Add a GUI that allows users to...
  - Switch between using the COSMAC VIP CHIP-8, SCHIP 1.1, and XO-CHIP emulators.
  - Individually toggle all of the supported CHIP-8 quirks for improved compatibility.
  - Change key bindings
  - Change the color of displayed pixels in emulator.

## Supported Quirks
CHIP-8 has had many extensions over the years, many of which coming with their
own quirks that break compatability with the original specification. In addition,
many CHIP-8 implementations and games expect specific quirks that will not work
on all emulators.

Here's the list of all configurable quirks supported by this emulator:

- Bit shift instructions shift `Vy` into `Vx`
  - Affects
  - If enabled, this quirk will force bit shift operations to shift `Vy` into `Vx`
    - `Vx = Vy << 1`
    - `Vx = Vy >> 1`
  - If disabled, this quirk will shift `Vx` by `1` and store the result in `Vx`.
    - `Vx = Vx << 1`
    - `Vx = Vx >> 1`

- Increment I register
  - If enabled, Fx55 and Fx65 will increment `I` by `x + 1`
  - If disabled, the value of `I` will not change

- Reset `VF` On Bitwise Operations
  - If enabled, `VF` is set to 0 for the AND, OR, and XOR instructions
  - If disabled, `VF` is left unchanged by the AND, OR, and XOR instructions

- Clear Screen When Switching Resolution
  - If enabled, the screen is cleared when switching from low resolution mode (64x32) to high resolution mode (128x64) and vise-versa.
  - If disabled, screen is not cleared when changing resolution, leading to artifacts on screen.

- Sprite Wrap On All Sides
  - If enabled, sprites will wrap when it moves into any of the 4 sides of the screen.
  - If disabled, sprites will clip when reaching the right or bottom side of screen. If it reaches the top or left side of the screen, the ENTIRE sprite's position
  will wrap.

- BXNN
  - If enabled, the BXNN instruction jumps the program counter to `XNN + Vx`
  - If disabled, the BNNN instruction jumps the program counter to `NNN + V0`

- Half Pixel Scroll in Low Resolution Mode
  - If enabled, when using the scroll instructions in low resolution mode, it will scroll by "half-pixel" units instead of by "whole pixels".
    - This is due to SUPER-CHIP and XO-CHIP using 128x64 displays, and switching to low resolution simulates a 64x32 display by making each pixel 2x2. Since the scrolling instructions were only made to work for the 128x64 display, it will still scroll by 1 pixel on the 128x64 display, even in low resolution mode.

  - If disabled, the scroll instructions will always scroll by 1 pixel using the current resolution, so scrolling down 1 unit in low resolution mode will move every pixel on the 128x64 display by 2.

## Specifications and Resources
Due to all of the quirks between different CHIP-8 extensions and implementations, finding accurate specifications for CHIP-8 and its extensions is surprisingly difficult.

However, thanks to the efforts of many people, the specifications for the CHIP-8
and its many extensions have been documented quite well, though there is
no single source where you can find all of the specific quirks between the CHIP-8
extensions.


For the machine description of the CHIP-8 virtual machine on the COSMAC VIP computer, I looked at the following links:

- [Matthew Mikolay](https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Technical-Reference)
- [Thomas P. Green](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)



For the original COSMAC VIP CHIP-8 intruction set, it is nicely documented here:

- [Laurence Scotford](https://www.laurencescotford.net/2020/07/25/chip-8-on-the-cosmac-vip-instruction-index/)

- [John Earnest](https://johnearnest.github.io/Octo/docs/chip8ref.pdf)




For SCHIP 1.1, I primarily used this document.
- [Original Spec By Erik Bryntse](http://devernay.free.fr/hacks/chip8/schip.txt)
- [Description of New Instructions](https://johnearnest.github.io/Octo/docs/SuperChip.html)


For XO-CHIP, there is luckily only one source of truth:
- [John Earnest](https://johnearnest.github.io/Octo/docs/XO-ChipSpecification.html)



If you want testing ROMs to test this emulator or even your own emulator, here
are some great resources:
- [CHIP-8 Test Suite](https://github.com/Timendus/chip8-test-suite)
  - This particular resource was EXTREMELY HELPFUL when implementing the CHIP-8 emulator. 
  - It also helped with implementing many of the CHIP-8 quirks.
  - The suite has support for the following specifications:
    - COSMAC VIP CHIP-8
    - SUPER-CHIP (modern)
    - SUPER-CHIP (legacy)
    - XO-CHIP 


For more information about the many CHIP-8 quirks that exist between extensions
and implementations, read the following sources:

- [Gulrak Quirk Chart](https://chip8.gulrak.net/)
- [HP48 SuperChip Research](https://github.com/Chromatophore/HP48-Superchip)
- [Chip8 Extension List](https://chip-8.github.io/extensions/)
- [Another Chip8 Extension List](https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Extensions-Reference)



For more information regarding CHIP-8:

- [Blog About the COSMAC VIP CHIP-8](https://www.laurencescotford.net/tag/chip-8/)
- [VIPER Newsletter Scans (the FIRST CHIP-8 specification)](https://github.com/mattmikolay/viper)



## Similar Projects
- [Octo IDE](https://johnearnest.github.io/Octo/index.html)
  - This is a web-based CHIP-8 IDE for creating CHIP-8 games for the
    original COSMAC VIP, the SUPER-CHIP 1.1, and the XO-CHIP.
  - This was created by the same author who wrote the XO-CHIP specification. 
- [JAXE Emulator](https://github.com/kurtjd/jaxe)