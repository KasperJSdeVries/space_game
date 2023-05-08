#include "vulkan_object_shader.h"

#include "renderer/vulkan/vulkan_buffer.h"
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

	// Global Descriptors
	VkDescriptorSetLayoutBinding global_ubo_layout_binding = {
		.binding            = 0,
		.descriptorCount    = 1,
		.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pImmutableSamplers = 0,
		.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
	};

	VkDescriptorSetLayoutCreateInfo global_layout_info = {
		.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings    = &global_ubo_layout_binding,
	};
	VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical_device,
										 &global_layout_info,
										 context->allocator,
										 &out_shader->global_descriptor_set_layout));

	VkDescriptorPoolSize global_pool_size = {
		.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = context->swapchain.image_count,
	};

	VkDescriptorPoolCreateInfo global_pool_info = {
		.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.poolSizeCount = 1,
		.pPoolSizes    = &global_pool_size,
		.maxSets       = context->swapchain.image_count,
	};
	VK_CHECK(vkCreateDescriptorPool(context->device.logical_device,
									&global_pool_info,
									context->allocator,
									&out_shader->global_descriptor_pool));

	VkDescriptorSetLayout layouts[] = {out_shader->global_descriptor_set_layout};
	u32 descriptor_set_layout_count = sizeof(layouts) / sizeof(VkDescriptorSetLayout);

	if (!vulkan_graphics_pipeline_create(context,
										 &context->main_render_pass,
										 attribute_count,
										 attribute_descriptions,
										 descriptor_set_layout_count,
										 layouts,
										 OBJECT_SHADER_STAGE_COUNT,
										 stage_create_infos,
										 viewport,
										 scissor,
										 false,
										 &out_shader->pipeline)) {
		SERROR("Failed to load graphics pipeline for object shader.");
		return false;
	}

	if (!vulkan_buffer_create(context,
							  sizeof(global_uniform_object) * 3,
							  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
								  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							  true,
							  &out_shader->global_uniform_buffer)) {
		SERROR("Vulkan buffer creation failed for object shader");
		return false;
	}

	VkDescriptorSetLayout global_layouts[] = {
		out_shader->global_descriptor_set_layout,
		out_shader->global_descriptor_set_layout,
		out_shader->global_descriptor_set_layout,
	};

	VkDescriptorSetAllocateInfo alloc_info = {
		.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool     = out_shader->global_descriptor_pool,
		.descriptorSetCount = 3,
		.pSetLayouts        = global_layouts,
	};
	VK_CHECK(vkAllocateDescriptorSets(context->device.logical_device, &alloc_info, out_shader->global_descriptor_sets));

	return true;
}

void vulkan_object_shader_destroy(vulkan_context *context, vulkan_object_shader *shader) {
	VkDevice logical_device = context->device.logical_device;

	vulkan_buffer_destroy(context, &shader->global_uniform_buffer);

	vulkan_pipeline_destroy(context, &shader->pipeline);

	vkDestroyDescriptorPool(logical_device, shader->global_descriptor_pool, context->allocator);

	vkDestroyDescriptorSetLayout(logical_device, shader->global_descriptor_set_layout, context->allocator);

	for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) { shader_module_destroy(context, i, shader->stages); }
}

void vulkan_object_shader_use(vulkan_context *context, vulkan_object_shader *shader) {
	u32 image_index = context->image_index;
	vulkan_pipeline_bind(&context->graphics_command_buffers[image_index],
						 VK_PIPELINE_BIND_POINT_GRAPHICS,
						 &shader->pipeline);
}

void vulkan_object_shader_update_global_state(vulkan_context *context, struct vulkan_object_shader *shader) {
	u32 image_index                   = context->image_index;
	VkCommandBuffer command_buffer    = context->graphics_command_buffers[image_index].handle;
	VkDescriptorSet global_descriptor = shader->global_descriptor_sets[image_index];

	if (!shader->descriptor_updated[image_index]) {
		u32 range  = sizeof(global_uniform_object);
		u64 offset = sizeof(global_uniform_object) * image_index;

		vulkan_buffer_load_data(context, &shader->global_uniform_buffer, offset, range, 0, &shader->global_ubo);

		VkDescriptorBufferInfo buffer_info = {
			.buffer = shader->global_uniform_buffer.handle,
			.offset = offset,
			.range  = range,
		};

		VkWriteDescriptorSet descriptor_write = {
			.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet          = shader->global_descriptor_sets[image_index],
			.dstBinding      = 0,
			.dstArrayElement = 0,
			.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.pBufferInfo     = &buffer_info,
		};

		vkUpdateDescriptorSets(context->device.logical_device, 1, &descriptor_write, 0, 0);
		shader->descriptor_updated[image_index] = true;
	}

	vkCmdBindDescriptorSets(command_buffer,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							shader->pipeline.pipeline_layout,
							0,
							1,
							&global_descriptor,
							0,
							0);
}

void vulkan_object_shader_update_object(vulkan_context *context, struct vulkan_object_shader *shader, mat4 model) {
	u32 image_index                = context->image_index;
	VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;

	vkCmdPushConstants(command_buffer,
					   shader->pipeline.pipeline_layout,
					   VK_SHADER_STAGE_VERTEX_BIT,
					   0,
					   sizeof(mat4),
					   &model);
}
