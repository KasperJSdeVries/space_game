#include "vulkan_object_shader.h"

#include "renderer/vulkan/vulkan_shader_module.h"

#include "core/logger.h"

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"

b8 vulkan_object_shader_create(vulkan_context *context,
                               vulkan_object_shader *out_shader) {
  char stage_type_strings[OBJECT_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
  VkShaderStageFlagBits stage_types[OBJECT_SHADER_STAGE_COUNT] = {
      VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

  for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
    if (!shader_module_create(context, BUILTIN_SHADER_NAME_OBJECT,
                              stage_type_strings[i], stage_types[i], i,
                              out_shader->stages)) {
      SERROR("Unable to create %s shader module for '%s'.",
             stage_type_strings[i], BUILTIN_SHADER_NAME_OBJECT);
      return false;
    }
  }

  // TODO: Descriptors

  return true;
}

void vulkan_object_shader_destroy(vulkan_context *context,
                                  vulkan_object_shader *shader) {
  for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
    shader_module_destroy(context, i, shader->stages);
  }
}

void vulkan_object_shader_use(vulkan_context *context,
                              vulkan_object_shader *shader) {
  (void)context;
  (void)shader;
}
