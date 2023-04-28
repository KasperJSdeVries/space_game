#include "vulkan_swapchain.h"

#include "vulkan_device.h"
#include "vulkan_image.h"

#include "core/logger.h"
#include "core/smemory.h"

#include <stdint.h>

void create(vulkan_context *context, u32 width, u32 height,
            b8 use_old_swapchain, vulkan_swapchain *swapchain);
void destroy(vulkan_context *context, vulkan_swapchain *swapchain);

void vulkan_swapchain_create(vulkan_context *context, u32 width, u32 height,
                             vulkan_swapchain *out_swapchain) {
  create(context, width, height, false, out_swapchain);
}

void vulkan_swapchain_recreate(vulkan_context *context, u32 width, u32 height,
                               vulkan_swapchain *out_swapchain) {
  destroy(context, out_swapchain);
  create(context, width, height, false, out_swapchain);
}

void vulkan_swapchain_destroy(vulkan_context *context,
                              vulkan_swapchain *swapchain) {
  destroy(context, swapchain);
}

b8 vulkan_swapchain_acquire_next_image_index(
    vulkan_context *context, vulkan_swapchain *swapchain, u64 timeout_ns,
    VkSemaphore image_available_semaphore, VkFence fence,
    u32 *out_image_index) {
  VkResult result = vkAcquireNextImageKHR(
      context->device.logical_device, swapchain->handle, timeout_ns,
      image_available_semaphore, fence, out_image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    vulkan_swapchain_recreate(context, context->framebuffer_width,
                              context->framebuffer_height, swapchain);
    return false;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    SFATAL("Failed to acquire swapchain image!");
    return false;
  }

  return true;
}

void vulkan_swapchain_present(vulkan_context *context,
                              vulkan_swapchain *swapchain,
                              VkQueue graphics_queue, VkQueue present_queue,
                              VkSemaphore render_complete_semaphore,
                              u32 present_image_index) {

  (void)graphics_queue;

  // Return the image to the swapchain for presentation.
  VkPresentInfoKHR present_info = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &render_complete_semaphore;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain->handle;
  present_info.pImageIndices = &present_image_index;
  present_info.pResults = 0;
  VkResult result = vkQueuePresentKHR(present_queue, &present_info);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    // Swapchain is out of date, suboptimal or a framebuffer resize has
    // occurred. Trigger swapchain recreation.
    vulkan_swapchain_recreate(context, context->framebuffer_width,
                              context->framebuffer_height, swapchain);
  } else if (result != VK_SUCCESS) {
    SFATAL("Failed to present swap chain image!");
  }
  // Increment (and loop) the index.
  context->current_frame =
      (context->current_frame + 1) % swapchain->max_frames_in_flight;
}

void create(vulkan_context *context, u32 width, u32 height,
            b8 use_old_swapchain, vulkan_swapchain *swapchain) {
  VkExtent2D swapchain_extent = {width, height};
  swapchain->max_frames_in_flight = 2;

  // Choose a swap surface format.
  b8 found = false;
  for (u32 i = 0; i < context->device.swapchain_support.format_count; ++i) {
    VkSurfaceFormatKHR format = context->device.swapchain_support.formats[i];
    // Preferred formats
    if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      swapchain->image_format = format;
      found = true;
      break;
    }
  }

  if (!found) {
    swapchain->image_format = context->device.swapchain_support.formats[0];
  }

  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  for (u32 i = 0; i < context->device.swapchain_support.present_mode_count;
       ++i) {
    VkPresentModeKHR mode = context->device.swapchain_support.present_modes[i];
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      present_mode = mode;
      break;
    }
  }

  // Requery swapchain support.
  vulkan_device_query_swapchain_support(context->device.physical_device,
                                        context->surface,
                                        &context->device.swapchain_support);

  // Swapchain extent
  if (context->device.swapchain_support.capabilities.currentExtent.width !=
      UINT32_MAX) {
    swapchain_extent =
        context->device.swapchain_support.capabilities.currentExtent;
  }

  // Clamp to the value allowed by the GPU.
  VkExtent2D min =
      context->device.swapchain_support.capabilities.minImageExtent;
  VkExtent2D max =
      context->device.swapchain_support.capabilities.maxImageExtent;
  swapchain_extent.width = SCLAMP(swapchain_extent.width, min.width, max.width);
  swapchain_extent.height =
      SCLAMP(swapchain_extent.height, min.height, max.height);

  u32 image_count =
      context->device.swapchain_support.capabilities.minImageCount + 1;
  if (context->device.swapchain_support.capabilities.maxImageCount > 0 &&
      image_count >
          context->device.swapchain_support.capabilities.maxImageCount) {
    image_count = context->device.swapchain_support.capabilities.maxImageCount;
  }

  // Swapchain create info
  VkSwapchainCreateInfoKHR swapchain_create_info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = context->surface,
      .minImageCount = image_count,
      .imageFormat = swapchain->image_format.format,
      .imageColorSpace = swapchain->image_format.colorSpace,
      .imageExtent = swapchain_extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .preTransform =
          context->device.swapchain_support.capabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = present_mode,
      .clipped = VK_TRUE,
  };

  if (context->device.graphics_queue_index !=
      context->device.present_queue_index) {
    u32 queue_familiy_indices[] = {context->device.graphics_queue_index,
                                   context->device.present_queue_index};
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_create_info.queueFamilyIndexCount = 2;
    swapchain_create_info.pQueueFamilyIndices = queue_familiy_indices;
  } else {
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = 0;
  }

  if (use_old_swapchain) {
    swapchain_create_info.oldSwapchain = swapchain->handle;
  } else {
    swapchain_create_info.oldSwapchain = 0;
  }

  VK_CHECK(vkCreateSwapchainKHR(context->device.logical_device,
                                &swapchain_create_info, context->allocator,
                                &swapchain->handle));

  // Start with a zero frame index
  context->current_frame = 0;

  // Images
  swapchain->image_count = 0;
  VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical_device,
                                   swapchain->handle, &swapchain->image_count,
                                   0));
  if (!swapchain->images) {
    swapchain->images = (VkImage *)sallocate(
        sizeof(VkImage) * swapchain->image_count, MEMORY_TAG_RENDERER);
  }
  if (!swapchain->views) {
    swapchain->views = (VkImageView *)sallocate(
        sizeof(VkImageView) * swapchain->image_count, MEMORY_TAG_RENDERER);
  }
  VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical_device,
                                   swapchain->handle, &swapchain->image_count,
                                   swapchain->images));

  // Views
  for (u32 i = 0; i < swapchain->image_count; ++i) {
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = swapchain->images[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchain->image_format.format,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    VK_CHECK(vkCreateImageView(context->device.logical_device, &view_info,
                               context->allocator, &swapchain->views[i]));
  }

  // Depth resources
  if (!vulkan_device_detect_depth_format(&context->device)) {
    context->device.depth_format = VK_FORMAT_UNDEFINED;
    SFATAL("Failed to find a supported format!");
  }

  // Create depth image and its view.
  vulkan_image_create(context, VK_IMAGE_TYPE_2D, swapchain_extent.width,
                      swapchain_extent.height, context->device.depth_format,
                      VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true,
                      VK_IMAGE_ASPECT_DEPTH_BIT, &swapchain->depth_attachment);

  SINFO("Swapchain created successfully.");
}

void destroy(vulkan_context *context, vulkan_swapchain *swapchain) {
  vkDeviceWaitIdle(context->device.logical_device);

  vulkan_image_destroy(context, &swapchain->depth_attachment);

  for (u32 i = 0; i < swapchain->image_count; ++i) {
    vkDestroyImageView(context->device.logical_device, swapchain->views[i],
                       context->allocator);
  }

  vkDestroySwapchainKHR(context->device.logical_device, swapchain->handle,
                        context->allocator);
}
