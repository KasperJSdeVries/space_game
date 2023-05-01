#pragma once

#include "defines.h"
#include "math_types.h"

#define S_PI 3.14159265258979323846f
#define S_PI_2 2.0f * S_PI
#define S_HALF_PI 0.5f * S_PI
#define S_QUARTER_PI 0.25f * S_PI
#define S_ONE_OVER_PI 1.0f / S_PI
#define S_ONE_OVER_2_PI 1.0f / S_PI_2
#define S_SQRT_TWO 1.41421356237309504880f
#define S_SQRT_THREE 1.73205080756887729352f
#define S_SQRT_ONE_OVER_TWO 0.70710678118654752440f
#define S_SQRT_ONE_OVER_THREE 0.57735026918962576450f
#define S_DEG2RAD_MULTIPLIER S_PI / 180.0f
#define S_RAD2DEG_MULTIPLIER 180.0f / S_PI

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

SINLINE b8 is_power_of_2(u64 value) { return (value != 0) && ((value & (value - 1)) == 0); }

SAPI i32 srandom();
SAPI i32 srandom_in_range(i32 min, i32 max);

SAPI f32 sfrandom();
SAPI f32 sfrandom_in_range(f32 min, f32 max);

// -------------------------------------------
// Vector 2
// -------------------------------------------

SINLINE vec2 vec2_create(f32 x, f32 y) { return (vec2){.x = x, .y = y}; }

SINLINE vec2 vec2_zero() { return (vec2){.x = 0.0f, .y = 0.0f}; }
SINLINE vec2 vec2_one() { return (vec2){.x = 1.0f, .y = 1.0f}; }

SINLINE vec2 vec2_up() { return (vec2){.x = 0.0f, .y = 1.0f}; }
SINLINE vec2 vec2_down() { return (vec2){.x = 0.0f, .y = -1.0f}; }
SINLINE vec2 vec2_left() { return (vec2){.x = -1.0f, .y = 0.0f}; }
SINLINE vec2 vec2_right() { return (vec2){.x = 1.0f, .y = 0.0f}; }

SINLINE vec2 vec2_add(vec2 v0, vec2 v1) { return (vec2){.x = v0.x + v1.x, .y = v0.y + v1.y}; }
SINLINE vec2 vec2_sub(vec2 v0, vec2 v1) { return (vec2){.x = v0.x - v1.x, .y = v0.y - v1.y}; }
SINLINE vec2 vec2_mul(vec2 v0, vec2 v1) { return (vec2){.x = v0.x * v1.x, .y = v0.y * v1.y}; }
SINLINE vec2 vec2_mul_scalar(vec2 v, f32 s) { return (vec2){.x = v.x * s, .y = v.y * s}; }
SINLINE vec2 vec2_div(vec2 v0, vec2 v1) { return (vec2){.x = v0.x / v1.x, .y = v0.y / v1.y}; }

SINLINE f32 vec2_length_squared(vec2 v) { return v.x * v.x + v.y * v.y; }
SINLINE f32 vec2_length(vec2 v) { return ssqrt(vec2_length_squared(v)); }

SINLINE void vec2_normalize(vec2 *v) {
	const f32 length = vec2_length(*v);
	v->x /= length;
	v->y /= length;
}
SINLINE vec2 vec2_normalized(vec2 v) {
	vec2_normalize(&v);
	return v;
}

SINLINE f32 vec2_dot(vec2 v0, vec2 v1) {
	f32 p = 0;
	p += v0.x * v1.x;
	p += v0.y * v1.y;
	return p;
}

SINLINE b8 vec2_compare(vec2 v0, vec2 v1, f32 tolerance) {
	return sabs(v0.x - v1.x) > tolerance || sabs(v0.y - v1.y) > tolerance ? false : true;
}

SINLINE f32 vec2_distance(vec2 v0, vec2 v1) {
	vec2 d = vec2_sub(v0, v1);
	return vec2_length(d);
}

// -------------------------------------------
// Vector 3
// -------------------------------------------
SINLINE vec3 vec3_create(f32 x, f32 y, f32 z) { return (vec3){.x = x, .y = y, .z = z}; }
SINLINE vec3 vec3_from_vec4(vec4 v) { return (vec3){.x = v.x, .y = v.y, .z = v.z}; }

SINLINE vec3 vec3_zero() { return (vec3){.x = 0.0f, .y = 0.0f, .z = 0.0f}; }
SINLINE vec3 vec3_one() { return (vec3){.x = 1.0f, .y = 1.0f, .z = 1.0f}; }

SINLINE vec3 vec3_up() { return (vec3){.x = 0.0f, .y = 1.0f, .z = 0.0f}; }
SINLINE vec3 vec3_down() { return (vec3){.x = 0.0f, .y = -1.0f, .z = 0.0f}; }
SINLINE vec3 vec3_left() { return (vec3){.x = -1.0f, .y = 0.0f, .z = 0.0f}; }
SINLINE vec3 vec3_right() { return (vec3){.x = 1.0f, .y = 0.0f, .z = 0.0f}; }
SINLINE vec3 vec3_forward() { return (vec3){.x = 0.0f, .y = 0.0f, .z = -1.0f}; }
SINLINE vec3 vec3_back() { return (vec3){.x = 0.0f, .y = 0.0f, .z = 1.0f}; }

SINLINE vec3 vec3_add(vec3 v0, vec3 v1) { return (vec3){.x = v0.x + v1.x, .y = v0.y + v1.y, .z = v0.z + v1.z}; }
SINLINE vec3 vec3_sub(vec3 v0, vec3 v1) { return (vec3){.x = v0.x - v1.x, .y = v0.y - v1.y, .z = v0.z - v1.z}; }
SINLINE vec3 vec3_mul(vec3 v0, vec3 v1) { return (vec3){.x = v0.x * v1.x, .y = v0.y * v1.y, .z = v0.z * v1.z}; }
SINLINE vec3 vec3_mul_scalar(vec3 v, f32 s) { return (vec3){.x = v.x * s, .y = v.y * s, .z = v.z * s}; }
SINLINE vec3 vec3_div(vec3 v0, vec3 v1) { return (vec3){.x = v0.x / v1.x, .y = v0.y / v1.y, .z = v0.z / v1.z}; }

SINLINE f32 vec3_length_squared(vec3 v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
SINLINE f32 vec3_length(vec3 v) { return ssqrt(vec3_length_squared(v)); }

SINLINE void vec3_normalize(vec3 *v) {
	const f32 length = vec3_length(*v);
	v->x /= length;
	v->y /= length;
	v->z /= length;
}
SINLINE vec3 vec3_normalized(vec3 v) {
	vec3_normalize(&v);
	return v;
}

SINLINE f32 vec3_dot(vec3 v0, vec3 v1) {
	f32 p = 0;
	p += v0.x * v1.x;
	p += v0.y * v1.y;
	p += v0.z * v1.z;
	return p;
}
SINLINE vec3 vec3_cross(vec3 v0, vec3 v1) {
	return (vec3){
		.x = v0.y * v1.z - v0.z * v1.y,
		.y = v0.z * v1.x - v0.x * v1.z,
		.z = v0.x * v1.y - v0.y * v1.x,
	};
}

SINLINE b8 vec3_compare(vec3 v0, vec3 v1, f32 tolerance) {
	return sabs(v0.x - v1.x) > tolerance || sabs(v0.y - v1.y) > tolerance || sabs(v0.z - v1.z) > tolerance ? false
																										   : true;
}

SINLINE f32 vec3_distance(vec3 v0, vec3 v1) {
	vec3 d = vec3_sub(v0, v1);
	return vec3_length(d);
}

// -------------------------------------------
// Vector 4
// -------------------------------------------

SINLINE vec4 vec4_create(f32 x, f32 y, f32 z, f32 w) {
#if defined(SPACE_USE_SIMD)
	return (vec4){.data = _mm_setr_ps(x, y, z, w)};
#else
	return (vec4){.x = x, .y = y, .z = z, .w = w};
#endif
}

SINLINE vec3 vec4_to_vec3(vec4 v) { return (vec3){.x = v.x, .y = v.y, .z = v.z}; }
SINLINE vec4 vec4_from_vec3(vec3 v, f32 w) {
#if defined(SPACE_USE_SIMD)
	return (vec4){.data = _mm_setr_ps(v.x, v.y, v.z, w)};
#else
	return (vec4){.x = v.x, .y = v.y, .z = v.z, .w = w};
#endif
}

SINLINE vec4 vec4_zero() { return vec4_create(0.0f, 0.0f, 0.0f, 0.0f); }
SINLINE vec4 vec4_one() { return vec4_create(1.0f, 1.0f, 1.0f, 1.0f); }

SINLINE vec4 vec4_add(vec4 v0, vec4 v1) {
	return (vec4){
		.x = v0.x + v1.x,
		.y = v0.y + v1.y,
		.z = v0.z + v1.z,
		.w = v0.w + v1.w,
	};
};
SINLINE vec4 vec4_sub(vec4 v0, vec4 v1) {
	return (vec4){
		.x = v0.x - v1.x,
		.y = v0.y - v1.y,
		.z = v0.z - v1.z,
		.w = v0.w - v1.w,
	};
};
SINLINE vec4 vec4_mul(vec4 v0, vec4 v1) {
	return (vec4){
		.x = v0.x * v1.x,
		.y = v0.y * v1.y,
		.z = v0.z * v1.z,
		.w = v0.w * v1.w,
	};
};
SINLINE vec4 vec4_div(vec4 v0, vec4 v1) {
	return (vec4){
		.x = v0.x / v1.x,
		.y = v0.y / v1.y,
		.z = v0.z / v1.z,
		.w = v0.w / v1.w,
	};
};

SINLINE f32 vec4_length_squared(vec4 v) { return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w; }
SINLINE f32 vec4_length(vec4 v) { return ssqrt(vec4_length_squared(v)); }

SINLINE void vec4_normalize(vec4 *v) {
	const f32 length = vec4_length(*v);
	v->x /= length;
	v->y /= length;
	v->z /= length;
	v->w /= length;
}
SINLINE vec4 vec4_normalized(vec4 v) {
	vec4_normalize(&v);
	return v;
}

SINLINE f32 vec4_dot(vec4 v0, vec4 v1) { return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w; }
SINLINE f32 vec4_dot_f32(f32 a0, f32 a1, f32 a2, f32 a3, f32 b0, f32 b1, f32 b2, f32 b3) {
	return a0 * b0 + a1 * b1 + a2 * b2 + a3 * b3;
}

// -------------------------------------------
// Square Matrices
// -------------------------------------------

// Helper function
SINLINE void mat_identity(f32 *result, u32 dim) {
	for (u32 y = 0; y < dim; ++y) {
		for (u32 x = 0; x < dim; ++x) { result[x + y * dim] = x == y ? 1.0f : 0.0f; }
	}
}

// Helper function
SINLINE void mat_mul(f32 *m0, f32 *m1, f32 *result, u32 dim) {
	mat_identity(result, dim);

	for (u32 y = 0; y < dim; ++y) {
		for (u32 x = 0; x < dim; ++x) {
			f32 value = 0;
			for (u32 i = 0; i < dim; ++i) { value += m0[i + dim * y] * m1[dim * i + x]; }
			result[x + y * dim] = value;
		}
	}
}

// -------------------------------------------
// 4x4 Matrix
// -------------------------------------------

SINLINE mat4 mat4_zero() {
	return (mat4){.data = {
					  0.0f,
					  0.0f,
					  0.0f,
					  0.0f, // row 0
					  0.0f,
					  0.0f,
					  0.0f,
					  0.0f, // row 1
					  0.0f,
					  0.0f,
					  0.0f,
					  0.0f, // row 2
					  0.0f,
					  0.0f,
					  0.0f,
					  0.0f // row 3
				  }};
}

SINLINE mat4 mat4_identity() {
	mat4 result;
	mat_identity(result.data, 4);
	return result;
}

SINLINE mat4 mat4_mul(mat4 m0, mat4 m1) {
	mat4 result = mat4_identity();
	mat_mul(m0.data, m1.data, result.data, 4);
	return result;
}

SINLINE mat4 mat4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_clip, f32 far_clip) {
	mat4 result = mat4_identity();

	f32 lr = 1.0f / (left - right);
	f32 bt = 1.0f / (bottom - top);
	f32 nf = 1.0f / (near_clip - far_clip);

	result.data[0]  = -2.0f * lr;
	result.data[5]  = -2.0f * bt;
	result.data[10] = 2.0f * nf;

	result.data[12] = (left + right) * lr;
	result.data[13] = (top + bottom) * bt;
	result.data[14] = (far_clip + near_clip) * nf;

	return result;
}

SINLINE mat4 mat4_perspective(f32 fov_rad, f32 aspect_ratio, f32 near_clip, f32 far_clip) {
	f32 half_tan_fov = stan(fov_rad * 0.5f);
	mat4 result      = mat4_zero();

	result.data[0]  = 1.0f / (aspect_ratio * half_tan_fov);
	result.data[5]  = 1.0f / half_tan_fov;
	result.data[10] = -((far_clip + near_clip) / (far_clip - near_clip));
	result.data[11] = -1.0;
	result.data[14] = -((2.0f * far_clip * near_clip) / (far_clip - near_clip));

	return result;
}

SINLINE mat4 mat4_look_at(vec3 position, vec3 target, vec3 up) {
	vec3 z_axis = vec3_sub(target, position);
	vec3_normalize(&z_axis);
	vec3 x_axis = vec3_normalized(vec3_cross(z_axis, up));
	vec3 y_axis = vec3_cross(x_axis, z_axis);

	return (mat4){.data = {
					  x_axis.x,
					  y_axis.x,
					  -z_axis.x,
					  0.0f, // row 0
					  x_axis.y,
					  y_axis.y,
					  -z_axis.y,
					  0.0f, // row 1
					  x_axis.z,
					  y_axis.z,
					  -z_axis.z,
					  0.0f, // row 2
					  -vec3_dot(x_axis, position),
					  -vec3_dot(y_axis, position),
					  vec3_dot(z_axis, position),
					  1.0f, // row 3
				  }};
}

SINLINE mat4 mat4_transposed(mat4 m) {
	mat4 result     = mat4_identity();
	result.data[0]  = m.data[0];
	result.data[1]  = m.data[4];
	result.data[2]  = m.data[8];
	result.data[3]  = m.data[12];
	result.data[4]  = m.data[1];
	result.data[5]  = m.data[5];
	result.data[6]  = m.data[9];
	result.data[7]  = m.data[13];
	result.data[8]  = m.data[2];
	result.data[9]  = m.data[6];
	result.data[10] = m.data[10];
	result.data[11] = m.data[14];
	result.data[12] = m.data[3];
	result.data[13] = m.data[7];
	result.data[14] = m.data[11];
	result.data[15] = m.data[15];
	return result;
}

SINLINE mat4 mat4_inverse(mat4 matrix) {
	const f32 *m = matrix.data;

	f32 t0  = m[10] * m[15];
	f32 t1  = m[14] * m[11];
	f32 t2  = m[6] * m[15];
	f32 t3  = m[14] * m[7];
	f32 t4  = m[6] * m[11];
	f32 t5  = m[10] * m[7];
	f32 t6  = m[2] * m[15];
	f32 t7  = m[14] * m[3];
	f32 t8  = m[2] * m[11];
	f32 t9  = m[10] * m[3];
	f32 t10 = m[2] * m[7];
	f32 t11 = m[6] * m[3];
	f32 t12 = m[8] * m[13];
	f32 t13 = m[12] * m[9];
	f32 t14 = m[4] * m[13];
	f32 t15 = m[12] * m[5];
	f32 t16 = m[4] * m[9];
	f32 t17 = m[8] * m[5];
	f32 t18 = m[0] * m[13];
	f32 t19 = m[12] * m[1];
	f32 t20 = m[0] * m[9];
	f32 t21 = m[8] * m[1];
	f32 t22 = m[0] * m[5];
	f32 t23 = m[4] * m[1];

	mat4 result;
	f32 *r = result.data;

	r[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) - (t1 * m[5] + t2 * m[9] + t5 * m[13]);
	r[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) - (t0 * m[1] + t7 * m[9] + t8 * m[13]);
	r[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) - (t3 * m[1] + t6 * m[5] + t11 * m[13]);
	r[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) - (t4 * m[1] + t9 * m[5] + t10 * m[9]);

	f32 d = 1.0f / (m[0] * r[0] + m[4] * r[1] + m[8] * r[2] + m[12] * r[3]);

	r[0]  = d * r[0];
	r[1]  = d * r[1];
	r[2]  = d * r[2];
	r[3]  = d * r[3];
	r[4]  = d * ((t1 * m[4] + t2 * m[8] + t5 * m[12]) - (t0 * m[4] + t3 * m[8] + t4 * m[12]));
	r[5]  = d * ((t0 * m[0] + t7 * m[8] + t8 * m[12]) - (t1 * m[0] + t6 * m[8] + t9 * m[12]));
	r[6]  = d * ((t3 * m[0] + t6 * m[4] + t11 * m[12]) - (t2 * m[0] + t7 * m[4] + t10 * m[12]));
	r[7]  = d * ((t4 * m[0] + t9 * m[4] + t10 * m[8]) - (t5 * m[0] + t8 * m[4] + t11 * m[8]));
	r[8]  = d * ((t12 * m[7] + t15 * m[11] + t16 * m[15]) - (t13 * m[7] + t14 * m[11] + t17 * m[15]));
	r[9]  = d * ((t13 * m[3] + t18 * m[11] + t21 * m[15]) - (t12 * m[3] + t19 * m[11] + t20 * m[15]));
	r[10] = d * ((t14 * m[3] + t19 * m[7] + t22 * m[15]) - (t15 * m[3] + t18 * m[7] + t23 * m[15]));
	r[11] = d * ((t17 * m[3] + t20 * m[7] + t23 * m[11]) - (t16 * m[3] + t21 * m[7] + t22 * m[11]));
	r[12] = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6]) - (t16 * m[14] + t12 * m[6] + t15 * m[10]));
	r[13] = d * ((t20 * m[14] + t12 * m[2] + t19 * m[10]) - (t18 * m[10] + t21 * m[14] + t13 * m[2]));
	r[14] = d * ((t18 * m[6] + t23 * m[14] + t15 * m[2]) - (t22 * m[14] + t14 * m[2] + t19 * m[6]));
	r[15] = d * ((t22 * m[10] + t16 * m[2] + t21 * m[6]) - (t20 * m[6] + t23 * m[10] + t17 * m[2]));

	return result;
}

SINLINE mat4 mat4_translation(vec3 position) {
	mat4 result     = mat4_identity();
	result.data[12] = position.x;
	result.data[13] = position.y;
	result.data[14] = position.z;
	return result;
}

SINLINE mat4 mat4_scale(vec3 scale) {
	mat4 result     = mat4_identity();
	result.data[0]  = scale.x;
	result.data[5]  = scale.y;
	result.data[10] = scale.z;
	return result;
}

SINLINE mat4 mat4_euler_x(f32 angle_rad) {
	mat4 result = mat4_identity();
	f32 c       = scos(angle_rad);
	f32 s       = ssin(angle_rad);

	result.data[0]  = c;
	result.data[2]  = s;
	result.data[8]  = -s;
	result.data[10] = c;

	return result;
}
SINLINE mat4 mat4_euler_y(f32 angle_rad) {
	mat4 result = mat4_identity();
	f32 c       = scos(angle_rad);
	f32 s       = ssin(angle_rad);

	result.data[0]  = c;
	result.data[2]  = -s;
	result.data[8]  = s;
	result.data[10] = c;

	return result;
}
SINLINE mat4 mat4_euler_z(f32 angle_rad) {
	mat4 result = mat4_identity();
	f32 c       = scos(angle_rad);
	f32 s       = ssin(angle_rad);

	result.data[0] = c;
	result.data[1] = s;
	result.data[4] = -s;
	result.data[5] = c;

	return result;
}
SINLINE mat4 mat4_euler_xyz(f32 x_rad, f32 y_rad, f32 z_rad) {
	mat4 rx = mat4_euler_x(x_rad);
	mat4 ry = mat4_euler_y(y_rad);
	mat4 rz = mat4_euler_z(z_rad);

	return mat4_mul(mat4_mul(rx, ry), rz);
}

SINLINE vec3 mat4_forward(mat4 m) {
	return vec3_normalized((vec3){
		.x = -m.data[2],
		.y = -m.data[6],
		.z = -m.data[10],
	});
}
SINLINE vec3 mat4_backward(mat4 m) {
	return vec3_normalized((vec3){
		.x = m.data[2],
		.y = m.data[6],
		.z = m.data[10],
	});
}
SINLINE vec3 mat4_up(mat4 m) {
	return vec3_normalized((vec3){
		.x = m.data[1],
		.y = m.data[5],
		.z = m.data[9],
	});
}
SINLINE vec3 mat4_down(mat4 m) {
	return vec3_normalized((vec3){
		.x = -m.data[1],
		.y = -m.data[5],
		.z = -m.data[9],
	});
}
SINLINE vec3 mat4_left(mat4 m) {
	return vec3_normalized((vec3){
		.x = -m.data[0],
		.y = -m.data[4],
		.z = -m.data[8],
	});
}
SINLINE vec3 mat4_right(mat4 m) {
	return vec3_normalized((vec3){
		.x = m.data[0],
		.y = m.data[4],
		.z = m.data[8],
	});
}

// -------------------------------------------
// Quaternions
// -------------------------------------------

SINLINE quat quat_identity() { return (quat){.x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f}; }

SINLINE f32 quat_normal(quat q) { return ssqrt(vec4_dot(q, q)); }
SINLINE quat quat_normalize(quat q) {
	f32 normal = quat_normal(q);
	return (quat){.x = q.x / normal, .y = q.y / normal, .z = q.z / normal, .w = q.w / normal};
}

SINLINE quat quat_conjugate(quat q) { return (quat){.x = -q.x, .y = -q.y, .z = -q.z, .w = q.w}; }

SINLINE quat quat_inverse(quat q) { return quat_normalize(quat_conjugate(q)); }

SINLINE quat quat_mul(quat q0, quat q1) {
	return (quat){
		.x = q0.x * q1.w + q0.y * q1.z - q0.z * q1.y + q0.w * q1.x,
		.y = -q0.x * q1.z + q0.y * q1.w + q0.z * q1.x + q0.w * q1.y,
		.z = q0.x * q1.y - q0.y * q1.x + q0.z * q1.w + q0.w * q1.z,
		.w = -q0.x * q1.x - q0.y * q1.y - q0.z * q1.z + q0.w * q1.w,
	};
}

SINLINE f32 quat_dot(quat q0, quat q1) { return vec4_dot(q0, q1); }

SINLINE mat4 quat_to_mat4(quat q) {
	mat4 result = mat4_identity();

	quat n = quat_normalize(q);

	result.data[0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
	result.data[1] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
	result.data[2] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;

	result.data[4] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
	result.data[5] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
	result.data[6] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;

	result.data[8]  = 2.0f * n.x * n.z - 2.0f * n.y * n.w;
	result.data[9]  = 2.0f * n.y * n.z + 2.0f * n.x * n.w;
	result.data[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;

	return result;
}

SINLINE mat4 quat_to_rotation_matrix(quat q, vec3 center) {
	mat4 result;

	f32 *r = result.data;
	r[0]   = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
	r[1]   = 2.0f * ((q.x * q.y) + (q.z * q.w));
	r[2]   = 2.0f * ((q.x * q.z) - (q.y * q.w));
	r[3]   = center.x - center.x * r[0] - center.y * r[1] - center.z * r[2];

	r[4] = 2.0f * ((q.x * q.y) - (q.z * q.w));
	r[5] = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
	r[6] = 2.0f * ((q.y * q.z) + (q.x * q.w));
	r[7] = center.y - center.x * r[4] - center.y * r[5] - center.z * r[6];

	r[8]  = 2.0f * ((q.x * q.z) + (q.y * q.w));
	r[9]  = 2.0f * ((q.y * q.z) - (q.x * q.w));
	r[10] = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
	r[11] = center.z - center.x * r[8] - center.y * r[9] - center.z * r[10];

	r[12] = 0.0f;
	r[13] = 0.0f;
	r[14] = 0.0f;
	r[15] = 1.0f;

	return result;
}

SINLINE quat quat_from_axis_angle(vec3 axis, f32 angle, b8 normalize) {
	const f32 half_angle = 0.5f * angle;
	f32 s                = ssin(half_angle);
	f32 c                = scos(half_angle);

	quat q = (quat){.x = s * axis.x, .y = s * axis.y, .z = s * axis.z, .w = c};

	return normalize ? quat_normalize(q) : q;
}

SINLINE quat quat_slerp(quat q0, quat q1, f32 percentage) {
	quat n0 = quat_normalize(q0);
	quat n1 = quat_normalize(q1);

	f32 dot = quat_dot(n0, n1);

	if (dot < 0.0f) {
		n1.x = -n1.x;
		n1.y = -n1.y;
		n1.z = -n1.z;
		n1.w = -n1.w;
		dot  = -dot;
	}

	f32 theta0     = sacos(dot);
	f32 theta      = theta0 * percentage;
	f32 sin_theta  = ssin(theta);
	f32 sin_theta0 = ssin(theta0);

	f32 s0 = scos(theta) - dot * sin_theta / sin_theta0;
	f32 s1 = sin_theta / sin_theta0;

	return (quat){.x = (n0.x * s0) + (n1.x * s1),
				  .y = (n0.y * s0) + (n1.y * s1),
				  .z = (n0.z * s0) + (n1.z * s1),
				  .w = (n0.w * s0) + (n1.w * s1)};
}

SINLINE f32 deg_to_rad(f32 degrees) { return degrees * S_DEG2RAD_MULTIPLIER; }
SINLINE f32 rad_to_deg(f32 radians) { return radians * S_RAD2DEG_MULTIPLIER; }
