#include "util.h"


struct uint128 uint128_left_shift(struct uint128 val, uint8_t shift_by) {
  struct uint128 res = val;
  if(shift_by == 0) return res;

  //all bits from LSB has been shifted over to the MSB
  if(shift_by > 63) {
    shift_by %= 64;

    res.msb = res.lsb;
    res.lsb = 0;
    
    //shift the rest of the way on the MSB, ignoring the extra shifted out bits
    res.msb <<= shift_by;
  }
  //some bits from LSB get shifted into MSB. 
  else {
    //make sure to do sign-extension on arithmetic right shift.
    uint64_t mask = ((int64_t) 0x8000000000000000) >> shift_by;
    uint64_t bits_going_into_msb = res.lsb & mask;

    res.lsb <<= shift_by;
    res.msb <<= shift_by;

    //insert bits shifted out of lsb into msb
    res.msb |= bits_going_into_msb;
  }

  return res;
}

struct uint128 uint128_logical_right_shift(struct uint128 val, uint8_t shift_by) {
  struct uint128 res = val;
  if(shift_by == 0) return res;


  //all bits from MSB has been shifted over to the LSB
  if(shift_by > 63) {
    shift_by %= 64;

    res.lsb = res.msb;
    res.msb = 0;
    
    //shift the rest of the way on the LSB, ignoring the extra shifted out bits
    res.lsb >>= shift_by;
  }
  //some bits from MSB get shifted into LSB. 
  else {
    //we want to grab *shift_by* of the least significant bits of the MSB
    //before right shifting. This is done by setting all of the bits of the bits
    //we need to 1.
    uint64_t mask = ((uint64_t) 1 << (shift_by)) - 1;
    uint64_t bits_going_into_lsb = res.msb & mask;

    //perform the actual shift
    res.lsb >>= shift_by;
    res.msb >>= shift_by;

    //insert bits shifted out of msb into most significant bits of lsb
    res.lsb |= bits_going_into_lsb << (64 - shift_by);
  }

  return res;
}

struct uint128 uint128_and(struct uint128 a, struct uint128 b) {
  struct uint128 res;
  res.lsb = a.lsb & b.lsb;
  res.msb = a.msb & b.msb;
  return res;
}


struct uint128 uint128_or(struct uint128 a, struct uint128 b) {
  struct uint128 res;
  res.lsb = a.lsb | b.lsb;
  res.msb = a.msb | b.msb;
  return res;
}

void uint128_set(struct uint128 *a, uint8_t pos, uint8_t val) {
  if(pos > 63) {
    pos %= 64;
    if(val) {
      a->msb |= (1 << pos);
    } else {
      a->msb &= ~(1 << pos);
      
    }
  } else {
    if(val) {
      a->lsb |= (1 << pos);
    } else {
      a->lsb &= ~(1 << pos);

    }
  }
}

struct uint128 uint128_xor(struct uint128 a, struct uint128 b) {
  struct uint128 res;
  res.lsb = a.lsb ^ b.lsb;
  res.msb = a.msb ^ b.msb;
  return res; 
}
