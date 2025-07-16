#include "chip8.h"
#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

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


//code from https://blog.regehr.org/archives/1063
//perform bit rotation so that bits on left wrap around to the right
uint64_t rotate_left_64(uint64_t input, uint8_t shift) {
  return (input << shift) | (input >> (-shift & 63));
}

//code from https://blog.regehr.org/archives/1063
//perform bit rotation so that bits on right wrap around to the left
uint64_t rotate_right_64(uint64_t input, uint8_t shift) {
  return (input >> shift) | (input << (-shift & 63));
}

int chip8_init(struct chip8 *vm, FILE *file) {

  //set seed for randomness
  srand(time(NULL));

  //light up all pixels in framebuffer
  for(uint8_t i = 0; i < 32; i++) {
    vm->fb[i] = ULLONG_MAX; 
  }

  vm->delay_timer = 0;
  vm->sound_timer = 0;
  vm->millis_timer60hz = 0;
  vm->keyboard_inputs = 0;
  vm->pc = CHIP8_PROG_START; //index within RAM
  vm->sp = 0;  // index within the stack.
  

  //optional, zero out registers
  vm->I = 0;
  for(uint8_t i = 0; i < 16; i++) {
    vm->V[i] = 0;
  }

  uint8_t has_no_error = 1;


  // load font
  memcpy(&vm->ram[CHIP8_HEX_FONT_START], FONT_DATA_HEX, sizeof(FONT_DATA_HEX));

  if(file != NULL) {
    has_no_error = chip8_load_rom(vm, file);
  }

  return has_no_error;
}



void chip8_update_timer(struct chip8 *vm, uint64_t delta_time_millis) {
  vm->millis_timer60hz += delta_time_millis;

  if(vm->millis_timer60hz > 16) {
    vm->millis_timer60hz -= 16;
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

int chip8_load_rom(struct chip8 *vm, FILE *file) {

  //only read the number of bytes that the Chip8 can hold into RAM. The rest of the 
  //file is ignored. If the ROM is smaller than the max number of bytes, the ROM
  //will still load successfully, but the uninitialized half of RAM will hold an undefined
  //set of bytes.

  const size_t max_bytes = CHIP8_RAM_LIMIT - CHIP8_PROG_START;
  size_t num_bytes_read = fread(vm->ram + CHIP8_PROG_START, max_bytes, 1, file);

  if(ferror(file)) {
    return 0;
  }
  return 1;
}


// Process the instruction. If the instruction is unrecognized, the PC is
// rolled back and this function returns 0.
// Returns 1 on success.
int chip8_process_instruction(struct chip8 *vm) {
  uint8_t high = vm->ram[vm->pc];  //MSB
  uint8_t low = vm->ram[vm->pc+1]; //LSB

  //uint16_t ins = high << 8;
  //ins |= low;



  uint16_t pc = vm->pc + 2;

  //TODO: Make loading bytes to RAM  endian-independent.
  // For now, assume bytes are stored in big-endian order.

  
  // check highest 4 bits
  switch(high >> 4) {
    case 0: {
      //CLS (0x00E0) - clear screen
      if(high == 0x00 & low == 0xE0) {
        for(uint8_t i = 0; i < CHIP8_HEIGHT; i++) {
          vm->fb[i] = 0;
        }
      } 
      //RET (0x00EE) - return from subroutine by popping address off the stack and setting the PC to that address.
      else if(high == 0x00 & low == 0xEE) {
        vm->sp--;
        pc = vm->stack[vm->sp];
      }
      //SYS (0nnn) - Jump to machine code routine at nnn. Was only needed on old computers
      //and is ignored by modern interpreters
     
      break;
    }
    //JP (1nnn) - Jump to location nnn
    case 1: {
      uint16_t loc = (((uint16_t)high & 0x0F) << 8) | low;
      pc = loc;
      break;
    }
    //CALL (2nnn) - Increment SP, put current PC on stack, and set PC to nnn
    case 2: {
      uint16_t loc = (((uint16_t)high & 0x0F) << 8) | low;

      //SP is zero indexed, so we insert, then increment
      vm->stack[vm->sp] = pc;
      vm->sp++;
      pc = loc;
      break;
    }
    //SE (3xkk) - Skip next instruction if Vx == kk
    case 3: {
      uint8_t x = high & 0x0F;
      if(vm->V[x] == low) {
        pc += 2;
      }
      break;
    }
    //SNE (4xkk) - Skip next instruction if Vx != kk
    case 4: {
      uint8_t x = high & 0x0F;
      if(vm->V[x] != low) {
        pc += 2;
      }
      break;
    }
    //SE (5xy0) - Skip next instruction if Vx == Vy
    case 5: {
      uint8_t x = high & 0x0F;
      uint8_t y = low >> 4;
      if(vm->V[x] == vm->V[y]) {
        pc += 2;
      }
      break;
    }

    //LD (6xkk) - Set Vx = kk
    case 6: {
      uint8_t x = high & 0x0F;
      vm->V[x] = low;
      break;
    }

    //ADD (7xkk) - Set Vx = Vx + kk
    case 7: {
      uint8_t x = high & 0x0F;
      vm->V[x] += low;
      break;
    }

    case 8: {
      uint8_t x = high & 0x0F;
      uint8_t y = low >> 4;

      switch(low & 0x0F) {
        //LD (8xy0) - Set Vx = Vy
        case 0: {
          vm->V[x] = vm->V[y];
          break;
        }

        //OR (8xy1) - Set Vx = Vx | Vy
        case 1: {
          vm->V[x] = vm->V[x] | vm->V[y];
          break;
        }
        //AND (8xy2) - Set Vx = Vx & Vy
        case 2: {
          vm->V[x] = vm->V[x] & vm->V[y];
          break;
        }
        //XOR (8xy3) - Set Vx = Vx ^ Vy
        case 3: {
          vm->V[x] = vm->V[x] ^ vm->V[y];
          break;
        }
        //ADD (8xy4) - Set Vx = Vx + Vy, set VF = carry
        case 4: {
          uint8_t old_x = vm->V[x];
          vm->V[x] = vm->V[x] + vm->V[y];

          //if result overflows, set carry register.
          vm->V[15] = old_x > vm->V[x];
          break;
        } 
        //SUB (8xy5) - Set Vx = Vx - Vy, set VF = NOT borrow
        case 5: {
          //VF = 1 if no undeflow, VF = 0 if underflow occurs
          vm->V[15] = vm->V[x] > vm->V[y];
          vm->V[x] = vm->V[x] - vm->V[y];
          break;
        }
        //SHR (8xy6) - Set Vx = Vx >> 1  (note that value of y does not matter and is unused).
        case 6: {
          vm->V[x] >>= 1;
          break;
        }

        //SUBN (8xy7) - Set Vx = Vy - Vx, set VF = NOT borrow.
        case 7: {
          //VF = 1 if no undeflow, VF = 0 if underflow occurs
          vm->V[15] = vm->V[y] > vm->V[x];
          vm->V[x] = vm->V[y] - vm->V[x];
          break;
        }

        //SHL (8xyE) - Set Vx = Vx << 1, ignore y.
        case 0xE: {
          vm->V[x] <<= 1;
          break;
        }

        default: goto unknown_ins;
      }

      break;
    }

    case 9: {
      if((low & 0x0F) != 0) {
        goto unknown_ins;
      }

      uint8_t x = high & 0x0F;
      uint8_t y = low >> 4;

      //SNE (9xy0) - Skip next instruction if Vx != Vy
      if(vm->V[x] != vm->V[y]) {
        pc += 2;
      }
      
      break;
    }

    //LD (Annn) - Set I = nnn
    case 0xA: {
      uint16_t loc = (((uint16_t)high & 0x0F) << 8) | low;
      vm->I = loc;
      break;
    }

    //LD (Bnnn) - Jump to location at nnn + V0
    case 0xB: {
      uint16_t loc = (((uint16_t)high & 0x0F) << 8) | low;
      pc = loc + vm->V[0];
      break;
    }

    //RND (Cxkk) - Set Vx = RANDOM_BYTE & kk
    case 0xC: {
      uint8_t x = high & 0x0F;
      uint8_t random = rand();
      vm->V[x] = random & low;
      break;
    }

    //DRW (Dxyn) - Draw n-byte sprite starting at memory location I at (Vx, Vy), 
    //set VF = 1 if collision with another 
    case 0xD: {
      uint8_t x = high & 0x0F;
      uint8_t y = low >> 4;
      uint8_t n = low & 0x0F;


      uint8_t collision = 0;

      //for each row to draw to
      for(uint8_t i = 0; i < n; i++) {

        //grab copy of row
        uint64_t old_row = vm->fb[vm->V[y] + i];

        //create mask for bits we are going to draw to.
        uint64_t mask = rotate_left_64(0xFF,  vm->V[x]);

        //zero out the bits we dont draw to in old_row.
        old_row &= mask;

        //create a empty 64-bit row, get an 8-bit row from our sprite, and
        //shift our sprite's row into the empty row, wrapping around as needed.
        uint64_t sprite_row = rotate_left_64(vm->ram[vm->I], vm->V[x]);


        // How do we know if collision occured?
        // 1. If a previously lit bit is drawn over by a lit bit from the sprite being drawn.

        //  00111011 (old)
        //^ 00010100 (sprite)
        //  ========
        //  00100100 (new)
        //     ^
        //  collided

        // To check if a collision occurred, simply AND the (old) and (sprite)

        //check if a collision would occur (aka: if previously lit bit gets overwritten by a lit
        //bit inside the sprite)
        //use OR so that if it was already set to 1, it stays at 1.
        collision |= old_row & (uint64_t) sprite_row;

        //draw row to framebuffer
        vm->fb[vm->V[y] + i] ^= sprite_row;
      }

      vm->V[15] = collision;
      break;
    }

    case 0xE: {
      uint8_t x = high & 0x0F;

      switch(low) {
        //SKP (Ex9E) - Skip next instruction if key with value of Vx is pressed
        case 0x9E: {
          if(vm->keyboard_inputs & (1 << vm->V[x])) {
            pc += 2;
          }
          break;
        }

        //SKNP (ExA1) - Skip next instruction if key with value of Vx is NOT pressed
        case 0xA1: {
          if((vm->keyboard_inputs & (1 << vm->V[x])) == 0) {
            pc += 2;
          }
          break;
        }

        default: goto unknown_ins;
      }

      break;
    }

    case 0xF: {
      uint8_t x = high & 0x0F;

      switch(low) {
        //LD (Fx07) - Set Vx = delay timer value
        case 0x07: {
          vm->V[x] = vm->delay_timer;
          break;
        }

        //LD (Fx0A) - Wait for key press, store what key was pressed in V[x]
        case 0x0A: {

          //if no keys have been pressed yet, immediately exit the function
          //to avoid incrementing PC. This halts the execution of the VM
          //while allowing the host process to not be suspended.
          if(vm->keyboard_inputs == 0) {
            return 1;
          }

          //if a key was pressed, find out what key it was and return its numberic value
          // (0-15) for keys (0-9 and A-F)


          //TODO: There is ambiguity on whether or not the VM will halt if it needs
          //to wait for a key press when 1 or more keys are already being pressed down.
          //Does it wait for the next keyboard press or does it just read one of they
          //keys that are already being pressed down?

          

          //FIXME. Because we are polling for keyboard inputs rather than using an interrupt,
          //we may run into a rare case where 2 or more keys are set at once.
          //For now, we will prioritize keys that are closer to 0.
          //In future, consider adding a boolean to halt the VM until a 'keyboard interrupt'
          //is sent. We can use SDL's AppEvent() callback to similate this interrupt.

          //find out which key was pressed.
          for(uint8_t i = 0; i < 16; i++) {
            if(vm->keyboard_inputs & (1 << i)) {
              vm->V[x] = i;
              break;
            }
          }

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
        //   If Vx = 9, it will set I = location of the '9' sprite
        //   If Vx = 10, it will set I = location of the 'A' sprite
        //   If Vx = 11, it will set I = location of the 'B' sprite
        //   If Vx = 15, it will set I = location of the 'F' sprite
        case 0x29: {
          vm->I = vm->ram[CHIP8_HEX_FONT_START + (CHIP8_HEX_FONT_SIZE * vm->V[x])];
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
          uint8_t i = 2;
          while(vx != 0) {
            vm->ram[vm->I+i] = vx % 10;
            vx /= 10;
            i--;
          }

          // if we have not set all 3 digits, set the rest to 0.
          while(1) {
            vm->ram[vm->I+i] = 0;
            if(i == 0) break;

            i--;
          }

          break;
        }


        //LD (Fx55) - Store registers V0 through Vx in memory starting at I.
        case 0x55: {
          for(uint8_t i = 0; i <= x; i++) {
            vm->ram[vm->I + i] = vm->V[i];
          }
          break;
        }

        //LD (Fx65) - Read registers V0 through Vx in memory starting at I
        case 0x65: {
          for(uint8_t i = 0; i <= x; i++) {
            vm->V[i] = vm->ram[vm->I + i];
          }
          break;
        }

        default: goto unknown_ins;
      }

      break;
    }

    default: goto unknown_ins;
  }


  //set vm's PC to the new value of PC.
  vm->pc = pc;
  return 1;

  unknown_ins: 
  return 0;

}