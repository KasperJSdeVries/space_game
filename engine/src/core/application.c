#include "core/application.h"

#include "core/clock.h"
#include "core/event.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/smemory.h"

#include "defines.h"
#include "game_types.h"
#include "platform/platform.h"

#include "renderer/renderer_frontend.h"

typedef struct application_state {
  game *game_instance;
  b8 is_running;
  b8 is_suspended;
  platform_state platform;
  u16 width;
  u16 height;
  clock clock;
  f64 last_time;
} application_state;

static b8 initialized = false;
static application_state app_state;

// Event handlers
b8 application_on_event(u16 code, void *sender, void *listener_instance,
                        event_context context);
b8 application_on_key(u16 code, void *sender, void *listener_instance,
                      event_context context);
b8 application_on_resize(u16 code, void *sender, void *listener_instance,
                         event_context context);

b8 application_create(game *game_instance) {
  if (initialized) {
    SERROR("application_create called more than once.");
    return false;
  }

  app_state.game_instance = game_instance;

  // Initialize subsystems;
  logging_initialize();
  input_initialize();

  app_state.is_running = true;
  app_state.is_suspended = false;

  if (!event_initialize()) {
    SERROR(
        "Event system failed initialization. Application can not continue.");
    return false;
  }

  event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
  event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
  event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
  event_register(EVENT_CODE_RESIZED, 0, application_on_resize);

  if (!platform_startup(&app_state.platform,
                        app_state.game_instance->app_config.name,
                        app_state.game_instance->app_config.start_pos_x,
                        app_state.game_instance->app_config.start_pos_y,
                        app_state.game_instance->app_config.start_width,
                        app_state.game_instance->app_config.start_height)) {
    SERROR("Could not start-up platform");
    return false;
  }

  // Renderer startup
  if (!renderer_initialize(game_instance->app_config.name,
                           &app_state.platform)) {
    SFATAL("Failed to initialize renderer. Aborting application.");
    return false;
  }

  // Initialize the game
  if (!app_state.game_instance->initialize(app_state.game_instance)) {
    SFATAL("Game failed to initialize.");
    return false;
  }

  app_state.game_instance->on_resize(app_state.game_instance, app_state.width,
                                     app_state.height);

  initialized = true;

  return true;
}

b8 application_run() {
  clock_start(&app_state.clock);
  clock_update(&app_state.clock);
  app_state.last_time = app_state.clock.elapsed;

  f64 running_time = 0;
  u16 frame_count = 0;
  f64 target_frame_seconds = 1.0f / 165;

  SINFO(get_memory_usage_string());

  while (app_state.is_running) {
    if (!platform_pump_messages(&app_state.platform)) {
      app_state.is_running = false;
    }

    if (!app_state.is_suspended) {
      clock_update(&app_state.clock);
      f64 current_time = app_state.clock.elapsed;
      f64 delta = (current_time - app_state.last_time);
      f64 frame_start_time = platform_get_absolute_time();

      if (!app_state.game_instance->update(app_state.game_instance,
                                           (f32)delta)) {
        SFATAL("Game update failed, shutting down.");
        app_state.is_running = false;
        break;
      }

      if (!app_state.game_instance->render(app_state.game_instance,
                                           (f32)delta)) {
        SFATAL("Game render failed, shutting down.");
        app_state.is_running = false;
        break;
      }

      // TODO: Render packet creation.
      render_packet packet;
      packet.delta_time = (f32)delta;
      renderer_draw_frame(&packet);

      f64 frame_end_time = platform_get_absolute_time();
      f64 frame_elapsed_time = frame_end_time - frame_start_time;
      running_time += frame_elapsed_time;
      f64 remaining_seconds = target_frame_seconds - frame_elapsed_time;

      if (remaining_seconds > 0) {
        u64 remaining_ms = ((u64)remaining_seconds * 1000);

        b8 limit_frames = false;
        if (remaining_ms > 0 && limit_frames) {
          platform_sleep(remaining_ms - 1);
        }

        frame_count++;
      }

      // NOTE: Input update/state copying should always be handled after any
      // input should be recorded; I.E. before this line. As a safety, input is
      // the last thing to be updated before this frame ends.
      input_update(delta);

      app_state.last_time = current_time;
    }
  }

  app_state.is_running = false;

  event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
  event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
  event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
  event_unregister(EVENT_CODE_RESIZED, 0, application_on_resize);

  event_shutdown();
  input_shutdown();

  renderer_shutdown();

  platform_shutdown(&app_state.platform);

  SINFO("Ran for %d frames (%f seconds)", frame_count, running_time);
  SINFO("Shut down successfully");

  return true;
}

void application_get_framebuffer_size(u32 *width, u32 *height) {
  *width = app_state.width;
  *height = app_state.height;
}

b8 application_on_event(u16 code, void *sender, void *listener_instance,
                        event_context context) {
  (void)sender;
  (void)listener_instance;
  (void)context;

  switch (code) {
  case EVENT_CODE_APPLICATION_QUIT: {
    SINFO("EVENT_CODE_APPLICATION_QUIT received, shutting down.");
    app_state.is_running = false;
    return true;
  }
  }
  return false;
}

b8 application_on_key(u16 code, void *sender, void *listener_instance,
                      event_context context) {

  (void)sender;
  (void)listener_instance;
  (void)context;

  switch (code) {
  case EVENT_CODE_KEY_PRESSED: {
    u16 key_code = context.data.u16[0];
    switch (key_code) {
    case KEY_ESCAPE: {
      // NOTE: firing an event to itself, but there may be other listeners.
      event_context data = {};
      event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

      // Block anything else from processing this key press.
      return true;
    }

    case KEY_A:
      SDEBUG("Explicit - A key pressed!");
      break;

    default:
      SDEBUG("'%c' key pressed in window.", key_code);
      break;
    }
  } break;

  case EVENT_CODE_KEY_RELEASED: {
    u16 key_code = context.data.u16[0];
    switch (key_code) {
    case KEY_B:
      SDEBUG("Explicit - B key released!");
      break;

    default:
      SDEBUG("'%c' key released in window.", key_code);
      break;
    }
  } break;
  }

  return false;
}

b8 application_on_resize(u16 code, void *sender, void *listener_instance,
                         event_context context) {
  (void)sender;
  (void)listener_instance;

  if (code == EVENT_CODE_RESIZED) {
    u16 width = context.data.u16[0];
    u16 height = context.data.u16[1];

    if (width != app_state.width || height != app_state.height) {
      app_state.width = width;
      app_state.height = height;

      SDEBUG("Window resize: %i, %i", width, height);

      if (width == 0 || height == 0) {
        SINFO("Window minimized, suspending application.");
        app_state.is_suspended = true;
        return true;
      } else {
        if (app_state.is_suspended) {
          SINFO("Window restored, resuming application.");
          app_state.is_suspended = false;
        }
        app_state.game_instance->on_resize(app_state.game_instance,
                                           app_state.width, app_state.height);
        renderer_on_resize(width, height);
      }
    }
  }

  return false;
}
