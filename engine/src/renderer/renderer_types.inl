#pragma once

#include "defines.h"
#include "math/math_types.inl"
#include "resources/resource_types.h"

#define TEXTURES_PER_GEOMETRY 16

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

typedef struct object_uniform_object {
	vec4 diffuse_color;
	vec4 m_reserved0;
	vec4 m_reserved1;
	vec4 m_reserved2;
} object_uniform_object;

typedef struct geometry_render_data {
	u32 object_id;
	mat4 model;
	texture *textures[TEXTURES_PER_GEOMETRY];
} geometry_render_data;

typedef struct renderer_backend {
	u64 frame_number;

	b8 (*initialize)(struct renderer_backend *backend, const char *application_name);

	void (*shutdown)(struct renderer_backend *backend);

	void (*resize)(struct renderer_backend *backend, u16 width, u16 height);

	b8 (*begin_frame)(struct renderer_backend *backend, f32 delta_time);
	void (*update_global_state)(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_colour, i32 mode);
	b8 (*end_frame)(struct renderer_backend *backend, f32 delta_time);

	void (*update_object)(geometry_render_data data);

	void (*create_texture)(const char *name,
						   b8 auto_release,
						   u32 width,
						   u32 height,
						   i32 channel_count,
						   const u8 *pixels,
						   b8 has_transparency,
						   texture *out_texture);
	void (*destroy_texture)(texture *texture);
} renderer_backend;

typedef struct render_packet {
	f32 delta_time;
} render_packet;
