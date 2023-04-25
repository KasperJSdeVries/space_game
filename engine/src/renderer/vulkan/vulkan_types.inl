#pragma once

#include "core/asserts.h"
#include "defines.h"

#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                         \
  { SPACE_ASSERT(expr == VK_SUCCESS); }

typedef struct vulkan_device {
  VkPhysicalDevice physical_device;
  VkDevice logical_device;
} vulkan_device;

typedef struct vulkan_context {
  VkInstance instance;
  VkAllocationCallbacks *allocator;

#if defined(_DEBUG)
  VkDebugUtilsMessengerEXT debug_messenger;
#endif

  VkSurfaceKHR surface;
  vulkan_device device;
} vulkan_context;
