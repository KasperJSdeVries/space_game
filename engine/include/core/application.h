#pragma once

#include "defines.h"

struct game;

// Application configuration.
typedef struct application_config {
  i16 start_pos_x;
  i16 start_pos_y;
  i16 start_width;
  i16 start_height;

  char *name;
} application_config;

SPACE_API b8 application_create(struct game *game_instance);

SPACE_API b8 application_run();