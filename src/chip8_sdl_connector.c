/*
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
#include <SDL3/SDL_dialog.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <limits.h>

#include "chip8_sdl_connector.h"


#define CHIP8_SDL_PIXEL_SIZE 2
#define CHIP8_SDL_PIXELS_BETWEEN_DEBUG_CHARS 2



void chip8_sdl_load_file(struct chip8_sdl_app_state *state, const char *filepath) {
  uint8_t already_has_rom = state->loader_status == CHIP8_LOADER_STATUS_SUCCESS;

  if(filepath == NULL) {
    //if we canceled out without picking a file, we should not change the loader status
    //if another ROM was already loaded.
    if(!already_has_rom) {
      state->loader_status = CHIP8_LOADER_STATUS_NO_FILE_SELECTED;
    }
    return;
  }

  FILE *file = fopen(filepath, "r");

  //TODO: display an specific error to the user
  if(file == NULL) {

    state->loader_status = CHIP8_LOADER_STATUS_CANNOT_OPEN_FILE;
    return;
  }

  //we only want 1 file selected
  int success = chip8_wrapper_reset(&state->chip, file);

  if(!success) {
    fclose(file);
    state->loader_status = CHIP8_LOADER_STATUS_CANNOT_READ_FILE;
    return;
  }

  state->loader_status = CHIP8_LOADER_STATUS_SUCCESS;

  fclose(file);

}

// Of type SDL_DialogFileCallback
void SDLCALL chip8_sdl_cb_on_file_loaded(void *userdata, const char * const *filelist, int filter) {
  struct chip8_sdl_app_state *state = userdata;

  //an error occurred
  if(filelist == NULL) {
    printf("Error opening the file picker UI!");
    SDL_Log("%s", SDL_GetError());
    chip8_sdl_load_file(state, NULL);
    return;
  }

  //no file selected
  if(filelist[0] == NULL) {
    chip8_sdl_load_file(state, NULL);
    return; 
  }

  //otherwise, filelist is a pointer to a list of char* for filenames

  chip8_sdl_load_file(state, filelist[0]);
}


void chip8_sdl_open_file_dialog(struct chip8_sdl_app_state *state) {
  //note that this function is async, so we need to use a callback function
  SDL_ShowOpenFileDialog(chip8_sdl_cb_on_file_loaded, (void*) state, state->window, NULL, 0, NULL, 0);
}


void chip8_sdl_draw_chip8(struct chip8_sdl_app_state *state, int start_x, int start_y) {
  SDL_FRect rect;

  //TODO: Add support for rendering 128x64 resolution

  //define borders of the rectangle where our emulator renders pixels. Make sure
  //that pixels stay within the frame by making the frame slightly larger than where the pixels are rendered.
  rect.x = start_x - 1;
  rect.y = start_y - 1;
  rect.h = 2 + CHIP8_SDL_PIXEL_SIZE * state->chip.core.fb_height;
  rect.w = 2 + CHIP8_SDL_PIXEL_SIZE * state->chip.core.fb_width;


  //render the frame holding our pixels
  SDL_SetRenderDrawColor(state->renderer, 0, 0, 255, 255);


  //SDL_RenderDrawRectF()
  //Watch out, SDL3 changes many functions within SDL2, including SDL_RenderDrawRectF().
  // Since most tutorials are in SDL2, make sure to double check the SDL3 docs when using
  // an outdated tutorial.

  //Note that the rectangle outline is 1 pixel thick
  SDL_RenderRect(state->renderer, &rect);


  rect.h = CHIP8_SDL_PIXEL_SIZE;
  rect.w = CHIP8_SDL_PIXEL_SIZE;
  rect.x = start_x;
  rect.y = start_y;

  //32 rows, 64 columns
  for(uint8_t r = 0; r < state->chip.core.fb_height; r++) {
    uint64_t columns = state->chip.core.fb[r];

    //we shift to right until the set bit is pushed out
    for(uint64_t c = (uint64_t)1 << 63; c != 0; c >>= 1) {
      //is on
      if(columns & c) {
        SDL_SetRenderDrawColor(state->renderer, 0, 255, 0, 255);
      } else {
        SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
      }


      SDL_RenderFillRect(state->renderer, &rect);

      rect.x += CHIP8_SDL_PIXEL_SIZE;

    }

    rect.x = start_x;
    rect.y += CHIP8_SDL_PIXEL_SIZE;
  }

}

void chip8_sdl_draw_debug_keys(struct chip8_sdl_app_state *state, int start_x, int start_y) {
  const char keys[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

  int x = start_x;
  for(uint16_t i = 1, k = 0; k < 16; i <<= 1, k++) {
    if(state->chip.core.keyboard_inputs & i) {
      SDL_SetRenderDrawColor(state->renderer, 0, 255, 0, 255);
    } else {
      SDL_SetRenderDrawColor(state->renderer, 0, 0, 255, 255);
    }

    char str[2] = {keys[k], '\0'};
    SDL_RenderDebugText(state->renderer, x, start_y, str);

    x += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + CHIP8_SDL_PIXELS_BETWEEN_DEBUG_CHARS;
  }
}



int chip8_sdl_key_to_chip8_key(const SDL_KeyboardEvent *e, enum chip8_key *c8_key) {

  //Note that the weird layout is due to my computer not having a numpad,
  // so the closest I can get to a 4x4 grid of keys is the following combination 
  // of scan codes.

  //Note that we are using scan-codes. This looks at the PHYSICAL location of the keyboard
  //and reads the character as if it was a QWERTY keyboard. This allows people with other keyboard
  //layouts to use the same PHYSICAL location of keys.
  switch(e->scancode) {
    case SDL_SCANCODE_1: *c8_key = CHIP8_KEY_1; return 1;
    case SDL_SCANCODE_2: *c8_key = CHIP8_KEY_2; return 1;
    case SDL_SCANCODE_3: *c8_key = CHIP8_KEY_3; return 1;
    case SDL_SCANCODE_4: *c8_key = CHIP8_KEY_C; return 1;
    case SDL_SCANCODE_Q: *c8_key = CHIP8_KEY_4; return 1;
    case SDL_SCANCODE_W: *c8_key = CHIP8_KEY_5; return 1;
    case SDL_SCANCODE_E: *c8_key = CHIP8_KEY_6; return 1;
    case SDL_SCANCODE_R: *c8_key = CHIP8_KEY_D; return 1;
    case SDL_SCANCODE_A: *c8_key = CHIP8_KEY_7; return 1;
    case SDL_SCANCODE_S: *c8_key = CHIP8_KEY_8; return 1;
    case SDL_SCANCODE_D: *c8_key = CHIP8_KEY_9; return 1;
    case SDL_SCANCODE_F: *c8_key = CHIP8_KEY_E; return 1;
    case SDL_SCANCODE_Z: *c8_key = CHIP8_KEY_A; return 1;
    case SDL_SCANCODE_X: *c8_key = CHIP8_KEY_0; return 1;
    case SDL_SCANCODE_C: *c8_key = CHIP8_KEY_B; return 1;
    case SDL_SCANCODE_V: *c8_key = CHIP8_KEY_F; return 1;
    default: return 0;
  }
}

void chip8_sdl_add_more_audio(struct chip8_sdl_app_state *state) {
  static float samples[512];  /* this will feed 512 samples each frame until we get to our maximum. */
  int i;
  
  uint8_t toggle = 0;
  //create square waveform for a consistant sounding tone

  for (i = 0; i < SDL_arraysize(samples); i++) {

    //remember, the pitch increases the more frequently you oscillate the waveform.
    //So dividing by 128 produces a higher pitch than 64
    if(i % (SDL_arraysize(samples) / 64) == 0) {
      toggle = !toggle;
    }

    //samples only seem to be valid if between -1.0f and 1.0f
    if(toggle) {
      samples[i] = 1.0f;
    } else {
      samples[i] = -1.0f;
      //samples[i] = 0.25f;
    }
  }


  /* feed the new data to the stream. It will queue at the end, and trickle out as the hardware needs more data. */
  SDL_PutAudioStreamData(state->stream, samples, sizeof (samples));
}

/* This function runs once at startup. */
void chip8_sdl_app_init(void **appstate, SDL_Window *window, SDL_Renderer *renderer, SDL_AudioStream *stream) {
  //https://wiki.libsdl.org/SDL3/SDL_AppInit

  //set seed for RNG
  srand(time(NULL));

  //to avoid making this global, we will declare a local static variable.
  //Static variables are more efficient than heap-allocating, and since we only
  //need a single instance of chip8_sdl_app_state, we will safely give its pointer to SDL.

  static struct chip8_sdl_app_state state;

  state.loader_status = CHIP8_LOADER_STATUS_NO_FILE_SELECTED;

  chip8_wrapper_init(&state.chip, CHIP8_VARIANT_SUPER);
  //chip8_wrapper_init(&state.chip, CHIP8_VARIANT_VIP);




  //make sure to run SDL_GetTicks AFTER everything is initialized. This prevents
  //the Chip8's timers from running until after everything else is loaded.
  state.last_frame_elapsed_millis = SDL_GetTicks();
  state.renderer = renderer;
  state.window = window;
  state.stream = stream;



  //when initialized, make SDL keep a pointer to our app state.
  //This pointer will be passed into all of SDL's callback functions, 
  //so we will be able to track program state without global variables.
  *appstate = &state;


  /* SDL_OpenAudioDeviceStream starts the device paused. You have to tell it to start! */
  //SDL_ResumeAudioStreamDevice(stream);
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult chip8_sdl_app_event(void *appstate, SDL_Event *event) {
  struct chip8_sdl_app_state *state = appstate;

  switch(event->type) {
    case SDL_EVENT_KEY_DOWN: {
      enum chip8_key key;
      if(chip8_sdl_key_to_chip8_key(&event->key, &key)) {
        chip8_set_key(&state->chip.core, key);
      } 
      else if(event->key.scancode == SDL_SCANCODE_ESCAPE) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
      } 
      //use SDL3's new SDL3_dialog.h header to display native dialogs for opening
      //files to load into the emulator.
      else if(event->key.scancode == SDL_SCANCODE_SPACE) {
        chip8_sdl_open_file_dialog(state);
      }
      
      //ignore all other keypresses

      break;
    }

    case SDL_EVENT_KEY_UP: {
      enum chip8_key key;
      if(chip8_sdl_key_to_chip8_key(&event->key, &key)) {
        chip8_remove_key(&state->chip.core, key);
      }

      //ignore all other keypresses

      break;
    }

    case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */

    default: break;
  }

  return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult chip8_sdl_app_iterate(void *appstate) {
  struct chip8_sdl_app_state *state = appstate;


  //update our timers
  uint64_t time_elapsed_millis = SDL_GetTicks();
  uint64_t delta = time_elapsed_millis - state->last_frame_elapsed_millis;
  state->last_frame_elapsed_millis = time_elapsed_millis;

  

  //only update Chip8 when ROM is actually loaded 
  if(state->loader_status == CHIP8_LOADER_STATUS_SUCCESS) {
    if(!chip8_wrapper_update(&state->chip, delta)) {
      SDL_Log("Cannot process instruction at address %d", state->chip.core.ram[state->chip.core.pc]);
      return SDL_APP_FAILURE;
    }

    if(state->chip.core.sound_timer != 0) {
      SDL_ResumeAudioStreamDevice(state->stream);
    } else {
      SDL_PauseAudioStreamDevice(state->stream);
    }
  }
  


  //play audio (once it unpauses)

  /* see if we need to feed the audio stream more data yet.
    We're being lazy here, but if there's less than half a second queued, generate more.
    A square wave is unchanging audio--easy to stream--but for video games, you'll want
    to generate significantly _less_ audio ahead of time! */
  const int minimum_audio = (8000 * sizeof (float)) / 2;  /* 8000 float samples per second. Half of that. */
  if (SDL_GetAudioStreamQueued(state->stream) < minimum_audio) {
    chip8_sdl_add_more_audio(state);
  }




  //render
  const char *message = "RyChip8";
  int w = 0, h = 0;
  float x, y;
  const float scale = 4.0f;


  /* Center the message and scale it up */
  SDL_GetRenderOutputSize(state->renderer, &w, &h);
  SDL_SetRenderScale(state->renderer, scale, scale);

  x = ((w / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message)) / 2;
  y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 8; // top 1/8th of screen

  /* Draw the message */
  SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
  SDL_RenderClear(state->renderer); //clear entire screen using the draw color 
  SDL_SetRenderDrawColor(state->renderer, 255, 255, 255, 255);
  SDL_RenderDebugText(state->renderer, x, y, message);

  // draw chip8 framebuffer
  x = ( (w / scale) - (state->chip.core.fb_width * CHIP8_SDL_PIXEL_SIZE)) / 2; //center horizontally
  y = ( (h / scale) - (state->chip.core.fb_height * CHIP8_SDL_PIXEL_SIZE)) / 2; //center veritcally

  switch(state->loader_status) {
    case CHIP8_LOADER_STATUS_SUCCESS: chip8_sdl_draw_chip8(state, x, y); break;
    case CHIP8_LOADER_STATUS_CANNOT_OPEN_FILE: {
      SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
      SDL_RenderDebugText(state->renderer, x, y, "Cannot open file!");
      break;
    }
    case CHIP8_LOADER_STATUS_CANNOT_READ_FILE: {
      SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
      SDL_RenderDebugText(state->renderer, x, y, "Cannot read file!");
      break;
    }
    case CHIP8_LOADER_STATUS_NO_FILE_SELECTED: {
      SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
      SDL_RenderDebugText(state->renderer, x, y, "No ROM loaded!");
      break;
    }
  }
  


  x = ( (w / scale) - ((SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + CHIP8_SDL_PIXELS_BETWEEN_DEBUG_CHARS) * 16)) / 2; //center horizontally
  y = 7 * ( (h / scale) - (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + CHIP8_SDL_PIXELS_BETWEEN_DEBUG_CHARS)) / 8; //top 3/4th of screen
  //
  chip8_sdl_draw_debug_keys(state, x, y);

  SDL_RenderPresent(state->renderer);


  return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void chip8_sdl_app_quit(void *appstate, SDL_AppResult result) {}
