#include "core/application.h"

#include "core/event.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/space_memory.h"
#include "defines.h"
#include "game_types.h"
#include "platform/platform.h"

typedef struct application_state {
  game *game_instance;
  b8 is_running;
  b8 is_suspended;
  platform_state platform;
  i16 width;
  i16 height;
  f64 last_time;
} application_state;

static b8 initialized = false;
static application_state app_state;

// Event handlers
b8 application_on_event(u16 code, void *sender, void *listener_instance,
                        event_context context);
b8 application_on_key(u16 code, void *sender, void *listener_instance,
                      event_context context);

b8 application_create(game *game_instance) {
  if (initialized) {
    SPACE_ERROR("application_create called more than once.");
    return false;
  }

  app_state.game_instance = game_instance;

  // Initialize subsystems;
  logging_initialize();
  input_initialize();

  app_state.is_running = true;
  app_state.is_suspended = false;

  if (!event_initialize()) {
    SPACE_ERROR(
        "Event system failed initialization. Application can not continue.");
    return false;
  }

  event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
  event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
  event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

  if (!platform_startup(&app_state.platform,
                        app_state.game_instance->app_config.name,
                        app_state.game_instance->app_config.start_pos_x,
                        app_state.game_instance->app_config.start_pos_y,
                        app_state.game_instance->app_config.start_width,
                        app_state.game_instance->app_config.start_height)) {
    SPACE_ERROR("Could not start-up platform");
    return false;
  }

  // Initialize the game
  if (!app_state.game_instance->initialize(app_state.game_instance)) {
    SPACE_FATAL("Game failed to initialize.");
    return false;
  }

  app_state.game_instance->on_resize(app_state.game_instance, app_state.width,
                                     app_state.height);

  initialized = true;

  return true;
}

b8 application_run() {
  SPACE_INFO(get_memory_usage_string());

  while (app_state.is_running) {
    if (!platform_pump_messages(&app_state.platform)) {
      app_state.is_running = false;
    }

    if (!app_state.is_suspended) {
      if (!app_state.game_instance->update(app_state.game_instance, 0.0f)) {
        SPACE_FATAL("Game update failed, shutting down.");
        app_state.is_running = false;
        break;
      }

      if (!app_state.game_instance->render(app_state.game_instance, 0.0f)) {
        SPACE_FATAL("Game render failed, shutting down.");
        app_state.is_running = false;
        break;
      }

      // NOTE: Input update/state copying should always be handled after any
      // input should be recorded; I.E. before this line. As a safety, input is
      // the last thing to be updated before this frame ends.
      input_update(0);
    }
  }

  app_state.is_running = false;

  SPACE_DEBUG("Shutting down");

  event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
  event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
  event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

  event_shutdown();
  input_shutdown();

  platform_shutdown(&app_state.platform);

  return true;
}

b8 application_on_event(u16 code, void *sender, void *listener_instance,
                        event_context context) {
  switch (code) {
  case EVENT_CODE_APPLICATION_QUIT: {
    SPACE_INFO("EVENT_CODE_APPLICATION_QUIT received, shutting down.");
    app_state.is_running = false;
    return true;
  }
  }
  return false;
}

b8 application_on_key(u16 code, void *sender, void *listener_instance,
                      event_context context) {
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
      SPACE_DEBUG("Explicit - A key pressed!");
      break;

    default:
      SPACE_DEBUG("'%c' key pressed in window.", key_code);
      break;
    }
  } break;

  case EVENT_CODE_KEY_RELEASED: {
    u16 key_code = context.data.u16[0];
    switch (key_code) {
    case KEY_B:
      SPACE_DEBUG("Explicit - B key released!");
      break;

    default:
      SPACE_DEBUG("'%c' key released in window.", key_code);
      break;
    }
  } break;
  }

  return false;
}
