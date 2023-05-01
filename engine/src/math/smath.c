#include "math/smath.h"

#include "platform/platform.h"

static b8 rand_seeded = false;

#include <math.h>
#undef __USE_MISC
#include <stdlib.h>

f32 ssin(f32 x) { return sinf(x); }

f32 scos(f32 x) { return cosf(x); }

f32 stan(f32 x) { return tanf(x); }

f32 sacos(f32 x) { return acosf(x); }

f32 ssqrt(f32 x) { return sqrtf(x); }

f32 sabs(f32 x) { return fabsf(x); }

i32 srandom() {
	if (!rand_seeded) {
		srand((u32)platform_get_absolute_time());
		rand_seeded = true;
	}
	return rand();
}

i32 srandom_in_range(i32 min, i32 max) { return (srandom() % (max - min + 1)) + min; }

f32 sfrandom() { return (f32)srandom() / (f32)RAND_MAX; }

f32 sfrandom_in_range(f32 min, f32 max) { return min + ((f32)srandom() / ((f32)RAND_MAX / (max - min))); }
