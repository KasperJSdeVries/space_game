#pragma once

#include "defines.h"
#include "math/math_types.inl"

typedef enum renderer_backend_type {
	RENDERER_BACKEND_TYPE_VULKAN,
	RENDERER_BACKEND_TYPE_OPENGL,
	RENDERER_BACKEND_TYPE_DIRECTX,
} renderer_backend_type;

typedef struct global_uniform_object {
	mat4 projection;
	mat4 view;
	mat4 m_reserved0;
	mat4 m_reserved1;
} global_uniform_object;

typedef struct renderer_backend {
	struct platform_state *platform_state;
	u64 frame_number;

	b8 (*initialize)(struct renderer_backend *backend,
					 const char *application_name,
					 struct platform_state *platform_state);

	void (*shutdown)(struct renderer_backend *backend);

	void (*resize)(struct renderer_backend *backend, u16 width, u16 height);

	b8 (*begin_frame)(struct renderer_backend *backend, f32 delta_time);
	void (*update_global_state)(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_colour, i32 mode);
	b8 (*end_frame)(struct renderer_backend *backend, f32 delta_time);
} renderer_backend;

typedef struct render_packet {
	f32 delta_time;
} render_packet;
