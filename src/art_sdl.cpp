#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

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
    screen = SDL_SetVideoMode (SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE | SDL_DOUBLEBUF | (SCREEN_FULLSCREEN*SDL_FULLSCREEN));
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
    SDL_FillRect(screen,NULL,0xaaccff);
}

void pixel(uint32_t x, uint32_t y, uint32_t c) {
    if (x>=0 && y>=0 && x<SCREEN_WIDTH && y<SCREEN_HEIGHT) {
        int64_t i = x+y*(SCREEN_WIDTH);
        pixs[i] = c;
    } else abort();
}

/** Draws a line. */
static void line(double x1, double y1, double x2, double y2, int c) {
    if (x1<0) { 
        if (x2<0) return;
        y1 = (y2*(0-x1) + y1*(x2-0))/(x2-x1);
        x1 = 0;
    }
    if (x2<0) { 
        y2 = (y1*(0-x2) + y2*(x1-0))/(x1-x2);
        x2 = 0;
    }
    if (x1>SCREEN_WIDTH) { 
        if (x2>SCREEN_WIDTH) return;
        y1 = (y2*(SCREEN_WIDTH-x1) + y1*(x2-SCREEN_WIDTH))/(x2-x1);
        x1 = SCREEN_WIDTH;
    }
    if (x2>SCREEN_WIDTH) { 
        y2 = (y1*(SCREEN_WIDTH-x2) + y2*(x1-SCREEN_WIDTH))/(x1-x2);
        x2 = SCREEN_WIDTH;
    }
    
    if (y1<0) { 
        if (y2<0) return;
        x1 = (x2*(0-y1) + x1*(y2-0))/(y2-y1);
        y1 = 0;
    }
    if (y2<0) { 
        x2 = (x1*(0-y2) + x2*(y1-0))/(y1-y2);
        y2 = 0;
    }
    if (y1>SCREEN_HEIGHT) { 
        if (y2>SCREEN_HEIGHT) return;
        x1 = (x2*(SCREEN_HEIGHT-y1) + x1*(y2-SCREEN_HEIGHT))/(y2-y1);
        y1 = SCREEN_HEIGHT;
    }
    if (y2>SCREEN_HEIGHT) { 
        x2 = (x1*(SCREEN_HEIGHT-y2) + x2*(y1-SCREEN_HEIGHT))/(y1-y2);
        y2 = SCREEN_HEIGHT;
    }
    
    int d = (int)(1+max(abs(x1-x2),abs(y1-y2)));
    for (int i=0; i<=d; i++) {
        double x=(x1+(x2-x1)*i/d);
        double y=(y1+(y2-y1)*i/d);
        if (x<SCREEN_WIDTH && y<SCREEN_HEIGHT) pixel(x,y,c);
    }
}

/** draws a 3d line. */
static void line(glm::dvec3 va, glm::dvec3 vb, int c) {
    if (va.z<=1e-2 && vb.z<=1e-2) return;
    // Cut of part behind viewer.
    if (va.z<=1e-2) {
        double w = (1e-2-va.z)/(vb.z-va.z);
        va = vb*w + va*(1-w);
    }
    if (vb.z<=1e-2) {
        double w = (1e-2-vb.z)/(va.z-vb.z);
        vb = va*w + vb*(1-w);
    }
    
    int64_t pxa = SCREEN_WIDTH/2  + va.x*SCREEN_HEIGHT/va.z;
    int64_t pya = SCREEN_HEIGHT/2 - va.y*SCREEN_HEIGHT/va.z;
    int64_t pxb = SCREEN_WIDTH/2  + vb.x*SCREEN_HEIGHT/vb.z;
    int64_t pyb = SCREEN_HEIGHT/2 - vb.y*SCREEN_HEIGHT/vb.z;
    
    line(pxa,pya, pxb,pyb, c);
}

void draw_box() {
    glm::dvec3 vertices[8]={
        glm::dvec3(-1,-1,-1),
        glm::dvec3( 1,-1,-1),
        glm::dvec3(-1, 1,-1),
        glm::dvec3( 1, 1,-1),
        glm::dvec3(-1,-1, 1),
        glm::dvec3( 1,-1, 1),
        glm::dvec3(-1, 1, 1),
        glm::dvec3( 1, 1, 1),
    };
    for (int a=0; a<8; a++) {
        vertices[a] = orientation * vertices[a];
    }
    for (int a=0; a<8; a++) {
        for (int b=0; b<8; b++) {
            int c = a^b;
            glm::dvec3 va = vertices[a];
            glm::dvec3 vb = vertices[b];
            switch(c) {
                case 1:
                case 2:
                case 4:
                    line(va,vb, 0x000000);
                    break;
                case 3:
                    line(va,vb, 0x0000fe>>!(a&b));
                    break;
                case 5:
                    line(va,vb, 0x00fe00>>!(a&b));
                    break;
                case 6:
                    line(va,vb, 0xfe0000>>!(a&b));
                    break;
            }
        }
    }
}

