/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2013,2014,2015  B.J. Conijn <bcmpinc@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SDL2/SDL.h>

#include "art.h"

using glm::min;
using glm::max;

// The screen surface
static SDL_Window *screen = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

surface surf;

static void sdl_die(const char * message) {
    fprintf (stderr, "%s: %s\n", message, SDL_GetError ());
    exit(2);
}

void init_screen(const char * caption) {
    // Initialize SDL 
    if (SDL_Init (SDL_INIT_VIDEO) < 0) sdl_die("Couldn't initialize SDL");
    atexit (SDL_Quit);
#if defined _WIN32 || defined _WIN64
    freopen( "CON", "w", stdout );
    freopen( "CON", "w", stderr );
#endif

    // Set 32-bits video mode
    if (SCREEN_FULLSCREEN) {
        screen = SDL_CreateWindow(caption, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
        SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    } else {
        screen = SDL_CreateWindow(caption, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    }
    if (screen == NULL) sdl_die("Couldn't set video mode");
    
    renderer = SDL_CreateRenderer(screen, -1, 0);
    if (renderer == NULL) sdl_die("Couldn't create renderer");
    if (SCREEN_FULLSCREEN) {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
        SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (texture == NULL) sdl_die("Couldn't create texture");

    // set the pixel pointer
    surf = surface(SCREEN_WIDTH, SCREEN_HEIGHT, true);
}

void flip_screen() {
    SDL_UpdateTexture(texture, NULL, surf.data, SCREEN_WIDTH * sizeof (uint32_t));
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
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
        if (x<SCREEN_WIDTH && y<SCREEN_HEIGHT) surf.pixel(x,y,c);
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

void draw_box(glm::dmat3 orientation) {
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

surface get_screen() {
    return surf;
}

namespace frustum {
    // Compute frustum parameters.
    // left, right, top and bottom are the bounds of the near plane.
    const int left   = -SCREEN_WIDTH/2;
    const int right  =  SCREEN_WIDTH/2;
    const int top    =  SCREEN_HEIGHT/2;
    const int bottom = -SCREEN_HEIGHT/2;
    const int near   =  SCREEN_HEIGHT; 
    const int cubepos=  SCREEN_WIDTH; // > sqrt(3)*SCREEN_WIDTH > hypot(SCREEN_WIDTH,SCREEN_HEIGHT,SCREEN_HEIGHT) > max dist of view plane.
    const int far    =  SCREEN_WIDTH * 2; // > sqrt(3)*cubepos 
}

view_pane get_view_pane() {
    view_pane r;
    r.left   = frustum::left   / (double)frustum::near;
    r.right  = frustum::right  / (double)frustum::near;
    r.top    = frustum::top    / (double)frustum::near;
    r.bottom = frustum::bottom / (double)frustum::near;
    return r;
}
