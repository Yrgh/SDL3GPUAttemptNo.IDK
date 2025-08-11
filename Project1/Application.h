#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>

#include "common.h"

/* Application
 * Base class for an application. Intended to be derived.
*/
class Application {
	SDL_AppResult m_state = SDL_APP_FAILURE;

protected:
	inline virtual void process_tick() {}
	inline virtual void process_sdl_event(SDL_Event &event) {}

	inline virtual void init() {}

public:
	// ONLY to be used by main.cpp
	void _init();
	inline virtual void _on_fail() {}
	inline virtual void _on_success() {}
	SDL_AppResult _process();
	SDL_AppResult _on_sdl_event(SDL_Event &event);

	void fail();
	void request_close();
};

