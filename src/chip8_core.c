#include "chip8_core.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

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

const uint8_t FONT_DATA_HEX[5 * 16] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0,  // "0"
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


//code from https://blog.regehr.org/archives/1063
//perform bit rotation so that bits on left wrap around to the right
static inline uint64_t rotate_left_64(uint64_t input, uint8_t shift) {
 // return (input << shift) | (input >> (-shift & 63));
  return (input << shift) | (input >> (64 - shift));
}

//code from https://blog.regehr.org/archives/1063
//perform bit rotation so that bits on right wrap around to the left
static inline uint64_t rotate_right_64(uint64_t input, uint8_t shift) {
  //return (input >> shift) | (input << (-shift & 63));
  return (input >> shift) | (input << (64 - shift));

}

static inline uint8_t reverse_bit_order(uint8_t n) {
  uint8_t r = 0;
  for(uint8_t i = 0; i < 8; i++) {
    if(n & (1 << i)) {
      r |= 1 << (8 - 1 - i);
    }
  }
  return r;
}

uint8_t chip8_key_to_num(enum chip8_key key) {
  switch(key) {
    case CHIP8_KEY_0: return 0;
    case CHIP8_KEY_1: return 1;
    case CHIP8_KEY_2: return 2;
    case CHIP8_KEY_3: return 3;
    case CHIP8_KEY_4: return 4;
    case CHIP8_KEY_5: return 5;
    case CHIP8_KEY_6: return 6;
    case CHIP8_KEY_7: return 7;
    case CHIP8_KEY_8: return 8;
    case CHIP8_KEY_9: return 9;
    case CHIP8_KEY_A: return 10;
    case CHIP8_KEY_B: return 11;
    case CHIP8_KEY_C: return 12;
    case CHIP8_KEY_D: return 13;
    case CHIP8_KEY_E: return 14;
    case CHIP8_KEY_F: return 15;

    //should never occur, use dummy value
    default: assert(0); return 255;
  }
}

int chip8_load_rom(struct chip8_core *vm, FILE *file) {

  //only read the number of bytes that the Chip8 can hold into RAM. The rest of the 
  //file is ignored. If the ROM is smaller than the max number of bytes, the ROM
  //will still load successfully, but the uninitialized half of RAM will hold an undefined
  //set of bytes.

  const size_t max_bytes = vm->ram_size - CHIP8_PROG_START;
  size_t num_bytes_read = fread(vm->ram + CHIP8_PROG_START, max_bytes, 1, file);

  if(ferror(file)) {
    return 0;
  }
  return 1;
}


int chip8_reset(struct chip8_core *vm, FILE *file) {
  //set seed for randomness
  srand(time(NULL));

  //turn off all pixels in framebuffer
  memset(vm->fb, 0, vm->fb_size);

  vm->delay_timer = 0;
  vm->sound_timer = 0;
  vm->millis_timer60hz = 0;
  vm->keyboard_inputs = 0;
  vm->pc = CHIP8_PROG_START; //index within RAM
  vm->sp = 0;  // index within the stack.

  vm->key_interrupt_flags = 0; //set all key interrupt flags to 0

  //optional, zero out registers
  vm->I = 0;
  memset(vm->V, 0, sizeof(vm->V));

  uint8_t has_no_error = 1;


  // load font
  memcpy(&vm->ram[CHIP8_HEX_FONT_START], FONT_DATA_HEX, sizeof(FONT_DATA_HEX));

  if(file != NULL) {
    has_no_error = chip8_load_rom(vm, file);
  }

  return has_no_error;
}



void chip8_update_timer(struct chip8_core *vm, uint64_t delta_time_millis) {
  vm->millis_timer60hz += delta_time_millis;

  // note that 60Hz = 60 cycles per second, or 16.666 repeating.
  // since the millis are integers, it's more accurate to use 17.

  if(vm->millis_timer60hz > 17) {
    vm->millis_timer60hz -= 17;
    //vm->millis_timer60hz = 0;
    
    //every 16 milliseconds, if either timer is non-zero, decrement by 1
    if(vm->delay_timer != 0) {
      vm->delay_timer--;
    }
    if(vm->sound_timer != 0) {
      vm->sound_timer--;
    } 
  }
}

static inline int chip8_ins_0(struct chip8_core *vm, uint8_t high, uint8_t low) {
  //CLS (0x00E0) - clear screen
  if(high == 0x00 & low == 0xE0) {
    memset(vm->fb, 0, vm->fb_size);
  } 
  //RET (0x00EE) - return from subroutine by popping address off the stack and setting the PC to that address.
  else if(high == 0x00 & low == 0xEE) {
    vm->sp--;
    vm->pc = vm->stack[vm->sp];
  }
  //SYS (0nnn) - Jump to machine code routine at nnn. Was only needed on old computers
  //and is ignored by modern interpreters

  return 1;
}

//JP (1nnn) - Jump to location nnn
static inline int chip8_ins_1(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint16_t loc = (((uint16_t)high & 0x0F) << 8) | low;
  vm->pc = loc;
  return 1;
}

//CALL (2nnn) - Increment SP, put current PC on stack, and set PC to nnn
static inline int chip8_ins_2(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint16_t loc = (((uint16_t)high & 0x0F) << 8) | low;

  //SP is zero indexed, so we insert, then increment
  vm->stack[vm->sp] = vm->pc;
  vm->sp++;
  vm->pc = loc;

  return 1;
}

//SE (3xkk) - Skip next instruction if Vx == kk
static inline int chip8_ins_3(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  if(vm->V[x] == low) {
    vm->pc += 2;
  }
  return 1;
}

//SNE (4xkk) - Skip next instruction if Vx != kk
static inline int chip8_ins_4(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  if(vm->V[x] != low) {
    vm->pc += 2;
  }
  return 1;
}

//SE (5xy0) - Skip next instruction if Vx == Vy
static inline int chip8_ins_5(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  uint8_t y = low >> 4;
  if(vm->V[x] == vm->V[y]) {
    vm->pc += 2;
  }

  return 1;
}

//LD (6xkk) - Set Vx = kk
static inline int chip8_ins_6(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  vm->V[x] = low;

  return 1;
}

//ADD (7xkk) - Set Vx = Vx + kk
static inline int chip8_ins_7(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  vm->V[x] += low;

  return 1;
}

static inline int chip8_ins_8(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  uint8_t y = low >> 4;

  //Note that instructions 8xy6 and 8xyE used to be undocumented,
  // but the very 1st Chip8 interpreter used 8xy6 like so:
  // VF = LSB of Vx, Vx = Vy >> 1
  // and used 8xyE like so:
  // VF = MSB of Vx, Vx = Vy << 1

  // However, in later versions and forks of Chip8, the Vy ended up being
  // ignored and Vx was used in its place instead.

  switch(low & 0x0F) {
    //LD (8xy0) - Set Vx = Vy
    case 0: {
      vm->V[x] = vm->V[y];
      break;
    }

    //OR (8xy1) - Set Vx = Vx | Vy
    case 1: {
      vm->V[x] = vm->V[x] | vm->V[y];
      if(vm->quirks & CHIP8_QUIRK_RESET_VF) vm->V[15] = 0;

      break;
    }
    //AND (8xy2) - Set Vx = Vx & Vy
    case 2: {
      vm->V[x] = vm->V[x] & vm->V[y];
      if(vm->quirks & CHIP8_QUIRK_RESET_VF) vm->V[15] = 0;

      break;
    }
    //XOR (8xy3) - Set Vx = Vx ^ Vy
    case 3: {
      vm->V[x] = vm->V[x] ^ vm->V[y];
      if(vm->quirks & CHIP8_QUIRK_RESET_VF) vm->V[15] = 0;

      break;
    }
    //ADD (8xy4) - Set Vx = Vx + Vy, set VF = carry
    case 4: {
      uint8_t old_x = vm->V[x];
      vm->V[x] += vm->V[y];

      //if result overflows, set carry register.
      vm->V[15] = old_x > vm->V[x];
      break;
    } 
    //SUB (8xy5) - Set Vx = Vx - Vy, set VF = NOT borrow
    case 5: {
      //VF = 1 if no undeflow, VF = 0 if underflow occurs
      uint8_t oldx = vm->V[x];
      vm->V[x] -= vm->V[y];

      //the flag register must be set AFTER you calculate the SUB. 
      //This avoids an issue where if Vx is VF, then it should be 
      //set to whether the operation overflowed or not.
      vm->V[15] = oldx >= vm->V[y];

      break;
    }
    //SHR (8xy6) - Set Vx = Vx >> 1  (note that value of y does not matter and is unused).
    // Also note that the VF = LSB of Vx before being shifted.
    case 6: {
      uint8_t vf = vm->V[x] & 1;
      if(vm->quirks & CHIP8_QUIRK_SHIFT_VY) vm->V[x] = vm->V[y] >> 1;
      else                                  vm->V[x] >>= 1;

      // make sure VF gets set AFTER the operation so that if Vx = VF, the
      // VF holds its LSB, not the result of the operation.

      vm->V[15] = vf;
      break;
    }

    //SUBN (8xy7) - Set Vx = Vy - Vx, set VF = NOT borrow.
    case 7: {
      //VF = 1 if no undeflow, VF = 0 if underflow occurs
      uint8_t oldx = vm->V[x];
      vm->V[x] = vm->V[y] - vm->V[x];

      //make sure VF flag is set LAST
      vm->V[15] = vm->V[y] >= oldx;

      break;
    }

    //SHL (8xyE) - Set Vx = Vx << 1, ignore y.
    //Also note to set VF = MSB of Vx before it is shifted
    case 0xE: {
      //make sure vf is 0 or 1. The AND operation will put the
      //selected bit at the MSB, so we must convert non-zero values to 1.
      uint8_t vf = (vm->V[x] & (1 << 7)) ? 1 : 0;
      if(vm->quirks & CHIP8_QUIRK_SHIFT_VY) vm->V[x] = vm->V[y] << 1;
      else                                  vm->V[x] <<= 1;

      //only set after operation is complete
      vm->V[15] = vf;

      break;
    }

    default: return 0;
  }

  return 1;
}

static inline int chip8_ins_9(struct chip8_core *vm, uint8_t high, uint8_t low) {
  if((low & 0x0F) != 0) {
    return 0;
  }

  uint8_t x = high & 0x0F;
  uint8_t y = low >> 4;

  //SNE (9xy0) - Skip next instruction if Vx != Vy
  if(vm->V[x] != vm->V[y]) {
    vm->pc += 2;
  }

  return 1;
}

//LD (Annn) - Set I = nnn
static inline int chip8_ins_A(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint16_t loc = (((uint16_t)high & 0x0F) << 8) | low;
  vm->I = loc;

  return 1;
}


//LD (Bnnn) - Jump to location at nnn + V0
//QUIRK - On CHIP-48 and SUPER-CHIP, while the specification states to 
//        jump to NNN + V0, there was an unintended change where BXNN actually 
//        jumps to XNN + Vx. This is not listed in the original SUPER-CHIP 1.1 reference,
//        but this behavior is present in most SUPER-CHIP interpreters.
//      
static inline int chip8_ins_B(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint16_t loc = (((uint16_t)high & 0x0F) << 8) | low;

  if(vm->quirks & CHIP8_QUIRK_BXNN) vm->pc = loc + vm->V[high & 0x0F];
  else                              vm->pc = loc + vm->V[0];

  return 1;
}


//RND (Cxkk) - Set Vx = RANDOM_BYTE & kk
static inline int chip8_ins_C(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  uint8_t random = rand();
  vm->V[x] = random & low;

  return 1;
}

//DRW (Dxyn) - Draw n-byte sprite starting at memory location I at (Vx, Vy), 
//set VF = 1 if collision with another 
static inline int chip8_ins_D(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  uint8_t y = low >> 4;
  uint8_t n = low & 0x0F;

  // Note that when drawing a sprite, if a lit pixel from the sprite draws
  // over a previously lit pixel, that pixel gets TURNED OFF. It does not stay on.

  //make sure collision bool matches the size of the row (64 bits). Otherwise, the result of a comparison
  // will be truncated when using AND
  uint64_t collision = 0;

  // because sprites are formated like so:
  /*
          Bits:
          7 6 5 4 3 2 1 0
  Byte 1: 0 1 1 1 1 1 1 0
  Byte 2: 0 1 0 0 0 0 0 0
  Byte 3: 0 1 1 1 1 1 1 0
  Byte 4: 0 1 0 0 0 0 0 0
  Byte 5: 0 1 1 1 1 1 1 0

  Our framebuffer must also match this format by making the most significant
  bit (63rd) correspond to X = 0 on the screen and the least significant bit (0th
  bit correspond to X = 63 on screen.

  */

  //Note: ONLY Initial X and Y coordinates get WRAPPED AROUND.
  //Make sure to use modulus
  uint8_t fbx = vm->V[x] % vm->fb_width;
  uint8_t fby = vm->V[y] % vm->fb_height;

  //when drawing the sprite, you want to CLIP them if they go off-screen,
  //NOT WRAPAROUND


  //for each row to draw to
  for(uint8_t i = 0; i < n; i++) {

    //Apply vertical wraparound if sprite overflows offscren.
    //uint8_t fby = (vm->V[y] + i) % (CHIP8_HEIGHT);

    //grab copy of row
    uint64_t old_row = vm->fb[fby];

    //create mask for bits we are going to draw to. 
    //Dont use bit rotation since we want to perform X-coord clipping on sprite row.
    uint64_t mask = ((uint64_t)0xFF << 56) >> fbx;

    //zero out the bits we dont draw to in old_row.
    old_row &= mask;

    //create a empty 64-bit row, get an 8-bit row from our sprite, and
    //shift our sprite's row into the empty row
    uint8_t sprite_row_data = vm->ram[vm->I + i];
    
    uint64_t sprite_row = ((uint64_t)sprite_row_data << 56) >> fbx;


    // How do we know if collision occured?
    // 1. If a previously lit bit is drawn over by a lit bit from the sprite being drawn.

    //  00111011 (old)
    //^ 00010100 (sprite)
    //  ========
    //  00101111 (new)
    //     ^
    //  collided. Note that collision can be detected by checking if the bit in both old_row and sprite_row
    //            are 1.

    // To check if a collision occurred, simply AND the (old) and (sprite)

    //use OR so that if it was already set to 1, it stays at 1.
    collision |= old_row & sprite_row;


    //draw row to framebuffer
    vm->fb[fby] ^= sprite_row;


    //implements clipping behavior of sprites for Y where the X and Y inside the DXYN instruction
    //wrap around, but the subsequent rows of the sprite get clipped off if going offscreen
    fby++;
    if(fby >= vm->fb_height) break;

  }

  //remember that V MUST BE 0 or 1, it cannot be any other value
  vm->V[15] = collision ? 1 : 0;

  return 1;
}

static inline int chip8_ins_E(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;

  switch(low) {
    //SKP (Ex9E) - Skip next instruction if key with value of Vx is pressed
    case 0x9E: {
      if(vm->keyboard_inputs & (1 << vm->V[x])) {
        vm->pc += 2;
      }
      break;
    }

    //SKNP (ExA1) - Skip next instruction if key with value of Vx is NOT pressed
    case 0xA1: {
      if((vm->keyboard_inputs & (1 << vm->V[x])) == 0) {
        vm->pc += 2;
      }
      break;
    }

    default: return 0;
  }

  return 1;
}

static inline int chip8_ins_F(struct chip8_core *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;

  switch(low) {
    //LD (Fx07) - Set Vx = delay timer value
    case 0x07: {
      vm->V[x] = vm->delay_timer;
      break;
    }

    //LD (Fx0A) - Wait for key press, store what key was pressed in V[x]
    case 0x0A: {
      uint8_t wait_for_keyboard = vm->key_interrupt_flags & CHIP8_KEY_INT_FLAG_WAITING;

      //if we are not waiting for keyboard yet, we are now.
      if(!wait_for_keyboard) {
        vm->key_interrupt_flags |= CHIP8_KEY_INT_FLAG_WAITING;

        //set released flag to 0 so that we can wait until the next
        //key release
        vm->key_interrupt_flags &= ~CHIP8_KEY_INT_FLAG_RELEASED;
        return 1;
      }

      //if we are waiting for keyboard, and a key was released
      vm->key_interrupt_flags = 0; //we are not waiting anymore for key

      //convert last_released_key enum to number between 0-15.

      vm->V[x] = chip8_key_to_num(vm->last_released_key);

      break;
    }

    //LD (Fx15) - Set delay timer = Vx
    case 0x15: {
      vm->delay_timer = vm->V[x];
      break;
    }

    //LD (Fx18) - Set sound timer = Vx
    case 0x18: {
      vm->sound_timer = vm->V[x];
      break;
    }

    //ADD (Fx1E) - Set I = I + Vx
    case 0x1E: {
      vm->I += vm->V[x];
      break;
    }

    //LD (Fx29) - Set I = location of sprite for digit Vx.
    // This loads a sprite from the hexadecimal font. Note that each sprite from this
    // font is 5 bytes long
    // Example:
    //   If Vx = 0, it will set I = location of the '0' sprite
    //   If Vx = 1, it will set I = location of the '1' sprite
    //   If Vx = 11, it will set I = location of the 'B' sprite
    //   If Vx = 15, it will set I = location of the 'F' sprite
    case 0x29: {
      vm->I = CHIP8_HEX_FONT_START + (CHIP8_HEX_FONT_SIZE * vm->V[x]);
      break;
    }

    //LD (Fx33) - Store BCD representation of Vx in memory locations I, I+1, and I+2
    case 0x33: {

      //Note that BCD is a type of binary encoding where each digit of a integer is stored
      //in its own 4-bit or 8-bit grouping.
      //Example, the number 255 (11111111 in binary) is stored as 0010 0101 0101 (each grouping represents a single digit)

      // In Chip8, each digit is stored in a byte, so 255 would look like this 
      // in Chip8's BCD format: 00000010 00000101 00000101

      uint8_t vx = vm->V[x];

      //starting at rightmost (LS) digit, insert the value of its digit
      //at I+2, then I+1, then I
      uint8_t i = 3;
      while(vx != 0) {
        i--;
        vm->ram[vm->I+i] = vx % 10;
        vx /= 10;
      }

      // if we have not set all 3 digits, set the rest to 0.
      while(i != 0) {
        i--;
        vm->ram[vm->I+i] = 0;
      }

      break;
    }


    //LD (Fx55) - Store registers V0 through Vx in memory starting at I.
    case 0x55: {
      for(uint8_t i = 0; i <= x; i++) {
        vm->ram[vm->I + i] = vm->V[i];
      }

      if(vm->quirks & CHIP8_QUIRK_INCREMENT_I) vm->I += x + 1;
      break;
    }

    //LD (Fx65) - Read registers V0 through Vx in memory starting at I
    case 0x65: {
      for(uint8_t i = 0; i <= x; i++) {
        vm->V[i] = vm->ram[vm->I + i];
      }
      if(vm->quirks & CHIP8_QUIRK_INCREMENT_I) vm->I += x + 1;

      break;
    }

    default: return 0;
  }

  return 1;
}


// Process the instruction. If the instruction is unrecognized, the PC is
// rolled back and this function returns 0.
// Returns 1 on success.
int chip8_process_instruction(struct chip8_core *vm) {

  uint8_t wait_for_keyboard = vm->key_interrupt_flags & CHIP8_KEY_INT_FLAG_WAITING;
  uint8_t was_key_released = vm->key_interrupt_flags & CHIP8_KEY_INT_FLAG_RELEASED;

  if(wait_for_keyboard && !was_key_released) {
    return 1;
  }


  uint8_t high = vm->ram[vm->pc];  //MSB
  uint8_t low = vm->ram[vm->pc+1]; //LSB


  //used to jump back to previous PC if a runtime error occurs (such as a unknown instruction).
  uint16_t old_pc = vm->pc;


  vm->pc += 2;

  //TODO: Make loading bytes to RAM  endian-independent.
  // For now, assume bytes are stored in big-endian order.

  uint8_t res = 0;
  
  // check highest 4 bits
  switch(high >> 4) {
    case 0: res = chip8_ins_0(vm, high, low); break;
    case 1: res = chip8_ins_1(vm, high, low); break;
    case 2: res = chip8_ins_2(vm, high, low); break;
    case 3: res = chip8_ins_3(vm, high, low); break;
    case 4: res = chip8_ins_4(vm, high, low); break;
    case 5: res = chip8_ins_5(vm, high, low); break;
    case 6: res = chip8_ins_6(vm, high, low); break;
    case 7: res = chip8_ins_7(vm, high, low); break;
    case 8: res = chip8_ins_8(vm, high, low); break;
    case 9: res = chip8_ins_9(vm, high, low); break;
    case 0xA: res = chip8_ins_A(vm, high, low); break;
    case 0xB: res = chip8_ins_B(vm, high, low); break;
    case 0xC: res = chip8_ins_C(vm, high, low); break;
    case 0xD: res = chip8_ins_D(vm, high, low); break;
    case 0xE: res = chip8_ins_E(vm, high, low); break;
    case 0xF: res = chip8_ins_F(vm, high, low); break;

    default: res = 0;


  }

  if(!res) {
    vm->pc = old_pc;
    return 0;
  }

  return 1;
}
