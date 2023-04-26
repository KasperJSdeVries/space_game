#include "vulkan_fence.h"

#include "core/logger.h"

void vulkan_fence_create(vulkan_context *context, b8 create_signaled,
                         vulkan_fence *out_fence) {
  out_fence->is_signaled = create_signaled;
  VkFenceCreateInfo fence_create_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = create_signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0,
  };

  VK_CHECK(vkCreateFence(context->device.logical_device, &fence_create_info,
                         context->allocator, &out_fence->handle));
}

void vulkan_fence_destroy(vulkan_context *context, vulkan_fence *fence) {
  if (fence->handle) {
    vkDestroyFence(context->device.logical_device, fence->handle,
                   context->allocator);
    fence->handle = 0;
  }
  fence->is_signaled = false;
}

b8 vulkan_fence_wait(vulkan_context *context, vulkan_fence *fence,
                     u64 timout_ns) {
  if (!fence->is_signaled) {
    VkResult result = vkWaitForFences(context->device.logical_device, 1,
                                      &fence->handle, true, timout_ns);
    switch (result) {
    case VK_SUCCESS:
      fence->is_signaled = true;
      return true;

    case VK_TIMEOUT:
      SPACE_WARN("vulkan_fence_wait - Timed out");
      break;

    case VK_ERROR_DEVICE_LOST:
      SPACE_ERROR("vulkan_fence_wait - VK_ERROR_DEVICE_LOST.");
      break;

    case VK_ERROR_OUT_OF_HOST_MEMORY:
      SPACE_ERROR("vulkan_fence_wait - VK_ERROR_OUT_OF_HOST_MEMORY.");
      break;

    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      SPACE_ERROR("vulkan_fence_wait - VK_ERROR_OUT_OF_DEVICE_MEMORY.");
      break;

    default:
      SPACE_ERROR("vulkan_fence_wait - An unknown error has occurred.");
      break;
    }

  } else {
    return true;
  }

  return false;
}

void vulkan_fence_reset(vulkan_context *context, vulkan_fence *fence) {
  if (fence->is_signaled) {
    VK_CHECK(vkResetFences(context->device.logical_device, 1, &fence->handle));
    fence->is_signaled = false;
  }
}
