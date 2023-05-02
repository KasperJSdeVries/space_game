#include "vulkan_object_shader.h"

#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_shader_module.h"

#include "core/logger.h"
#include "core/smemory.h"

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"

b8 vulkan_object_shader_create(vulkan_context *context, vulkan_object_shader *out_shader) {
	char stage_type_strings[OBJECT_SHADER_STAGE_COUNT][5]        = {"vert", "frag"};
	VkShaderStageFlagBits stage_types[OBJECT_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT,
																	VK_SHADER_STAGE_FRAGMENT_BIT};

	for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
		if (!shader_module_create(context,
								  BUILTIN_SHADER_NAME_OBJECT,
								  stage_type_strings[i],
								  stage_types[i],
								  i,
								  out_shader->stages)) {
			SERROR("Unable to create %s shader module for '%s'.", stage_type_strings[i], BUILTIN_SHADER_NAME_OBJECT);
			return false;
		}
	}

	// Pipeline creation
	VkViewport viewport = {
		.x        = 0.0f,
		.y        = (f32)context->framebuffer_height,
		.width    = (f32)context->framebuffer_width,
		.height   = -(f32)context->framebuffer_height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	VkRect2D scissor = {
		.offset =
			{
				.x = 0,
				.y = 0,
			},
		.extent =
			{
				.width  = context->framebuffer_width,
				.height = context->framebuffer_height,
			},
	};

	VkFormat formats[] = {
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32_SFLOAT,
	};

	u64 sizes[] = {
		sizeof(vec3),
		sizeof(vec3),
	};

	const u32 attribute_count = sizeof(formats) / sizeof(VkFormat);

	u32 offset = 0;
	VkVertexInputAttributeDescription attribute_descriptions[attribute_count];

	for (u32 i = 0; i < attribute_count; ++i) {
		attribute_descriptions[i] = (VkVertexInputAttributeDescription){
			.binding  = 0,
			.location = i,
			.format   = formats[i],
			.offset   = offset,
		};
		offset += (u32)sizes[i];
	}

	// Stages
	// NOTE: Should match the number of shader->stages.
	VkPipelineShaderStageCreateInfo stage_create_infos[OBJECT_SHADER_STAGE_COUNT];
	szero_memory(stage_create_infos, sizeof(stage_create_infos));
	for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
		stage_create_infos[i] = out_shader->stages[i].shader_stage_create_info;
	}

	// TODO: Descriptors

	if (!vulkan_graphics_pipeline_create(context,
										 &context->main_render_pass,
										 attribute_count,
										 attribute_descriptions,
										 0,
										 0,
										 OBJECT_SHADER_STAGE_COUNT,
										 stage_create_infos,
										 viewport,
										 scissor,
										 false,
										 &out_shader->pipeline)) {
		SERROR("Failed to load graphics pipeline for object shader.");
		return false;
	}

	return true;
}

void vulkan_object_shader_destroy(vulkan_context *context, vulkan_object_shader *shader) {
	vulkan_pipeline_destroy(context, &shader->pipeline);

	for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) { shader_module_destroy(context, i, shader->stages); }
}

void vulkan_object_shader_use(vulkan_context *context, vulkan_object_shader *shader) {
	u32 image_index = context->image_index;
	vulkan_pipeline_bind(&context->graphics_command_buffers[image_index],
						 VK_PIPELINE_BIND_POINT_GRAPHICS,
						 &shader->pipeline);
}
