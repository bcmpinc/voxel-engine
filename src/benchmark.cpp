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

#include <map>
#include <string>

#include <sys/stat.h>

#include "timing.h"
#include "art.h"
#include "octree.h"

using namespace std;

struct Scene {
    const char * filename;
    glm::dvec3 position;
    glm::dmat3 orientation;
};

static const int32_t SCENE_DEPTH = 26;
static const double SCALE = 1<<SCENE_DEPTH;

static const Scene scene [] = {
    {"sibenik",  glm::dvec3( 0.0000,  0.0000,  0.0000),  glm::dmat3(-0.119, -0.430, -0.895,   0.249,  0.860, -0.446,   0.961, -0.275,  0.005)},
    {"sibenik",  glm::dvec3(-0.0462, -0.0302,  0.0088),  glm::dmat3(-0.275,  0.188,  0.943,   0.091,  0.981, -0.170,  -0.957,  0.039, -0.286)},
    {"sibenik",  glm::dvec3( 0.0500, -0.0320,  0.0027),  glm::dmat3(-0.231, -0.166, -0.959,  -0.815,  0.572,  0.097,   0.532,  0.803, -0.267)},
    {"test",     glm::dvec3( 0.0000,  0.0000,  0.0000),  glm::dmat3( 0.746,  0.666, -0.004,  -0.001, -0.005, -1.000,  -0.666,  0.746, -0.003)},
    {"test",     glm::dvec3(-0.8817, -0.9762, -0.8588),  glm::dmat3( 0.480,  0.287,  0.829,  -0.015,  0.947, -0.320,  -0.877,  0.141,  0.459)},
    {"test",     glm::dvec3(-0.9067, -0.9857, -0.8461),  glm::dmat3( 0.990,  0.101,  0.099,  -0.050,  0.906, -0.421,  -0.132,  0.412,  0.902)},
    {"tower",    glm::dvec3( 0.0000,  0.0000,  0.0000),  glm::dmat3( 0.203,  0.145,  0.968,   0.882, -0.456, -0.117,   0.425,  0.878, -0.220)},
    {"tower",    glm::dvec3( 0.0289, -0.0006, -0.0022),  glm::dmat3( 0.982,  0.156, -0.109,  -0.064, -0.268, -0.961,  -0.179,  0.951, -0.253)},
    {"tower",    glm::dvec3(-0.0752, -0.0228,  0.0563),  glm::dmat3( 0.390,  0.223, -0.893,  -0.707, -0.549, -0.446,  -0.590,  0.805, -0.056)},
    {"mounaloa", glm::dvec3(-0.2968, -0.5169, -0.1121),  glm::dmat3( 0.007, -0.900, -0.435,  -0.181,  0.427, -0.886,   0.984,  0.085, -0.160)},
    {"mounaloa", glm::dvec3(-0.5774, -0.9343,  0.0917),  glm::dmat3(-0.201, -0.307, -0.930,  -0.452,  0.871, -0.190,   0.869,  0.382, -0.314)},
    {"mounaloa", glm::dvec3(-0.3199, -0.9544,  0.0734),  glm::dmat3(-0.026,  0.332,  0.943,  -0.314,  0.893, -0.323,  -0.949, -0.305,  0.081)},
    {"sponge",   glm::dvec3( 0.1029, -0.2744,  0.5448),  glm::dmat3( 0.206, -0.427, -0.880,  -0.243,  0.849, -0.469,   0.948,  0.311,  0.071)},
    {"sponge",   glm::dvec3(-0.3940, -0.5276,  0.7983),  glm::dmat3( 0.617, -0.780,  0.105,  -0.629, -0.569, -0.529,   0.472,  0.261, -0.842)},
    {"sponge",   glm::dvec3(-0.3950, -0.5224,  0.8151),  glm::dmat3(-0.211, -0.750,  0.627,  -0.848,  0.460,  0.265,  -0.487, -0.475, -0.733)},
};

static const int scenes = sizeof(scene)/sizeof(scene[0]);
static const int N = 0;
static double results[scenes];

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char ** argv) {
    init_screen("Voxel renderer - benchmark");
    if (argc>=2) {
        mkdir("bshots",0755);
    }

    surface fsaa(new uint32_t[SCREEN_WIDTH*SCREEN_HEIGHT*16], SCREEN_WIDTH*4, SCREEN_HEIGHT*4);
    
    // mainloop
    for (int i=0; i<scenes; i++) {
        // Load file
        char infile[64];
        sprintf(infile, "../vxl/%s.oc2", scene[i].filename);
        octree_file in(infile);
        
        // Set camera
        glm::dvec3 position = scene[i].position * SCALE;
        glm::dmat3 orientation = scene[i].orientation;

        // Run tests
        double times[N];
        for (int j=-1; j<N; j++) {
            Timer t;
            //clear_screen();
            std::fill_n(fsaa.data, fsaa.width*fsaa.height, 0xaaccffu);
            octree_draw(&in, fsaa, get_view_pane(), position, orientation);
            surface s = get_screen();
            for (int y=0; y<SCREEN_HEIGHT; y++) {
                for (int x=0; x<SCREEN_WIDTH; x++) {
                    uint64_t c = 0x0200020002;
                    c += (fsaa.data[x*4+0+(y*4+0)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+1+(y*4+0)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+2+(y*4+0)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+3+(y*4+0)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+0+(y*4+1)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+1+(y*4+1)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+2+(y*4+1)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+3+(y*4+1)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+0+(y*4+2)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+1+(y*4+2)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+2+(y*4+2)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+3+(y*4+2)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+0+(y*4+3)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+1+(y*4+3)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+2+(y*4+3)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c += (fsaa.data[x*4+3+(y*4+3)*SCREEN_WIDTH*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                    c >>= 4;
                    c &= 0x0000ff0000ff00ff;
                    c |= c >> 32;
                    c &= 0xffffff;
                    s.pixel(x,y,c);
                }
            }
            flip_screen();
            if (j>=0) {
                times[j] = t.elapsed();
            }
        }
        
        if (N>0) {
            // Print results
            printf("Test %2d:", i);
            for (int j=0; j<N; j++) {
                printf(" %7.2f", times[j]);
            }
        }
        
        if (N>2) {
            // Compute average
            std::sort(times,times+N);
            results[i]=0;
            for (int j=1; j<N-1; j++) {
                results[i] += times[j];
            }
            results[i] /= N-2;
            printf(" | %7.2f\n", results[i]);
        }
        fflush(stdout);
        
        // Output png
        if (argc>=2) {
            char outfile[64];
            sprintf(outfile, "bshots/%.10s-%02d-%s.png", argv[1], i, scene[i].filename);
            get_screen().export_png(outfile);
        }
    }

    printf("\nBenchmark results:");
    double sum = 0;
    for (int i=0; i<scenes; i++) {
        printf(" %7.2f", results[i]);
        sum += results[i];
    }
    printf(" | %7.2f\n", sum/scenes);
    return 0;
}

