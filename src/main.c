/*
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_dialog.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <limits.h>


#include "chip8_sdl_connector.h"


int parse_command_line_args(struct chip8_init *init, int argc, char **argv) {
  char *chip_rom = NULL;
  enum chip8_emu_type type;

  uint8_t emu_selected = 0;

  for(uint32_t i = 1; i < argc; i++) {
    if(strcmp(argv[i], "--type") == 0) {
      if(emu_selected) {
        printf("Error: Duplicate --type argument!\n");
        return 0;
      }

      emu_selected = 1;

      i++;

      if(i >= argc) {
        printf("Error: Invalid argument after --type! Argument must be VIP, SUPER, or XO. \n");
        return 0;
      }

      char *emu_type = argv[i];

      if(strcmp(emu_type, "VIP") == 0) {
        type = CHIP8_VARIANT_VIP;
      } else if(strcmp(emu_type, "SUPER") == 0) {
        type = CHIP8_VARIANT_SUPER;
      } else if(strcmp(emu_type, "XO") == 0) {
        type = CHIP8_VARIANT_XO;
      } else {
        printf("Error: Invalid argument after --type! Argument must be VIP, SUPER, or XO. \n");
        return 0;
      }
    } else {

      if(chip_rom != NULL) {
        printf("Error: You cannot run multiple CHIP-8 ROMs!\n");
        return 0;
      }

      chip_rom = argv[i];
    }
  }

  init->rom_file = chip_rom;
  init->type = type;

  return 1;
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
  struct chip8_init init;
  if(!parse_command_line_args(&init, argc, argv)) {
    return SDL_APP_FAILURE; 
  }

  //https://wiki.libsdl.org/SDL3/SDL_AppInit

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  static SDL_Window *window;
  static SDL_Renderer *renderer;
  static SDL_AudioStream *stream;


 // SDL_WindowFlags window_flags = SDL_WINDOW_FULLSCREEN;
  SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;

  
  /* Create the window */
  if (!SDL_CreateWindowAndRenderer("RYCE8", 800, 600, window_flags, &window, &renderer)) {
    SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_AudioSpec spec;

  /* We're just playing a single thing here, so we'll use the simplified option.
      We are always going to feed audio in as mono, float32 data at 8000Hz.
      The stream will convert it to whatever the hardware wants on the other side. */
  spec.channels = 1;
  spec.format = SDL_AUDIO_F32;
  spec.freq = 8000;
  stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
  if (!stream) {
    SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }


  if(!chip8_sdl_app_init(appstate, &init, window, renderer, stream)) {
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  return chip8_sdl_app_event(appstate, event);
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
  return chip8_sdl_app_iterate(appstate);
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  chip8_sdl_app_quit(appstate, result);
}
