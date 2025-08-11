#include "Application.h"

void Application::fail() {
	m_state = SDL_APP_FAILURE;
}

void Application::_init() {
	m_state = SDL_APP_CONTINUE;

	SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	init();
}

SDL_AppResult Application::_process() {
	process_tick();
	return m_state;
}

SDL_AppResult Application::_on_sdl_event(SDL_Event &event) {
	process_sdl_event(event);
	return m_state;
}

void Application::request_close() {
	m_state = SDL_APP_SUCCESS;
}
