#include "game.h"

#include <core/input.h>
#include <core/logger.h>
#include <core/smemory.h>
#include <defines.h>
#include <math/smath.h>

#include <renderer/renderer_frontend.h>

void recalculate_viem_matrix(game_state *state) {
	if (!state->camera_view_dirty) return;

	mat4 rotation    = mat4_euler_xyz(state->camera_euler.x, state->camera_euler.y, state->camera_euler.z);
	mat4 translation = mat4_translation(state->camera_position);

	state->view = mat4_mul(rotation, translation);
	state->view = mat4_inverse(state->view);

	state->camera_view_dirty = false;
}

void camera_pitch(game_state *state, f32 pitch_by) {
	state->camera_euler.x += pitch_by;

	f32 limit             = deg_to_rad(89.0f);
	state->camera_euler.x = SCLAMP(state->camera_euler.x, -limit, limit);

	state->camera_view_dirty = true;
}

void camera_yaw(game_state *state, f32 yaw_by) {
	state->camera_euler.y += yaw_by;
	state->camera_view_dirty = true;
}

b8 game_initialize(game *game_instance) {
	game_state *state = game_instance->state;

	state->camera_position = (vec3){{0, 0, 30.0f}};
	state->camera_euler    = vec3_zero();

	state->camera_view_dirty = true;

	renderer_set_view(state->view);

	SDEBUG("Initializing game.");
	return true;
}

b8 game_update(game *game_instance, f32 delta_time) {
	static u64 alloc_count = 0;
	u64 prev_alloc_count   = alloc_count;
	alloc_count            = get_memory_alloc_count();
	if (input_is_key_up(KEY_M) && input_was_key_down(KEY_M)) {
		SDEBUG("Allocations: %llu (%llu this frame)", alloc_count, alloc_count - prev_alloc_count);
	}

	game_state *state = game_instance->state;

	if (input_is_key_down(KEY_LEFT)) { camera_yaw(state, 1.0f * delta_time); }
	if (input_is_key_down(KEY_RIGHT)) { camera_yaw(state, -1.0f * delta_time); }
	if (input_is_key_down(KEY_UP)) { camera_pitch(state, 1.0f * delta_time); }
	if (input_is_key_down(KEY_DOWN)) { camera_pitch(state, -1.0f * delta_time); }

	f32 temp_move_speed = 10.0f;
	vec3 velocity       = vec3_zero();

	if (input_is_key_down(KEY_COMMA)) {
		vec3 forward = mat4_forward(state->view);
		velocity     = vec3_add(velocity, forward);
	}
	if (input_is_key_down(KEY_O)) {
		vec3 backward = mat4_backward(state->view);
		velocity      = vec3_add(velocity, backward);
	}
	if (input_is_key_down(KEY_A)) {
		vec3 left = mat4_left(state->view);
		velocity  = vec3_add(velocity, left);
	}
	if (input_is_key_down(KEY_E)) {
		vec3 right = mat4_right(state->view);
		velocity   = vec3_add(velocity, right);
	}
	if (input_is_key_down(KEY_APOSTROPHE)) { velocity.y += 1.0f; }
	if (input_is_key_down(KEY_PERIOD)) { velocity.y += 1.0f; }

	vec3 z = vec3_zero();
	if (!vec3_compare(z, velocity, 0.0002f)) {
		vec3_normalize(&velocity);
		state->camera_position =
			vec3_add(state->camera_position, vec3_mul_scalar(vec3_mul_scalar(velocity, temp_move_speed), delta_time));
		state->camera_view_dirty = true;
	}

	recalculate_viem_matrix(state);

	renderer_set_view(state->view);

	return true;
}

b8 game_render(game *game_instance, f32 delta_time) {
	(void)game_instance;
	(void)delta_time;

	return true;
}

void game_on_resize(game *game_instance, u32 width, u32 height) {
	(void)game_instance;
	(void)width;
	(void)height;
}
