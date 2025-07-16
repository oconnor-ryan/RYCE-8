#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

//all Chip8 programs are loaded at address 512 in RAM
#define CHIP8_PROG_START 512

#define CHIP8_WIDTH 64
#define CHIP8_HEIGHT 32


// Font Data for the letters A-F and digits 0-9
/*
"0"	Binary	Hex
****
*  *
*  *
*  *
****

0xF0
0x90
0x90
0x90
0xF0
	
"1"	Binary	Hex
  * 
 ** 
  * 
  * 
 ***

"2"	Binary	Hex
****
   *
****
*   
****
	
"3"	Binary	Hex
****
   *
****
   *
****

"4"	Binary	Hex
*  *
*  *
****
   *
   *
	
"5"	Binary	Hex
****
*   
****
   *
****

"6"	Binary	Hex
****
*   
****
*  *
****
	
"7"	Binary	Hex
****
   *
  * 
 *  
 *  

"8"	Binary	Hex
****
*  *
****
*  *
****
	
"9"	Binary	Hex
****
*  *
****
   *
****

"A"	Binary	Hex
****
*  *
****
*  *
*  *
	
"B"	Binary	Hex
*** 
*  *
*** 
*  *
*** 

"C"	Binary	Hex
****
*   
*   
*   
****
	
"D"	Binary	Hex
*** 
*  *
*  *
*  *
*** 

"E"	Binary	Hex
****
*   
****
*   
****
	
"F"	Binary	Hex
****
*   
****
*   
*   
*/


// Stores the font data to display characters 0-9 and A-F
const uint8_t FONT_DATA_HEX[5 * 16] = {
  0xF0, 0x90, 0x90, 0x90, 0xF,  // "0"
  0x20, 0x60, 0x20, 0x20, 0x70, // "1"
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // "2"
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // "3"
  0x90, 0x90, 0xF0, 0x10, 0x10, // "4"
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // "5"
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // "6"
  0xF0, 0x10, 0x20, 0x40, 0x40, // "7"
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // "8"
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // "9"
  0xF0, 0x90, 0xF0 ,0x90, 0x90, // "A"
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // "B"
  0xF0, 0x80, 0x80, 0x80, 0xF0, // "C"
  0xE0, 0x90, 0x90, 0x90, 0xE0, // "D"
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // "E"
  0xF0, 0x80, 0xF0, 0x80, 0x80  // "F"
};

struct chip8 {

  // Note that the stack and framebuffer are not part of main memory. While the original CHIP-8 interpreters
  // took up the 1st 512 bytes of RAM of a 4K RAM computer and stored its stack in main memory, most modern CHIP-8 emulators
  // now use the 1st 512 bytes to load other data, such as fonts. In addition, the stack was
  // moved out of main memory to allow a few extra bytes for programs to run.

  /* memory */

  uint8_t ram[4096];


  //array of 24 2-byte values used to store return addresses after completing a subroutine. 
  //Thus, there are up to 24 nested subroutines available.
  //The original RCA 1802 microprocesser Chip8 was implemented on states that the stack is 48 bytes long
  uint16_t stack[24]; 





  /* Registers */

  //general purpose register
  uint8_t V[16];

  uint16_t I; //generally used to store memory addresses. Addresses are 12 bits long, so only the 1st 12 bits are generally used.

  //if these registers are non-zero, they automatically decrement by
  //1 at a rate of 60 Hz
  uint8_t sound_timer;
  uint8_t delay_timer;



  //these "registers" are not accessible by Chip8 programs
  uint16_t pc; //program counter
  uint8_t sp; //stack pointer (can be 16 bits, but i chose 8 bits since the stack size is 64 bytes)


  /* Input */

  //there are 16 possible inputs that can be received from the keyboard. Each key is represented as 
  //1 bit. If a bit is 1, the key is being pressed down. If the bit is 0, it is not being pressed.
  uint16_t keyboard_inputs;

  /* Output */
  
  // framebuffer data. Its a 64x32 monochrome display, meaning that only 1 bit is needed to display each pixel
  uint64_t fb[CHIP8_HEIGHT]; 


};

#endif //CHIP8_H

