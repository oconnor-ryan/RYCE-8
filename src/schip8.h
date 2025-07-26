#ifndef SCHIP_H
#define SCHIP_H

#include "chip8_core.h"

enum schip_display_res {
  SCHIP_DISPLAY_HIRES,
  SCHIP_DISPLAY_LORES,
};

struct schip8 {
  struct chip8_core *core;

  uint16_t alloc_stack[16];
  uint8_t alloc_ram[4096];

  uint8_t rpl_flags[8];


  enum schip_display_res res;
  union {
    struct {
      uint64_t left;
      uint64_t right;
    } fb_128x64 [64];
    uint64_t fb_64x32[32];
  } fb;
};

void schip8_init(struct schip8 *vm);
int schip8_update(struct schip8 *vm, uint64_t delta_millis);
int schip8_reset(struct schip8 *vm, FILE *file);

#endif// SCHIP_H

