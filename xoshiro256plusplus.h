#include <stdint.h>

#ifndef XOSHIRO_H
#define XOSHIRO_H

static inline uint64_t rotl(const uint64_t x, int k);

//static uint64_t s[4];

uint64_t next(void);

void jump(void);

void long_jump(void);

#endif /* XOSHIRO_H */
