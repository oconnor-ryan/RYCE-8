#include "xo_chip.h"

void xochip_init(struct xochip *vm) {
  //set pointers to allocated memory
  vm->core->ram = vm->alloc_ram;
  vm->core->stack = (uint16_t*) (vm->alloc_ram + CHIP8_STACK_START);
  vm->core->fb = (uint64_t *) vm->fb;

  //initialize sizes
  vm->core->ram_size = sizeof(vm->alloc_ram);
  vm->core->stack_size = 16;
  vm->core->fb_size = sizeof(vm->fb);

  //start at lores by default
  vm->res = SCHIP_DISPLAY_LORES;

  memset(vm->rpl_flags, 0, sizeof(vm->rpl_flags));

  vm->core->quirks = CHIP8_QUIRK_BXNN 
  | CHIP8_QUIRK_CLR_SCN_ON_LORES
  | CHIP8_QUIRK_INCREMENT_I
  //| CHIP8_QUIRK_HALF_PIXEL_SCROLL_LOW_RES
  ;
}

int xochip_reset(struct xochip *vm, FILE *file) {
  memset(&vm->fb, 0, sizeof(vm->fb));
  vm->res = SCHIP_DISPLAY_LORES;
  memcpy(&vm->core->ram[SCHIP_LARGE_FONT_LOC], SCHIP_HEX_FONT, sizeof(SCHIP_HEX_FONT));

  return chip8_reset(vm->core, file);
}


int xochip_ins_00(struct xochip *vm, uint8_t low) {
  if((low >> 4) != 0xD) {
    return 0;
  }

  //00DN - Scroll up by N pixels

  uint8_t n = low & 0x0F;

  //if the "half scroll" quirk for lores scrolling is NOT enabled
  //and the emulator is in lores mode,
  //we need to scroll down 2 pixels instead of 1.
  if(!(vm->core->quirks & CHIP8_QUIRK_HALF_PIXEL_SCROLL_LOW_RES) 
  && vm->res == SCHIP_DISPLAY_LORES) {
    n *= 2;
  }
  

  //start at top set of rows and shift them all upward.
  for(uint8_t num_rows_from_top = n; num_rows_from_top < SCHIP8_HEIGHT; num_rows_from_top++) {
    uint8_t move_from = num_rows_from_top;
    uint8_t move_to = move_from - n;
    vm->fb[move_to] = vm->fb[move_from];
    //make sure to clear row being moved from
    memset(vm->fb[move_from], 0, sizeof(vm->fb[move_from]));
  }

  return 1;

}


//SE (3xkk) - Skip next instruction if Vx == kk
//ALSO skip over double-wide 0xF000 NNNN instruction
int xochip_ins_3xnn(struct xochip *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  if(vm->core->V[x] == low) {
    uint16_t ins_being_skipped_over;
    memcpy(&ins_being_skipped_over, &vm->core->ram[vm->core->pc], 2);

    //skip over double-wide 0xF000 instruction
    if(ins_being_skipped_over == 0xF000) {
      vm->core->pc += 2;
    }
    vm->core->pc += 2;

  }

  return 1;
}

//SNE (4xkk) - Skip next instruction if Vx != kk
//ALSO skip over double-wide 0xF000 NNNN instruction
static inline int xochip_ins_4(struct xochip *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  if(vm->core->V[x] != low) {
    uint16_t ins_being_skipped_over;
    memcpy(&ins_being_skipped_over, &vm->core->ram[vm->core->pc], 2);

    //skip over double-wide 0xF000 instruction
    if(ins_being_skipped_over == 0xF000) {
      vm->core->pc += 2;
    }
    vm->core->pc += 2;
  }
  return 1;
}

int xochip_ins_5xy(struct xochip *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  uint8_t y = low >> 4;

  switch(low & 0x0F) {
    //SE (5xy0) - Skip next instruction if Vx == Vy
    //ALSO skip over double-wide 0xF000 instruction
    case 0: {
      
      if(vm->core->V[x] == vm->core->V[y]) {
        uint16_t ins_being_skipped_over;
        memcpy(&ins_being_skipped_over, &vm->core->ram[vm->core->pc], 2);

        //skip over double-wide 0xF000 instruction
        if(ins_being_skipped_over == 0xF000) {
          vm->core->pc += 2;
        }
        vm->core->pc += 2;
      }

      break;
    }
    //save vx - vy (0x5XY2) save an inclusive range of registers to memory starting at i
    case 2: {
      for(uint8_t i = 0; x + i <= y; i++) {
        vm->core->ram[vm->core->I + i] = vm->core->V[x+i];
      }
      break;
    }

    //load vx - vy (0x5XY3) load an inclusive range of registers from memory starting at i.
    case 3: {
      for(uint8_t i = 0; x + i <= y; i++) {
        vm->core->V[x+i] = vm->core->ram[vm->core->I + i];
      }
      break;
    }

    default: return 0;
  }

  return 1;
  
}

static inline int xochip_ins_9(struct xochip *vm, uint8_t high, uint8_t low) {
  if((low & 0x0F) != 0) {
    return 0;
  }

  uint8_t x = high & 0x0F;
  uint8_t y = low >> 4;

  //SNE (9xy0) - Skip next instruction if Vx != Vy
  if(vm->core->V[x] != vm->core->V[y]) {
    uint16_t ins_being_skipped_over;
    memcpy(&ins_being_skipped_over, &vm->core->ram[vm->core->pc], 2);

    //skip over double-wide 0xF000 instruction
    if(ins_being_skipped_over == 0xF000) {
      vm->core->pc += 2;
    }
    vm->core->pc += 2;
  }

  return 1;
}

int xochip_ins_E(struct xochip *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;

  switch(low) {
    //SKP (Ex9E) - Skip next instruction if key with value of Vx is pressed
    // ALSO skip the double wide 0xF000 NNNN instruction if its there
    case 0x9E: {
      if(vm->core->keyboard_inputs & (1 << vm->core->V[x])) {
        uint16_t ins_being_skipped_over;
        memcpy(&ins_being_skipped_over, &vm->core->ram[vm->core->pc], 2);

        //skip over double-wide 0xF000 instruction
        if(ins_being_skipped_over == 0xF000) {
          vm->core->pc += 2;
        }

        vm->core->pc += 2;
      }
      break;
    }

    //SKNP (ExA1) - Skip next instruction if key with value of Vx is NOT pressed
    // ALSO skip the double wide 0xF000 NNNN instruction if its there
    case 0xA1: {
      if((vm->core->keyboard_inputs & (1 << vm->core->V[x])) == 0) {
        uint16_t ins_being_skipped_over;
        memcpy(&ins_being_skipped_over, &vm->core->ram[vm->core->pc], 2);

        //skip over double-wide 0xF000 instruction
        if(ins_being_skipped_over == 0xF000) {
          vm->core->pc += 2;
        }
        vm->core->pc += 2;
      }
      break;
    }
    default: return 0;
  }
  return 1;
}

int xochip_ins_F(struct xochip *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;

  switch(low) {
    //saveflags vx (0xFN75) save v0-vn to flag registers. (generalizing SCHIP).
    case 0x75: {
      for(uint8_t i = 0; i <= x; i++) {
        vm->rpl_flags[i] = vm->core->V[i];
      }
      break;
    }
    //loadflags vx (0xFN85) restore v0-vn from flag registers. (generalizing SCHIP).
    case 0x85: {
      for(uint8_t i = 0; i <= x; i++) {
        vm->core->V[i] = vm->rpl_flags[i];
      }
      break;
    }

    //plane n (0xFN01) select zero or more drawing planes by bitmask (0 <= n <= 3).
    case 0x01: {

      break;
    }

    //pitch := vx (0xFX3A) set the audio pattern playback rate to 4000*2^((vx-64)/48)Hz.
    case 0x3A: {

      break;
    }

    default: {

      //i := long NNNN (0xF000, 0xNNNN) load i with a 16-bit address.
      if(high == 0xF0 && low == 0x00) {
        memcpy(&vm->core->I, &vm->core->ram[vm->core->pc], sizeof(uint16_t));
        vm->core->pc += 2;
        break;
      }

      //audio (0xF002) store 16 bytes starting at i in the audio pattern buffer.
      if(high == 0xF0 && low == 0x02) {

      }
    }



  }

  return 1;

}

int xochip_new_instructions(struct xochip *vm) {
  uint8_t high = vm->core->ram[vm->core->pc];
  uint8_t low = vm->core->ram[vm->core->pc+1];

  //INCREMENT PC!!!
  // TODO: You should probably put this in the Chip8 wrapper to avoid 
  //having to repeat this code for every Chip8 variant.
  uint16_t old_pc = vm->core->pc;
  vm->core->pc += 2;


  uint8_t result = 0;

  switch(high >> 4) {
    case 3: result = xochip_ins_3xnn(vm, high, low); break;
    case 4: result = xochip_ins_4(vm, high, low); break;
    case 5: result = xochip_ins_5xy(vm, high, low); break;
    case 9: result = xochip_ins_9(vm, high, low); break;
    case 0xE: result = xochip_ins_E(vm, high, low); break;
    case 0xF: result = xochip_ins_F(vm, high, low); break;
    default: break;

  }



  if(!result) {
    //jump back
    vm->core->pc = old_pc;
    return 0;
  }
}

int xochip_process_instruction(struct xochip *vm) {
  if(!xochip_new_instructions(vm) && !schip8_process_instruction_general(vm->fb, vm->core, vm->rpl_flags, vm->res, vm->will_exit)) {
    return 0;
  }

  return 1;
}
