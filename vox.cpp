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

position pos={0,0,-16};

// The screen surface
static SDL_Surface *screen = NULL;

// Quiting?
static bool quit = false;

// Buttons
class button {
    public:
    enum {
        W, A, S, D,
        SPACE, C,
        
        STATES
    };
};
static bool button_state[button::STATES];
static bool mousemove=false;
static bool moves=true;

// Position
static const int MILLISECONDS_PER_FRAME = 50;
static const float rotatespeed = 180, movespeed = 10;  
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
    const double dt = 1.0/60.0;
    moves=false;
    float dist=movespeed * dt;
    
    if (button_state[button::W]) {
        pos.x -= pos.crho * pos.sphi * dist;
        pos.y += pos.srho * dist;
        pos.z += pos.crho * pos.cphi * dist;
        moves=true;
    }
    if (button_state[button::S]) {
        pos.x += pos.crho * pos.sphi * dist;
        pos.y -= pos.srho * dist;
        pos.z -= pos.crho * pos.cphi * dist;
        moves=true;
    }
    if (button_state[button::A]) {
        pos.x -= pos.cphi * dist;
        pos.z -= pos.sphi * dist;
        moves=true;
    }
    if (button_state[button::D]) {
        pos.x += pos.cphi * dist;
        pos.z += pos.sphi * dist;
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
    //freopen( "CON", "w", stdout );
    //freopen( "CON", "w", stderr );

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
        pos.sphi = sin(phi*pid180);
        pos.cphi = cos(phi*pid180);
        pos.srho = sin(rho*pid180);
        pos.crho = cos(rho*pid180);

        Timer t;
        if (moves) {
            //SDL_FillRect(screen,NULL, 0x000000);
            draw();
            SDL_Flip (screen);
            printf("%4.2f\n", t.elapsed());
        }
        if (t.elapsed()-10<=MILLISECONDS_PER_FRAME) {
            SDL_Delay((int)(MILLISECONDS_PER_FRAME-t.elapsed()));
        }
        sim();
        pollevent();
    }
    
    return 0;
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
