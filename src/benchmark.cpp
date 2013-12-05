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

#include "timing.h"
#include "events.h"
#include "art.h"
#include "octree.h"

using namespace std;

struct Scene {
    const char * filename;
    glm::dvec3 position;
    glm::dmat3 orientation;
};

const static Scene scene [] = {
    {"sibenik",  glm::dvec3(         0,          0,          0),  glm::dmat3(-0.119, -0.430, -0.895,   0.249,  0.860, -0.446,   0.961, -0.275,  0.005)},
    {"sibenik",  glm::dvec3( -12398374,   -8116292,    2362616),  glm::dmat3(-0.275,  0.188,  0.943,   0.091,  0.981, -0.170,  -0.957,  0.039, -0.286)},
    {"sibenik",  glm::dvec3(  13429612,   -8589723,     711460),  glm::dmat3(-0.231, -0.166, -0.959,  -0.815,  0.572,  0.097,   0.532,  0.803, -0.267)},
    {"test",     glm::dvec3(         0,          0,          0),  glm::dmat3( 0.746,  0.666, -0.004,  -0.001, -0.005, -1.000,  -0.666,  0.746, -0.003)},
    {"test",     glm::dvec3(-236692088, -262056697, -230541618),  glm::dmat3( 0.480,  0.287,  0.829,  -0.015,  0.947, -0.320,  -0.877,  0.141,  0.459)},
    {"test",     glm::dvec3(-243394570, -264601303, -227131863),  glm::dmat3( 0.990,  0.101,  0.099,  -0.050,  0.906, -0.421,  -0.132,  0.412,  0.902)},
    {"tower",    glm::dvec3(         0,          0,          0),  glm::dmat3( 0.203,  0.145,  0.968,   0.882, -0.456, -0.117,   0.425,  0.878, -0.220)},
    {"tower",    glm::dvec3(   7770173,    -171475,    -586533),  glm::dmat3( 0.982,  0.156, -0.109,  -0.064, -0.268, -0.961,  -0.179,  0.951, -0.253)},
    {"tower",    glm::dvec3( -20197393,   -6116918,   15116412),  glm::dmat3( 0.390,  0.223, -0.893,  -0.707, -0.549, -0.446,  -0.590,  0.805, -0.056)},
    {"mounaloa", glm::dvec3( -79664186, -138753655,  -30086212),  glm::dmat3( 0.007, -0.900, -0.435,  -0.181,  0.427, -0.886,   0.984,  0.085, -0.160)},
    {"mounaloa", glm::dvec3(-154999273, -250811876,   24605684),  glm::dmat3(-0.201, -0.307, -0.930,  -0.452,  0.871, -0.190,   0.869,  0.382, -0.314)},
    {"mounaloa", glm::dvec3( -85871897, -256201957,   19700443),  glm::dmat3(-0.026,  0.332,  0.943,  -0.314,  0.893, -0.323,  -0.949, -0.305,  0.081)},
    {"sponge",   glm::dvec3(  27612764,  -73668610,  146247443),  glm::dmat3( 0.206, -0.427, -0.880,  -0.243,  0.849, -0.469,   0.948,  0.311,  0.071)},
    {"sponge",   glm::dvec3(-105770856, -141629176,  214304513),  glm::dmat3( 0.617, -0.780,  0.105,  -0.629, -0.569, -0.529,   0.472,  0.261, -0.842)},
    {"sponge",   glm::dvec3(-106040634, -140228475,  218791417),  glm::dmat3(-0.211, -0.750,  0.627,  -0.848,  0.460,  0.265,  -0.487, -0.475, -0.733)},
};
const static int scenes = sizeof(scene)/sizeof(scene[0]);
const static int N = 5;
double results[scenes];
///////////////////////////////////////////////////////////////////////////////
int main() {
    init_screen("Voxel renderer - benchmark");
    
    // mainloop
    for (int i=0; i<scenes; i++) {
        char infile[32];
        sprintf(infile, "vxl/%s.oct", scene[i].filename);
        octree_file in(infile);
        position = scene[i].position;
        orientation = scene[i].orientation;
        double times[N];
        for (int j=0; j<N; j++) {
            Timer t;
            octree_draw(&in);
            flip_screen();
            times[j] = t.elapsed();
            next_frame(times[j]);
        }
        printf("Test %2d:", i);
        for (int j=0; j<N; j++) {
            printf(" %7.2f", times[j]);
        }
        printf("\n");
        fflush(stdout);
        std::sort(times,times+N);
        results[i]=0;
        for (int j=1; j<N-1; j++) {
            results[i] += times[j];
        }
        results[i] /= N-2;
    }

    printf("\nBenchmark results:");
    for (int i=0; i<scenes; i++) {
        printf(" %7.2f", results[i]);
    }
    printf("\n");
    return 0;
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
