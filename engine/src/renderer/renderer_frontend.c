#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/space_memory.h"

// Backend render context.
static renderer_backend *backend = 0;

b8 renderer_initialize(const char *application_name,
                       struct platform_state *platform_state) {
  backend = space_allocate(sizeof(renderer_backend), MEMORY_TAG_RENDERER);

  // TODO: make this configurable.
  renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, platform_state,
                          backend);

  if (!backend->initialize(backend, application_name, platform_state)) {
    SPACE_FATAL("Renderer backend failed to initialize. Shutting down.");
    return false;
  }

  return true;
}

void renderer_shutdown() {
  backend->shutdown(backend);
  space_free(backend, sizeof(renderer_backend), MEMORY_TAG_RENDERER);
}

b8 renderer_begin_frame(f32 delta_time) {
  return backend->begin_frame(backend, delta_time);
}

b8 renderer_end_frame(f32 delta_time) {
  b8 result = backend->end_frame(backend, delta_time);
  backend->frame_number++;
  return result;
}

void renderer_on_resize(u16 width, u16 height) {
  if (backend) {
    backend->resize(backend, width, height);
  } else {
    SPACE_WARN("renderer backend does not exist to accept resize: %i, %i",
               width, height);
  }
}

b8 renderer_draw_frame(render_packet *packet) {
  // If the begin frame returned successfully, mid-frame operations may
  // continue.
  if (renderer_begin_frame(packet->delta_time)) {
    // TODO: mid-frame operations

    // End the frame. If this fails, it is likely unrecoverable.
    b8 result = renderer_end_frame(packet->delta_time);

    if (!result) {
      SPACE_ERROR("renderer_end_frame failed. Application shutting down.");
      return false;
    }
  }

  return true;
}
