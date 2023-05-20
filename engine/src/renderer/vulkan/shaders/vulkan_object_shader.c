#include "vulkan_object_shader.h"

#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_shader_module.h"

#include "core/logger.h"
#include "core/smemory.h"
#include "math/smath.h"

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
		VK_FORMAT_R32G32B32_SFLOAT, // Position
		VK_FORMAT_R32G32_SFLOAT,    // Texture coordinate
	};
	u64 sizes[] = {
		sizeof(vec3),
		sizeof(vec2),
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

	// Local/Object descriptors
	const u32 local_sampler_count                                            = 1;
	VkDescriptorType descriptor_types[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT] = {
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	};

	VkDescriptorSetLayoutBinding bindings[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
	szero_memory(&bindings, sizeof(VkDescriptorSetLayoutBinding) * VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT);
	for (u32 i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; ++i) {
		bindings[i] = (VkDescriptorSetLayoutBinding){
			.binding         = i,
			.descriptorCount = 1,
			.descriptorType  = descriptor_types[i],
			.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
		};
	}

	VkDescriptorSetLayoutCreateInfo layout_info = {
		.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT,
		.pBindings    = bindings,
	};
	VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical_device,
										 &layout_info,
										 context->allocator,
										 &out_shader->object_descriptor_set_layout));

	VkDescriptorPoolSize object_pool_sizes[] = {
		// for uniform buffers.
		{
			.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = VULKAN_OBJECT_MAX_OBJECT_COUNT,
		},
		// for image samplers.
		{
			.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = local_sampler_count * VULKAN_OBJECT_MAX_OBJECT_COUNT,
		},
	};
	u32 object_pool_size_count = sizeof(object_pool_sizes) / sizeof(VkDescriptorPoolSize);

	VkDescriptorPoolCreateInfo object_pool_info = {
		.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.poolSizeCount = object_pool_size_count,
		.pPoolSizes    = object_pool_sizes,
		.maxSets       = VULKAN_OBJECT_MAX_OBJECT_COUNT,
	};

	VK_CHECK(vkCreateDescriptorPool(context->device.logical_device,
									&object_pool_info,
									context->allocator,
									&out_shader->object_descriptor_pool));

	VkDescriptorSetLayout layouts[] = {out_shader->global_descriptor_set_layout,
									   out_shader->object_descriptor_set_layout};
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
							  sizeof(global_uniform_object) * DISPLAY_BUFFER_COUNT,
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
	u32 global_layout_count = sizeof(global_layouts) / sizeof(VkDescriptorSetLayout);

	VkDescriptorSetAllocateInfo alloc_info = {
		.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool     = out_shader->global_descriptor_pool,
		.descriptorSetCount = global_layout_count,
		.pSetLayouts        = global_layouts,
	};
	VK_CHECK(vkAllocateDescriptorSets(context->device.logical_device, &alloc_info, out_shader->global_descriptor_sets));

	if (!vulkan_buffer_create(context,
							  sizeof(object_uniform_object),
							  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
								  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							  true,
							  &out_shader->object_uniform_buffer)) {
		SERROR("Material instance buffer creation failed for shader.");
		return false;
	}

	return true;
}

void vulkan_object_shader_destroy(vulkan_context *context, vulkan_object_shader *shader) {
	VkDevice logical_device = context->device.logical_device;

	// Buffers.
	vulkan_buffer_destroy(context, &shader->global_uniform_buffer);
	vulkan_buffer_destroy(context, &shader->object_uniform_buffer);

	// Pipeline.
	vulkan_pipeline_destroy(context, &shader->pipeline);

	// Object descriptor.
	vkDestroyDescriptorPool(logical_device, shader->object_descriptor_pool, context->allocator);
	vkDestroyDescriptorSetLayout(logical_device, shader->object_descriptor_set_layout, context->allocator);

	// Global descriptor.
	vkDestroyDescriptorPool(logical_device, shader->global_descriptor_pool, context->allocator);
	vkDestroyDescriptorSetLayout(logical_device, shader->global_descriptor_set_layout, context->allocator);

	// Shaders.
	for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) { shader_module_destroy(context, i, shader->stages); }
}

void vulkan_object_shader_use(vulkan_context *context, vulkan_object_shader *shader) {
	u32 image_index = context->image_index;
	vulkan_pipeline_bind(&context->graphics_command_buffers[image_index],
						 VK_PIPELINE_BIND_POINT_GRAPHICS,
						 &shader->pipeline);
}

void vulkan_object_shader_update_global_state(vulkan_context *context,
											  struct vulkan_object_shader *shader,
											  f32 delta_time) {
	(void)delta_time;

	u32 image_index                   = context->image_index;
	VkCommandBuffer command_buffer    = context->graphics_command_buffers[image_index].handle;
	VkDescriptorSet global_descriptor = shader->global_descriptor_sets[image_index];

	vkCmdBindDescriptorSets(command_buffer,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							shader->pipeline.pipeline_layout,
							0,
							1,
							&global_descriptor,
							0,
							0);

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
}

void vulkan_object_shader_update_object(vulkan_context *context,
										struct vulkan_object_shader *shader,
										geometry_render_data data) {
	u32 image_index                = context->image_index;
	VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].handle;

	vkCmdPushConstants(command_buffer,
					   shader->pipeline.pipeline_layout,
					   VK_SHADER_STAGE_VERTEX_BIT,
					   0,
					   sizeof(mat4),
					   &data.model);

	vulkan_object_shader_object_state *object_state = &shader->object_states[data.object_id];
	VkDescriptorSet object_descriptor_set           = object_state->descriptor_sets[image_index];

	// TODO: check if this needs to be run.
	VkWriteDescriptorSet descriptor_writes[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
	szero_memory(descriptor_writes, sizeof(descriptor_writes));
	u32 descriptor_count = 0;
	u32 descriptor_index = 0;

	u32 range  = sizeof(object_uniform_object);
	u64 offset = sizeof(object_uniform_object) * data.object_id;
	object_uniform_object obo;

	// TODO: get diffuse colour from material.
	// Loops from black to white to black.
	static f32 accumulator = 0.0f;
	accumulator += context->frame_delta_time;
	f32 r             = (ssin(accumulator) + 1.0f) / 2.0f;
	f32 g             = (ssin(accumulator + 4.0f) + 1.0f) / 2.0f;
	f32 b             = (ssin(accumulator + 2.0f) + 1.0f) / 2.0f;
	obo.diffuse_color = vec4_create(r, g, b, 1.0f);

	vulkan_buffer_load_data(context, &shader->object_uniform_buffer, offset, range, 0, &obo);

	// Only do this if the descriptor has not yet been updated.
	if (object_state->descriptor_states[descriptor_index].generations[image_index] == INVALID_ID) {
		VkDescriptorBufferInfo buffer_info = {
			.buffer = shader->object_uniform_buffer.handle,
			.offset = offset,
			.range  = range,
		};

		VkWriteDescriptorSet descriptor = {
			.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet          = object_descriptor_set,
			.dstBinding      = descriptor_index,
			.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.pBufferInfo     = &buffer_info,
		};

		descriptor_writes[descriptor_count] = descriptor;
		descriptor_count++;

		object_state->descriptor_states[descriptor_index].generations[image_index] = 1;
	}
	descriptor_index++;

	// TODO: samplers
	const u32 sampler_count = 1;
	VkDescriptorImageInfo image_infos[sampler_count];
	for (u32 sampler_index = 0; sampler_index < sampler_count; ++sampler_index) {
		texture *t                 = data.textures[sampler_index];
		u32 *descriptor_generation = &object_state->descriptor_states[descriptor_index].generations[image_index];

		if (t && (*descriptor_generation != t->generation || *descriptor_generation == INVALID_ID)) {
			vulkan_texture_data *internal_data = t->internal_data;

			image_infos[sampler_index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_infos[sampler_index].imageView   = internal_data->image.view;
			image_infos[sampler_index].sampler     = internal_data->sampler;

			VkWriteDescriptorSet descriptor = {
				.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet          = object_descriptor_set,
				.dstBinding      = descriptor_index,
				.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.pImageInfo      = &image_infos[sampler_index],
			};

			descriptor_writes[descriptor_count] = descriptor;
			descriptor_count++;

			if (t->generation != INVALID_ID) { *descriptor_generation = t->generation; }

			descriptor_index++;
		}
	}

	if (descriptor_count > 0) {
		vkUpdateDescriptorSets(context->device.logical_device, descriptor_count, descriptor_writes, 0, 0);
	}

	vkCmdBindDescriptorSets(command_buffer,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							shader->pipeline.pipeline_layout,
							1,
							1,
							&object_descriptor_set,
							0,
							0);
}

b8 vulkan_object_shader_acquire_resources(vulkan_context *context, vulkan_object_shader *shader, u32 *out_object_id) {
	// TODO: free list
	*out_object_id = shader->object_uniform_buffer_index;
	shader->object_uniform_buffer_index++;

	u32 object_id                                   = *out_object_id;
	vulkan_object_shader_object_state *object_state = &shader->object_states[object_id];
	for (u32 i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; ++i) {
		for (u32 j = 0; j < DISPLAY_BUFFER_COUNT; ++j) {
			object_state->descriptor_states[i].generations[j] = INVALID_ID;
		}
	}

	VkDescriptorSetLayout layouts[] = {
		shader->object_descriptor_set_layout,
		shader->object_descriptor_set_layout,
		shader->object_descriptor_set_layout,
	};
	u32 layout_count = sizeof(layouts) / sizeof(VkDescriptorSetLayout);

	VkDescriptorSetAllocateInfo alloc_info = {
		.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool     = shader->object_descriptor_pool,
		.descriptorSetCount = layout_count,
		.pSetLayouts        = layouts,
	};

	VkResult result =
		vkAllocateDescriptorSets(context->device.logical_device, &alloc_info, object_state->descriptor_sets);
	if (result != VK_SUCCESS) {
		SERROR("Error allocating descriptor sets in shader!");
		return false;
	}

	return true;
}

void vulkan_object_shader_release_resources(vulkan_context *context, vulkan_object_shader *shader, u32 object_id) {
	vulkan_object_shader_object_state *object_state = &shader->object_states[object_id];

	VkResult result = vkFreeDescriptorSets(context->device.logical_device,
										   shader->object_descriptor_pool,
										   DISPLAY_BUFFER_COUNT,
										   object_state->descriptor_sets);
	if (result != VK_SUCCESS) { SERROR("Error freeing object shader descriptor sets!"); }

	for (u32 i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; ++i) {
		for (u32 j = 0; j < DISPLAY_BUFFER_COUNT; ++j) {
			object_state->descriptor_states[i].generations[j] = INVALID_ID;
		}
	}

	// TODO: add the object_id to the list of free object ids.
}
