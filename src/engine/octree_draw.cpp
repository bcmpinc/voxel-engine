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
#include <string.h>
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
    return _mm_cvtsi128_si32(_mm_srli_si128(a, 4*index));
#endif
}

template<> inline int extract_epi32<0>(__m128i a) {
    return _mm_cvtsi128_si32(a);
}

struct SearchNode {
    /** The index of the quadnode that will be rendered to. It is assumed that it is not yet fully rendered. */
    int32_t quadnode;
    /** The index of the current octree node that is being rendered. For leaf nodes (and their 'childs') octnode will be a color and >= 0xff000000u. */
    uint32_t octnode;
    /** Distance to the viewer. */
    int32_t depth;
    int32_t depth2;
    int32_t dd[8];
    /** The quadnode projected on the parallel plane containing the furthest corner of the current octree node. 
     * It stores the distance from this furthest corner to the (left, right, top, bottom) edge of the projected quadnode. */
    __m128i bound;
    /** Represent how this projection changes when traversing an edge to one of the other corners. */
    __m128i dx, dy, dz;
    /** Computed by `compute_frustum()`, a magic variable used for frustum occlusion. */
    __m128i frustum;
    /** The location of the center of the octree node, relative to the viewer in octree space. */
    __m128i pos;
    //__m128i padding0;
    
    inline void compute_frustum();
    inline int delta() const;
    inline bool inside_frustum() const;
};// __attribute__ ((aligned (128)));

/** Computes the sum max(dx,0)+max(dy,0)+max(dz,0). */
void SearchNode::compute_frustum() {
    const __m128i nil = _mm_setzero_si128();
    frustum = nil;
#ifdef __SSE4_1__    
    frustum = _mm_sub_epi32(frustum, _mm_max_epi32(dx, nil));
    frustum = _mm_sub_epi32(frustum, _mm_max_epi32(dy, nil));
    frustum = _mm_sub_epi32(frustum, _mm_max_epi32(dz, nil));
#else
    frustum = _mm_sub_epi32(frustum, _mm_and_si128(dx, _mm_cmpgt_epi32(dx, nil)));
    frustum = _mm_sub_epi32(frustum, _mm_and_si128(dy, _mm_cmpgt_epi32(dy, nil)));
    frustum = _mm_sub_epi32(frustum, _mm_and_si128(dz, _mm_cmpgt_epi32(dz, nil)));
#endif
}

/** Compute an estimate of the node size on screen. */
int SearchNode::delta() const{
    __m128i b = _mm_add_epi32(_mm_add_epi32(bound, dx), _mm_add_epi32(dy, dz));
    return extract_epi32<0>(_mm_add_epi32(b,_mm_srli_si128(b,4)));
}

/** Compute whether this node is still within the frustum. */
bool SearchNode::inside_frustum() const{
    return !movemask_epi32(_mm_cmplt_epi32(bound, frustum));
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
  
static SearchNode root_array[4096];
static int prepare(int size){
    int fixed = 0;
    while (size > fixed && size<=4096-8) {
        const SearchNode& __restrict node = root_array[fixed];
        if (node.delta() >= 2<<SCENE_DEPTH) {
            fixed++;
        } else if (node.octnode < 0xff000000) {
            // Traverse octree
            SearchNode new_node(node);
            new_node.depth--;
            for (int i=0; i<8; i++) new_node.dd[i]/=2;
            __m128i octant = _mm_cmplt_epi32(node.pos, _mm_setzero_si128());
            int furthest = movemask_epi32(_mm_shuffle_epi32(octant, 0xc6));
            FOR_k_IS_0_TO_7({
                int i = furthest^(7-k);
                if (root[node.octnode].has_index(i)) {
                    int j = root[node.octnode].position(i);
                    new_node.bound = _mm_slli_epi32(node.bound, 1);
                    new_node.depth2 = node.depth2 + new_node.dd[i];
                    if ((C^i)&DX) new_node.bound = _mm_add_epi32(new_node.bound,new_node.dx);
                    if ((C^i)&DY) new_node.bound = _mm_add_epi32(new_node.bound,new_node.dy);
                    if ((C^i)&DZ) new_node.bound = _mm_add_epi32(new_node.bound,new_node.dz);
                    if (new_node.inside_frustum()) {
                        count_oct++;
                        new_node.octnode = root[node.octnode].child[j];
                        new_node.pos = _mm_add_epi32(node.pos, _mm_slli_epi32(DELTA[i], node.depth));
                        root_array[size] = new_node;
                        size++;
                    }
                }
            });
            root_array[fixed++].quadnode = 0;
        } else {
            // Don't traverse leaf nodes here.
            fixed++;
        }
    }
    return size;
}

/** Core of the voxel rendering algorithm. 
 * @return true if quadtree node is rendered.
 */
static bool traverse_quad(const SearchNode& __restrict node);

static bool traverse_oct(const SearchNode& __restrict node){
    if (node.delta() >= 2<<SCENE_DEPTH) { return traverse_quad(node); }
    count++;
    // Recursion
    __m128i octant = _mm_cmplt_epi32(node.pos, _mm_setzero_si128());
    int furthest = movemask_epi32(_mm_shuffle_epi32(octant, 0xc6));
    SearchNode new_node(node);
    new_node.depth--;
    for (int i=0; i<8; i++) new_node.dd[i]/=2;
    if (node.octnode < 0xff000000) {
        // Traverse octree
        FOR_k_IS_0_TO_7({
            int i = furthest^k;
            if (root[node.octnode].has_index(i)) {
                int j = root[node.octnode].position(i);
                new_node.bound = _mm_slli_epi32(node.bound, 1);
                new_node.depth2 = node.depth2 + new_node.dd[i];
                if ((C^i)&DX) new_node.bound = _mm_add_epi32(new_node.bound,new_node.dx);
                if ((C^i)&DY) new_node.bound = _mm_add_epi32(new_node.bound,new_node.dy);
                if ((C^i)&DZ) new_node.bound = _mm_add_epi32(new_node.bound,new_node.dz);
                if (new_node.inside_frustum()) {
                    count_oct++;
                    new_node.octnode = root[node.octnode].child[j];
                    new_node.pos = _mm_add_epi32(node.pos, _mm_slli_epi32(DELTA[i], node.depth));
                    if (traverse_quad(new_node)) return true;
                }
            }
        });
    } else {
        // Duplicate leaf node
        FOR_k_IS_0_TO_6({
            int i = furthest^k;
            new_node.bound = _mm_slli_epi32(node.bound, 1);
            new_node.depth2 = node.depth2 + new_node.dd[i];
            if ((C^i)&DX) new_node.bound = _mm_add_epi32(new_node.bound,new_node.dx);
            if ((C^i)&DY) new_node.bound = _mm_add_epi32(new_node.bound,new_node.dy);
            if ((C^i)&DZ) new_node.bound = _mm_add_epi32(new_node.bound,new_node.dz);
            if (new_node.inside_frustum()) {
                count_oct++;
                new_node.pos = _mm_add_epi32(node.pos, _mm_slli_epi32(DELTA[i], node.depth));
                if (traverse_quad(new_node)) return true;
            }
        });
    }
    return false;
}

static bool traverse_quad(const SearchNode& __restrict node) {    
    count++;
    // Traverse quadtree 
    int mask = face.children[node.quadnode];
    __m128i mid_bound = _mm_srai_epi32(_mm_sub_epi32(node.bound, _mm_shuffle_epi32(node.bound,0xb1)), 1);
    __m128i mid_dx = _mm_srai_epi32(_mm_sub_epi32(node.dx, _mm_shuffle_epi32(node.dx,0xb1)), 1);
    __m128i mid_dy = _mm_srai_epi32(_mm_sub_epi32(node.dy, _mm_shuffle_epi32(node.dy,0xb1)), 1);
    __m128i mid_dz = _mm_srai_epi32(_mm_sub_epi32(node.dz, _mm_shuffle_epi32(node.dz,0xb1)), 1);
    SearchNode new_node(node);
    FOR_i_IS_4_TO_7({ // Using a fixed size loop as blend_epi32 requires a compile-time constant as mask.
        if (mask&(1<<i)) {
            constexpr int new_mask = quad_mask[i];
            new_node.quadnode = node.quadnode*4+i;
            new_node.bound = blend_epi32<new_mask>(mid_bound, node.bound);
            new_node.dx = blend_epi32<new_mask>(mid_dx, node.dx);
            new_node.dy = blend_epi32<new_mask>(mid_dy, node.dy);
            new_node.dz = blend_epi32<new_mask>(mid_dz, node.dz);
            new_node.compute_frustum();
            if (new_node.inside_frustum()) { // frustum occlusion
                if (node.quadnode<quadtree::M) {
                    if (traverse_oct(new_node)) {
                        mask &= ~(1<<i); 
                    }
                    count_quad++;
                } else {
                    glm::dvec3 dpos(extract_epi32<0>(node.pos), extract_epi32<1>(node.pos), extract_epi32<2>(node.pos));
                    double depth = glm::dot(dpos, look_dir);
                    int32_t udepth(depth);
                    uint32_t color = (node.octnode < 0xff000000u) ? root[node.octnode].avgcolor : node.octnode;
                    // Depth color.
                    int32_t depth2 = node.depth2;
                    if (udepth - depth2 < -32) color = 0xffff00ff;
                    if (udepth - depth2 >  32) color = 0xffffff00;
                    face.draw(new_node.quadnode, color, udepth); // Rendering
                    mask &= ~(1<<i);
                }
            }
        }
    });
    face.children[node.quadnode] = mask;
    return mask == 0;
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
    int ddepth[8];
    int max_z=-1<<31;
    for (int i=0; i<8; i++) {
        // Compute position of octree corners in camera-space
        __m128i vert = _mm_slli_epi32(DELTA[i], SCENE_DEPTH);
        int * vertex = (int*)&vert;
        glm::dvec3 coord = orientation * glm::dvec3(vertex[0], vertex[1], vertex[2]);
        ddepth[i] = coord.z;
        coord -= orientation * position;
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
    
    SearchNode node;
    node.quadnode = -1;
    node.octnode = 0;
    node.depth2 = -glm::dot(position, look_dir); // Depth2 computation has some rounding errors.
    memcpy(node.dd, ddepth, sizeof(ddepth));
    node.depth = SCENE_DEPTH-1;
    node.pos = _mm_set_epi32(0, -(int)position.z, -(int)position.y, -(int)position.x);
    node.bound = bounds[C];
    node.dx = _mm_sub_epi32(bounds[C^DX], bounds[C]);
    node.dy = _mm_sub_epi32(bounds[C^DY], bounds[C]);
    node.dz = _mm_sub_epi32(bounds[C^DZ], bounds[C]);
    node.compute_frustum();
    root_array[0] = node;
    int size = prepare(1);
    for (int i=size-1; i>=0; i--) {
        if (root_array[i].quadnode)
            traverse_quad(root_array[i]);
    }
    
    timer_query = t_query.elapsed();

    std::printf("%7.2f | Prepare:%4.2f Query:%7.2f | Count:%10d Oct:%10d Quad:%10d | %4d\n", t_global.elapsed(), timer_prepare, timer_query, count, count_oct, count_quad, size);
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
