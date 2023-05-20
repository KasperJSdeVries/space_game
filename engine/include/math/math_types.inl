#pragma once

#include "defines.h"

typedef union vec2_u {
	f32 elements[2];
	struct {
		union {
			f32 x, r, s, u, w;
		};
		union {
			f32 y, g, t, v, h;
		};
	};
} vec2;

typedef union vec3_u {
	f32 elements[3];
	struct {
		union {
			f32 x, r, s, u;
		};
		union {
			f32 y, g, t, v;
		};
		union {
			f32 z, b, p, w;
		};
	};
} vec3;

typedef union vec4_u {
	// #if defined(SPACE_USE_SIMD)
	//   alignas(16) __m128 data;
	// #endif
	//   alignas(16) f32 elements[4];
	struct {
		union {
			f32 x, r, s;
		};
		union {
			f32 y, g, t;
		};
		union {
			f32 z, b, p;
		};
		union {
			f32 w, a, q;
		};
	};
	f32 elements[4];
} vec4;

typedef vec4 quat;

typedef union mat4_u {
	/* alignas(16) */ f32 data[16];
	/* alignas(16) */ vec4 rows[4];
} mat4;

typedef struct vertex_3d {
	vec3 position;
	vec2 texture_coordinate;
} vertex_3d;
