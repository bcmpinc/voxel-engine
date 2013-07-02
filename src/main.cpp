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
#include "octree.h"

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

void load_cubemap(SDL_Surface ** cubemap) {
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
    if (argc != 2) {
        fprintf(stderr,"Please specify the file to load (without 'vxl/' & '.oct').\n");
        exit(2);
    }

    // Determine the file names.
    char * name = argv[1];
    int length=strlen(name);
    char infile[length+9];
    sprintf(infile, "vxl/%s.oct", name);
    octree_file in(infile);

    init_screen("Voxel renderer");
    position = glm::dvec3(0, -1000000, 0);
    
    SDL_Surface * cubemap[6];
    load_cubemap(cubemap);
    uint32_t * cubepixs[6];
    for (int i=0; i<6; i++) {
        cubepixs[i] = new uint32_t[1<<20];
        for (int y=0; y<1024; y++) {
            for (int x=0; x<1024; x++) {
                cubepixs[i][x+(1023-y)*1024] = ((uint32_t*)cubemap[i]->pixels)[(x>>2)+(y>>2)*256];
            }
        }
    }
    
    // mainloop
    while (!quit) {
        Timer t;
        if (moves) {
            octree_draw(cubepixs, in.root);
            flip_screen();
            
            glm::dvec3 eye(orientation[2]);
            //printf("%6.2f | %lf %lf %lf | %.3lf %.3lf %.3lf \n", t.elapsed(), 
            //       position.x,position.y,position.z, eye.x,eye.y,eye.z);
            fflush(stdout);
        }
        next_frame(t.elapsed());
        handle_events();
    }
    
    return 0;
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
