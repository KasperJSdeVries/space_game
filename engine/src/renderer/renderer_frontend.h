#pragma once

#include "renderer_types.inl"

struct static_mesh_data;
struct platform_state;

b8 render_system_initialize(u64 *memory_requirement, void *state, const char *application_name);
void renderer_system_shutdown(void *state);

void renderer_on_resize(u16 width, u16 height);

b8 renderer_draw_frame(render_packet *packet);

void renderer_set_view(mat4 view);

void renderer_create_texture(const char *name,
							 b8 auto_release,
							 u32 width,
							 u32 height,
							 i32 channel_count,
							 const u8 *pixels,
							 b8 has_transparency,
							 texture *out_texture);
void renderer_destroy_texture(texture *texture);
