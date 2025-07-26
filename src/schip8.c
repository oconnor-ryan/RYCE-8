#include "schip8.h"
#include <string.h>

void schip8_init(struct schip8 *vm) {
  //set pointers to allocated memory
  vm->core->ram = vm->alloc_ram;
  vm->core->stack = (uint16_t*) (vm->alloc_ram + CHIP8_STACK_START);
  vm->core->fb = vm->fb.fb_64x32;

  //initialize sizes
  vm->core->ram_size = sizeof(vm->alloc_ram);
  vm->core->stack_size = 16;
  vm->core->fb_width = 64;
  vm->core->fb_height = 32;
  vm->core->fb_size = sizeof(vm->fb.fb_64x32);

  //start at lores by default
  vm->res = SCHIP_DISPLAY_LORES;

  memset(vm->rpl_flags, 0, sizeof(vm->rpl_flags));

  vm->core->quirks = CHIP8_QUIRK_BXNN | CHIP8_QUIRK_CLR_SCN_ON_LORES;

}

int schip8_reset(struct schip8 *vm, FILE *file) {
  memset(&vm->fb, 0, sizeof(vm->fb));
  vm->res = SCHIP_DISPLAY_LORES;
  return chip8_reset(vm->core, file);

}

int schip8_process_instruction(struct schip8 *vm) {

  return 0;
}

int schip8_update(struct schip8 *vm, uint64_t delta_millis) {
  
  //we first look for the new instructions, then the old instructions.
  //if current instruction belongs to neither of these sets, throw error
  if(!schip8_process_instruction(vm) && !chip8_process_instruction(vm->core)) {
    return 0;
  }

  chip8_update_timer(vm->core, delta_millis);
  return 1;
}

