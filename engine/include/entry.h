#pragma once

#include "core/application.h"
#include "core/logger.h"
#include "core/smemory.h"
#include "game_types.h"

// Externally-defined function to create a game.
extern b8 create_game(game *out_game);

// The main entry point of the application.
int main(void) {
	// Request the game instance from the application.
	game game_instance = {};
	if (!create_game(&game_instance)) {
		SFATAL("Could not create game!");
		return -1;
	}

	// Ensure the function pointers exist.
	if (!game_instance.render || !game_instance.update || !game_instance.initialize || !game_instance.on_resize) {
		SFATAL("The game's function pointers must be assigned!");
		return -2;
	}

	// Initialization
	if (!application_create(&game_instance)) {
		SINFO("Application failed to create!");
		return 1;
	}

	// Begin the game loop;
	if (!application_run()) {
		SINFO("Application did not shutdown gracefully.");
		return 2;
	}

	return 0;
}
