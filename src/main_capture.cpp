/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2013  B.J. Conijn <bcmpinc@users.sourceforge.net>

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

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

#include "timing.h"
#include "events.h"
#include "art.h"
#include "octree.h"
#include "capture.h"

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

    init_screen("Voxel capture renderer");
    position = glm::dvec3(0, -1000000, 0);
    
    uint32_t cubemap = prepare_cubemap();
    
    char capturefile[32];
    mkdir("capture",0755);
    sprintf(capturefile, "capture/cap%08d.mp4", getpid());
    capture_start(capturefile);
    
    // mainloop
    while (!quit) {
        Timer t;
        if (moves) {
            octree_draw(&in, cubemap);
            draw_cubemap(cubemap);
            flip_screen();
            capture_shoot(cubemap);
            
            glm::dvec3 eye(orientation[2]);
            //printf("%6.2f | %lf %lf %lf | %.3lf %.3lf %.3lf \n", t.elapsed(), 
            //       position.x,position.y,position.z, eye.x,eye.y,eye.z);
            fflush(stdout);
        }
        next_frame(t.elapsed());
        handle_events();
    }
    
    capture_end();
    return 0;
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
