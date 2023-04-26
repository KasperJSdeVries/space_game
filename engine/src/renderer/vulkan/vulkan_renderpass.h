#pragma once

#include "vulkan_types.inl"

void vulkan_render_pass_create(vulkan_context *context,
                               vulkan_render_pass *out_renderpass, f32 x, f32 y,
                               f32 w, f32 h, f32 r, f32 g, f32 b, f32 a,
                               f32 depth, u32 stencil);

void vulkan_render_pass_destroy(vulkan_context *context,
                                vulkan_render_pass *renderpass);

void vulkan_render_pass_begin(vulkan_command_buffer *command_buffer,
                              vulkan_render_pass *renderpass,
                              VkFramebuffer frame_buffer);

void vulkan_render_pass_end(vulkan_command_buffer *command_buffer,
                            vulkan_render_pass *renderpass);
