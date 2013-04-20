#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <SDL/SDL.h>
#include "timing.h"
#include "common.h"

using namespace std;

// pointer to the pixels (32 bit)
int * pixs;

position pos = {1<<19, 1<<17, 3<<17};

// The screen surface
static SDL_Surface *screen = NULL;

// Quiting?
static bool quit = false;

// Buttons
class button {
    public:
    enum {
        W, A, S, D,
        SPACE, C, SHIFT,
        
        STATES
    };
};
static bool button_state[button::STATES];
static bool mousemove=false;
static bool moves=true;

// Position
static const int ROTATIONAL_BITS = 16;
static const int MILLISECONDS_PER_FRAME = 50;
static const float rotatespeed = 180, movespeed = 1<<12;  
static float phi, rho;
static const float pid180=3.1415926535/180;

// checks user input
void pollevent() {
    SDL_Event event;
  
    /* Check for events */
    while (SDL_PollEvent (&event)) {
        switch (event.type) {
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            bool state = (event.type == SDL_KEYDOWN);
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    quit = true;
                    break;
                case SDLK_w:
                    button_state[button::W] = state;
                    break;
                case SDLK_a:
                    button_state[button::A] = state;
                    break;
                case SDLK_s:
                    button_state[button::S] = state;
                    break;
                case SDLK_d:
                    button_state[button::D] = state;
                    break;
                case SDLK_c:
                    button_state[button::C] = state;
                    break;
                case SDLK_SPACE:
                    button_state[button::SPACE] = state;
                    break;
                case SDLK_LSHIFT:
                    button_state[button::SHIFT] = state;
                    break;
                default:
                    break;
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            SDL_ShowCursor(mousemove);
            mousemove=!mousemove;
            SDL_WM_GrabInput(mousemove?SDL_GRAB_ON:SDL_GRAB_OFF);
            break;
        }
        case SDL_MOUSEMOTION: {
            if (mousemove) {
                phi -= event.motion.xrel*0.3;
                rho -= event.motion.yrel*0.3;
                if (rho<-90) rho=-90;
                if (rho>90) rho=90;
                moves=true;
            }
            break;
        }
        case SDL_QUIT:
            quit = true;
            break;
        default:
            break;
        }
    }
}

void sim() {
    moves=false;
    int64_t dist = movespeed;
    if (button_state[button::SHIFT]) {
        dist *= 16;
    }
    
    if (button_state[button::W]) {
        pos.x -= pos.crho * (pos.sphi * dist >> ROTATIONAL_BITS) >> ROTATIONAL_BITS;
        pos.y += pos.srho * dist >> ROTATIONAL_BITS;
        pos.z += pos.crho * (pos.cphi * dist >> ROTATIONAL_BITS) >> ROTATIONAL_BITS;
        moves=true;
    }
    if (button_state[button::S]) {
        pos.x += pos.crho * (pos.sphi * dist >> ROTATIONAL_BITS) >> ROTATIONAL_BITS;
        pos.y -= pos.srho * dist >> ROTATIONAL_BITS;
        pos.z -= pos.crho * (pos.cphi * dist >> ROTATIONAL_BITS) >> ROTATIONAL_BITS;
        moves=true;
    }
    if (button_state[button::A]) {
        pos.x -= pos.cphi * dist >> ROTATIONAL_BITS;
        pos.z -= pos.sphi * dist >> ROTATIONAL_BITS;
        moves=true;
    }
    if (button_state[button::D]) {
        pos.x += pos.cphi * dist >> ROTATIONAL_BITS;
        pos.z += pos.sphi * dist >> ROTATIONAL_BITS;
        moves=true;
    }
    if (button_state[button::C]) {
        pos.y -= dist;
        moves=true;
    }
    if (button_state[button::SPACE]) {
        pos.y += dist;
        moves=true;
    }
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {

    // Initialize SDL 
    if (SDL_Init (SDL_INIT_VIDEO) < 0) {
        fprintf (stderr, "Couldn't initialize SDL: %s\n", SDL_GetError ());
        exit (2);
    }
    atexit (SDL_Quit);
#ifdef WINDOWS_NT
    freopen( "CON", "w", stdout );
    freopen( "CON", "w", stderr );
#endif

    // Set 32-bits video mode (eventually emulated)
    screen = SDL_SetVideoMode (SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);
    if (screen == NULL) {
        fprintf (stderr, "Couldn't set video mode: %s\n", SDL_GetError ());
        exit (3);
    }
    SDL_WM_SetCaption ("Voxel renderer", NULL);

    // set the pixel pointer
    pixs=(int*)screen->pixels;
    
    init();

    // mainloop
    while (!quit) {
        pos.sphi = (1<<ROTATIONAL_BITS)*sin(phi*pid180);
        pos.cphi = (1<<ROTATIONAL_BITS)*cos(phi*pid180);
        pos.srho = (1<<ROTATIONAL_BITS)*sin(rho*pid180);
        pos.crho = (1<<ROTATIONAL_BITS)*cos(rho*pid180);

        Timer t;
        if (moves) {
            //SDL_FillRect(screen,NULL, 0x000000);
            draw();
            SDL_Flip (screen);
            printf("%6.2f | %ld %ld %ld | %3.0f %3.0f | %10ld\n", t.elapsed(),pos.x>>10,pos.y>>10,pos.z>>10, rho, phi, pos.points_rendered);
            fflush(stdout);
        }
        int delay = MILLISECONDS_PER_FRAME-t.elapsed();
        if (delay>10) {
            SDL_Delay(delay);
        }
        sim();
        pollevent();
    }
    
    return 0;
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
