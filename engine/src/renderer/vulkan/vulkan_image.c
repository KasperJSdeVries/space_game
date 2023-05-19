#include "vulkan_image.h"

#include "vulkan_device.h"

#include "core/logger.h"
#include <vulkan/vulkan_core.h>

void vulkan_image_create(vulkan_context *context,
						 VkImageType image_type,
						 u32 width,
						 u32 height,
						 VkFormat format,
						 VkImageTiling tiling,
						 VkImageUsageFlags usage,
						 VkMemoryPropertyFlags memory_flags,
						 b32 create_view,
						 VkImageAspectFlags view_aspect_flags,
						 vulkan_image *out_image) {
	(void)image_type;

	// Copy params
	out_image->width  = width;
	out_image->height = height;

	VkImageCreateInfo image_create_info = {
		.sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent =
			{
				.width  = width,
				.height = height,
				.depth  = 1, // TODO: Make configurable
			},
		.mipLevels     = 4, // TODO: Support mip-mapping
		.arrayLayers   = 1, // TODO: Support image layers
		.format        = format,
		.tiling        = tiling,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage         = usage,
		.samples       = VK_SAMPLE_COUNT_1_BIT,     // TODO: Make configurable
		.sharingMode   = VK_SHARING_MODE_EXCLUSIVE, // TODO: Make configurable
	};

	VK_CHECK(vkCreateImage(context->device.logical_device, &image_create_info, context->allocator, &out_image->handle));

	// Query memory requirements.
	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(context->device.logical_device, out_image->handle, &memory_requirements);

	i32 memory_type = context->find_memory_index(memory_requirements.memoryTypeBits, memory_flags);
	if (memory_type == -1) { SERROR("Required memory type not found. Image not valid."); }

	// Allocate memory
	VkMemoryAllocateInfo memory_allocate_info = {
		.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize  = memory_requirements.size,
		.memoryTypeIndex = (u32)memory_type,
	};
	VK_CHECK(vkAllocateMemory(context->device.logical_device,
							  &memory_allocate_info,
							  context->allocator,
							  &out_image->memory));

	// Bind the memory
	VK_CHECK(vkBindImageMemory(context->device.logical_device,
							   out_image->handle,
							   out_image->memory,
							   0)); // TODO: configurable memory offset.

	// Create the view.
	if (create_view) {
		out_image->view = 0;
		vulkan_image_view_create(context, format, out_image, view_aspect_flags);
	}
}

void vulkan_image_view_create(vulkan_context *context,
							  VkFormat format,
							  vulkan_image *image,
							  VkImageAspectFlags aspect_flags) {
	VkImageViewCreateInfo view_create_info = {
		.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image    = image->handle,
		.viewType = VK_IMAGE_VIEW_TYPE_2D, // TODO: Make configurable.
		.format   = format,
		.subresourceRange =
			{
				.aspectMask = aspect_flags,

				// TODO: Make configurable.
				.baseMipLevel   = 0,
				.levelCount     = 1,
				.baseArrayLayer = 0,
				.layerCount     = 1,
			},
	};

	VK_CHECK(vkCreateImageView(context->device.logical_device, &view_create_info, context->allocator, &image->view));
}

void vulkan_image_transition_layout(vulkan_context *context,
									vulkan_command_buffer *command_buffer,
									vulkan_image *image,
									VkFormat format,
									VkImageLayout old_layout,
									VkImageLayout new_layout) {
	(void)format;

	VkImageMemoryBarrier barrier = {
		.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout           = old_layout,
		.newLayout           = new_layout,
		.srcQueueFamilyIndex = context->device.graphics_queue_index,
		.dstQueueFamilyIndex = context->device.graphics_queue_index,
		.image               = image->handle,
		.subresourceRange =
			{
				.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel   = 0,
				.levelCount     = 1,
				.baseArrayLayer = 0,
				.layerCount     = 1,
			},
	};

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags dest_stage;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			   && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		SFATAL("Unsupported layout transition!");
		return;
	}

	vkCmdPipelineBarrier(command_buffer->handle,
						 source_stage,
						 dest_stage,
						 0,
						 0,
						 VK_NULL_HANDLE,
						 0,
						 VK_NULL_HANDLE,
						 1,
						 &barrier);
}

void vulkan_image_copy_from_buffer(vulkan_context *context,
								   vulkan_image *image,
								   VkBuffer buffer,
								   vulkan_command_buffer *command_buffer) {
	(void)context;

	VkBufferImageCopy region = {
		.bufferOffset      = 0,
		.bufferRowLength   = 0,
		.bufferImageHeight = 0,

		.imageSubresource =
			{
				.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel       = 0,
				.baseArrayLayer = 0,
				.layerCount     = 1,
			},

		.imageExtent =
			{
				.width  = image->width,
				.height = image->height,
				.depth  = 1,
			},
	};

	vkCmdCopyBufferToImage(command_buffer->handle,
						   buffer,
						   image->handle,
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   1,
						   &region);
}

void vulkan_image_destroy(vulkan_context *context, vulkan_image *image) {
	if (image->view) {
		vkDestroyImageView(context->device.logical_device, image->view, context->allocator);
		image->view = 0;
	}
	if (image->memory) {
		vkFreeMemory(context->device.logical_device, image->memory, context->allocator);
		image->memory = 0;
	}
	if (image->handle) {
		vkDestroyImage(context->device.logical_device, image->handle, context->allocator);
		image->handle = 0;
	}
}
