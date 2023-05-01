#pragma once

#include "vulkan_types.inl"

b8 shader_module_create(vulkan_context *context, const char *name,
                        const char *type_str,
                        VkShaderStageFlagBits shader_stage_flags,
                        u32 stage_index, vulkan_shader_stage *shader_stages);

void shader_module_destroy(vulkan_context *context, u32 stage_index,
                           vulkan_shader_stage *shader_stages);
