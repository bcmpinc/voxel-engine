#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <SDL.h>
#include <SDL_image.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "timing.h"
#include "events.h"
#include "art.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    init_screen("Cubemap test renderer");
    
    uint32_t cubemap = load_cubemap("img/cubemap%d.png");
    
    // mainloop
    while (!quit) {
        Timer t;
        if (moves) {
            draw_cubemap(cubemap);
            //draw_box();
            flip_screen();
            
            glm::dvec3 eye(orientation[2]);
            printf("%6.2f | %.3lf %.3lf %.3lf \n", t.elapsed(), eye.x,eye.y,eye.z);
            fflush(stdout);
        }
        next_frame(t.elapsed());
        handle_events();
    }
    return 0;
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
