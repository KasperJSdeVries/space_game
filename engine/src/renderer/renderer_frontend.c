#include "renderer_frontend.h"
#include <crtdbg.h>

#include "renderer_backend.h"

#include "core/logger.h"
#include "core/smemory.h"
#include "math/smath.h"

typedef struct renderer_system_state {
	renderer_backend backend;
	mat4 projection;
} renderer_system_state;

// Backend render context.
static renderer_system_state *state_ptr = 0;

b8 render_system_initialize(u64 *memory_requirement, void *state, const char *application_name) {
	*memory_requirement = sizeof(renderer_system_state);

	if (state == 0) { return true; }

	state_ptr = state;

	// TODO: make this configurable.
	renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, &state_ptr->backend);

	if (!state_ptr->backend.initialize(&state_ptr->backend, application_name)) {
		SFATAL("Renderer backend failed to initialize. Shutting down.");
		return false;
	}

	return true;
}

void renderer_system_shutdown(void *state) {
	(void)state;
	if (state_ptr) { state_ptr->backend.shutdown(&state_ptr->backend); }
	state_ptr = 0;
}

b8 renderer_begin_frame(f32 delta_time) {
	if (!state_ptr) { return false; }
	return state_ptr->backend.begin_frame(&state_ptr->backend, delta_time);
}

b8 renderer_end_frame(f32 delta_time) {
	if (!state_ptr) { return false; }

	b8 result = state_ptr->backend.end_frame(&state_ptr->backend, delta_time);
	state_ptr->backend.frame_number++;
	return result;
}

void renderer_on_resize(u16 width, u16 height) {
	if (state_ptr) {
		state_ptr->backend.resize(&state_ptr->backend, width, height);
	} else {
		SWARN("renderer backend does not exist to accept resize: %i, %i", width, height);
	}
}

b8 renderer_draw_frame(render_packet *packet) {
	// If renderer_begin_frame returned successfully, mid-frame operations may
	// continue.
	if (renderer_begin_frame(packet->delta_time)) {
		// TODO: mid-frame operations

		mat4 projection = mat4_perspective(deg_to_rad(90), 16.0f / 9.0f, 0.1f, 1000.0f);

		static f32 z = -1.0f;
		z -= 0.1f;
		mat4 view = mat4_translation((vec3){.x = 0, .y = 0, .z = z});

		state_ptr->backend.update_global_state(projection, view, vec3_zero(), vec4_one(), 0);

		static f32 angle = 0.01f;
		angle += 0.01f;
		quat rotation = quat_from_axis_angle(vec3_forward(), angle, false);
		mat4 model    = quat_to_rotation_matrix(rotation, vec3_zero());

		state_ptr->backend.update_object(model);

		// End the frame. If this fails, it is likely unrecoverable.
		b8 result = renderer_end_frame(packet->delta_time);

		if (!result) {
			SERROR("renderer_end_frame failed. Application shutting down.");
			return false;
		}
	}

	return true;
}
