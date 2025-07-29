#include "chip8.h"
#include <assert.h>

void chip8_wrapper_init(struct chip8 *vm, enum chip8_emu_type type) {
  vm->emu = type;

  switch(vm->emu) {
    case CHIP8_VARIANT_VIP: {
      vm->vm.vip.core = &vm->core;
      vip_chip8_init(&vm->vm.vip);
      break;
    }
    case CHIP8_VARIANT_SUPER: {
      vm->vm.super.core = &vm->core;
      schip8_init(&vm->vm.super);
      break;
    }
    case CHIP8_VARIANT_XO: {
      assert(0);
    }
  }
}

int chip8_wrapper_process_instruction(struct chip8 *vm) {
  switch(vm->emu) {
    case CHIP8_VARIANT_VIP: return vip_chip8_process_instruction(&vm->vm.vip); break;
    case CHIP8_VARIANT_SUPER: return schip8_process_instruction(&vm->vm.super); break;
    case CHIP8_VARIANT_XO: assert(0); return 0;
  }
}

void chip8_wrapper_update_timer(struct chip8 *vm, uint64_t delta_millis) {
  chip8_update_timer(&vm->core, delta_millis);
}



int chip8_wrapper_reset(struct chip8 *vm, FILE *file) {
  switch(vm->emu) {
    case CHIP8_VARIANT_VIP: return vip_chip8_reset(&vm->vm.vip, file); break;
    case CHIP8_VARIANT_SUPER: return schip8_reset(&vm->vm.super, file); break;
    case CHIP8_VARIANT_XO: assert(0); return 0;
  }
}
