#ifndef XO_CHIP_H
#define XO_CHIP_H

#include "schip8.h"

struct xochip {
  struct chip8_core *core;

  uint16_t alloc_stack[16];
  uint8_t alloc_ram[0xFFFF];

  uint8_t rpl_flags[16];

  uint8_t will_exit;

  enum schip_display_res res;


  uint8_t audio_buffer[16];

  struct uint128 fb_bg [64];
  struct uint128 fb_fg [64];

  uint8_t selected_fb;

};

void xochip_init(struct xochip *vm);
int xochip_process_instruction(struct xochip *vm);
int xochip_reset(struct xochip *vm, FILE *file);

#endif// XO_CHIP_H
