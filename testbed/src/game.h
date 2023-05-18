#pragma once

#include <defines.h>
#include <game_types.h>

#include <math/smath.h>

typedef struct game_state {
	f32 delta_time;
	mat4 view;
	vec3 camera_position;
	vec3 camera_euler;
	b8 camera_view_dirty;
} game_state;

b8 game_initialize(game *game_instance);

b8 game_update(game *game_instance, f32 delta_time);

b8 game_render(game *game_instance, f32 delta_time);

void game_on_resize(game *game_instance, u32 width, u32 height);
