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

  vulkan_swapchain_support_info swapchain_support;

  u32 graphics_queue_index;
  u32 present_queue_index;
  u32 transfer_queue_index;
  u32 compute_queue_index;

  VkQueue graphics_queue;
  VkQueue present_queue;
  VkQueue transfer_queue;
  VkQueue compute_queue;

  VkCommandPool graphics_command_pool;

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceMemoryProperties memory;

  VkFormat depth_format;
} vulkan_device;

typedef struct vulkan_image {
  VkImage handle;
  VkDeviceMemory memory;
  VkImageView view;
  u32 width;
  u32 height;
} vulkan_image;

typedef enum vulkan_render_pass_state {
  RENDER_PASS_STATE_READY,
  RENDER_PASS_STATE_RECORDING,
  RENDER_PASS_STATE_IN_RENDER_PASS,
  RENDER_PASS_STATE_RECORDING_ENDED,
  RENDER_PASS_STATE_SUBMITTED,
  RENDER_PASS_STATE_NOT_ALLOCATED,
} vulkan_render_pass_state;

typedef struct vulkan_render_pass {
  VkRenderPass handle;
  f32 x, y, w, h;
  f32 r, g, b, a;

  f32 depth;
  u32 stencil;

  vulkan_render_pass_state state;
} vulkan_render_pass;

typedef struct vulkan_swapchain {
  VkSurfaceFormatKHR image_format;
  u8 max_frames_in_flight;
  VkSwapchainKHR handle;
  u32 image_count;
  VkImage *images;
  VkImageView *views;

  vulkan_image depth_attachment;
} vulkan_swapchain;

typedef enum vulkan_command_buffer_state {
  COMMAND_BUFFER_STATE_READY,
  COMMAND_BUFFER_STATE_RECORDING,
  COMMAND_BUFFER_STATE_IN_RENDER_PASS,
  COMMAND_BUFFER_STATE_RECORDING_ENDED,
  COMMAND_BUFFER_STATE_SUBMITTED,
  COMMAND_BUFFER_STATE_NOT_ALLOCATED,
} vulkan_command_buffer_state;

typedef struct vulkan_command_buffer {
  VkCommandBuffer handle;

  vulkan_command_buffer_state state;
} vulkan_command_buffer;

typedef struct vulkan_context {
  u32 framebuffer_width;
  u32 framebuffer_height;

  VkInstance instance;
  VkAllocationCallbacks *allocator;
  VkSurfaceKHR surface;

#if defined(_DEBUG)
  VkDebugUtilsMessengerEXT debug_messenger;
#endif

  vulkan_device device;

  vulkan_swapchain swapchain;
  vulkan_render_pass main_render_pass;

  // darray
  vulkan_command_buffer *graphics_command_buffers;

  u32 image_index;
  u32 current_frame;

  b8 recreating_swapchain;

  i32 (*find_memory_index)(u32 type_filter, u32 property_flags);
} vulkan_context;
