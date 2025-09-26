#ifndef SCHIP_H
#define SCHIP_H

#include "chip8_core.h"
#include "util.h"

#define SCHIP_LARGE_FONT_LOC CHIP8_HEX_FONT_START + sizeof(FONT_DATA_HEX)

#define SCHIP8_HEIGHT 64
#define SCHIP8_WIDTH 128

enum schip_display_res {
  SCHIP_DISPLAY_HIRES,
  SCHIP_DISPLAY_LORES,
};


extern const uint8_t SCHIP_HEX_FONT[10*16];


struct schip8 {
  struct chip8_core *core;

  uint16_t alloc_stack[16];
  uint8_t alloc_ram[4096];

  uint8_t rpl_flags[8];

  uint8_t will_exit;

  enum schip_display_res res;


  struct uint128 fb [64];
};

int schip8_process_instruction_general(struct uint128 *fb, struct chip8_core *core, uint8_t *rpl_flags, enum schip_display_res *res, uint8_t *will_exit);

void schip8_init(struct schip8 *vm);
int schip8_process_instruction(struct schip8 *vm);
int schip8_reset(struct schip8 *vm, FILE *file);

#endif// SCHIP_H

