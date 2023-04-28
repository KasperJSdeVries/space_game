#include "vulkan_image.h"

#include "vulkan_device.h"

#include "core/logger.h"

void vulkan_image_create(vulkan_context *context, VkImageType image_type,
                         u32 width, u32 height, VkFormat format,
                         VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags memory_flags, b32 create_view,
                         VkImageAspectFlags view_aspect_flags,
                         vulkan_image *out_image) {
  (void)image_type;

  // Copy params
  out_image->width = width;
  out_image->height = height;

  VkImageCreateInfo image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .extent =
          {
              .width = width,
              .height = height,
              .depth = 1, // TODO: Make configurable
          },
      .mipLevels = 4,   // TODO: Support mip-mapping
      .arrayLayers = 1, // TODO: Support image layers
      .format = format,
      .tiling = tiling,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .usage = usage,
      .samples = VK_SAMPLE_COUNT_1_BIT,         // TODO: Make configurable
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // TODO: Make configurable
  };

  VK_CHECK(vkCreateImage(context->device.logical_device, &image_create_info,
                         context->allocator, &out_image->handle));

  // Query memory requirements.
  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(context->device.logical_device,
                               out_image->handle, &memory_requirements);

  i32 memory_type = context->find_memory_index(
      memory_requirements.memoryTypeBits, memory_flags);
  if (memory_type == -1) {
    SERROR("Required memory type not found. Image not valid.");
  }

  // Allocate memory
  VkMemoryAllocateInfo memory_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = (u32)memory_type,
  };
  VK_CHECK(vkAllocateMemory(context->device.logical_device,
                            &memory_allocate_info, context->allocator,
                            &out_image->memory));

  // Bind the memory
  VK_CHECK(vkBindImageMemory(context->device.logical_device, out_image->handle,
                             out_image->memory,
                             0)); // TODO: configurable memory offset.

  // Create the view.
  if (create_view) {
    out_image->view = 0;
    vulkan_image_view_create(context, format, out_image, view_aspect_flags);
  }
}

void vulkan_image_view_create(vulkan_context *context, VkFormat format,
                              vulkan_image *image,
                              VkImageAspectFlags aspect_flags) {
  VkImageViewCreateInfo view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image->handle,
      .viewType = VK_IMAGE_VIEW_TYPE_2D, // TODO: Make configurable.
      .format = format,
      .subresourceRange =
          {
              .aspectMask = aspect_flags,

              // TODO: Make configurable.
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

  VK_CHECK(vkCreateImageView(context->device.logical_device, &view_create_info,
                             context->allocator, &image->view));
}

void vulkan_image_destroy(vulkan_context *context, vulkan_image *image) {
  if (image->view) {
    vkDestroyImageView(context->device.logical_device, image->view,
                       context->allocator);
    image->view = 0;
  }
  if (image->memory) {
    vkFreeMemory(context->device.logical_device, image->memory,
                 context->allocator);
    image->memory = 0;
  }
  if (image->handle) {
    vkDestroyImage(context->device.logical_device, image->handle,
                   context->allocator);
    image->handle = 0;
  }
}
