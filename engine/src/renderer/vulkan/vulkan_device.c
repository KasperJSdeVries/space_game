#include "vulkan_device.h"

#include "core/logger.h"
#include "core/smemory.h"
#include "core/sstring.h"

#include "containers/darray.h"

typedef struct vulkan_physical_device_requirements {
	b8 graphics;
	b8 present;
	b8 compute;
	b8 transfer;
	// darray
	const char **device_extension_names;
	b8 sampler_anistropy;
} vulkan_physical_device_requirements;

typedef struct vulkan_physical_device_queue_family_info {
	u32 graphics_family_index;
	u32 present_family_index;
	u32 compute_family_index;
	u32 transfer_family_index;
} vulkan_physical_device_queue_family_info;

b8 select_physical_device(vulkan_context *context);
b8 physical_device_meets_requirements(VkPhysicalDevice device,
									  VkSurfaceKHR surface,
									  const VkPhysicalDeviceProperties *properties,
									  const VkPhysicalDeviceFeatures *features,
									  const vulkan_physical_device_requirements *requirements,
									  vulkan_physical_device_queue_family_info *out_queue_family_info,
									  vulkan_swapchain_support_info *out_swapchain_support);
u32 rate_physical_device(VkPhysicalDevice device,
						 const VkPhysicalDeviceProperties *properties,
						 const VkPhysicalDeviceFeatures *features);

b8 vulkan_device_create(vulkan_context *context) {
	if (!select_physical_device(context)) { return false; }

	SINFO("Creating logical device.");

	// NOTE: Do not create additional queues for shared indices.
	b8 transfer_shares_graphics_queue = context->device.graphics_queue_index == context->device.transfer_queue_index;
	b8 compute_shares_graphics_queue  = context->device.graphics_queue_index == context->device.compute_queue_index;
	b8 present_shares_graphics_queue  = context->device.graphics_queue_index == context->device.present_queue_index;
	b8 present_shares_compute_queue   = context->device.compute_queue_index == context->device.present_queue_index;

	u32 index_count = 1;
	if (!transfer_shares_graphics_queue) { index_count++; }
	if (!compute_shares_graphics_queue) { index_count++; }
	if (!(present_shares_graphics_queue || present_shares_compute_queue)) { index_count++; }

	u32 indices[index_count];
	u8 index         = 0;
	indices[index++] = context->device.graphics_queue_index;
	if (!transfer_shares_graphics_queue) { indices[index++] = context->device.transfer_queue_index; }
	if (!compute_shares_graphics_queue) { indices[index++] = context->device.compute_queue_index; }
	if (!(present_shares_graphics_queue || present_shares_compute_queue)) {
		indices[index++] = context->device.present_queue_index;
	}

	VkDeviceQueueCreateInfo queue_create_infos[index_count];
	for (u32 i = 0; i < index_count; ++i) {
		queue_create_infos[i].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_infos[i].queueFamilyIndex = indices[i];
		queue_create_infos[i].queueCount       = 1;
		queue_create_infos[i].flags            = 0;
		queue_create_infos[i].pNext            = 0;
		f32 queue_priority                     = 1.0f;
		queue_create_infos[i].pQueuePriorities = &queue_priority;
	}

	// Request device features.
	// TODO: Should be config driven
	VkPhysicalDeviceFeatures device_features = {};
	device_features.samplerAnisotropy        = VK_TRUE; // Request anisotropy
	device_features.geometryShader           = VK_TRUE; // Request geometryShader

	const char *extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

	VkDeviceCreateInfo device_create_info = {
		.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount    = index_count,
		.pQueueCreateInfos       = queue_create_infos,
		.pEnabledFeatures        = &device_features,
		.enabledExtensionCount   = 1,
		.ppEnabledExtensionNames = &extension_names,
	};

	// Create the device.
	VK_CHECK(vkCreateDevice(context->device.physical_device,
							&device_create_info,
							context->allocator,
							&context->device.logical_device));

	SINFO("Logical device created.");

	// Get queues.
	vkGetDeviceQueue(context->device.logical_device,
					 context->device.graphics_queue_index,
					 0,
					 &context->device.graphics_queue);
	vkGetDeviceQueue(context->device.logical_device,
					 context->device.present_queue_index,
					 0,
					 &context->device.present_queue);
	vkGetDeviceQueue(context->device.logical_device,
					 context->device.transfer_queue_index,
					 0,
					 &context->device.transfer_queue);
	vkGetDeviceQueue(context->device.logical_device,
					 context->device.compute_queue_index,
					 0,
					 &context->device.compute_queue);
	SINFO("Queues obtained.");

	VkCommandPoolCreateInfo pool_create_info = {
		.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = context->device.graphics_queue_index,
		.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	};
	VK_CHECK(vkCreateCommandPool(context->device.logical_device,
								 &pool_create_info,
								 context->allocator,
								 &context->device.graphics_command_pool));
	SINFO("Graphics command pool created.");

	return true;
}

void vulkan_device_destroy(vulkan_context *context) {
	SDEBUG("Destroying command pools...");
	vkDestroyCommandPool(context->device.logical_device, context->device.graphics_command_pool, context->allocator);

	context->device.graphics_queue = 0;
	context->device.present_queue  = 0;
	context->device.transfer_queue = 0;
	context->device.compute_queue  = 0;

	SDEBUG("Destroying logical device...");
	if (context->device.logical_device) {
		vkDestroyDevice(context->device.logical_device, context->allocator);
		context->device.logical_device = 0;
	}

	SDEBUG("Releasing physical device resources...");
	context->device.physical_device = 0;

	vulkan_device_clear_swapchain_support_info(&context->device.swapchain_support);

	context->device.graphics_queue_index = INVALID_ID;
	context->device.present_queue_index  = INVALID_ID;
	context->device.transfer_queue_index = INVALID_ID;
	context->device.compute_queue_index  = INVALID_ID;
}

void vulkan_device_query_swapchain_support(VkPhysicalDevice physical_device,
										   VkSurfaceKHR surface,
										   vulkan_swapchain_support_info *out_support_info) {
	// Surface capabilities
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &out_support_info->capabilities));

	// Surface formats
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_support_info->format_count, 0));

	if (out_support_info->format_count != 0) {
		if (!out_support_info->formats) {
			out_support_info->formats
				= sallocate(sizeof(VkSurfaceFormatKHR) * out_support_info->format_count, MEMORY_TAG_RENDERER);
		}
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device,
													  surface,
													  &out_support_info->format_count,
													  out_support_info->formats));

		// Present modes
		VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device,
														   surface,
														   &out_support_info->present_mode_count,
														   0));

		if (out_support_info->present_mode_count != 0) {
			if (!out_support_info->present_modes) {
				out_support_info->present_modes
					= sallocate(sizeof(VkPresentModeKHR) * out_support_info->present_mode_count, MEMORY_TAG_RENDERER);
			}
			VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device,
															   surface,
															   &out_support_info->present_mode_count,
															   out_support_info->present_modes));
		}
	}
}

void vulkan_device_clear_swapchain_support_info(vulkan_swapchain_support_info *support_info) {
	if (support_info->formats) {
		sfree(support_info->formats, sizeof(VkSurfaceFormatKHR) * support_info->format_count, MEMORY_TAG_RENDERER);
	}
	if (support_info->present_modes) {
		sfree(support_info->present_modes,
			  sizeof(VkSurfaceFormatKHR) * support_info->present_mode_count,
			  MEMORY_TAG_RENDERER);
	}
	support_info->format_count       = 0;
	support_info->formats            = 0;
	support_info->present_mode_count = 0;
	support_info->present_modes      = 0;

	szero_memory(&support_info->capabilities, sizeof(support_info->capabilities));
}

b8 vulkan_device_detect_depth_format(vulkan_device *device) {
	// Format candidates
	const u64 candidate_count = 3;
	VkFormat candidates[]     = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

	u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	for (u64 i = 0; i < candidate_count; ++i) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(device->physical_device, candidates[i], &properties);

		if ((properties.linearTilingFeatures & flags) == flags) {
			device->depth_format = candidates[i];
			return true;
		} else if ((properties.optimalTilingFeatures & flags) == flags) {
			device->depth_format = candidates[i];
			return true;
		}
	}

	return false;
}

b8 select_physical_device(vulkan_context *context) {
	u32 physical_device_count = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0));
	if (physical_device_count == 0) {
		SFATAL("No devices which support Vulkan were found.");
		return false;
	}

	VkPhysicalDevice physical_devices[physical_device_count];
	VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices));

	u32 highest_rating = 0;
	for (u32 i = 0; i < physical_device_count; ++i) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

		VkPhysicalDeviceMemoryProperties memory;
		vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

		// TODO: These requirements should be driven by engine configuration.
		vulkan_physical_device_requirements requirements = {
			.graphics          = true,
			.present           = true,
			.transfer          = true,
			.compute           = false,
			.sampler_anistropy = true,
		};
		requirements.device_extension_names = darray_create(const char *);
		darray_push(requirements.device_extension_names, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		vulkan_physical_device_queue_family_info queue_info = {};
		vulkan_swapchain_support_info swapchain_info        = {};

		if (physical_device_meets_requirements(physical_devices[i],
											   context->surface,
											   &properties,
											   &features,
											   &requirements,
											   &queue_info,
											   &swapchain_info)) {
			u32 score = rate_physical_device(physical_devices[i], &properties, &features);
			if (score > highest_rating) {
				highest_rating = score;

				context->device.physical_device = physical_devices[i];

				context->device.swapchain_support = swapchain_info;

				context->device.graphics_queue_index = queue_info.graphics_family_index;
				context->device.present_queue_index  = queue_info.present_family_index;
				context->device.transfer_queue_index = queue_info.transfer_family_index;
				context->device.compute_queue_index  = queue_info.compute_family_index;

				context->device.properties = properties;
				context->device.features   = features;
				context->device.memory     = memory;
			}
		}
	}

	if (!context->device.physical_device) {
		SERROR("No physical devices were found which meet the requirements.");
		return false;
	}

	VkPhysicalDeviceProperties properties   = context->device.properties;
	VkPhysicalDeviceMemoryProperties memory = context->device.memory;

	SINFO("Selected device: '%s'.", properties.deviceName);

	switch (properties.deviceType) {
		default:
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			SINFO("GPU type is Unknown.");
			break;

		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			SINFO("GPU type is Integrated.");
			break;

		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			SINFO("GPU type is Discrete.");
			break;

		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			SINFO("GPU type is Virtual.");
			break;

		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			SINFO("GPU type is CPU.");
			break;
	}

	SINFO("GPU Driver version: %d.%d.%d",
		  VK_VERSION_MAJOR(properties.driverVersion),
		  VK_VERSION_MINOR(properties.driverVersion),
		  VK_VERSION_PATCH(properties.driverVersion));
	SINFO("Vulkan API version: %d.%d.%d",
		  VK_VERSION_MAJOR(properties.apiVersion),
		  VK_VERSION_MINOR(properties.apiVersion),
		  VK_VERSION_PATCH(properties.apiVersion));

	// Memory information
	for (u32 i = 0; i < memory.memoryHeapCount; ++i) {
		f32 memory_size_gib = (((f32)memory.memoryHeaps[i].size) / 1024.0f / 1024.0f / 1024.0f);
		if (memory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
			SINFO("Local GPU memory: %.2f GiB", memory_size_gib);
		} else {
			SINFO("Shared System memory: %.2f GiB", memory_size_gib);
		}
	}

	SINFO("Physical device selected.");

	return true;
}

b8 physical_device_meets_requirements(VkPhysicalDevice device,
									  VkSurfaceKHR surface,
									  const VkPhysicalDeviceProperties *properties,
									  const VkPhysicalDeviceFeatures *features,
									  const vulkan_physical_device_requirements *requirements,
									  vulkan_physical_device_queue_family_info *out_queue_family_info,
									  vulkan_swapchain_support_info *out_swapchain_support) {
	// Evaluate the device properties to determine if it meets the needs of
	// our application.
	out_queue_family_info->graphics_family_index = INVALID_ID;
	out_queue_family_info->present_family_index  = INVALID_ID;
	out_queue_family_info->compute_family_index  = INVALID_ID;
	out_queue_family_info->transfer_family_index = INVALID_ID;

	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
	VkQueueFamilyProperties queue_families[queue_family_count];
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

	// Look at each queue family and see what queues it supports.
	SINFO("Grapics | Present | Compute | Transfer | Name");
	u8 min_transfer_score        = 255;
	u32 max_transfer_queue_count = 0;
	for (u32 i = 0; i < queue_family_count; ++i) {
		u8 current_transfer_score = 0;

		// Graphics queue?
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			out_queue_family_info->graphics_family_index = i;
			++current_transfer_score;
		}

		// Compute queue?
		if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
			out_queue_family_info->compute_family_index = i;
			++current_transfer_score;
		}

		// Transfer queue?
		if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
			// Take the index if it is the current lowest. This increases the
			// likelihood that it is a dedicated transfer queue.
			if (current_transfer_score < min_transfer_score) {
				min_transfer_score                           = current_transfer_score;
				max_transfer_queue_count                     = queue_families[i].queueCount;
				out_queue_family_info->transfer_family_index = i;

				// Ensure the highest queue count for the chosen transfer queue.
			} else if (current_transfer_score == min_transfer_score) {
				if (queue_families[i].queueCount >= max_transfer_queue_count) {
					max_transfer_queue_count                     = queue_families[i].queueCount;
					out_queue_family_info->transfer_family_index = i;
				}
			}
		}

		// Present queue?
		VkBool32 supports_present = VK_FALSE;
		VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present));
		if (supports_present) {
			// Enforce that the present queue is also the graphics queue.
			// NOTE: Could be made to prefer the compute queue family if we do
			// post-processing using the compute queue.
			if (i == out_queue_family_info->graphics_family_index) { out_queue_family_info->present_family_index = i; }
		}
	}

	SINFO("%7d | %7d | %7d | %8d | %s",
		  out_queue_family_info->graphics_family_index != INVALID_ID,
		  out_queue_family_info->graphics_family_index != INVALID_ID,
		  out_queue_family_info->graphics_family_index != INVALID_ID,
		  out_queue_family_info->graphics_family_index != INVALID_ID,
		  properties->deviceName);

	if ((!requirements->graphics
		 || (requirements->graphics && out_queue_family_info->graphics_family_index != INVALID_ID))
		&& (!requirements->present
			|| (requirements->present && out_queue_family_info->present_family_index != INVALID_ID))
		&& (!requirements->compute
			|| (requirements->compute && out_queue_family_info->compute_family_index != INVALID_ID))
		&& (!requirements->transfer
			|| (requirements->transfer && out_queue_family_info->transfer_family_index != INVALID_ID))) {
		SINFO("Device meets queue requirements.");
		SDEBUG("Graphics Family Index: %i", out_queue_family_info->graphics_family_index);
		SDEBUG("Present Family Index: %i", out_queue_family_info->present_family_index);
		SDEBUG("Transfer Family Index: %i", out_queue_family_info->transfer_family_index);
		SDEBUG("Compute Family Index: %i", out_queue_family_info->compute_family_index);

		// Query the swapchain support.
		vulkan_device_query_swapchain_support(device, surface, out_swapchain_support);

		if (out_swapchain_support->format_count < 1 || out_swapchain_support->present_mode_count < 1) {
			vulkan_device_clear_swapchain_support_info(out_swapchain_support);
			SINFO("Required swapchain support not present, skipping device.");
			return false;
		}

		// Device extensions
		if (requirements->device_extension_names) {
			u32 available_extension_count               = 0;
			VkExtensionProperties *available_extensions = 0;
			VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count, 0));

			if (available_extension_count != 0) {
				available_extensions
					= sallocate(sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
				VK_CHECK(
					vkEnumerateDeviceExtensionProperties(device, 0, &available_extension_count, available_extensions));

				u64 required_extension_count = darray_length(requirements->device_extension_names);
				for (u64 i = 0; i < required_extension_count; ++i) {
					b8 found = false;
					for (u32 j = 0; j < available_extension_count; ++j) {
						if (string_equal(requirements->device_extension_names[i],
										 available_extensions[j].extensionName)) {
							found = true;
							break;
						}
					}

					if (!found) {
						SINFO("Required extension not found: '%s', skipping device.",
							  requirements->device_extension_names[i]);
						sfree(available_extensions,
							  sizeof(VkExtensionProperties) * available_extension_count,
							  MEMORY_TAG_RENDERER);
						return false;
					}
				}

				sfree(available_extensions,
					  sizeof(VkExtensionProperties) * available_extension_count,
					  MEMORY_TAG_RENDERER);
			}
		}

		// Sampler anisotropy
		if (requirements->sampler_anistropy && !features->samplerAnisotropy) {
			SINFO("Device does not suport sampler anisotropy, skipping.");
			return false;
		}

		return true;
	}

	return false;
}

u32 rate_physical_device(VkPhysicalDevice device,
						 const VkPhysicalDeviceProperties *properties,
						 const VkPhysicalDeviceFeatures *features) {
	(void)device;
	(void)features;

	// TODO: Improve rating formula.

	u32 score = 0;

	switch (properties->deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			score += 1000;
			break;

		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			score += 500;
			break;

		default:
			break;
	}

	score += properties->limits.maxImageDimension2D / 128;

	return score;
}
