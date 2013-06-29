#include <SDL.h>
#include <SDL_image.h>

#include "events.h"
#include "art.h"

using glm::min;
using glm::max;

namespace {
  // The screen surface
  SDL_Surface *screen = NULL;
  
  // pointer to the pixels (32 bit)
  int * pixs;
}

void init_screen(const char * caption) {
    // Initialize SDL 
    if (SDL_Init (SDL_INIT_VIDEO) < 0) {
        fprintf (stderr, "Couldn't initialize SDL: %s\n", SDL_GetError ());
        exit (2);
    }
    atexit (SDL_Quit);
#if defined _WIN32 || defined _WIN64
    freopen( "CON", "w", stdout );
    freopen( "CON", "w", stderr );
#endif

    // Set 32-bits video mode (eventually emulated)
    screen = SDL_SetVideoMode (SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);
    if (screen == NULL) {
        fprintf (stderr, "Couldn't set video mode: %s\n", SDL_GetError ());
        exit (3);
    }
    SDL_WM_SetCaption (caption, NULL);

    // set the pixel pointer
    pixs=(int*)screen->pixels;  
}

void flip_screen() {
    SDL_Flip (screen);
}

void pix(uint32_t x, uint32_t y, uint32_t c) {
    if (x>=0 && y>=0 && x<SCREEN_WIDTH && y<SCREEN_HEIGHT) {
        int64_t i = x+y*(SCREEN_WIDTH);
        pixs[i] = c;
    }
}
#define CLAMP(x,l,u) (x<l?l:x>u?u:x)
uint32_t rgb(uint32_t r, uint32_t g, uint32_t b) {
    return CLAMP(r,0,255)<<16|CLAMP(g,0,255)<<8|CLAMP(b,0,255);
}
uint32_t rgb(float r, float g, float b) {
    return rgb((uint32_t)(r+0.5),(uint32_t)(g+0.5),(uint32_t)(b+0.5));
}

