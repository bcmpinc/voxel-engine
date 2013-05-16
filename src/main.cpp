#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <SDL/SDL.h>

#include "timing.h"
#include "events.h"
#include "art.h"

void init_octree();
void draw_octree();

using namespace std;

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    init_screen("Voxel renderer");
    
    position = glm::dvec3(1<<19, 1<<17, 3<<17);

    init_octree();

    // mainloop
    while (!quit) {
        Timer t;
        if (moves) {
            clear_screen(0x8080b0);
            draw_box();
            draw_axis();
            draw_octree();
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
