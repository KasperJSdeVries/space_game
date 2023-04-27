#include "vulkan_backend.h"

#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_fence.h"
#include "vulkan_framebuffer.h"
#include "vulkan_platform.h"
#include "vulkan_renderpass.h"
#include "vulkan_result.h"
#include "vulkan_swapchain.h"
#include "vulkan_types.inl"

#include "core/application.h"
#include "core/asserts.h"
#include "core/logger.h"
#include "core/space_memory.h"
#include "core/space_string.h"

#include "containers/darray.h"

#include <stdint.h>

// Static Vulkan context
static vulkan_context context;
static u32 cached_framebuffer_width = 0;
static u32 cached_framebuffer_height = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data);

i32 find_memory_index(u32 type_filter, u32 property_flags);

void create_command_buffers(renderer_backend *backend);
void regenerate_framebuffers(renderer_backend *backend,
                             vulkan_swapchain *swapchain,
                             vulkan_render_pass *render_pass);
b8 recreate_swapchain(renderer_backend *backend);

b8 vulkan_renderer_backend_initialize(renderer_backend *backend,
                                      const char *application_name,
                                      struct platform_state *platform_state) {

  // Function pointers
  context.find_memory_index = find_memory_index;

  // TODO: Custom allocator.
  context.allocator = NULL;

  application_get_framebuffer_size(&cached_framebuffer_width,
                                   &cached_framebuffer_height);
  context.framebuffer_width =
      (cached_framebuffer_width != 0) ? cached_framebuffer_width : 800;
  context.framebuffer_height =
      (cached_framebuffer_height != 0) ? cached_framebuffer_height : 600;
  cached_framebuffer_width = 0;
  cached_framebuffer_height = 0;

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

  // Swapchain framebuffers.
  context.swapchain.framebuffers =
      darray_reserve(vulkan_framebuffer, context.swapchain.image_count);
  regenerate_framebuffers(backend, &context.swapchain,
                          &context.main_render_pass);

  create_command_buffers(backend);

  // Create sync objects
  // Create sync objects.
  context.image_available_semaphores =
      darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
  context.queue_complete_semaphores =
      darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
  context.in_flight_fences =
      darray_reserve(vulkan_fence, context.swapchain.max_frames_in_flight);

  for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    vkCreateSemaphore(context.device.logical_device, &semaphore_create_info,
                      context.allocator,
                      &context.image_available_semaphores[i]);
    vkCreateSemaphore(context.device.logical_device, &semaphore_create_info,
                      context.allocator, &context.queue_complete_semaphores[i]);

    // Create the fence in a signaled state, indicating that the first frame has
    // already been "rendered". This will prevent the application from waiting
    // indefinitely for the first frame to render since it cannot be rendered
    // until a frame is "rendered" before it.
    vulkan_fence_create(&context, true, &context.in_flight_fences[i]);
  }

  // In flight fences should not yet exist at this point, so clear the list.
  // These are stored in pointers because the initial state should be 0, and
  // will be 0 when not in use. Acutal fences are not owned by this list.
  context.images_in_flight =
      darray_reserve(vulkan_fence, context.swapchain.image_count);
  for (u32 i = 0; i < context.swapchain.image_count; ++i) {
    context.images_in_flight[i] = 0;
  }
  SPACE_INFO("Vulkan renderer initialized successfully.");

  return true;
}

void vulkan_renderer_backend_shutdown(renderer_backend *backend) {
  (void)backend;

  vkDeviceWaitIdle(context.device.logical_device);

  SPACE_INFO("Destroying sync objects...");
  for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
    if (context.image_available_semaphores[i]) {
      vkDestroySemaphore(context.device.logical_device,
                         context.image_available_semaphores[i],
                         context.allocator);
      context.image_available_semaphores[i] = 0;
    }
    if (context.queue_complete_semaphores[i]) {
      vkDestroySemaphore(context.device.logical_device,
                         context.queue_complete_semaphores[i],
                         context.allocator);
      context.queue_complete_semaphores[i] = 0;
    }
    vulkan_fence_destroy(&context, &context.in_flight_fences[i]);
  }
  darray_destroy(context.image_available_semaphores);
  context.image_available_semaphores = 0;

  darray_destroy(context.queue_complete_semaphores);
  context.queue_complete_semaphores = 0;

  darray_destroy(context.in_flight_fences);
  context.in_flight_fences = 0;

  darray_destroy(context.images_in_flight);
  context.images_in_flight = 0;

  SPACE_INFO("Freeing command buffers...");
  for (u32 i = 0; i < context.swapchain.image_count; ++i) {
    if (context.graphics_command_buffers[i].handle) {
      vulkan_command_buffer_free(&context, context.device.graphics_command_pool,
                                 &context.graphics_command_buffers[i]);
    }
  }
  darray_destroy(context.graphics_command_buffers);
  context.graphics_command_buffers = 0;

  SPACE_INFO("Destroying framebuffers...");
  for (u32 i = 0; i < context.swapchain.image_count; ++i) {
    vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);
  }

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

  cached_framebuffer_width = width;
  cached_framebuffer_height = height;
  context.framebuffer_size_generation++;

  SPACE_DEBUG("Vulkan renderer backend -> resized: w/h/gen: %i/%i/%llu", width,
              height, context.framebuffer_size_generation);
}

b8 vulkan_renderer_backend_begin_frame(renderer_backend *backend,
                                       f32 delta_time) {
  (void)delta_time;

  vulkan_device *device = &context.device;

  // check if recreating swap chain and boot out.
  if (context.recreating_swapchain) {
    VkResult result = vkDeviceWaitIdle(device->logical_device);
    if (!vulkan_result_is_success(result)) {
      SPACE_ERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (1) "
                  "failed: '%s'",
                  vulkan_result_string(result, true));
      return false;
    }
    SPACE_INFO("Recreating swapchain, booting.");
    return false;
  }

  // Check if the framebuffer has resized. If so, a new swapchain must be
  // created.
  if (context.framebuffer_size_generation !=
      context.framebuffer_size_last_generation) {
    VkResult result = vkDeviceWaitIdle(device->logical_device);
    if (!vulkan_result_is_success(result)) {
      SPACE_ERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (2) "
                  "failed: '%s'",
                  vulkan_result_string(result, true));
      return false;
    }

    if (!recreate_swapchain(backend)) {
      return false;
    }

    SPACE_INFO("Resized, booting.");
    return false;
  }

  // Wait for the execution of the current frame to complete. The fence being
  // free will allow this one to move on.
  if (!vulkan_fence_wait(&context,
                         &context.in_flight_fences[context.current_frame],
                         UINT64_MAX)) {
    SPACE_WARN("In-flight fence wait failure!");
    return false;
  }

  // Acquire the next image from the swapchain. Pass along the semaphore that
  // should be signaled when this completes. This same semaphore will later be
  // waited on by the queue submission to ensure this image is available.
  if (!vulkan_swapchain_acquire_next_image_index(
          &context, &context.swapchain, UINT64_MAX,
          context.image_available_semaphores[context.current_frame], 0,
          &context.image_index)) {
    return false;
  }

  // Begin recording commands.
  vulkan_command_buffer *command_buffer =
      &context.graphics_command_buffers[context.image_index];
  vulkan_command_buffer_reset(command_buffer);
  vulkan_command_buffer_begin(command_buffer, false, false, false);

  // Dynamic state
  VkViewport viewport = {
      .x = 0.0f,
      .y = (f32)context.framebuffer_height,
      .width = (f32)context.framebuffer_width,
      .height = -(f32)context.framebuffer_height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };

  // Scissor
  VkRect2D scissor = {
      .offset =
          {
              .x = 0,
              .y = 0,
          },
      .extent =
          {
              .width = context.framebuffer_width,
              .height = context.framebuffer_height,
          },
  };

  vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport);
  vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);

  context.main_render_pass.w = (f32)context.framebuffer_width;
  context.main_render_pass.h = (f32)context.framebuffer_height;

  vulkan_render_pass_begin(
      command_buffer, &context.main_render_pass,
      context.swapchain.framebuffers[context.image_index].handle);

  return true;
}

b8 vulkan_renderer_backend_end_frame(renderer_backend *backend,
                                     f32 delta_time) {
  (void)backend;
  (void)delta_time;

  vulkan_command_buffer *command_buffer =
      &context.graphics_command_buffers[context.image_index];

  vulkan_render_pass_end(command_buffer, &context.main_render_pass);

  vulkan_command_buffer_end(command_buffer);

  if (context.images_in_flight[context.image_index] != VK_NULL_HANDLE) {
    vulkan_fence_wait(&context, context.images_in_flight[context.image_index],
                      UINT64_MAX);
  }

  // Mark the image fence as in-use by this frame.
  context.images_in_flight[context.image_index] =
      &context.in_flight_fences[context.current_frame];

  // Reset the fence for use on the next frame
  vulkan_fence_reset(&context,
                     &context.in_flight_fences[context.current_frame]);

  // Submit the queue and wait for the operation to complete.
  // Begin queue submission
  VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

      // Command buffer(s) to be executed.
      .commandBufferCount = 1,
      .pCommandBuffers = &command_buffer->handle,

      // The semaphore(s) to be signaled when the queue is complete.
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &context.queue_complete_semaphores[context.current_frame],

      // Wait semaphore ensures that the operation cannot begin until the image
      // is available.
      .waitSemaphoreCount = 1,
      .pWaitSemaphores =
          &context.image_available_semaphores[context.current_frame],
  };

  // Each semaphore waits on the corresponding pipeline stage to complete. 1:1
  // ratio. VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents subsequent
  // colour attachment writes from executing until the semaphore signals (i.e.
  // one frame is presented at a time)
  VkPipelineStageFlags flags[1] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.pWaitDstStageMask = flags;

  VkResult result =
      vkQueueSubmit(context.device.graphics_queue, 1, &submit_info,
                    context.in_flight_fences[context.current_frame].handle);
  if (result != VK_SUCCESS) {
    SPACE_ERROR("vkQueueSubmit failed with result: %s",
                vulkan_result_string(result, true));
    return false;
  }

  vulkan_command_buffer_update_submitted(command_buffer);
  // End queue submission.

  vulkan_swapchain_present(
      &context, &context.swapchain, context.device.graphics_queue,
      context.device.present_queue,
      context.queue_complete_semaphores[context.current_frame],
      context.image_index);

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

void regenerate_framebuffers(renderer_backend *backend,
                             vulkan_swapchain *swapchain,
                             vulkan_render_pass *render_pass) {
  (void)backend;

  for (u32 i = 0; i < swapchain->image_count; ++i) {
    // TODO: make this dynamic based on the currently configured attachments
    u32 attachment_count = 2;
    VkImageView attachments[] = {
        swapchain->views[i],
        swapchain->depth_attachment.view,
    };

    vulkan_framebuffer_create(&context, render_pass, context.framebuffer_width,
                              context.framebuffer_height, attachment_count,
                              attachments, &context.swapchain.framebuffers[i]);
  }
}

b8 recreate_swapchain(renderer_backend *backend) {
  if (context.recreating_swapchain) {
    SPACE_DEBUG("recreate_swapchain called when already recreating. Booting.");
    return false;
  }

  if (context.framebuffer_width == 0 || context.framebuffer_height == 0) {
    SPACE_DEBUG("recreate_swapchain called when window is < 1 in a dimension. "
                "Booting.");
    return false;
  }

  context.recreating_swapchain = true;

  vkDeviceWaitIdle(context.device.logical_device);

  for (u32 i = 0; i < context.swapchain.image_count; ++i) {
    context.images_in_flight[i] = 0;
  }

  vulkan_device_query_swapchain_support(context.device.physical_device,
                                        context.surface,
                                        &context.device.swapchain_support);
  vulkan_device_detect_depth_format(&context.device);

  vulkan_swapchain_recreate(&context, cached_framebuffer_width,
                            cached_framebuffer_height, &context.swapchain);

  context.framebuffer_width = cached_framebuffer_width;
  context.framebuffer_height = cached_framebuffer_height;
  context.main_render_pass.w = (f32)cached_framebuffer_width;
  context.main_render_pass.h = (f32)cached_framebuffer_height;
  cached_framebuffer_width = 0;
  cached_framebuffer_height = 0;

  context.framebuffer_size_last_generation =
      context.framebuffer_size_generation;

  for (u32 i = 0; i < context.swapchain.image_count; ++i) {
    vulkan_command_buffer_free(&context, context.device.graphics_command_pool,
                               &context.graphics_command_buffers[i]);
  }

  for (u32 i = 0; i < context.swapchain.image_count; ++i) {
    vulkan_framebuffer_destroy(&context, &context.swapchain.framebuffers[i]);
  }

  context.main_render_pass.x = 0;
  context.main_render_pass.y = 0;
  context.main_render_pass.w = (f32)context.framebuffer_width;
  context.main_render_pass.h = (f32)context.framebuffer_height;

  regenerate_framebuffers(backend, &context.swapchain,
                          &context.main_render_pass);

  create_command_buffers(backend);

  context.recreating_swapchain = false;

  return true;
}
