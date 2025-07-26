#ifndef VIP_CHIP8_H
#define VIP_CHIP8_H

#include "chip8_core.h"

struct vip_chip8 {
  struct chip8_core *core;

  uint8_t alloc_ram[4096];
  uint64_t alloc_fb[32];


};


void vip_chip8_init(struct vip_chip8 *vm);
int vip8_chip8_update(struct vip_chip8 *vm, uint64_t delta_millis);
int vip8_chip8_reset(struct vip_chip8 *vm, FILE *file);

#endif// VIP_CHIP8_H

