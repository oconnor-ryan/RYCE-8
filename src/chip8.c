#include "chip8.h"
#include <limits.h>

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


int chip8_init(struct chip8 *vm, FILE *file) {
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