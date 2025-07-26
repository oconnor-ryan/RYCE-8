#include "vip_chip8.h"


void vip_chip8_init(struct vip_chip8 *vm) {
  //pass our COSMAC VIP CHIP8 specifications to Chip8 Core.
  vm->core->ram = vm->alloc_ram;
  vm->core->fb = vm->alloc_fb;
  vm->core->stack = (uint16_t*) (vm->alloc_ram + CHIP8_STACK_START);

  vm->core->fb_height = 32;
  vm->core->fb_width = 64;
  vm->core->fb_size = sizeof(vm->alloc_fb);  
  vm->core->ram_size = sizeof(vm->alloc_ram);
  vm->core->stack_size = 12;

  //define our quirks
  vm->core->quirks = CHIP8_QUIRK_INCREMENT_I 
  | CHIP8_QUIRK_RESET_VF 
  | CHIP8_QUIRK_SHIFT_VY;

}

int vip8_chip8_update(struct vip_chip8 *vm, uint64_t delta_millis) {
  if(!chip8_process_instruction(vm->core)) {
    return 0;
  }
  chip8_update_timer(vm->core, delta_millis);
  return 1;
}

int vip8_chip8_reset(struct vip_chip8 *vm, FILE *file) {
  return chip8_reset(vm->core, file);
}