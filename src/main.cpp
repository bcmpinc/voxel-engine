/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2013,2014  B.J. Conijn <bcmpinc@users.sourceforge.net>

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
    bool capture = false;
    const char * filename = nullptr;
    for (int i=1; i<argc; i++) { 
        if (argv[i][0]=='-') {
            if (strcmp(argv[i], "-capture") == 0) {
                capture = true;
            } else {
                fprintf(stderr,"unrecognized option: %s\n", argv[i]);
            }
        } else {
            if (filename) goto usage;
            filename = argv[i];
        }
    }
    if (filename == nullptr) {
        usage:
        fprintf(stderr,"Usage: %s [-capture] octree_file\n", argv[0]);
        exit(2);
    }

    // Determine the file names.
    octree_file in(filename);

    init_screen("Voxel renderer");
    position = glm::dvec3(0, 0, 0);
    Capture c;
    if (capture) {
#ifdef FOUND_LIBAV
        char capturefile[32];
        mkdir("capture",0755);
        sprintf(capturefile, "capture/cap%08d.mp4", getpid());
        c = Capture(capturefile, get_screen());
#else
        fprintf(stderr, "Cannot capture: compiled without libffmpeg\n");
#endif
    }

    surface surf = get_screen();
    surf.depth = new uint32_t[surf.width * surf.height];

    // mainloop
    while (!quit) {
        Timer t;
        if (moves) {
            surf.clear(0xaaccffu);
            octree_draw(&in, surf, get_view_pane(),position, orientation);
            surf.apply_ssao(20,0.1);
            //draw_box(orientation);

            c.shoot();
            flip_screen();
            
            if (false) {
                printf("{\"%s\",  glm::dvec3(%10.0lf, %10.0lf, %10.0lf),  glm::dmat3(%6.3lf, %6.3lf, %6.3lf,  %6.3lf, %6.3lf, %6.3lf,  %6.3lf, %6.3lf, %6.3lf)},\n", filename,
                    position.x,position.y,position.z, 
                    orientation[0].x,orientation[0].y,orientation[0].z,
                    orientation[1].x,orientation[1].y,orientation[1].z,
                    orientation[2].x,orientation[2].y,orientation[2].z
                );
                fflush(stdout);
            }
        }
        next_frame(t.elapsed());
        handle_events();
    }
    return 0;
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
