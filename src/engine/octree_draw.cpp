/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2013,2014,2015  B.J. Conijn <bcmpinc@users.sourceforge.net>

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
#include <cassert>
#include <algorithm>
#include <xmmintrin.h>

#include "quadtree.h"
#include "timing.h"
#include "octree.h"

#define static_assert(test, message) typedef char static_assert__##message[(test)?1:-1]

using std::max;
using std::min;

static quadtree face;
static octree * root;
static int C; //< The corner that is furthest away from the camera.
static int count, count_oct, count_quad;
static glm::dvec3 look_dir;

constexpr static int make_mask(int a, int b, int c, int d) {
    return (a<<0)+(b<<1)+(c<<2)+(d<<3);
}

constexpr int quad_mask[8] = {
    0,0,0,0,
    make_mask(1,0,0,1),
    make_mask(0,1,0,1),
    make_mask(1,0,1,0),
    make_mask(0,1,1,0),
};

static const int32_t SCENE_DEPTH = 26;

static const int DX=4, DY=2, DZ=1;
static const __m128i DELTA[8]={
    _mm_set_epi32(0,-1,-1,-1),
    _mm_set_epi32(0, 1,-1,-1),
    _mm_set_epi32(0,-1, 1,-1),
    _mm_set_epi32(0, 1, 1,-1),
    _mm_set_epi32(0,-1,-1, 1),
    _mm_set_epi32(0, 1,-1, 1),
    _mm_set_epi32(0,-1, 1, 1),
    _mm_set_epi32(0, 1, 1, 1),
};

static inline int movemask_epi32(__m128i v) {
    return _mm_movemask_ps(_mm_castsi128_ps(v));
}

// Passing mask as a template parameter, as it must be a compile time constant (for SSE4.1).
template<int mask>
static inline __m128i blend_epi32(__m128i a, __m128i b) {
#ifdef __SSE4_1__    
    return _mm_castps_si128(_mm_blend_ps(_mm_castsi128_ps(a),_mm_castsi128_ps(b),mask));
#else
    static const __m128i mask128i = _mm_set_epi32((mask&8)?~0:0, (mask&4)?~0:0, (mask&2)?~0:0, (mask&1)?~0:0);
    return _mm_or_si128(_mm_andnot_si128(mask128i,a),_mm_and_si128(mask128i,b));
#endif
}

// Passing mask as a template parameter, as it must be a compile time constant (for SSE4.1).
template<int index>
static inline int extract_epi32(__m128i a) {
#ifdef __SSE4_1__    
    return _mm_extract_epi32(a, index);
#else
    return _mm_cvtsi128_si32(_mm_srli_si128(a, 4*index)); // TODO: fix
#endif
}

template<> inline int extract_epi32<0>(__m128i a) {
    return _mm_cvtsi128_si32(a);
}

/** Computes the sum max(dx,0)+max(dy,0)+max(dz,0). */
static inline __m128i compute_frustum(__m128i dx, __m128i dy, __m128i dz) {
    const __m128i nil = _mm_setzero_si128();
    __m128i frustum = nil;
#ifdef __SSE4_1__    
    frustum = _mm_sub_epi32(frustum, _mm_max_epi32(dx, nil));
    frustum = _mm_sub_epi32(frustum, _mm_max_epi32(dy, nil));
    frustum = _mm_sub_epi32(frustum, _mm_max_epi32(dz, nil));
#else
    frustum = _mm_sub_epi32(frustum, _mm_and_si128(dx, _mm_cmpgt_epi32(dx, nil)));
    frustum = _mm_sub_epi32(frustum, _mm_and_si128(dy, _mm_cmpgt_epi32(dy, nil)));
    frustum = _mm_sub_epi32(frustum, _mm_and_si128(dz, _mm_cmpgt_epi32(dz, nil)));
#endif
    return frustum;
}

#define FOR_i_IS_4_TO_7(code) \
  {constexpr int i = 4; code} \
  {constexpr int i = 5; code} \
  {constexpr int i = 6; code} \
  {constexpr int i = 7; code} 

#define FOR_k_IS_0_TO_7(code) \
  {constexpr int k = 0; code} \
  {constexpr int k = 1; code} \
  {constexpr int k = 2; code} \
  {constexpr int k = 3; code} \
  {constexpr int k = 4; code} \
  {constexpr int k = 5; code} \
  {constexpr int k = 6; code} \
  {constexpr int k = 7; code} 

#define FOR_k_IS_0_TO_6(code) \
  {const int k = 0; code} \
  {const int k = 1; code} \
  {const int k = 2; code} \
  {const int k = 3; code} \
  {const int k = 4; code} \
  {const int k = 5; code} \
  {const int k = 6; code}

/** Core of the voxel rendering algorithm.
 * @param quadnode the index of the quadnode that will be rendered to. It is assumed that it is not yet fully rendered.
 * @param octnode the index of the current octree node that is being rendered. For leaf nodes (and their 'childs') octnode will be a color and >= 0xff000000u.
 * @param bound is the quadnode projected on the parallel plane containing the furthest corner of the current octree node.
 *              It stores the distance from this furthest corner to the (left, right, top, bottom) edge of the projected quadnode.
 * @param dx,dy,dz represent how this projection changes when traversing an edge to one of the other corners.
 * @param frustum equal to `compute_frustum(dx,dy,dz)`, a magic variable used for frustum occlusion.
 * @param pos is the location of the center of the octree node, relative to the viewer in octree space.
 * @param depth limits the number of nested traverse calls, to prevent stack overflows.
 * @return true if quadtree node is rendered 
 */
static bool traverse(
    const int32_t quadnode, const uint32_t octnode,
    const __m128i bound, const __m128i dx, const __m128i dy, const __m128i dz, const __m128i frustum,
    const __m128i pos, const int depth
){    
    count++;
    // Recursion
    int delta = extract_epi32<0>(_mm_add_epi32(bound,_mm_srli_si128(bound,4)));
    if (depth>=0 && delta < 2<<SCENE_DEPTH) {
        __m128i octant = _mm_cmplt_epi32(pos, _mm_setzero_si128());
        int furthest = movemask_epi32(_mm_shuffle_epi32(octant, 0xc6));
        if (octnode < 0xff000000) {
            // Traverse octree
            FOR_k_IS_0_TO_7({
                int i = furthest^k;
                if (root[octnode].has_index(i)) {
                    int j = root[octnode].position(i);
                    __m128i new_bound = _mm_slli_epi32(bound, 1);
                    if ((C^i)&DX) new_bound = _mm_add_epi32(new_bound,dx);
                    if ((C^i)&DY) new_bound = _mm_add_epi32(new_bound,dy);
                    if ((C^i)&DZ) new_bound = _mm_add_epi32(new_bound,dz);
                    if (!movemask_epi32(_mm_cmplt_epi32(new_bound, frustum))) { // frustum occlusion
                        count_oct++;
                        if (traverse(quadnode, root[octnode].child[j], new_bound, dx, dy, dz, frustum, _mm_add_epi32(pos, _mm_slli_epi32(DELTA[i], depth)), depth-1)) return true;
                    }
                }
            });
        } else {
            // Duplicate leaf node
            FOR_k_IS_0_TO_6({
                int i = furthest^k;
                __m128i new_bound = _mm_slli_epi32(bound, 1);
                if ((C^i)&DX) new_bound = _mm_add_epi32(new_bound,dx);
                if ((C^i)&DY) new_bound = _mm_add_epi32(new_bound,dy);
                if ((C^i)&DZ) new_bound = _mm_add_epi32(new_bound,dz);
                if (!movemask_epi32(_mm_cmplt_epi32(new_bound, frustum))) { // frustum occlusion
                    count_oct++;
                    if (traverse(quadnode, octnode, new_bound, dx, dy, dz, frustum, _mm_add_epi32(pos, _mm_slli_epi32(DELTA[i], depth)), depth-1)) return true;
                }
            });
        }
        return false;
    } else {
        // Traverse quadtree 
        int mask = face.children[quadnode];
        __m128i mid_bound = _mm_srai_epi32(_mm_sub_epi32(bound, _mm_shuffle_epi32(bound,0xb1)), 1);
        __m128i mid_dx = _mm_srai_epi32(_mm_sub_epi32(dx, _mm_shuffle_epi32(dx,0xb1)), 1);
        __m128i mid_dy = _mm_srai_epi32(_mm_sub_epi32(dy, _mm_shuffle_epi32(dy,0xb1)), 1);
        __m128i mid_dz = _mm_srai_epi32(_mm_sub_epi32(dz, _mm_shuffle_epi32(dz,0xb1)), 1);
        FOR_i_IS_4_TO_7({ // Using a fixed size loop as blend_epi32 requires a compile-time constant as mask.
            if (mask&(1<<i)) {
                constexpr int new_mask = quad_mask[i];
                __m128i new_bound = blend_epi32<new_mask>(mid_bound, bound);
                __m128i new_dx = blend_epi32<new_mask>(mid_dx, dx);
                __m128i new_dy = blend_epi32<new_mask>(mid_dy, dy);
                __m128i new_dz = blend_epi32<new_mask>(mid_dz, dz);
                __m128i new_frustum = compute_frustum(new_dx, new_dy, new_dz);
                if (!movemask_epi32(_mm_cmplt_epi32(new_bound, new_frustum))) { // frustum occlusion
                    if (quadnode<quadtree::M) {
                        if (traverse(quadnode*4+i, octnode, new_bound, new_dx, new_dy, new_dz, new_frustum, pos, depth)) {
                            mask &= ~(1<<i); 
                        }
                        count_quad++;
                    } else {
                        glm::dvec3 dpos(extract_epi32<0>(pos), extract_epi32<1>(pos), extract_epi32<2>(pos));
                        double depth = glm::dot(dpos, look_dir);
                        uint32_t udepth(depth);
                        uint32_t color = (octnode < 0xff000000u) ? root[octnode].avgcolor : octnode;
                        face.draw(quadnode*4+i, color, udepth); // Rendering
                        mask &= ~(1<<i);
                    }
                }
            }
        });
        face.children[quadnode] = mask;
        return mask == 0;
    }
}

/** Render the octree to the provided surface for the given viewpane, position and orientation.
 * @param file the octree that is being rendered.
 * @param surf the surface that is being rendered to.
 * @param position the position of the camera.
 * @param orientation the orientation of the camera (which is assumed to be orthogonal).
 */
void octree_draw(octree_file* file, surface surf, view_pane view, glm::dvec3 position, glm::dmat3 orientation) {
    Timer t_global;
    
    double timer_prepare;
    double timer_query;
    
    // Make sure that the quadtree is big enough that it can contain the rendered surface.
    // If these checks fail, increase quadtree::dim in quadtree.h.
    assert(quadtree::SIZE >= surf.width);
    assert(quadtree::SIZE >= surf.height);
    
    double quadtree_bounds[] = {
        view.left,
       (view.left + (view.right -view.left)*(double)quadtree::SIZE/surf.width ),
       (view.top  + (view.bottom-view.top )*(double)quadtree::SIZE/surf.height),
        view.top,
    };
    // On the other hand, if quadtree::dim is too high, it can cause an overflow in the computation of bounds[].
    // If these checks fail, decrease quadtree::dim in quadtree.h.
#ifndef NDEBUG
    int overflow_limit = 0x3fffffff >> SCENE_DEPTH;
    for (int i=0; i<4; i++) {
        assert(-overflow_limit < quadtree_bounds[i] && quadtree_bounds[i] < overflow_limit);
    }
#endif

    root = file->root;
    face.surf = surf;
    look_dir = glm::dvec3(0,0,1) * orientation;
    
    Timer t_prepare;
    // Prepare the occlusion quadtree
    face.build();
    timer_prepare = t_prepare.elapsed();

    Timer t_query;
    count_oct = count_quad = count = 0;
    // Do the actual rendering of the scene (i.e. execute the query).
    __m128i bounds[8];
    int max_z=-1<<31;
    for (int i=0; i<8; i++) {
        // Compute position of octree corners in camera-space
        __m128i vert = _mm_slli_epi32(DELTA[i], SCENE_DEPTH);
        int * vertex = (int*)&vert;
        glm::dvec3 coord = orientation * (glm::dvec3(vertex[0], vertex[1], vertex[2]) - position);
        bounds[i] = _mm_set_epi32(
            (int)(coord.z*quadtree_bounds[3] - coord.y),
           -(int)(coord.z*quadtree_bounds[2] - coord.y),
            (int)(coord.z*quadtree_bounds[1] - coord.x),
           -(int)(coord.z*quadtree_bounds[0] - coord.x)
        );
        if (max_z < coord.z) {
            max_z = coord.z;
            C = i;
        }
    }
    __m128i pos = _mm_set_epi32(0, -(int)position.z, -(int)position.y, -(int)position.x);
    __m128i new_dx = _mm_sub_epi32(bounds[C^DX], bounds[C]);
    __m128i new_dy = _mm_sub_epi32(bounds[C^DY], bounds[C]);
    __m128i new_dz = _mm_sub_epi32(bounds[C^DZ], bounds[C]);
    __m128i new_frustum = compute_frustum(new_dx, new_dy, new_dz);
    traverse(-1, 0, bounds[C], new_dx, new_dy, new_dz, new_frustum, pos, SCENE_DEPTH-1);
    timer_query = t_query.elapsed();

    std::printf("%7.2f | Prepare:%4.2f Query:%7.2f | Count:%10d Oct:%10d Quad:%10d\n", t_global.elapsed(), timer_prepare, timer_query, count, count_oct, count_quad);
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
