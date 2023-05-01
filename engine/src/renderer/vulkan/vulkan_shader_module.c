#include "vulkan_shader_module.h"

#include "core/filesystem.h"
#include "core/logger.h"
#include "core/smemory.h"
#include "core/sstring.h"

b8 shader_module_create(vulkan_context *context, char const *name,
                        char const *type_str,
                        VkShaderStageFlagBits shader_stage_flags,
                        u32 stage_index, vulkan_shader_stage *shader_stages) {
  char file_name[512];
  string_format(file_name, "assets/shaders/%s.%s.spv", name, type_str);

  szero_memory(&shader_stages[stage_index].create_info,
               sizeof(VkShaderModuleCreateInfo));
  shader_stages[stage_index].create_info.sType =
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

  file_handle handle;
  if (!filesystem_open(file_name, FILE_MODE_READ, true, &handle)) {
    SERROR("Unable to open file: %s.", file_name);
    return false;
  }

  u64 size = 0;
  u8 *file_buffer = 0;
  if (!filesystem_read_all_bytes(&handle, &file_buffer, &size)) {
    SERROR("Unable to read shader module: %s.", file_name);
    return false;
  }

  shader_stages[stage_index].create_info.codeSize = size;
  shader_stages[stage_index].create_info.pCode = (u32 *)file_buffer;

  filesystem_close(&handle);

  VK_CHECK(vkCreateShaderModule(
      context->device.logical_device, &shader_stages[stage_index].create_info,
      context->allocator, &shader_stages[stage_index].handle));

  szero_memory(&shader_stages[stage_index].shader_stage_create_info,
               sizeof(VkPipelineShaderStageCreateInfo));
  shader_stages[stage_index].shader_stage_create_info =
      (VkPipelineShaderStageCreateInfo){
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = shader_stage_flags,
          .module = shader_stages[stage_index].handle,
          .pName = "main",
      };

  if (file_buffer) {
    sfree(file_buffer, sizeof(u8) * size, MEMORY_TAG_STRING);
    file_buffer = 0;
  }

  return true;
}

void shader_module_destroy(vulkan_context *context, u32 stage_index,
                           vulkan_shader_stage *shader_stages) {
  vkDestroyShaderModule(context->device.logical_device,
                        shader_stages[stage_index].handle, context->allocator);
}
