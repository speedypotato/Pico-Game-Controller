#pragma once
#include <math.h>
static inline float circular_distance(float a, float b, float m) {
  float d = fabsf(a - b); return d < (m - d) ? d : (m - d);
}
