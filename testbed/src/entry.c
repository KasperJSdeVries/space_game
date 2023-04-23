#include "game.h"
#include <entry.h>

// TODO: Remove this (see below.)
#include <stdlib.h>

b8 create_game(game *out_game) {
  out_game->app_config.start_pos_x = 100;
  out_game->app_config.start_pos_y = 100;
  out_game->app_config.start_width = 1280;
  out_game->app_config.start_height = 720;
  out_game->app_config.name = "Space Testbed Game";

  out_game->initialize = game_initialize;
  out_game->update = game_update;
  out_game->render = game_render;
  out_game->on_resize = game_on_resize;

  // TODO: use engine provided allocation functions.
  out_game->state = malloc(sizeof(game_state));

  return true;
}
