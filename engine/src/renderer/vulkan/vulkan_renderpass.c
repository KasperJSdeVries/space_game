#include "vulkan_renderpass.h"

#include "core/smemory.h"

void vulkan_render_pass_create(vulkan_context *context,
                               vulkan_render_pass *out_render_pass, f32 x,
                               f32 y, f32 w, f32 h, f32 r, f32 g, f32 b, f32 a,
                               f32 depth, u32 stencil) {

  out_render_pass->x = x;
  out_render_pass->y = y;
  out_render_pass->w = w;
  out_render_pass->h = h;

  out_render_pass->r = r;
  out_render_pass->g = g;
  out_render_pass->b = b;
  out_render_pass->a = a;

  out_render_pass->depth = depth;
  out_render_pass->stencil = stencil;

  // Attachments
  // TODO: make this configurable
  u32 attachment_description_count = 2;
  VkAttachmentDescription attachment_descriptions[attachment_description_count];

  // Color attachment
  VkAttachmentDescription color_attachment = {
      .format = context->swapchain.image_format.format, // TODO: configurable
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout =
          VK_IMAGE_LAYOUT_UNDEFINED, // Do not expect any particular layout
                                     // before render pass starts.
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // Transitioned to after
                                                      // the render pass
      .flags = 0,
  };

  attachment_descriptions[0] = color_attachment;

  VkAttachmentReference color_attachment_reference = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  // Depth attachment, if there is one
  VkAttachmentDescription depth_attachment = {
      .format = context->device.depth_format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .flags = 0,
  };

  attachment_descriptions[1] = depth_attachment;

  VkAttachmentReference depth_attachment_reference = {
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  // TODO: other attachment types (input, resolve, preserve)

  // Main subpass
  VkSubpassDescription subpass = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,

      // Color attachment data
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_reference,

      // Depth stencil data
      .pDepthStencilAttachment = &depth_attachment_reference,

      // Input from a shader
      .inputAttachmentCount = 0,
      .pInputAttachments = 0,

      // Attachments used for multisampling colour attachments
      .pResolveAttachments = 0,

      // Attachments not used in this subpass, but must be preserved for next.
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = 0,
  };

  // Render pass dependencies.
  // TODO: Make this configurable
  VkSubpassDependency dependency = {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = 0,
      .dstSubpass = 0,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                       VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dependencyFlags = 0,
  };

  VkRenderPassCreateInfo render_pass_create_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = attachment_description_count,
      .pAttachments = attachment_descriptions,
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency,
      .pNext = 0,
      .flags = 0,
  };

  VK_CHECK(vkCreateRenderPass(context->device.logical_device,
                              &render_pass_create_info, context->allocator,
                              &out_render_pass->handle));
}

void vulkan_render_pass_destroy(vulkan_context *context,
                                vulkan_render_pass *render_pass) {
  if (render_pass && render_pass->handle) {
    vkDestroyRenderPass(context->device.logical_device, render_pass->handle,
                        context->allocator);
    render_pass->handle = 0;
  }
}

void vulkan_render_pass_begin(vulkan_command_buffer *command_buffer,
                              vulkan_render_pass *renderpass,
                              VkFramebuffer frame_buffer) {

  VkClearValue clear_values[2];
  szero_memory(clear_values, sizeof(VkClearValue) * 2);
  clear_values[0].color.float32[0] = renderpass->r;
  clear_values[0].color.float32[1] = renderpass->g;
  clear_values[0].color.float32[2] = renderpass->b;
  clear_values[0].color.float32[3] = renderpass->a;
  clear_values[1].depthStencil.depth = renderpass->depth;
  clear_values[1].depthStencil.stencil = renderpass->stencil;

  VkRenderPassBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = renderpass->handle,
      .framebuffer = frame_buffer,
      .renderArea =
          {
              .offset =
                  {
                      .x = (i32)renderpass->x,
                      .y = (i32)renderpass->y,
                  },
              .extent =
                  {
                      .width = (u32)renderpass->w,
                      .height = (u32)renderpass->h,
                  },
          },
      .clearValueCount = 2,
      .pClearValues = clear_values,
  };

  vkCmdBeginRenderPass(command_buffer->handle, &begin_info,
                       VK_SUBPASS_CONTENTS_INLINE);
  command_buffer->state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

void vulkan_render_pass_end(vulkan_command_buffer *command_buffer,
                            vulkan_render_pass *renderpass) {
  (void)renderpass;

  vkCmdEndRenderPass(command_buffer->handle);
  command_buffer->state = COMMAND_BUFFER_STATE_RECORDING;
}
