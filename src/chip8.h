#ifndef CHIP8_H
#define CHIP8_H

#include "vip_chip8.h"
#include "schip8.h"


enum chip8_emu_type {
  CHIP8_VARIANT_VIP,
  CHIP8_VARIANT_SUPER,
  CHIP8_VARIANT_XO,
};

struct chip8_init {
  char *rom_file;
  enum chip8_emu_type type;
};

struct chip8 {
  enum chip8_emu_type emu;
  struct chip8_core core;

  union {
    struct vip_chip8 vip;
    struct schip8 super;
    //struct xo_chip8 xo;

  } vm;
};


void chip8_wrapper_init(struct chip8 *vm, enum chip8_emu_type type);
int chip8_wrapper_process_instruction(struct chip8 *vm);
void chip8_wrapper_update_timer(struct chip8 *vm, uint64_t delta_millis);

int chip8_wrapper_reset(struct chip8 *vm, FILE *file);

#endif// CHIP8_H
