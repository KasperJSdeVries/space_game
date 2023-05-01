#pragma once

#include "vulkan_types.inl"

void vulkan_render_pass_create(vulkan_context *context,
							   vulkan_render_pass *out_render_pass,
							   vec2 position,
							   vec2 dimensions,
							   vec4 colour,
							   f32 depth,
							   u32 stencil);

void vulkan_render_pass_destroy(vulkan_context *context, vulkan_render_pass *render_pass);

void vulkan_render_pass_begin(vulkan_command_buffer *command_buffer,
							  vulkan_render_pass *renderpass,
							  VkFramebuffer frame_buffer);

void vulkan_render_pass_end(vulkan_command_buffer *command_buffer, vulkan_render_pass *renderpass);
