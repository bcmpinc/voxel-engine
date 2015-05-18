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
    uint32_t * hblur = new uint32_t[surf.width * surf.height];
    uint32_t * blur = new uint32_t[surf.width * surf.height];

    // mainloop
    while (!quit) {
        Timer t;
        if (moves) {
            clear_screen();
            octree_draw(&in, surf, get_view_pane(),position, orientation);
            //draw_box(orientation);
            
            uint32_t line = 0;
            for (uint32_t y=0; y<surf.height; y++) {
                uint32_t acc = surf.depth[line+0]+surf.depth[line+1]+surf.depth[line+2];
                hblur[line+0] = acc / 3;
                acc += surf.depth[line+3];
                hblur[line+1] = acc / 4;
                for (uint32_t x=4; x<surf.width; x++) {
                    acc += surf.depth[line+x];
                    hblur[line+x-2] = acc / 5;
                    acc -= surf.depth[line+x-4];
                }
                line += surf.width;
                hblur[line-2] = acc / 4;
                acc += surf.depth[line-4];
                hblur[line-1] = acc / 3;
            }
            uint32_t W = surf.width;
            uint32_t H = surf.height;
            for (uint32_t x=0; x<surf.width; x++) {
                uint32_t acc = hblur[x+0*W]+hblur[x+1*W]+hblur[x+2*W];
                blur[x+0*W] = acc / 3;
                acc += hblur[x+3*W];
                blur[x+1*W] = acc / 4;
                for (uint32_t y=4; y<surf.height; y++) {
                    acc += hblur[x+y*W];
                    blur[x+(y-2)*W] = acc / 5;
                    acc -= hblur[x+(y-4)*W];
                }
                blur[x+(H-2)*W] = acc / 4;
                acc += hblur[x+(H-4)*W];
                blur[x+(H-1)*W] = acc / 3;
            }
            for (uint32_t y=0; y<surf.height; y++) {
                for (uint32_t x=0; x<surf.width; x+=4) {
                    int i = x+y*W;
                    __m128i diff = _mm_sub_epi32(_mm_load_si128((__m128i*)(surf.depth+i)), _mm_load_si128((__m128i*)(blur+i)));
                    diff = _mm_srai_epi32(diff, 12);
                    diff = _mm_max_epi32(diff, _mm_set1_epi32(-60));
                    diff = _mm_min_epi32(diff, _mm_set1_epi32( 60));
                    diff = _mm_shuffle_epi8(diff, _mm_set_epi8(12,12,12,12,8,8,8,8,4,4,4,4,0,0,0,0));
                    __m128i col = _mm_add_epi8(_mm_load_si128((__m128i*)(surf.data+i)), _mm_set1_epi8(128));
                    col = _mm_subs_epi8(col, diff);
                    col = _mm_add_epi8(col, _mm_set1_epi8(128));
                    _mm_store_si128((__m128i*)(surf.data+i), col);
                    _mm_store_si128((__m128i*)(surf.depth+i), _mm_set1_epi8(-1));
                }
            }
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
