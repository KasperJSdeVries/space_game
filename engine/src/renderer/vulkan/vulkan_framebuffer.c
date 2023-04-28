#include "vulkan_framebuffer.h"

#include "core/smemory.h"
#include <vulkan/vulkan_core.h>

void vulkan_framebuffer_create(vulkan_context *context,
                               vulkan_render_pass *render_pass, u32 width,
                               u32 height, u32 attachment_count,
                               VkImageView *attachments,
                               vulkan_framebuffer *out_framebuffer) {
  // Take a copy of the attachments, render pass, and attachment count
  out_framebuffer->attachments =
      sallocate(sizeof(VkImageView) * attachment_count, MEMORY_TAG_RENDERER);
  out_framebuffer->attachment_count = attachment_count;
  for (u32 i = 0; i < attachment_count; ++i) {
    out_framebuffer->attachments[i] = attachments[i];
  }
  out_framebuffer->render_pass = render_pass;

  // Create info
  VkFramebufferCreateInfo framebuffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = render_pass->handle,
      .attachmentCount = attachment_count,
      .pAttachments = out_framebuffer->attachments,
      .width = width,
      .height = height,
      .layers = 1,
  };

  VK_CHECK(vkCreateFramebuffer(context->device.logical_device,
                               &framebuffer_create_info, context->allocator,
                               &out_framebuffer->handle));
}

void vulkan_framebuffer_destroy(vulkan_context *context,
                                vulkan_framebuffer *framebuffer) {
  vkDestroyFramebuffer(context->device.logical_device, framebuffer->handle,
                       context->allocator);
  if (framebuffer->attachments) {
    sfree(framebuffer->attachments,
          sizeof(VkImageView) * framebuffer->attachment_count,
          MEMORY_TAG_RENDERER);
    framebuffer->attachments = 0;
  }
  framebuffer->handle = 0;
  framebuffer->attachment_count = 0;
  framebuffer->render_pass = 0;
}
