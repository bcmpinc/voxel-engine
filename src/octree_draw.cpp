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
}

static_assert(quadtree::SIZE >= SCREEN_HEIGHT, quadtree_height_too_small);
static_assert(quadtree::SIZE >= SCREEN_WIDTH,  quadtree_width_too_small);

// Array with x1, x2, y1, y2. Note that x2-x1 = y2-y1.
typedef int32_t v4si __attribute__ ((vector_size (16)));

template<int C>
struct FaceRenderer {
    static const int AX=4, AY=2, AZ=1;

    /** Returns true if quadtree node is rendered 
     * Function is assumed to be called only if quadtree node is not yet fully rendered.
     * The bounds array is ordered as DELTA.
     * C is the corner that is furthest away from the camera.
     */
    static bool traverse(
        int quadnode, uint32_t octnode, uint32_t octcolor, v4si bounds[8]
    ){
        if (quadnode<(int)quadtree::L) {
            // Traverse quadtree 
            if (face.map[quadnode*4+4]) traverse(quadnode*4+4, octnode, octcolor, bounds); 
            if (face.map[quadnode*4+5]) traverse(quadnode*4+5, octnode, octcolor, bounds); 
            if (face.map[quadnode*4+6]) traverse(quadnode*4+6, octnode, octcolor, bounds); 
            if (face.map[quadnode*4+7]) traverse(quadnode*4+7, octnode, octcolor, bounds); 
        } else {
            // Rendering
            if (face.map[quadnode*4+4]) face.set_face(quadnode*4+4, octcolor); 
            if (face.map[quadnode*4+5]) face.set_face(quadnode*4+5, octcolor); 
            if (face.map[quadnode*4+6]) face.set_face(quadnode*4+6, octcolor); 
            if (face.map[quadnode*4+7]) face.set_face(quadnode*4+7, octcolor); 
        }
        if (quadnode>=0) {
            face.compute(quadnode);
            return !face.map[quadnode];
        } else {
            return face.children[0]==0;
        }
        
#if 0
        // behind camera occlusion
        if (bounds[C][1] - bounds[C][0]<=0) return false;
        
        // frustum occlusion
        int check=15; // x1, x2, y1, y2
        for (int i = 0; i<8; i++) {
            if (bounds[i][0]<0) check &=~1;
            if (bounds[i][1]>0) check &=~2;
            if (bounds[i][2]<0) check &=~4;
            if (bounds[i][3]>0) check &=~8;
        }
        if (check) return false;
        
        // Recursion
        if (d <= 2*ONE) {
            // Traverse octree
            
            // occlusion
            int xn = (x-xp)*2; // x3, x4=xn+dn
            int yn = (y-yp)*2; 
            int dn = (d-dp)*2;
            x*=2;
            y*=2;
            d*=2;
                        
            // Recursive calls
            if (~octnode) {
                octree &s = root[octnode];
                if (dn>0) {
                    if (occ_x.t1 && occ_y.t1 && s.avgcolor[C         ]>=0 && traverse(quadnode, s.child[C         ], s.avgcolor[C         ], xn+DX*ONE,yn+DY*ONE,dn,xp,yp,dp)) return true;
                    if (occ_x.t2 && occ_y.t1 && s.avgcolor[C^AX      ]>=0 && traverse(quadnode, s.child[C^AX      ], s.avgcolor[C^AX      ], xn-DX*ONE,yn+DY*ONE,dn,xp,yp,dp)) return true;
                    if (occ_x.t1 && occ_y.t2 && s.avgcolor[C   ^AY   ]>=0 && traverse(quadnode, s.child[C   ^AY   ], s.avgcolor[C   ^AY   ], xn+DX*ONE,yn-DY*ONE,dn,xp,yp,dp)) return true;
                    if (occ_x.t2 && occ_y.t2 && s.avgcolor[C^AX^AY   ]>=0 && traverse(quadnode, s.child[C^AX^AY   ], s.avgcolor[C^AX^AY   ], xn-DX*ONE,yn-DY*ONE,dn,xp,yp,dp)) return true;
                }
                if (occ_x.t3 && occ_y.t3 && s.avgcolor[C      ^AZ]>=0 && traverse(quadnode, s.child[C      ^AZ], s.avgcolor[C      ^AZ], x+DX*ONE,y+DY*ONE,d,xp,yp,dp)) return true;
                if (occ_x.t4 && occ_y.t3 && s.avgcolor[C^AX   ^AZ]>=0 && traverse(quadnode, s.child[C^AX   ^AZ], s.avgcolor[C^AX   ^AZ], x-DX*ONE,y+DY*ONE,d,xp,yp,dp)) return true;
                if (occ_x.t3 && occ_y.t4 && s.avgcolor[C   ^AY^AZ]>=0 && traverse(quadnode, s.child[C   ^AY^AZ], s.avgcolor[C   ^AY^AZ], x+DX*ONE,y-DY*ONE,d,xp,yp,dp)) return true;
                if (occ_x.t4 && occ_y.t4 && s.avgcolor[C^AX^AY^AZ]>=0 && traverse(quadnode, s.child[C^AX^AY^AZ], s.avgcolor[C^AX^AY^AZ], x-DX*ONE,y-DY*ONE,d,xp,yp,dp)) return true;
            } else {
                if (dn>0) {
                    // Skip nearest cube to avoid infinite recursion.
                    if (occ_x.t2 && occ_y.t1 && traverse(quadnode, ~0u, octcolor, xn-DX*ONE,yn+DY*ONE,dn,xp,yp,dp)) return true;
                    if (occ_x.t1 && occ_y.t2 && traverse(quadnode, ~0u, octcolor, xn+DX*ONE,yn-DY*ONE,dn,xp,yp,dp)) return true;
                    if (occ_x.t2 && occ_y.t2 && traverse(quadnode, ~0u, octcolor, xn-DX*ONE,yn-DY*ONE,dn,xp,yp,dp)) return true;
                }
                if (occ_x.t3 && occ_y.t3 && traverse(quadnode, ~0u, octcolor, x+DX*ONE,y+DY*ONE,d,xp,yp,dp)) return true;
                if (occ_x.t4 && occ_y.t3 && traverse(quadnode, ~0u, octcolor, x-DX*ONE,y+DY*ONE,d,xp,yp,dp)) return true;
                if (occ_x.t3 && occ_y.t4 && traverse(quadnode, ~0u, octcolor, x+DX*ONE,y-DY*ONE,d,xp,yp,dp)) return true;
                if (occ_x.t4 && occ_y.t4 && traverse(quadnode, ~0u, octcolor, x-DX*ONE,y-DY*ONE,d,xp,yp,dp)) return true;
            }
            return false;
        } else {
            assert((d&1)==0);
            assert((dp&1)==0);
            d/=2;
            dp/=2;
            int xm  = x  + d; 
            int xmp = xp + dp; 
            int ym  = y  + d; 
            int ymp = yp + dp; 
            if (quadnode<quadtree::L) {
                // Traverse quadtree 
                if (face.map[quadnode*4+4] && xm-(1-DX)*xmp>-ONE && ym-(1-DY)*ymp>-ONE) traverse(quadnode*4+4, octnode, octcolor, x,  y,  d, xp,  yp,  dp); 
                if (face.map[quadnode*4+5] && ONE>xm-(1+DX)*xmp  && ym-(1-DY)*ymp>-ONE) traverse(quadnode*4+5, octnode, octcolor, xm, y,  d, xmp, yp,  dp); 
                if (face.map[quadnode*4+6] && xm-(1-DX)*xmp>-ONE && ONE>ym-(1+DY)*ymp ) traverse(quadnode*4+6, octnode, octcolor, x,  ym, d, xp,  ymp, dp); 
                if (face.map[quadnode*4+7] && ONE>xm-(1+DX)*xmp  && ONE>ym-(1+DY)*ymp ) traverse(quadnode*4+7, octnode, octcolor, xm, ym, d, xmp, ymp, dp); 
            } else {
                // Rendering
                if (face.map[quadnode*4+4] && xm-(1-DX)*xmp>-ONE && ym-(1-DY)*ymp>-ONE) face.set_face(quadnode*4+4, octcolor); 
                if (face.map[quadnode*4+5] && ONE>xm-(1+DX)*xmp  && ym-(1-DY)*ymp>-ONE) face.set_face(quadnode*4+5, octcolor); 
                if (face.map[quadnode*4+6] && xm-(1-DX)*xmp>-ONE && ONE>ym-(1+DY)*ymp ) face.set_face(quadnode*4+6, octcolor); 
                if (face.map[quadnode*4+7] && ONE>xm-(1+DX)*xmp  && ONE>ym-(1+DY)*ymp ) face.set_face(quadnode*4+7, octcolor); 
            }
            face.compute(quadnode);
            return !face.map[quadnode];
        }
#endif
    }
};
    
static const double SCENE_SIZE = 1<<(sizeof(int32_t)-4);

static const glm::dvec3 DELTA[8]={
    glm::dvec3(-1,-1,-1) * SCENE_SIZE,
    glm::dvec3(-1,-1, 1) * SCENE_SIZE,
    glm::dvec3(-1, 1,-1) * SCENE_SIZE,
    glm::dvec3(-1, 1, 1) * SCENE_SIZE,
    glm::dvec3( 1,-1,-1) * SCENE_SIZE,
    glm::dvec3( 1,-1, 1) * SCENE_SIZE,
    glm::dvec3( 1, 1,-1) * SCENE_SIZE,
    glm::dvec3( 1, 1, 1) * SCENE_SIZE,
};

static const glm::dvec4 quadtree_bounds(
    frustum::left  /(double)frustum::near,
    frustum::right /(double)frustum::near*quadtree::SIZE/SCREEN_WIDTH,
    frustum::top   /(double)frustum::near,
    frustum::bottom/(double)frustum::near*quadtree::SIZE/SCREEN_HEIGHT
);

/** Render the octree to the OpenGL cubemap texture. 
 */
void octree_draw(octree_file * file) {
    Timer t_global;
    
    double timer_prepare;
    double timer_query;
    double timer_transfer;
    
    root = file->root;
    
    // The orientation matrix is (asumed to be) orthogonal, and therefore can be inversed by transposition.
    glm::dmat3 inverse_orientation = glm::transpose(orientation);
    
    Timer t_prepare;
        
    // Prepare the occlusion quadtree
    face.build(SCREEN_WIDTH, SCREEN_HEIGHT);
    
    timer_prepare = t_prepare.elapsed();
        
    Timer t_query;
    
    // Do the actual rendering of the scene (i.e. execute the query).
    v4si bounds[8];
    for (int i=0; i<8; i++) {
        // Compute position of octree corners in camera-space
        glm::dvec3 coord = inverse_orientation * (DELTA[i] - position);
        v4si b = {
            (int)(coord.z*quadtree_bounds[0] - coord.x),
            (int)(coord.z*quadtree_bounds[1] - coord.x),
            (int)(coord.z*quadtree_bounds[2] - coord.y),
            (int)(coord.z*quadtree_bounds[3] - coord.y),
        };
        bounds[i] = b;
    }
    FaceRenderer<0>::traverse(-1, 0, 0x000000, bounds);
    
    
    timer_query = t_query.elapsed();

    Timer t_transfer;
    
    // Send the image data to OpenGL.
    // glTexImage2D( cubetargets[i], 0, 4, quadtree::SIZE, quadtree::SIZE, 0, GL_BGRA, GL_UNSIGNED_BYTE, face.face);
    
    timer_transfer = t_transfer.elapsed();
            
    printf("%6.2f | Prepare:%4.2f Query:%7.2f Transfer:%5.2f\n", t_global.elapsed(), timer_prepare, timer_query, timer_transfer);
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
