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



void load_cubemap(uint32_t * cubemap, const char * format) {
    glGenTextures(6,cubemap);
    int len = strlen(format);
    char buf[len];
    for (int i=0; i<6; i++) {
        sprintf(buf, format, i);
        load_texture(cubemap[i], buf);
    }
}


///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    init_screen("Cubemap test renderer");
    
    uint32_t cubepixs[6];
    load_cubemap(cubepixs, "img/cubemap%d.png");
    
    // mainloop
    while (!quit) {
        Timer t;
        if (moves) {
            for (int i=0; i<6; i++) {
                draw_cubemap(cubepixs[i], i);
            }
            draw_box();
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
