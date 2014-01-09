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

#include "timing.h"
#include "events.h"
#include "art.h"
#include "octree.h"

using namespace std;


///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr,"Usage: %s octree_file\n", argv[0]);
        exit(2);
    }

    // Determine the file names.
    const char * filename = argv[1];
    octree_file in(filename);

    init_screen("Voxel renderer");
    position = glm::dvec3(0, 0, 0);
    
    // mainloop
    while (!quit) {
        Timer t;
        if (moves) {
            clear_creen();
            octree_draw(&in);
            //draw_box();
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
