#ifndef SCHIP_H
#define SCHIP_H

#include "chip8_core.h"
#include "util.h"

enum schip_display_res {
  SCHIP_DISPLAY_HIRES,
  SCHIP_DISPLAY_LORES,
};



struct schip8 {
  struct chip8_core *core;

  uint16_t alloc_stack[16];
  uint8_t alloc_ram[4096];

  uint8_t rpl_flags[8];

  uint8_t will_exit;

  enum schip_display_res res;


  union {
    struct uint128 x128_64 [64];
    uint64_t x64_32[32];
  } fb;
};

void schip8_init(struct schip8 *vm);
int schip8_process_instruction(struct schip8 *vm);
int schip8_reset(struct schip8 *vm, FILE *file);

#endif// SCHIP_H

