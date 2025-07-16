#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdio.h>

#define CHIP8_RAM_LIMIT 4096

//all Chip8 programs are loaded at address 512 in RAM
#define CHIP8_PROG_START 512

#define CHIP8_WIDTH 64
#define CHIP8_HEIGHT 32
#define CHIP8_STACK_LIMIT 24

/*
  Keyboard Layout: 

   1	2	3	C
   4	5	6	D
   7	8	9	E
   A	0	B	F
*/
enum chip8_key {
   CHIP8_KEY_0 = 1 << 0,
   CHIP8_KEY_1 = 1 << 1,
   CHIP8_KEY_2 = 1 << 2,
   CHIP8_KEY_3 = 1 << 3,
   CHIP8_KEY_4 = 1 << 4,
   CHIP8_KEY_5 = 1 << 5,
   CHIP8_KEY_6 = 1 << 6,
   CHIP8_KEY_7 = 1 << 7,
   CHIP8_KEY_8 = 1 << 8,
   CHIP8_KEY_9 = 1 << 9,
   CHIP8_KEY_A = 1 << 10,
   CHIP8_KEY_B = 1 << 11,
   CHIP8_KEY_C = 1 << 12,
   CHIP8_KEY_D = 1 << 13,
   CHIP8_KEY_E = 1 << 14,
   CHIP8_KEY_F = 1 << 15
}; 

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
extern const uint8_t FONT_DATA_HEX[5 * 16];

struct chip8 {

  // Note that the stack and framebuffer are not part of main memory. While the original CHIP-8 interpreters
  // took up the 1st 512 bytes of RAM of a 4K RAM computer and stored its stack in main memory, most modern CHIP-8 emulators
  // now use the 1st 512 bytes to load other data, such as fonts. In addition, the stack was
  // moved out of main memory to allow a few extra bytes for programs to run.

  /* memory */

  uint8_t ram[CHIP8_RAM_LIMIT];


  //array of 24 2-byte values used to store return addresses after completing a subroutine. 
  //Thus, there are up to 24 nested subroutines available.
  //The original RCA 1802 microprocesser Chip8 was implemented on states that the stack is 48 bytes long
  uint16_t stack[CHIP8_STACK_LIMIT]; 





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



  // Misc

  // A timer used to track how many milliseconds have passed since the timer had last elapsed.
  // This is a 60Hz timer, meaning that there are 60 cycles per second.
  // After every cycle, this timer should update the delay_timer and sound_timer if necessary.
  uint64_t millis_timer60hz;

};


static inline void chip8_set_key(struct chip8 *vm, enum chip8_key key) {
  vm->keyboard_inputs |= key;
}

static inline void chip8_remove_key(struct chip8 *vm, enum chip8_key key) {
  vm->keyboard_inputs &= ~key;
}


int chip8_init(struct chip8 *vm, FILE *file);

int chip8_load_rom(struct chip8 *vm, FILE *file);
void chip8_update_timer(struct chip8 *vm, uint64_t delta_time_millis);


#endif //CHIP8_H

