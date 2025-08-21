#define SDL_MAIN_USE_CALLBACKS

#include "AppImpl.h"

#include <iostream>

#include <SDL3/SDL_main.h>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_EXCEPTION
#include "MiniLibs/tiny_gltf.h"

SDL_malloc_func original_malloc;
SDL_calloc_func original_calloc;
SDL_realloc_func original_realloc;
SDL_free_func original_free;

extern "C" {
void *my_malloc(size_t size) {
	total_allocs++;
	return original_malloc(size);
}

void *my_calloc(size_t nmemb, size_t size) {
	total_allocs++;
	return original_calloc(nmemb, size);
}

void *my_realloc(void *mem, size_t size) {
	if (!mem) total_allocs++;
	if (size == 0 && mem) total_allocs--;
	return original_realloc(mem, size);
}

void my_free(void *mem) {
	total_allocs--;
	original_free(mem);
}
}

void at_exit() {
	std::cout << "Leaked allocations: " << total_allocs << "\n";
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
	atexit(at_exit);
	SDL_GetOriginalMemoryFunctions(&original_malloc, &original_calloc, &original_realloc, &original_free);
	SDL_SetMemoryFunctions(my_malloc, my_calloc, my_realloc, my_free);

	std::cout << "Allocating app\n";
	AppImpl *app = new AppImpl();
	
	app->_init();
	
	*appstate = app;

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
	AppImpl *app = (AppImpl *) appstate;

	return app->_process();
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
	AppImpl *app = (AppImpl *) appstate;

	return app->_on_sdl_event(*event);
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
	AppImpl *app = (AppImpl *) appstate;
	
	if (result == SDL_APP_SUCCESS) {
		app->_on_success();
	} else {
		app->_on_fail();
	}

	std::cout << "Deallocating app\n";
	delete app;
}