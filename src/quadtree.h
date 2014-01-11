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

#ifndef VOXEL_QUADTREE_H
#define VOXEL_QUADTREE_H
#include <stdint.h>
#include <glm/glm.hpp>

/**
 * A quadtree like structure with 4x4 children per intermediate node (rather than 2x2).
 */
struct quadtree {
    static const unsigned int CHILD_COUNT = 16; // Number of 'childpointers' per intermediate node.
    static const unsigned int LAYERS = 5; // Number of layers in the tree (1 = only the rootnode).
    static const unsigned int SIZE = 1<<(LAYERS*2); // Width of the quadtree.
    static const unsigned int N = (CHILD_COUNT<<(LAYERS*4))/(CHILD_COUNT-1); // Number of children in the array.
    static const unsigned int M = N/CHILD_COUNT; // Length of the array
    static const unsigned int L = M/CHILD_COUNT; // Start of the last layer of intermediate nodes.
    
    /** 
     * The quadtree is stored in a heap-like fashion as a single array of bitmasks.
     * The child nodes of map[i] are map[16*i+1], ..., map[16*i+16].
     * Root is at map[0], first layer at map[1], .., map[16], second layer starts at map[17].
     */
    union {
        uint16_t  map[M];
    };

    inline void set_bit(int pos)   {map[pos/CHILD_COUNT] |=   1<<(pos%CHILD_COUNT); }
    inline void unset_bit(int pos) {map[pos/CHILD_COUNT] &= ~(1<<(pos%CHILD_COUNT));}

    quadtree();
    void set(int x, int y);
    void set_face(int node, int bit, int color);
    void compute(int i);
    void build_fill(int i);
    void build_check(int width, int height, int i, int size);
    void build(int width, int height);
};


#endif
