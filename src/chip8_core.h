#ifndef CHIP8_CORE_H
#define CHIP8_CORE_H

#include <stdint.h>
#include <stdio.h>


//the original display resolution of the CHIP8 VM
#define CHIP8_WIDTH 64
#define CHIP8_HEIGHT 32

//all Chip8 programs are loaded at address 512 in RAM
#define CHIP8_PROG_START 512

#define CHIP8_HEX_FONT_START 0
#define CHIP8_HEX_FONT_SIZE 5
#define CHIP8_STACK_START 480 //enough to store max of 16 16-bit addresses, which is the max stack size of XO-CHIP and SCHIP8.


//This lists all of the CHIP-8 quirks between the following CHIP-8 specifications:
// - Original COSMAC VIP CHIP-8 interpreter
// - SUPER-CHIP 1.1 (or SCHIP-MODERN)
// - XO-CHIP
// A quirk is defined as a behavior where 2 CHIP-8 specifications share the same 
// instruction and have different behaviors for that instruction. 
// Note that this only contains quirks that I want to support, so the list of quirks
// may not be exhausive.
enum chip8_quirk {
  CHIP8_QUIRK_SHIFT_VY                   = 1 << 0, // if true, perform Vx = Vy << 1, otherwise Vx == Vx << 1
  CHIP8_QUIRK_INCREMENT_I                = 1 << 1, // if true, Fx55 and Fx65 will increment I by x + 1, otherwise it is left unchanged
  CHIP8_QUIRK_RESET_VF                   = 1 << 2, // if true, reset VF in bitwise AND, OR, and XOR operations
  CHIP8_QUIRK_CLR_SCN_ON_LORES           = 1 << 3, // if true, clear the screen upon switching to lores on SCHIP, otherwise leave artifacts behind.
  CHIP8_QUIRK_WRAP_SPRITE                = 1 << 4, // if true, entire sprite wraps around instead of clipping at the right and bottom borders
  CHIP8_QUIRK_BXNN                       = 1 << 5, // if true, BxNN - Jump to xNN + Vx, otherwise BNNN - Jump to V0 + NNN
  CHIP8_QUIRK_HALF_PIXEL_SCROLL_LOW_RES  = 1 << 6, // if true, each pixel in lores is a 2x2 physical pixel, and when scrolling down, it will scroll 1 phyiscal pixel instead of the currently selected pixel size.
};



//key interrupt flags
enum chip8_key_int_flag {
  CHIP8_KEY_INT_FLAG_WAITING =  1 << 0,
  CHIP8_KEY_INT_FLAG_RELEASED = 1 << 1
};


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


// Stores the font data to display characters 0-9 and A-F
extern const uint8_t FONT_DATA_HEX[5 * 16];


//these are properties that ALL SUPPORTED CHIP-8 variants have.

struct chip8_core {

  //our stack can be stored within the 512 bytes of RAM.
  uint16_t *stack;
  uint8_t stack_size;


  //note that memory is fully allocated here since XO-CHIP has 64K while VIP CHIP8 and SCHIP8
  //have 4K
  uint8_t *ram;
  uint16_t ram_size; //we will store how many bytes of RAM we use here.

  /* Registers */

  //general purpose registers
  uint8_t V[16];

  uint16_t I; //generally used to store memory addresses and to access/modify memory

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
  uint64_t *fb;
  uint16_t fb_size;

  // Misc


  //used to track when a key is released for the Fx0A instruction.
  // if (key_interrupt_flags & CHIP8_KEY_INT_FLAG_WAITING) is 1, we are
  // at the Fx0A instruction waiting for a key press.
  //
  // if (key_interrupt_flags & CHIP8_KEY_INT_FLAG_RELEASED) is 1, a key
  // was released, and we can now set the WAITING and RELEASED flag to 0.
  uint8_t key_interrupt_flags;
  enum chip8_key last_released_key;

  // A timer used to track how many milliseconds have passed since the timer had last elapsed.
  // This is a 60Hz timer, meaning that there are 60 cycles per second.
  // After every cycle, this timer should update the delay_timer and sound_timer if necessary.
  uint64_t millis_timer60hz;


  uint16_t quirks; 

  

};


static inline void chip8_set_key(struct chip8_core *vm, enum chip8_key key) {
  vm->keyboard_inputs |= key;
}

static inline void chip8_remove_key(struct chip8_core *vm, enum chip8_key key) {
  vm->keyboard_inputs &= ~key;

  //store the last key release for the Fx0A instruction
  vm->key_interrupt_flags |= CHIP8_KEY_INT_FLAG_RELEASED; //set bit
  vm->last_released_key = key;
}

void chip8_draw_64x32(uint64_t *fb, uint8_t *V, uint16_t I, uint8_t *ram, uint8_t high, uint8_t low);

int chip8_process_instruction(struct chip8_core *core);
void chip8_update_timer(struct chip8_core *vm, uint64_t delta_time_millis);
int chip8_reset(struct chip8_core *vm, FILE *file);



#endif //CHIP8_CORE_H

