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

#include "timing.h"
#include "octree.h"

#define static_assert(test, message) typedef char static_assert__##message[(test)?1:-1]

using std::max;
using std::min;

static surface face;
static octree * root;
static int C; //< The corner that is furthest away from the camera.
static int count, count_oct, count_quad;
static int32_t ddepth[8];

constexpr static int make_mask(int a, int b, int c, int d) {
    return (a<<0)+(b<<1)+(c<<2)+(d<<3);
}

constexpr int quad_mask[8] = {
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

struct OctNode {
    /** The index of the current octree node that is being rendered. For leaf nodes (and their 'childs') octnode will be a color and >= 0xff000000u. */
    uint32_t octnode;
    int32_t depth;
    /** Distance to the viewer. */
    int32_t depth2;
    /** The quadnode projected on the parallel plane containing the furthest corner of the current octree node. 
     * It stores the distance from this furthest corner to the (left, right, top, bottom) edge of the projected quadnode. */
    __m128i bound;
};

struct QuadNode {
    uint32_t x, y;
    int32_t depth;
    /** Represent how this projection changes when traversing an edge to one of the other corners. */
    __m128i dx, dy, dz;
    /** Computed by `compute_frustum()`, a magic variable used for frustum occlusion. */
    __m128i frustum;
    
    inline void compute_frustum();
    inline int delta(const OctNode& a) const;
    inline bool contains(const OctNode& a) const;
};

static inline bool operator<(const OctNode& a, const OctNode& b) {
    return a.depth2 < b.depth2;
}

/** Computes the sum max(dx,0)+max(dy,0)+max(dz,0). */
void QuadNode::compute_frustum() {
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
int QuadNode::delta(const OctNode& a) const{
    __m128i b = _mm_add_epi32(_mm_add_epi32(a.bound, dx), _mm_add_epi32(dy, dz));
    return extract_epi32<0>(_mm_add_epi32(b,_mm_srli_si128(b,4)));
}

/** Compute whether this node is still within the frustum. */
bool QuadNode::contains(const OctNode& a) const{
    return !movemask_epi32(_mm_cmplt_epi32(a.bound, frustum));
}

#define FOR_i_IS_0_TO_3(code) \
  {constexpr int i = 0; code} \
  {constexpr int i = 1; code} \
  {constexpr int i = 2; code} \
  {constexpr int i = 3; code} 

#define FOR_k_IS_0_TO_7(code) \
  {constexpr int k = 0; code} \
  {constexpr int k = 1; code} \
  {constexpr int k = 2; code} \
  {constexpr int k = 3; code} \
  {constexpr int k = 4; code} \
  {constexpr int k = 5; code} \
  {constexpr int k = 6; code} \
  {constexpr int k = 7; code} 

static OctNode nodes[65536];
static int prepare(const QuadNode& quad, int size){
    int fixed = 0;
    while (size > fixed && size<=4096-8) {
        const OctNode& __restrict node = nodes[fixed];
        if (quad.delta(node) >= 1<<SCENE_DEPTH) {
            fixed++;
        } else if (node.octnode < 0xff000000) {
            // Traverse octree
            OctNode new_node(node);
            new_node.depth++;
            FOR_k_IS_0_TO_7({
                if (root[node.octnode].has_index(k)) {
                    int j = root[node.octnode].position(k);
                    new_node.bound = _mm_slli_epi32(node.bound, 1);
                    new_node.depth2 = node.depth2 + ((ddepth[k] + (1<<node.depth)) >> new_node.depth);
                    if ((C^k)&DX) new_node.bound = _mm_add_epi32(new_node.bound,quad.dx);
                    if ((C^k)&DY) new_node.bound = _mm_add_epi32(new_node.bound,quad.dy);
                    if ((C^k)&DZ) new_node.bound = _mm_add_epi32(new_node.bound,quad.dz);
                    if (quad.contains(new_node)) {
                        count_oct++;
                        new_node.octnode = root[node.octnode].child[j];
                        nodes[size] = new_node;
                        size++;
                    }
                }
            });
            nodes[fixed] = nodes[--size];
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
static void traverse(const QuadNode& quad, int max_n, OctNode* begin, OctNode* end){
    count++;
    OctNode* new_node=end;
    OctNode* node;
    for (node=begin; node!=end && new_node-node <= max_n; node++) {
        if (quad.delta(*node) >= 2<<SCENE_DEPTH) {
            *new_node = *node;
            new_node++;
            continue;
        }
        // Recursion
        // Traverse octree / Duplicate leaf node
        FOR_k_IS_0_TO_7({
            if (node->octnode >= 0xff000000 || root[node->octnode].has_index(k)) {
                *new_node = *node;
                new_node->depth++;
                new_node->bound = _mm_slli_epi32(node->bound, 1);
                new_node->depth2 = node->depth2 + ((ddepth[k] + (1<<node->depth)) >> new_node->depth);
                if ((C^k)&DX) new_node->bound = _mm_add_epi32(new_node->bound,quad.dx);
                if ((C^k)&DY) new_node->bound = _mm_add_epi32(new_node->bound,quad.dy);
                if ((C^k)&DZ) new_node->bound = _mm_add_epi32(new_node->bound,quad.dz);
                if (quad.contains(*new_node)) { // frustum occlusion
                    count_oct++;
                    if (node->octnode < 0xff000000) {
                        new_node->octnode = root[node->octnode].child[root[node->octnode].position(k)];
                    }
                    new_node++;
                }
            }
        });
    }
    if (node == new_node) return;
    begin = node;
    end = new_node;

    count++;
    std::sort(begin, end);
    // Traverse quadtree 
    __m128i mid_dx = _mm_srai_epi32(_mm_sub_epi32(quad.dx, _mm_shuffle_epi32(quad.dx,0xb1)), 1);
    __m128i mid_dy = _mm_srai_epi32(_mm_sub_epi32(quad.dy, _mm_shuffle_epi32(quad.dy,0xb1)), 1);
    __m128i mid_dz = _mm_srai_epi32(_mm_sub_epi32(quad.dz, _mm_shuffle_epi32(quad.dz,0xb1)), 1);
    FOR_i_IS_0_TO_3({ // Using a fixed size loop as blend_epi32 requires a compile-time constant as mask.
        constexpr int mask = quad_mask[i];
        QuadNode new_quad(quad);
        new_quad.depth--;
        if (i&1) new_quad.x += 1<<new_quad.depth;
        if (i&2) new_quad.y += 1<<new_quad.depth;
        if (new_quad.x < face.width && new_quad.y < face.height) {
            new_quad.dx = blend_epi32<mask>(mid_dx, new_quad.dx);
            new_quad.dy = blend_epi32<mask>(mid_dy, new_quad.dy);
            new_quad.dz = blend_epi32<mask>(mid_dz, new_quad.dz);
            new_quad.compute_frustum();
            
            OctNode* new_node = end;
            for (OctNode* node=begin; node!=end; node++) {
                *new_node = *node;
                __m128i mid_bound = _mm_srai_epi32(_mm_sub_epi32(node->bound, _mm_shuffle_epi32(node->bound,0xb1)), 1);
                new_node->bound = blend_epi32<mask>(mid_bound, node->bound);
                if (new_quad.contains(*new_node)) { // frustum occlusion
                    if (new_quad.depth > 0) {
                        new_node++;
                        count_quad++;
                        if (new_node - end >= max_n/4) break;
                    } else {
                        uint32_t color = (node->octnode < 0xff000000u) ? root[node->octnode].avgcolor : node->octnode;
                        face.pixel(new_quad.x, new_quad.y, color, node->depth2); // Rendering
                        break;
                    }
                }
            }
            if (end != new_node) {
                traverse(new_quad, max_n/2, end, new_node);
            }
        }
    });
}

/** Render the octree to the provided surface for the given viewpane, position and orientation.
 * @param file the octree that is being rendered.
 * @param surf the surface that is being rendered to.
 * @param position the position of the camera.
 * @param orientation the orientation of the camera (which is assumed to be orthogonal).
 */
void octree_draw(octree_file* file, surface surf, view_pane view, glm::dvec3 position, glm::dmat3 orientation) {
    Timer t_global;
    
    uint32_t quad_depth = 0;
    uint32_t quad_size = 1;
    while (quad_size < surf.width || quad_size < surf.height) {
        quad_depth++;
        quad_size = 1<<quad_depth;
    }
    
    double quadtree_bounds[] = {
        view.left,
       (view.left + (view.right -view.left)*(double)quad_size/surf.width ),
       (view.top  + (view.bottom-view.top )*(double)quad_size/surf.height),
        view.top,
    };

#ifndef NDEBUG
    // Check if the computed quadtree_bounds won't be able to cause an integer overflow.
    int overflow_limit = 0x3fffffff >> SCENE_DEPTH;
    for (int i=0; i<4; i++) {
        assert(-overflow_limit < quadtree_bounds[i] && quadtree_bounds[i] < overflow_limit);
    }
#endif

    root = file->root;
    face = surf;
    glm::dvec3 look_dir = glm::dvec3(0,0,1) * orientation;
    
    count_oct = count_quad = count = 0;
    // Do the actual rendering of the scene (i.e. execute the query).
    __m128i bounds[8];
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
    
    QuadNode quad;
    OctNode node;
    quad.x = 0;
    quad.y = 0;
    quad.depth = quad_depth;
    node.octnode = 0;
    node.depth2 = -glm::dot(position, look_dir); // Depth2 computation has some rounding errors.
    node.depth = 0;
    node.bound = bounds[C];
    quad.dx = _mm_sub_epi32(bounds[C^DX], bounds[C]);
    quad.dy = _mm_sub_epi32(bounds[C^DY], bounds[C]);
    quad.dz = _mm_sub_epi32(bounds[C^DZ], bounds[C]);
    quad.compute_frustum();
    nodes[0] = node;
    int size = prepare(quad, 1);
    traverse(quad, 16<<quad_depth, nodes, nodes+size);
    
    std::printf("%7.2f | Count:%10d Oct:%10d Quad:%10d | %4d\n", t_global.elapsed(), count, count_oct, count_quad, size);
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
