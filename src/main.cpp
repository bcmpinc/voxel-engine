#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <SDL.h>
#include <SDL_image.h>
#include <GL/gl.h>

#include "timing.h"
#include "events.h"
#include "art.h"
#include "octree.h"

using namespace std;


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
    
    uint32_t cubemap; // = load_cubemap("img/cubemap%d.png");
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // mainloop
    while (!quit) {
        Timer t;
        if (moves) {
            octree_draw(&in, cubemap);
            draw_cubemap(cubemap);
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
