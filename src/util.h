#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

//note that there exists a __uint128_t type in GCC, but this is a compiler extension.
//I want to avoid using compiler extensions and stick to what ISO C provides,
//so I will implement my own 128-bit unsigned integer.
struct uint128 {
  uint64_t msb;
  uint64_t lsb;
};

struct uint128 uint128_left_shift(struct uint128 val, uint8_t shift_by);
struct uint128 uint128_logical_right_shift(struct uint128 val, uint8_t shift_by);
struct uint128 uint128_and(struct uint128 a, struct uint128 b);
struct uint128 uint128_xor(struct uint128 a, struct uint128 b);
struct uint128 uint128_or(struct uint128 a, struct uint128 b);

void uint128_set(struct uint128 *a, uint8_t pos, uint8_t val);




#endif// UTIL_H
