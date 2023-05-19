#pragma once

#include "renderer/renderer_backend.h"

b8 vulkan_renderer_backend_initialize(renderer_backend *backend, const char *application_name);
void vulkan_renderer_backend_shutdown(renderer_backend *backend);

void vulkan_renderer_backend_on_resize(renderer_backend *backend, u16 width, u16 height);

b8 vulkan_renderer_backend_begin_frame(renderer_backend *backend, f32 delta_time);
void vulkan_renderer_update_global_state(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_colour, i32 mode);
b8 vulkan_renderer_backend_end_frame(renderer_backend *backend, f32 delta_time);
void vulkan_renderer_update_object(mat4 model);

void vulkan_renderer_create_texture(const char *name,
									b8 auto_release,
									u32 width,
									u32 height,
									i32 channel_count,
									const u8 *pixels,
									b8 has_transparency,
									texture *out_texture);
void vulkan_renderer_destroy_texture(texture *texture);
