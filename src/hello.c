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
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <limits.h>

#include "chip8.h"

#define PIXEL_SIZE 2
#define DEBUG_KEY_SPACE_BETWEEN_CHARS 2

//we can technically store these in our app_state, 
//but for now, we will keep them here.
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static SDL_AudioStream *stream = NULL;


// stores the state of our GUI application
struct app_state {
  struct chip8 vm;
  Uint64 last_frame_elapsed_millis;

   
  uint8_t use_io_test;
  struct {
    Uint64 timer1;
    Uint64 timer2;
  };
 
};


void randomize_fb(struct app_state *state) {
  //Note that rand() returns a value between 0 and (at least) 32767 (or the maximum of a signed 16 bit int).
  // However, for my compiler, the range is 0 through 0x7fffffff, which is the max value of a signed 32-bit int.
  
  // Note that it does not include negative numbers, so it does not generate a random bit for the most significant bit.
  // This always leaves 1 column of pixels the same color.


  // To allow randomness for all 64 bits, we need to generate random 16-bit integers.
  // Since 16-bit integers can fit the full range of numbers from 0 to RAND_MAX, we will generate
  // 4 random 16-bit integers and place them at the appropriate spot within the 64-bit integer.

  for(uint8_t r = 0; r < 32; r++) {
    state->vm.fb[r] = 0;

    uint16_t *p = (uint16_t*) (state->vm.fb + r);
    *p = rand();
    *(p+1) = rand();
    *(p+2) = rand();
    *(p+3) = rand();

  }
}

void draw_chip8(struct app_state *state, int start_x, int start_y) {

  

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
    uint64_t columns = state->vm.fb[r];

    //we shift to left until the set bit is pushed out
    for(uint64_t c = 1; 1 ; c <<= 1) {
      //is on
      if(columns & c) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
      } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      }


      SDL_RenderFillRect(renderer, &rect);

      rect.x += PIXEL_SIZE;

      if(c & ((uint64_t)1 << 63)) {
        break;
      }
    }

    rect.x = start_x;
    rect.y += PIXEL_SIZE;
  }

}

void draw_chip8_debug_keys(struct app_state *state, int start_x, int start_y) {
  const char keys[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

  int x = start_x;
  for(uint16_t i = 1, k = 0; k < 16; i <<= 1, k++) {
    if(state->vm.keyboard_inputs & i) {
      SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    } else {
      SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    }

    char str[2] = {keys[k], '\0'};
    SDL_RenderDebugText(renderer, x, start_y, str);

    x += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + DEBUG_KEY_SPACE_BETWEEN_CHARS;
  }
}



int sdl_key_to_chip8_key(const SDL_KeyboardEvent *e, enum chip8_key *c8_key) {

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

void add_more_audio() {
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
  SDL_PutAudioStreamData(stream, samples, sizeof (samples));
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
  //https://wiki.libsdl.org/SDL3/SDL_AppInit

  //to avoid making this global, we will declare a local static variable.
  //Static variables are more efficient than heap-allocating, and since we only
  //need a single instance of app_state, we will safely give its pointer to SDL.

  static struct app_state state;

  


  

  //set seed for RNG
  srand(time(NULL));



  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

 // SDL_WindowFlags window_flags = SDL_WINDOW_FULLSCREEN;
  SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;

  /* Create the window */
  if (!SDL_CreateWindowAndRenderer("RyChip8", 800, 600, window_flags, &window, &renderer)) {
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


  if(argc > 2) {
    SDL_Log("You can only insert 0 or 1 arguments in command line!");
    return SDL_APP_FAILURE;
  }

  FILE *input = NULL;
  if(argc == 2) {
    input = fopen(argv[1], "r");
    if(input == NULL) {
      SDL_Log("Cannot open specified file!");
      return SDL_APP_FAILURE;
    }
  }




  //complete initializing app state and bind it to SDL
  state.use_io_test = input == NULL;
  state.timer1 = 0;
  state.timer2 = 4000; //so we dont have to wait a full 8 seconds at startup
  state.last_frame_elapsed_millis = SDL_GetTicks();


  if(!chip8_init(&state.vm, input)) {
    SDL_Log("Failed to initialize Chip8 Emulator!");
    fclose(input);

    return SDL_APP_FAILURE;
  }

  fclose(input);
  


  //when initialized, make SDL keep a pointer to our app state.
  //This pointer will be passed into all of SDL's callback functions, 
  //so we will be able to track program state without global variables.
  *appstate = &state;





  /* SDL_OpenAudioDeviceStream starts the device paused. You have to tell it to start! */
  //SDL_ResumeAudioStreamDevice(stream);


  return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  struct app_state *state = appstate;

  switch(event->type) {
    case SDL_EVENT_KEY_DOWN: {
      enum chip8_key key;
      if(sdl_key_to_chip8_key(&event->key, &key)) {
        chip8_set_key(&state->vm, key);
      } else if(event->key.scancode == SDL_SCANCODE_ESCAPE) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
      }
        //ignore all other keypresses

      break;
    }

    case SDL_EVENT_KEY_UP: {
      enum chip8_key key;
      if(sdl_key_to_chip8_key(&event->key, &key)) {
        chip8_remove_key(&state->vm, key);
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
SDL_AppResult SDL_AppIterate(void *appstate)
{
  struct app_state *state = appstate;


  //update our timers
  uint64_t time_elapsed_millis = SDL_GetTicks();
  uint64_t delta = time_elapsed_millis - state->last_frame_elapsed_millis;
  state->last_frame_elapsed_millis = time_elapsed_millis;

  if(state->use_io_test) {
    state->timer1 += delta;
    state->timer2 += delta;

    // every x milliseconds, randomize the pixels in the framebuffer
    if(state->timer1 > 25) {
      state->timer1 = 0; //reset timer
      randomize_fb(state);
    }


    //every 4 seconds, replay the beep sound
    if(state->timer2 > 8000) {
      state->timer2 = 0;

      //gives us approx 4 seconds of beep
      state->vm.sound_timer = 255;
    }
  }

  

  //Update the 60 Hz timer along with the VM's delay_timer and sound_timer registers
  // 60Hz = every 16 milliseconds
  chip8_update_timer(&state->vm, delta);

  if(state->vm.sound_timer != 0) {
    SDL_ResumeAudioStreamDevice(stream);
  } else {
    SDL_PauseAudioStreamDevice(stream);
  }


  //play audio (once it unpauses)

  /* see if we need to feed the audio stream more data yet.
    We're being lazy here, but if there's less than half a second queued, generate more.
    A square wave is unchanging audio--easy to stream--but for video games, you'll want
    to generate significantly _less_ audio ahead of time! */
  const int minimum_audio = (8000 * sizeof (float)) / 2;  /* 8000 float samples per second. Half of that. */
  if (SDL_GetAudioStreamQueued(stream) < minimum_audio) {
    add_more_audio();
  }




  //render
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

  draw_chip8(state, x, y);


  x = ( (w / scale) - ((SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + DEBUG_KEY_SPACE_BETWEEN_CHARS) * 16)) / 2; //center horizontally
  y = 7 * ( (h / scale) - (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + DEBUG_KEY_SPACE_BETWEEN_CHARS)) / 8; //top 3/4th of screen
  //
  draw_chip8_debug_keys(state, x, y);

  SDL_RenderPresent(renderer);



  // process chip8
  if(!chip8_process_instruction(&state->vm)) {
    SDL_Log("Cannot process instruction at address %d", state->vm.ram[state->vm.pc]);
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
