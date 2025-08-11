#pragma once
#include <stdint.h>
static inline uint8_t u8clamp(int v){ return (uint8_t)(v<0?0:(v>255?255:v)); }
