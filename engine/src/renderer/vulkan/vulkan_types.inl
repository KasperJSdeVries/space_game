#pragma once

#include "core/asserts.h"
#include "defines.h"
#include "math/math_types.inl"
#include "renderer/renderer_types.inl"

#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                                                                 \
	{ SPACE_ASSERT(expr == VK_SUCCESS); }

#define OBJECT_SHADER_STAGE_COUNT 2
#define VULKAN_OBJECT_MAX_OBJECT_COUNT 1024
#define VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT 2
#define DISPLAY_BUFFER_COUNT 3

typedef struct vulkan_buffer {
	u64 total_size;
	VkBuffer handle;
	VkBufferUsageFlagBits usage;
	b8 is_locked;
	VkDeviceMemory memory;
	i32 memory_index;
	u32 memory_property_flags;
} vulkan_buffer;

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
	vec2 position;
	vec2 dimensions;
	vec4 colour;

	f32 depth;
	u32 stencil;

	vulkan_render_pass_state state;
} vulkan_render_pass;

typedef struct vulkan_framebuffer {
	VkFramebuffer handle;
	u32 attachment_count;
	VkImageView *attachments;
	vulkan_render_pass *render_pass;
} vulkan_framebuffer;

typedef struct vulkan_swapchain {
	VkSurfaceFormatKHR image_format;
	u8 max_frames_in_flight;
	VkSwapchainKHR handle;
	u32 image_count;
	VkImage *images;
	VkImageView *views;

	vulkan_image depth_attachment;

	// darray of framebuffers used for on-screen rendering.
	vulkan_framebuffer *framebuffers;
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

typedef struct {
	VkFence handle;
	b8 is_signaled;
} vulkan_fence;

typedef struct vulkan_shader_stage {
	VkShaderModuleCreateInfo create_info;
	VkShaderModule handle;
	VkPipelineShaderStageCreateInfo shader_stage_create_info;
} vulkan_shader_stage;

typedef struct vulkan_pipeline {
	VkPipeline handle;
	VkPipelineLayout pipeline_layout;
} vulkan_pipeline;

typedef struct vulkan_descriptor_state {
	u32 generations[DISPLAY_BUFFER_COUNT];
} vulkan_descriptor_state;

typedef struct vulkan_object_shader_object_state {
	VkDescriptorSet descriptor_sets[DISPLAY_BUFFER_COUNT];

	vulkan_descriptor_state descriptor_states[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
} vulkan_object_shader_object_state;

typedef struct vulkan_object_shader {
	vulkan_shader_stage stages[OBJECT_SHADER_STAGE_COUNT];

	VkDescriptorPool global_descriptor_pool;
	VkDescriptorSetLayout global_descriptor_set_layout;

	VkDescriptorSet global_descriptor_sets[DISPLAY_BUFFER_COUNT];
	b8 descriptor_updated[DISPLAY_BUFFER_COUNT];

	global_uniform_object global_ubo;

	vulkan_buffer global_uniform_buffer;

	VkDescriptorPool object_descriptor_pool;
	VkDescriptorSetLayout object_descriptor_set_layout;
	vulkan_buffer object_uniform_buffer;
	// TODO: manage a free list of some kind here instead.
	u32 object_uniform_buffer_index;

	// TODO: Make dynamic.
	vulkan_object_shader_object_state object_states[VULKAN_OBJECT_MAX_OBJECT_COUNT];

	vulkan_pipeline pipeline;
} vulkan_object_shader;

typedef struct vulkan_context {
	f32 frame_delta_time;

	u32 framebuffer_width;
	u32 framebuffer_height;
	u64 framebuffer_size_generation;
	u64 framebuffer_size_last_generation;

	VkInstance instance;
	VkAllocationCallbacks *allocator;
	VkSurfaceKHR surface;

#if defined(_DEBUG)
	VkDebugUtilsMessengerEXT debug_messenger;
#endif

	vulkan_device device;

	vulkan_swapchain swapchain;
	vulkan_render_pass main_render_pass;

	vulkan_buffer object_vertex_buffer;
	vulkan_buffer object_index_buffer;

	// darray
	vulkan_command_buffer *graphics_command_buffers;

	// darray
	VkSemaphore *image_available_semaphores;

	// darray
	VkSemaphore *queue_complete_semaphores;

	u32 in_flight_fence_count;
	vulkan_fence *in_flight_fences;

	// Holds pointers to fences which exist and are owned elsewhere.
	vulkan_fence **images_in_flight;

	u32 image_index;
	u32 current_frame;

	b8 recreating_swapchain;

	vulkan_object_shader object_shader;

	u64 geometry_vertex_offset;
	u64 geometry_index_offset;

	i32 (*find_memory_index)(u32 type_filter, u32 property_flags);
} vulkan_context;

typedef struct vulkan_texture_data {
	vulkan_image image;
	VkSampler sampler;
} vulkan_texture_data;
