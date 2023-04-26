#include "vulkan_backend.h"

#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_platform.h"
#include "vulkan_renderpass.h"
#include "vulkan_swapchain.h"
#include "vulkan_types.inl"

#include "core/asserts.h"
#include "core/logger.h"
#include "core/space_memory.h"
#include "core/space_string.h"

#include "containers/darray.h"

// Static Vulkan context
static vulkan_context context;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data);

i32 find_memory_index(u32 type_filter, u32 property_flags);

void create_command_buffers(renderer_backend *backend);

b8 vulkan_renderer_backend_initialize(renderer_backend *backend,
                                      const char *application_name,
                                      struct platform_state *platform_state) {

  // Function pointers
  context.find_memory_index = find_memory_index;

  // TODO: Custom allocator.
  context.allocator = NULL;

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .apiVersion = VK_API_VERSION_1_3,
      .pApplicationName = application_name,
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "Space Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
  };

  VkInstanceCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
  };

  // Obtain a list of required extensions.
  const char **required_extensions = darray_create(const char *);
  darray_push(required_extensions,
              &VK_KHR_SURFACE_EXTENSION_NAME); // Generic surface extension
  platform_get_required_extension_names(
      &required_extensions); // Platform-specific extensions

#if defined(_DEBUG)
  darray_push(required_extensions,
              &VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Debug utilities

  SPACE_DEBUG("Required extensions:");
  u32 length = (u32)darray_length(required_extensions);
  for (u32 i = 0; i < length; ++i) {
    SPACE_DEBUG("  %s", required_extensions[i]);
  }
#endif

  create_info.enabledExtensionCount = (u32)darray_length(required_extensions);
  create_info.ppEnabledExtensionNames = required_extensions;

  // Validation layers
  const char **required_validation_layers_names = 0;
  u32 required_validation_layer_count = 0;

#if defined(_DEBUG)
  SPACE_INFO("Validation layers enabled. Enumerating...");

  // The list of validation layers required.
  required_validation_layers_names = darray_create(const char *);
  darray_push(required_validation_layers_names, &"VK_LAYER_KHRONOS_validation");
  required_validation_layer_count =
      (u32)darray_length(required_validation_layers_names);

  // Obtain a list of available validation layers
  u32 available_layer_count = 0;
  VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, 0));
  VkLayerProperties *available_layers =
      darray_reserve(VkLayerProperties, available_layer_count);
  VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count,
                                              available_layers));

  // Verify all required layers are available
  for (u32 i = 0; i < required_validation_layer_count; i++) {
    SPACE_INFO("Searching for layer: %s...",
               required_validation_layers_names[i]);
    b8 found = false;
    for (u32 j = 0; j < available_layer_count; ++j) {
      if (string_equal(required_validation_layers_names[i],
                       available_layers[j].layerName)) {
        found = true;
        SPACE_INFO("  Found.");
        break;
      }
    }

    if (!found) {
      SPACE_FATAL("Required validation layer is missing: %s",
                  required_validation_layers_names[i]);
      return false;
    }
  }
  SPACE_INFO("All required validation layers present.");
#endif

  create_info.enabledLayerCount = required_validation_layer_count;
  create_info.ppEnabledLayerNames = required_validation_layers_names;

  VK_CHECK(
      vkCreateInstance(&create_info, context.allocator, &context.instance));
  SPACE_INFO("Vulkan instance created.");

// Debugger
#if defined(_DEBUG)
  SPACE_DEBUG("Creating Vulkan Debugger...");
  u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = log_severity,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
      .pfnUserCallback = vk_debug_callback,
      .pUserData = 0,
  };

  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          context.instance, "vkCreateDebugUtilsMessengerEXT");
  SPACE_ASSERT_MESSAGE(func, "Failed to create debug messenger!");
  VK_CHECK(func(context.instance, &debug_create_info, context.allocator,
                &context.debug_messenger));
  SPACE_DEBUG("Vulkan Debugger created.");

#endif

  // Surface creation
  if (!platform_create_vulkan_surface(platform_state, &context)) {
    SPACE_ERROR("Failed to create surface!");
    return false;
  }

  // Device creation
  if (!vulkan_device_create(&context)) {
    SPACE_ERROR("Failed to create device!");
    return false;
  }

  // Swapchain
  vulkan_swapchain_create(&context, context.framebuffer_width,
                          context.framebuffer_height, &context.swapchain);

  vulkan_render_pass_create(
      &context, &context.main_render_pass, 0, 0, (f32)context.framebuffer_width,
      (f32)context.framebuffer_height, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0);

  create_command_buffers(backend);

  SPACE_INFO("Vulkan renderer initialized successfully.");

  return true;
}

void vulkan_renderer_backend_shutdown(renderer_backend *backend) {
  (void)backend;

  SPACE_INFO("Freeing command buffers...");
  for (u32 i = 0; i < context.swapchain.image_count; ++i) {
    if (context.graphics_command_buffers[i].handle) {
      vulkan_command_buffer_free(&context, context.device.graphics_command_pool,
                                 &context.graphics_command_buffers[i]);
    }
  }
  darray_destroy(context.graphics_command_buffers);
  context.graphics_command_buffers = 0;

  vulkan_render_pass_destroy(&context, &context.main_render_pass);

  SPACE_INFO("Destroying Vulkan swapchain...");
  vulkan_swapchain_destroy(&context, &context.swapchain);

  SPACE_INFO("Destroying Vulkan device...");
  vulkan_device_destroy(&context);

  SPACE_INFO("Destroying Vulkan surface...");
  vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);

#if defined(_DEBUG)
  SPACE_INFO("Destroying Vulkan debugger...");
  if (context.debug_messenger) {
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            context.instance, "vkDestroyDebugUtilsMessengerEXT");
    func(context.instance, context.debug_messenger, context.allocator);
  }
#endif

  SPACE_INFO("Destroying Vulkan instance...");
  vkDestroyInstance(context.instance, context.allocator);
}

void vulkan_renderer_backend_on_resize(renderer_backend *backend, u16 width,
                                       u16 height) {
  (void)backend;
  (void)width;
  (void)height;
}

b8 vulkan_renderer_backend_begin_frame(renderer_backend *backend,
                                       f32 delta_time) {
  (void)backend;
  (void)delta_time;

  return true;
}

b8 vulkan_renderer_backend_end_frame(renderer_backend *backend,
                                     f32 delta_time) {
  (void)backend;
  (void)delta_time;

  return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                  VkDebugUtilsMessageTypeFlagsEXT message_types,
                  const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                  void *user_data) {
  (void)user_data;
  (void)message_types;

  switch (message_severity) {
  default:
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    SPACE_ERROR(callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    SPACE_WARN(callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    SPACE_INFO(callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    SPACE_TRACE(callback_data->pMessage);
    break;
  }
  return VK_FALSE;
}

i32 find_memory_index(u32 type_filter, u32 property_flags) {
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(context.device.physical_device,
                                      &memory_properties);

  for (u32 i = 0; i < memory_properties.memoryTypeCount; ++i) {
    if (type_filter & (1 << i) &&
        (memory_properties.memoryTypes[i].propertyFlags & property_flags) ==
            property_flags) {
      return (i32)i;
    }
  }

  SPACE_WARN("Unable to find suitable memory type!");
  return -1;
}

void create_command_buffers(renderer_backend *backend) {
  (void)backend;

  if (!context.graphics_command_buffers) {
    context.graphics_command_buffers =
        darray_reserve(vulkan_command_buffer, context.swapchain.image_count);
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
      space_zero_memory(&context.graphics_command_buffers[i],
                        sizeof(vulkan_command_buffer));
    }
  }

  for (u32 i = 0; i < context.swapchain.image_count; ++i) {
    if (context.graphics_command_buffers[i].handle) {
      vulkan_command_buffer_free(&context, context.device.graphics_command_pool,
                                 &context.graphics_command_buffers[i]);
    }
    space_zero_memory(&context.graphics_command_buffers[i],
                      sizeof(vulkan_command_buffer));
    vulkan_command_buffer_allocate(&context,
                                   context.device.graphics_command_pool, true,
                                   &context.graphics_command_buffers[i]);
  }

  SPACE_INFO("Vulkan command buffers created.");
}
