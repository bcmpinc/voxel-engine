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
#include <algorithm>
//#include <GL/gl.h>

#include "art.h"
#include "events.h"
#include "quadtree.h"
#include "timing.h"
#include "octree.h"

#define static_assert(test, message) typedef char static_assert__##message[(test)?1:-1]

using std::max;
using std::min;

namespace {
    quadtree face;
    octree * root;
    int C;
    int count;
}

static_assert(quadtree::SIZE >= SCREEN_HEIGHT, quadtree_height_too_small);
static_assert(quadtree::SIZE >= SCREEN_WIDTH,  quadtree_width_too_small);

// Array with x1, x2, y1, y2. Note that x2-x1 = y2-y1.
typedef int32_t v4si __attribute__ ((vector_size (16)));

const v4si quad_permutation[8] = {
    {},{},{},{},
    {0,0,3,3},{1,1,3,3},{0,0,2,2},{1,1,2,2},
};

static const int32_t SCENE_DEPTH = 26;

static const int DX=4, DY=2, DZ=1;
static const v4si DELTA[8]={
    {-1,-1,-1},
    {-1,-1, 1},
    {-1, 1,-1},
    {-1, 1, 1},
    { 1,-1,-1},
    { 1,-1, 1},
    { 1, 1,-1},
    { 1, 1, 1},
};

const v4si nil = {};

/** Returns true if quadtree node is rendered 
 * Function is assumed to be called only if quadtree node is not yet fully rendered.
 * The bounds array is ordered as DELTA.
 * C is the corner that is furthest away from the camera.
 * Furthermore, pos is the location of the center of the octree node, relative to the viewer in octree space.
 */
static bool traverse(
    const int32_t quadnode, const uint32_t octnode, const uint32_t octcolor, 
    const v4si bound, const v4si dx, const v4si dy, const v4si dz,  
    const v4si pos, const int depth
){    
    v4si ltz;
    v4si gtz;
    v4si new_bound;
    count++;
    // Recursion
    if (depth>=0 && bound[1] - bound[0] <= 2<<SCENE_DEPTH) {
        // Traverse octree
        octree &s = root[octnode];
        v4si octant = -(pos<0);
        int furthest = (octant[0]<<2)|(octant[1]<<1)|(octant[2]<<0);
        for (int k = 0; k<8; k++) {
            int i = furthest^k;
            if (~octnode && s.avgcolor[i]<0) continue;
            new_bound = bound<<1;
            if ((C^i)&DX) new_bound += dx;
            if ((C^i)&DY) new_bound += dy;
            if ((C^i)&DZ) new_bound += dz;
            ltz = (new_bound - (dx<0)*dx - (dy<0)*dy - (dz<0)*dz)<0;
            gtz = (new_bound - (dx>0)*dx - (dy>0)*dy - (dz>0)*dz)>0;
            if ((ltz[0] & gtz[1] & ltz[2] & gtz[3]) == 0) continue; // frustum occlusion
            if (~octnode) {
                if (traverse(quadnode, s.child[i], s.avgcolor[i], new_bound, dx, dy, dz, pos + (DELTA[i]<<depth), depth-1)) return true;
            } else {
                if (traverse(quadnode, ~0u, octcolor, new_bound, dx, dy, dz, pos + (DELTA[i]<<depth), depth-1)) return true;
            }
        }
        return false;
    } else {
        // Traverse quadtree 
        for (int i = 4; i<8; i++) {
            if (!face.map[quadnode*4+i]) continue;
            new_bound = (bound + __builtin_shuffle(bound,quad_permutation[i])) >> 1;
            v4si new_dx = (dx + __builtin_shuffle(dx,quad_permutation[i])) >> 1;
            v4si new_dy = (dy + __builtin_shuffle(dy,quad_permutation[i])) >> 1;
            v4si new_dz = (dz + __builtin_shuffle(dz,quad_permutation[i])) >> 1;
            ltz = (new_bound - (new_dx<0)*new_dx - (new_dy<0)*new_dy - (new_dz<0)*new_dz)<0;
            gtz = (new_bound - (new_dx>0)*new_dx - (new_dy>0)*new_dy - (new_dz>0)*new_dz)>0;
            if ((ltz[0] & gtz[1] & ltz[2] & gtz[3]) == 0) continue; // frustum occlusion
            if (quadnode<(int)quadtree::L)
                traverse(quadnode*4+i, octnode, octcolor, new_bound, new_dx, new_dy, new_dz, pos, depth); 
            else
                face.set_face(quadnode*4+i, octcolor); // Rendering
        }
        if (quadnode>=0) {
            face.compute(quadnode);
            return !face.map[quadnode];
        } else {
            return face.children[0]==0;
        }
    }
}
    
static const double quadtree_bounds[] = {
    frustum::left  /(double)frustum::near,
   (frustum::left + (frustum::right -frustum::left)*(double)quadtree::SIZE/SCREEN_WIDTH )/frustum::near,
   (frustum::top  + (frustum::bottom-frustum::top )*(double)quadtree::SIZE/SCREEN_HEIGHT)/frustum::near,
    frustum::top   /(double)frustum::near,
};

/** Render the octree to the OpenGL cubemap texture. 
 */
void octree_draw(octree_file * file) {
    Timer t_global;
    
    double timer_prepare;
    double timer_query;
    double timer_transfer;
    
    root = file->root;
    
    Timer t_prepare;
        
    // Prepare the occlusion quadtree
    face.build(SCREEN_WIDTH, SCREEN_HEIGHT);
    
    timer_prepare = t_prepare.elapsed();

    Timer t_query;
    count=0;
    // Do the actual rendering of the scene (i.e. execute the query).
    v4si bounds[8];
    int max_z=-1<<31;
    for (int i=0; i<8; i++) {
        // Compute position of octree corners in camera-space
        v4si vertex = DELTA[i]<<SCENE_DEPTH;
        glm::dvec3 coord = orientation * (glm::dvec3(vertex[0], vertex[1], vertex[2]) - position);
        v4si b = {
            (int)(coord.z*quadtree_bounds[0] - coord.x),
            (int)(coord.z*quadtree_bounds[1] - coord.x),
            (int)(coord.z*quadtree_bounds[2] - coord.y),
            (int)(coord.z*quadtree_bounds[3] - coord.y),
        };
        bounds[i] = b;
        if (max_z < coord.z) {
            max_z = coord.z;
            C = i;
        }
    }
    v4si pos = {(int)position.x, (int)position.y, (int)position.z};
    traverse(
        -1, 0, 0, bounds[C], 
        (bounds[C^DX]-bounds[C]), 
        (bounds[C^DY]-bounds[C]), 
        (bounds[C^DZ]-bounds[C]), 
        -pos, SCENE_DEPTH-1
    );
    
    
    timer_query = t_query.elapsed();

    Timer t_transfer;
    
    // Send the image data to OpenGL.
    // glTexImage2D( cubetargets[i], 0, 4, quadtree::SIZE, quadtree::SIZE, 0, GL_BGRA, GL_UNSIGNED_BYTE, face.face);
    
    timer_transfer = t_transfer.elapsed();
            
    std::printf("%7.2f | Prepare:%4.2f Query:%7.2f Transfer:%5.2f | %10d\n", t_global.elapsed(), timer_prepare, timer_query, timer_transfer, count);
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
