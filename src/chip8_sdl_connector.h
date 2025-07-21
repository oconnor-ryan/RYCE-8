#ifndef CHIP8_SDL_CONNECTOR_H
#define CHIP8_SDL_CONNECTOR_H

#include <SDL3/SDL.h>
#include "chip8.h"

enum chip8_sdl_loader_status {
  CHIP8_LOADER_STATUS_SUCCESS,
  CHIP8_LOADER_STATUS_CANNOT_OPEN_FILE,
  CHIP8_LOADER_STATUS_CANNOT_READ_FILE,
  CHIP8_LOADER_STATUS_NO_FILE_SELECTED,

};

// stores the state of our GUI application
struct chip8_sdl_app_state {
  struct chip8 vm;
  Uint64 last_frame_elapsed_millis;
  enum chip8_sdl_loader_status loader_status;

  SDL_Window *window; 
  SDL_Renderer *renderer; 
  SDL_AudioStream *stream;
};


void chip8_sdl_app_init(void **appstate, SDL_Window *window, SDL_Renderer *renderer, SDL_AudioStream *stream);
SDL_AppResult chip8_sdl_app_event(void *appstate, SDL_Event *event);
SDL_AppResult chip8_sdl_app_iterate(void *appstate);
void chip8_sdl_app_quit(void *appstate, SDL_AppResult result);

#endif// CHIP8_SDL_CONNECTOR_H
