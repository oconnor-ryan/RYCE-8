#include "schip8.h"
#include <string.h>

#define SCHIP8_HEIGHT 64
#define SCHIP8_WIDTH 128


//the original SUPER-CHIP did not define A-F in its large fontset,
//though consider adding on in the future just for fun.
const uint8_t SCHIP_HEX_FONT[10*16] = {
  0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C,
  0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C,
  0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF,
  0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C,
  0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06,
  0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C,
  0x3E, 0x7C, 0xE0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C,
  0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60,
  0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C,
  0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void schip8_init(struct schip8 *vm) {
  //set pointers to allocated memory
  vm->core->ram = vm->alloc_ram;
  vm->core->stack = (uint16_t*) (vm->alloc_ram + CHIP8_STACK_START);
  vm->core->fb = vm->fb.x64_32;

  //initialize sizes
  vm->core->ram_size = sizeof(vm->alloc_ram);
  vm->core->stack_size = 16;
  vm->core->fb_size = sizeof(vm->fb.x128_64);

  //start at lores by default
  vm->res = SCHIP_DISPLAY_LORES;

  memset(vm->rpl_flags, 0, sizeof(vm->rpl_flags));

  vm->core->quirks = CHIP8_QUIRK_BXNN 
  | CHIP8_QUIRK_CLR_SCN_ON_LORES
  //| CHIP8_QUIRK_HALF_PIXEL_SCROLL_LOW_RES
  ;

}

/* Note that scrolling instructions will NOT WRAP sprites. */
static inline int schip8_ins_00(struct schip8 *vm, uint8_t low) {
  //00CN*    Scroll display N lines down
  if(low >> 4 == 0xC) {
    uint8_t n = low & 0x0F;

    //if the "half scroll" quirk for lores scrolling is NOT enabled
    //and the emulator is in lores mode,
    //we need to scroll down 2 pixels instead of 1.
    if(!(vm->core->quirks & CHIP8_QUIRK_HALF_PIXEL_SCROLL_LOW_RES) 
    && vm->res == SCHIP_DISPLAY_LORES) {
      n *= 2;
    }
    

    //start at bottom set of rows and shift them all downward.
    for(uint8_t num_rows_from_bottom = n; num_rows_from_bottom < SCHIP8_HEIGHT; num_rows_from_bottom++) {
      uint8_t move_from = SCHIP8_HEIGHT-1 - num_rows_from_bottom;
      uint8_t move_to = move_from + n;
      vm->fb.x128_64[move_to] = vm->fb.x128_64[move_from];
      //make sure to clear row being moved from
      memset(&vm->fb.x128_64[move_from], 0, sizeof(vm->fb.x128_64[move_from]));
    }

    return 1;
  }

  switch(low) {
    //00FB*    Scroll display 4 pixels right
    case 0xFB: {
      uint8_t shift_amount = 4;

      //if the "half scroll" quirk for lores scrolling is NOT enabled
      //and the emulator is in lores mode,
      //we need to scroll down 2 pixels instead of 1.
      if(!(vm->core->quirks & CHIP8_QUIRK_HALF_PIXEL_SCROLL_LOW_RES) 
      && vm->res == SCHIP_DISPLAY_LORES) {
        shift_amount = 8;
      }

      for(uint8_t r = 0; r < SCHIP8_HEIGHT; r++) {
        vm->fb.x128_64[r] = uint128_logical_right_shift(vm->fb.x128_64[r], shift_amount);
      }
      break;
    }
    //00FC*    Scroll display 4 pixels left
    case 0xFC: {
      uint8_t shift_amount = 4;

      //if the "half scroll" quirk for lores scrolling is NOT enabled
      //and the emulator is in lores mode,
      //we need to scroll down 2 pixels instead of 1.
      if(!(vm->core->quirks & CHIP8_QUIRK_HALF_PIXEL_SCROLL_LOW_RES) 
      && vm->res == SCHIP_DISPLAY_LORES) {
        shift_amount = 8;
      }

      for(uint8_t r = 0; r < SCHIP8_HEIGHT; r++) {
        vm->fb.x128_64[r] = uint128_left_shift(vm->fb.x128_64[r], shift_amount);
      }

      break;
    }

    //00FD*    Exit CHIP interpreter
    case 0xFD: {
      vm->will_exit = 1;
      break;
    }

    //00FE*    Disable extended screen mode
    case 0xFE: {
      vm->res = SCHIP_DISPLAY_LORES;
      break;
    }

    //00FF*    Enable extended screen mode for full-screen graphics
    case 0xFF: {

      //if its already enabled, ignore.
      vm->res = SCHIP_DISPLAY_HIRES;

      /*

      //scale 64x32 framebuffer to 128x64 framebuffer.
      const uint64_t mask3 = (uint64_t)3;

      uint64_t fb[32];
      
      //to avoid issues modifying the framebuffer in place,
      //we will copy it to a temporary array.
      memcpy(fb, vm->fb.x64_32, sizeof(fb));

      for(uint8_t r = 0; r < 32; r++) {
        uint64_t row = fb[r]; 
        for(uint8_t c = 0; c < 32; c++) {
          // use 3 so that for every 
          uint8_t is_lit = (row & ((uint64_t) 1 << c)) ? 1 : 0;
          //uint8_t is_lit = (row & (((uint64_t) 1 << 63) >> c)) ? 3 : 0;

          if(is_lit) {
            vm->fb.x128_64[2*r].lsb |= mask3 << 2*c;
            vm->fb.x128_64[2*r + 1].lsb |= mask3 << 2*c;

          } else {
            vm->fb.x128_64[2*r].lsb &= ~ (mask3 << 2*c);
            vm->fb.x128_64[2*r + 1].lsb &= ~ (mask3 << 2*c);

          }
        }
        for(uint8_t c = 32; c < 64; c++) {
          uint8_t is_lit = (row & ((uint64_t) 1 << c)) ? 1 : 0;
          //uint8_t is_lit = (row & (((uint64_t) 1 << 63) >> c)) ? 3 : 0;
          if(is_lit) {
            vm->fb.x128_64[2*r].msb |= mask3 << 2*c;
            vm->fb.x128_64[2*r + 1].msb |= mask3 << 2*c;

          } else {
            vm->fb.x128_64[2*r].msb &= ~(mask3 << 2*c);
            vm->fb.x128_64[2*r + 1].msb &= ~(mask3 << 2*c);
          }
          
        }
      }
      */
      break;
    }

    default: return 0;
  }

  return 1;
}

/*
  This works pretty much the same as the standard draw function for 
  COSMAC VIP, except that in order to emulate the artifacts left behind by
  switching from lores to hires and vise-verse, we must render the 64x32 display
  on a 128x64 framebuffer and scale each 1x1 pixel to a 2x2 pixel.

  This means for each sprite row, we draw onto 2 framebuffer rows and
  make sure that we draw 2 columns for each 1x1 pixel in the sprite.
*/
void schip8_draw_64x32(struct schip8 *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  uint8_t y = low >> 4;
  uint8_t n = low & 0x0F;

  // Note that when drawing a sprite, if a lit pixel from the sprite draws
  // over a previously lit pixel, that pixel gets TURNED OFF. It does not stay on.

  //make sure collision bool matches the size of the row (64 bits). Otherwise, the result of a comparison
  // will be truncated when using AND
  uint64_t collision = 0;

  // because sprites are formated like so:
  /*
          Bits:
          7 6 5 4 3 2 1 0
  Byte 1: 0 1 1 1 1 1 1 0
  Byte 2: 0 1 0 0 0 0 0 0
  Byte 3: 0 1 1 1 1 1 1 0
  Byte 4: 0 1 0 0 0 0 0 0
  Byte 5: 0 1 1 1 1 1 1 0

  Our framebuffer must also match this format by making the most significant
  bit (63rd) correspond to X = 0 on the screen and the least significant bit (0th
  bit correspond to X = 63 on screen.

  */

  //Note: ONLY Initial X and Y coordinates get WRAPPED AROUND.
  //Make sure to use modulus
  uint8_t fbx = vm->core->V[x]*2 % SCHIP8_WIDTH;
  uint8_t fby = vm->core->V[y]*2 % SCHIP8_HEIGHT;

  //when drawing the sprite, you want to CLIP them if they go off-screen,
  //NOT WRAPAROUND



  // Note that the original SCHIP 1.1 spec did not specify 
  // what occurs when using Dxy0 in lores mode. On COSMAC VIP, its a NOP.
  //
  // On the HP48 calculator SUPER-CHIP was 1st implemented on, 
  // using the Dxy0 instruction in lores mode would render a 8x16 sprite.
  //
  // For now, I will choose to replicate the HP48's behavior.
  // TODO: Consider making this a quirk by allowing the user to choose
  // whether Dxy0 is a NOP or draws a 8x16 sprite in lores mode.
  if(n == 0) n = 16;

  //for each row to draw to
  for(uint8_t i = 0; i < n; i++) {

    //grab copy of row, make sure to grab 2 rows since we are rendering 2x2 pixels.
    struct uint128 old_row1 = vm->fb.x128_64[fby];
    struct uint128 old_row2 = vm->fb.x128_64[fby+1];


    //create mask for bits we are going to draw to. 
    //Dont use bit rotation since we want to perform X-coord clipping on sprite row.
    struct uint128 mask;
    mask.msb = ((uint64_t)0xFFFF << 48);
    mask.lsb = 0;

    mask = uint128_logical_right_shift(mask, fbx);


    //zero out the bits we dont draw to in old_row.
    old_row1 = uint128_and(old_row1, mask);
    old_row2 = uint128_and(old_row2, mask);


    //create a empty 64-bit row, get an 8-bit row from our sprite, and
    //shift our sprite's row into the empty row
    uint8_t sprite_row_data_raw = vm->core->ram[vm->core->I + i];

    //for every pixel, repeat its value on 2 columns so that it can be rendered as a 2x2 pixel
    // on a 128x64 framebuffer.
    uint16_t sprite_row_data = 0;

    for(uint8_t shift = 0; shift < 8; shift += 1) {
      uint16_t is_lit = (uint16_t)((1 << shift) & sprite_row_data_raw) ? 3 : 0;
      sprite_row_data |= is_lit << (shift*2);
    }

    
    struct uint128 sprite_row1;
    sprite_row1.msb = (uint64_t)sprite_row_data << 48;
    sprite_row1.lsb = 0;
    struct uint128 sprite_row2 = sprite_row1;

    sprite_row1 = uint128_logical_right_shift(sprite_row1, fbx);
    sprite_row2 = uint128_logical_right_shift(sprite_row2, fbx);



    // How do we know if collision occured?
    // 1. If a previously lit bit is drawn over by a lit bit from the sprite being drawn.

    //  00111011 (old)
    //^ 00010100 (sprite)
    //  ========
    //  00101111 (new)
    //     ^
    //  collided. Note that collision can be detected by checking if the bit in both old_row and sprite_row
    //            are 1.

    // To check if a collision occurred, simply AND the (old) and (sprite)

    //use OR so that if it was already set to 1, it stays at 1.
    collision |= old_row1.lsb & sprite_row1.lsb;
    collision |= old_row1.msb & sprite_row1.msb;
    collision |= old_row2.lsb & sprite_row2.lsb;
    collision |= old_row2.msb & sprite_row2.msb;



    //draw row to framebuffer
    vm->fb.x128_64[fby] = uint128_xor(vm->fb.x128_64[fby], sprite_row1);
    vm->fb.x128_64[fby+1] = uint128_xor(vm->fb.x128_64[fby+1], sprite_row2);



    //implements clipping behavior of sprites for Y where the X and Y inside the DXYN instruction
    //wrap around, but the subsequent rows of the sprite get clipped off if going offscreen
    fby+=2;
    if(fby >= SCHIP8_HEIGHT) break;

  }

  //remember that V MUST BE 0 or 1, it cannot be any other value
  vm->core->V[15] = collision ? 1 : 0;
}

void schip8_draw_128x64(struct schip8 *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;
  uint8_t y = low >> 4;
  uint8_t n = low & 0x0F;

  // unlike lores mode, the VF register will be set equal to the number of
  // rows that collided with something plus the number of rows that get clipped
  // from the BOTTOM of the screen (not the other sides).
  // VF = (num_rows_with_collisions) + (num_bottom_clipped_rows).

  uint8_t num_collided_or_clipped_rows = 0;

  // because sprites are formated like so:
  /*
          Bits:
          7 6 5 4 3 2 1 0
  Byte 1: 0 1 1 1 1 1 1 0
  Byte 2: 0 1 0 0 0 0 0 0
  Byte 3: 0 1 1 1 1 1 1 0
  Byte 4: 0 1 0 0 0 0 0 0
  Byte 5: 0 1 1 1 1 1 1 0

  Our framebuffer must also match this format by making the most significant
  bit (63rd) correspond to X = 0 on the screen and the least significant bit (0th
  bit correspond to X = 63 on screen.

  */

  //Note: ONLY Initial X and Y coordinates get WRAPPED AROUND.
  //Make sure to use modulus
  uint8_t fbx = vm->core->V[x] % SCHIP8_WIDTH;
  uint8_t fby = vm->core->V[y] % SCHIP8_HEIGHT;

  //when drawing the sprite, you want to CLIP them if they go off-screen,
  //NOT WRAPAROUND

  //draw 16x16 sprite
  if(n == 0) {
    //for each row to draw to
    for(uint8_t i = 0; i < 16; i++) {

      //grab copy of row
      struct uint128 old_row = vm->fb.x128_64[fby];


      //create mask for bits we are going to draw to. 
      //Dont use bit rotation since we want to perform X-coord clipping on sprite row.
      struct uint128 mask;

      //because this is 16x16, remember to mask for 16 bits, NOT the 8 bits that we normally
      //get for sprites.
      mask.msb = ((uint64_t)0xFFFF << 48);
      mask.lsb = 0;

      mask = uint128_logical_right_shift(mask, fbx);


      //zero out the bits we dont draw to in old_row.
      old_row = uint128_and(old_row, mask);


      //create a empty 64-bit row, get an 8-bit row from our sprite, and
      //shift our sprite's row into the empty row
      uint8_t sprite_row_data = vm->core->ram[vm->core->I + i];
      
      struct uint128 sprite_row;
      sprite_row.msb = (uint64_t)sprite_row_data << 48;
      sprite_row.lsb = 0;

      sprite_row = uint128_logical_right_shift(sprite_row, fbx);



      // To check if a collision occurred, simply AND the (old) and (sprite)
      //use OR so that if it was already set to 1, it stays at 1.
      uint64_t c = old_row.lsb & sprite_row.lsb;
      c |= old_row.msb & sprite_row.msb;

      //if a collision occurred, increment the number of collisions
      num_collided_or_clipped_rows += c ? 1 : 0;



      //draw row to framebuffer
      vm->fb.x128_64[fby] = uint128_xor(vm->fb.x128_64[fby], sprite_row);



      //implements clipping behavior of sprites for Y where the X and Y inside the DXYN instruction
      //wrap around, but the subsequent rows of the sprite get clipped off if going offscreen
      fby += 1;
      if(fby >= SCHIP8_HEIGHT) {

        //add the number of rows we could not render to collision
        num_collided_or_clipped_rows += 16 - (i+1);
        break;
      }

    }

    //remember that V MUST BE 0 or 1, it cannot be any other value
    //vm->core->V[15] = collision ? 1 : 0;
  } else {
    //for each row to draw to
    for(uint8_t i = 0; i < n; i++) {

      //grab copy of row
      struct uint128 old_row = vm->fb.x128_64[fby];


      //create mask for bits we are going to draw to. 
      //Dont use bit rotation since we want to perform X-coord clipping on sprite row.
      struct uint128 mask;
      mask.msb = ((uint64_t)0xFF << 56);
      mask.lsb = 0;

      mask = uint128_logical_right_shift(mask, fbx);


      //zero out the bits we dont draw to in old_row.
      old_row = uint128_and(old_row, mask);


      //create a empty 64-bit row, get an 8-bit row from our sprite, and
      //shift our sprite's row into the empty row
      uint8_t sprite_row_data = vm->core->ram[vm->core->I + i];
      
      struct uint128 sprite_row;
      sprite_row.msb = (uint64_t)sprite_row_data << 56;
      sprite_row.lsb = 0;

      sprite_row = uint128_logical_right_shift(sprite_row, fbx);


      // To check if a collision occurred, simply AND the (old) and (sprite)
      //use OR so that if it was already set to 1, it stays at 1.
      uint64_t c = old_row.lsb & sprite_row.lsb;
      c |= old_row.msb & sprite_row.msb;

      //if a collision occurred, increment the number of collisions
      num_collided_or_clipped_rows += c ? 1 : 0;



      //draw row to framebuffer
      vm->fb.x128_64[fby] = uint128_xor(vm->fb.x128_64[fby], sprite_row);



      //implements clipping behavior of sprites for Y where the X and Y inside the DXYN instruction
      //wrap around, but the subsequent rows of the sprite get clipped off if going offscreen
      fby += 1;
      if(fby >= SCHIP8_HEIGHT) {
         //add the number of rows we could not render to collision
        num_collided_or_clipped_rows += n - (i+1);
        break;
      }

    }
  }

  vm->core->V[15] = num_collided_or_clipped_rows;
}



static inline int schip8_ins_D(struct schip8 *vm, uint8_t high, uint8_t low) {
  if(vm->res == SCHIP_DISPLAY_LORES) {
    schip8_draw_64x32(vm, high, low);
    return 1;
  } else {
    schip8_draw_128x64(vm, high, low);
    return 1;
  }
}


static inline int schip8_ins_F(struct schip8 *vm, uint8_t high, uint8_t low) {
  uint8_t x = high & 0x0F;

  switch(low) {
    //FX30*    Point I to 10-byte font sprite for digit VX (0..9)
    case 0x30: {
      vm->core->I = vm->core->ram[SCHIP_LARGE_FONT_LOC + vm->core->V[x]];
      break;
    }

    //FX75*    Store V0..VX in RPL user flags (X <= 7)
    case 0x75: {
      memcpy(vm->rpl_flags, vm->core->V, x <= 7 ? x : 7);
      break;
    }

    //FX85*    Read V0..VX from RPL user flags (X <= 7)
    case 0x85: {
      memcpy(vm->core->V, vm->rpl_flags, x <= 7 ? x : 7);
      break;
    }

    default: return 0;
  }

  return 1;
}


int schip8_reset(struct schip8 *vm, FILE *file) {
  memset(&vm->fb, 0, sizeof(vm->fb));
  vm->res = SCHIP_DISPLAY_LORES;
  return chip8_reset(vm->core, file);

}

int schip8_process_new_instruction(struct schip8 *vm) {

  uint8_t high = vm->core->ram[vm->core->pc];
  uint8_t low = vm->core->ram[vm->core->pc+1];

  //INCREMENT PC!!!
  // TODO: You should probably put this in the Chip8 wrapper to avoid 
  //having to repeat this code for every Chip8 variant.
  uint16_t old_pc = vm->core->pc;
  vm->core->pc += 2;


  uint8_t result = 0;
  if(high == 0x00) {
    result = schip8_ins_00(vm, low);
  } else if((high >> 4) == 0xD) {
    result = schip8_ins_D(vm, high, low);
  } else if((high >> 4) == 0xF) {
    result = schip8_ins_F(vm, high, low);
  } 

  if(!result) {
    //jump back
    vm->core->pc = old_pc;
    return 0;
  }

  return 1;
}

int schip8_process_instruction(struct schip8 *vm) {
  
  //we first look for the new instructions, then the old instructions.
  //if current instruction belongs to neither of these sets, throw error
  if(!schip8_process_new_instruction(vm) && !chip8_process_instruction(vm->core)) {
    return 0;
  }

  return 1;
}

