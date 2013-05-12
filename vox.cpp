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

glm::dmat4 view;

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
static const int MILLISECONDS_PER_FRAME = 50;
static const double rotatespeed = 0.3, movespeed = 1<<12;  

// checks user input
static void pollevent() {
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
                view = glm::rotate(
                    view, 
                    rotatespeed * event.motion.xrel,
                    glm::dvec3(view[1])
                );
                view = glm::rotate(
                    view, 
                    rotatespeed * event.motion.yrel,
                    glm::dvec3(view[0])
                );
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
    double dist = movespeed;
    if (button_state[button::SHIFT]) {
        dist *= 16;
    }
    
    if (button_state[button::W]) {
        view = glm::translate(view, dist*glm::dvec3(view[2]));
        moves=true;
    }
    if (button_state[button::S]) {
        view = glm::translate(view, -dist*glm::dvec3(view[2]));
        moves=true;
    }
    if (button_state[button::A]) {
        view = glm::translate(view, -dist*glm::dvec3(view[0]));
        moves=true;
    }
    if (button_state[button::D]) {
        view = glm::translate(view, dist*glm::dvec3(view[0]));
        moves=true;
    }
    if (button_state[button::C]) {
        view = glm::translate(view, glm::dvec3(0,-dist,0));
        moves=true;
    }
    if (button_state[button::SPACE]) {
        view = glm::translate(view, glm::dvec3(0,dist,0));
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
    
    view = glm::translate(view, glm::dvec3(1<<19, 1<<17, 3<<17));

    init();

    // mainloop
    while (!quit) {
        Timer t;
        if (moves) {
            //SDL_FillRect(screen,NULL, 0x000000);
            draw();
            SDL_Flip (screen);
            glm::dvec3 pos(view[3]);
            glm::dvec3 eye(view[2]);
            printf("%6.2f | %lf %lf %lf | %.3lf %.3lf %.3lf \n", t.elapsed(), 
                   pos.x,pos.y,pos.z, eye.x,eye.y,eye.z);
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
