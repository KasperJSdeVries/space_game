#pragma once

#include "core/asserts.h"
#include "defines.h"

#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                         \
  { SPACE_ASSERT(expr == VK_SUCCESS); }

typedef struct vulkan_swapchain_support_info {
  VkSurfaceCapabilitiesKHR capabilities;
  u32 format_count;
  VkSurfaceFormatKHR *formats;
  u32 present_mode_count;
  VkPresentModeKHR *present_modes;
} vulkan_swapchain_support_info;

typedef struct vulkan_device {
  VkPhysicalDevice physical_device;
  VkDevice logical_device;

  vulkan_swapchain_support_info swapchain_info;

  u32 graphics_queue_index;
  u32 present_queue_index;
  u32 transfer_queue_index;
  u32 compute_queue_index;

  VkQueue graphics_queue;
  VkQueue present_queue;
  VkQueue transfer_queue;
  VkQueue compute_queue;

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceMemoryProperties memory;
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
