#include "core/application.h"
#include "core/event.h"
#include "core/logger.h"
#include "core/space_memory.h"
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

b8 application_create(game *game_instance) {
  if (initialized) {
    SPACE_ERROR("application_create called more than once.");
    return false;
  }

  app_state.game_instance = game_instance;

  // Initialize subsystems;
  logging_initialize();

  app_state.is_running = true;
  app_state.is_suspended = false;

  if (!event_initialize()) {
    SPACE_ERROR(
        "Event system failed initialization. Application can not continue.");
    return false;
  }

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
    }
  }

  app_state.is_running = false;

  SPACE_DEBUG("Shutting down");

  event_shutdown();

  platform_shutdown(&app_state.platform);

  return true;
}
