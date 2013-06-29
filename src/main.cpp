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

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    init_screen("Voxel renderer");
    
    position = glm::dvec3(0, -1000000, 0);
    octree * root = init_octree();
    
    // mainloop
    while (!quit) {
        Timer t;
        if (moves) {
            draw_octree(root);
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
