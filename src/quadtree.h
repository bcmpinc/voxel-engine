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

struct quadtree {
    static const uint32_t dim = 10;
    static const uint32_t N = (4<<dim<<dim)/3-1;
    static const uint32_t M = N/4-1;
    static const uint32_t L = M/4-1;
    static const uint32_t SIZE = 1<<dim;
    
    /** 
     * The quadtree is stored in a heap-like fashion as a single array.
     * The child nodes of map[i] are map[4*i+1], ..., map[4*i+4].
     */
    union {
        uint8_t  map[N];
        uint32_t children[N/4];
    };
        
    quadtree();
    void set(uint32_t x, uint32_t y);
    void set_face(uint32_t v, uint32_t color);
    void compute(uint32_t i);
    void build_fill(uint32_t i);
    void build_check(int width, int height, uint32_t i, int size);
    void build(int width, int height);
};


#endif
