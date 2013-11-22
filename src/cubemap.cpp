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
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "timing.h"
#include "events.h"
#include "art.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
int main() {
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
