#ifndef CHIP8_SDL_CONNECTOR_H
#define CHIP8_SDL_CONNECTOR_H

#include <SDL3/SDL.h>
#include "chip8.h"


// stores the state of our GUI application
struct chip8_sdl_app_state {

  struct chip8 chip;

  Uint64 last_frame_elapsed_millis;

  SDL_Window *window; 
  SDL_Renderer *renderer; 
  SDL_AudioStream *stream;
};


int chip8_sdl_app_init(void **appstate, struct chip8_init *init, SDL_Window *window, SDL_Renderer *renderer, SDL_AudioStream *stream);
SDL_AppResult chip8_sdl_app_event(void *appstate, SDL_Event *event);
SDL_AppResult chip8_sdl_app_iterate(void *appstate);
void chip8_sdl_app_quit(void *appstate, SDL_AppResult result);

#endif// CHIP8_SDL_CONNECTOR_H
