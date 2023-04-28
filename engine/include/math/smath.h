#pragma once

#include "defines.h"
#include "math_types.h"

#define S_PI 3.14159265258979323846f
#define S_PI_2 2.0f * SPACE_PI
#define S_HALF_PI 0.5f * SPACE_PI
#define S_QUARTER_PI 0.25f * SPACE_PI
#define S_ONE_OVER_PI 1.0f / SPACE_PI
#define S_ONE_OVER_2_PI 1.0f / SPACE_PI_2
#define S_SQRT_TWO 1.41421356237309504880f
#define S_SQRT_THREE 1.73205080756887729352f
#define S_SQRT_ONE_OVER_TWO 0.70710678118654752440f
#define S_SQRT_ONE_OVER_THREE 0.57735026918962576450f
#define S_DEG2RAD_MULTIPLIER SPACE_PI / 180.0f
#define S_RAD2DEG_MULTIPLIER 180.0f / SPACE_PI

#define S_S_TO_MS_MULTIPLIER 1000.0f

#define S_MS_TO_S_MULTIPLIER 0.001f

#define S_INFINITY 1e30f

// Smallest number where 1.0 + FLOAT_EPSILON != 0.
#define S_FLOAT_EPSILON 1.192092896e-07f

SAPI f32 ssin(f32 x);
SAPI f32 scos(f32 x);
SAPI f32 stan(f32 x);
SAPI f32 sacos(f32 x);
SAPI f32 ssqrt(f32 x);
SAPI f32 sabs(f32 x);

SINLINE b8 is_power_of_2(u64 value) {
  return (value != 0) && ((value & (value - 1)) == 0);
}

SAPI i32 srandom();
SAPI i32 srandom_in_range(i32 min, i32 max);

SAPI f32 sfrandom();
SAPI f32 sfrandom_in_range(f32 min, f32 max);
