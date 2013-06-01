#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <SDL.h>
#include <SDL_image.h>

#include "timing.h"
#include "events.h"
#include "art.h"

void init_octree();
void draw_octree();

using namespace std;

SDL_PixelFormat fmt = {
  NULL,
  32,
  4,
  0,0,0,0,
  16,8,0,24,
  0xff0000, 0xff00, 0xff, 0xff000000,
  0,
  0
};

SDL_Surface * cubemap[6];
void load_cubemap() {
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    cubemap[0] = SDL_ConvertSurface(IMG_Load("img/cubemap0.png"), &fmt, SDL_SWSURFACE);
    cubemap[1] = SDL_ConvertSurface(IMG_Load("img/cubemap1.png"), &fmt, SDL_SWSURFACE);
    cubemap[2] = SDL_ConvertSurface(IMG_Load("img/cubemap2.png"), &fmt, SDL_SWSURFACE);
    cubemap[3] = SDL_ConvertSurface(IMG_Load("img/cubemap3.png"), &fmt, SDL_SWSURFACE);
    cubemap[4] = SDL_ConvertSurface(IMG_Load("img/cubemap4.png"), &fmt, SDL_SWSURFACE);
    cubemap[5] = SDL_ConvertSurface(IMG_Load("img/cubemap5.png"), &fmt, SDL_SWSURFACE);
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    init_screen("Voxel renderer");
    
    position = glm::dvec3(1<<19, 1<<17, 3<<17);

    //init_octree();
    
    load_cubemap();

    // mainloop
    while (!quit) {
        Timer t;
        if (moves) {
            clear_screen(0x8080b0);
            draw_cubemap(cubemap);
            draw_box();
            //draw_axis();
            //draw_cube();
            //draw_octree();
            flip_screen();
            
            glm::dvec3 eye(orientation[2]);
            printf("%6.2f | %lf %lf %lf | %.3lf %.3lf %.3lf \n", t.elapsed(), 
                   position.x,position.y,position.z, eye.x,eye.y,eye.z);
            fflush(stdout);
        }
        next_frame(t.elapsed());
        handle_events();
    }
    
    return 0;
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
