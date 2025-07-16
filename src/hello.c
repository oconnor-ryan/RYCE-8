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

#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <limits.h>

#include "chip8.h"

#define PIXEL_SIZE 2

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static struct chip8 vm;

static Uint64 last_frame_elapsed_millis;
static uint64_t delta = 0;



void randomize_fb() {
  //Note that rand() returns a value between 0 and (at least) 32767 (or the maximum of a signed 16 bit int).
  // However, for my compiler, the range is 0 through 0x7fffffff, which is the max value of a signed 32-bit int.
  
  // Note that it does not include negative numbers, so it does not generate a random bit for the most significant bit.
  // This always leaves 1 column of pixels the same color.


  // To allow randomness for all 64 bits, we need to generate random 16-bit integers.
  // Since 16-bit integers can fit the full range of numbers from 0 to RAND_MAX, we will generate
  // 4 random 16-bit integers and place them at the appropriate spot within the 64-bit integer.

  for(uint8_t r = 0; r < 32; r++) {
    vm.fb[r] = 0;

    uint16_t *p = (uint16_t*) (vm.fb + r);
    *p = rand();
    *(p+1) = rand();
    *(p+2) = rand();
    *(p+3) = rand();

  }
}

void draw_chip8(int w, int h, int start_x, int start_y) {

  

  SDL_FRect rect;


  //define borders of the rectangle where our emulator renders pixels. Make sure
  //that pixels stay within the frame by making the frame slightly larger than where the pixels are rendered.
  rect.x = start_x - 1;
  rect.y = start_y - 1;
  rect.h = 2 + PIXEL_SIZE * CHIP8_HEIGHT;
  rect.w = 2 + PIXEL_SIZE * CHIP8_WIDTH;


  //render the frame holding our pixels
  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);


  //SDL_RenderDrawRectF()
  //Watch out, SDL3 changes many functions within SDL2, including SDL_RenderDrawRectF().
  // Since most tutorials are in SDL2, make sure to double check the SDL3 docs when using
  // an outdated tutorial.

  //Note that the rectangle outline is 1 pixel thick
  SDL_RenderRect(renderer, &rect);


  rect.h = PIXEL_SIZE;
  rect.w = PIXEL_SIZE;
  rect.x = start_x;
  rect.y = start_y;

  //32 rows, 64 columns
  for(uint8_t r = 0; r < CHIP8_HEIGHT; r++) {
    uint64_t columns = vm.fb[r];

    //we shift to left until the set bit is pushed out
    for(uint64_t c = 1, i = 0; i < CHIP8_WIDTH; c <<= 1, i++) {
      //is on
      if(columns & c) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
      } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      }


      SDL_RenderFillRect(renderer, &rect);

      rect.x += PIXEL_SIZE;
    }

    rect.x = start_x;
    rect.y += PIXEL_SIZE;
  }

}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{

  //randomize_fb();

  for(uint8_t i = 0; i < 32; i++) {
    vm.fb[i] = ULLONG_MAX; //entire screen is lit up
  }

  srand(time(NULL));

 // SDL_WindowFlags window_flags = SDL_WINDOW_FULLSCREEN;
  SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;

  /* Create the window */
  if (!SDL_CreateWindowAndRenderer("Hello World", 800, 600, window_flags, &window, &renderer)) {
    SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  last_frame_elapsed_millis = SDL_GetTicks();

  return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  if (event->type == SDL_EVENT_KEY_DOWN ||
    event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
  }
  return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
  uint64_t time_elapsed_millis = SDL_GetTicks();

  delta += time_elapsed_millis - last_frame_elapsed_millis;

  // every x milliseconds, randomize the pixels in the framebuffer
  if(delta > 50) {
    delta = 0;
    randomize_fb();
  }

  last_frame_elapsed_millis = time_elapsed_millis;


  const char *message = "RyChip8";
  int w = 0, h = 0;
  float x, y;
  const float scale = 4.0f;

  /* Center the message and scale it up */
  SDL_GetRenderOutputSize(renderer, &w, &h);
  SDL_SetRenderScale(renderer, scale, scale);

  x = ((w / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message)) / 2;
  y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 8; // top 1/8th of screen

  /* Draw the message */
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer); //clear entire screen using the draw color 
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderDebugText(renderer, x, y, message);

  // draw chip8 framebuffer

  x = ( (w / scale) - (CHIP8_WIDTH * PIXEL_SIZE)) / 2; //center horizontally
  y = ( (h / scale) - (CHIP8_HEIGHT * PIXEL_SIZE)) / 2; //center veritcally

  draw_chip8(w, h, x, y);

  SDL_RenderPresent(renderer);

  return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
