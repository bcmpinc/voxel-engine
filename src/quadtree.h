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

#ifndef VOXEL_QUADTREE_H
#define VOXEL_QUADTREE_H
#include <stdint.h>

struct quadtree {
public:
    static const uint32_t dim = 10;
    static const uint32_t N = (4<<dim<<dim)/3-1;
    static const uint32_t M = N/4-1;
    static const uint32_t L = M/4-1;
    static const uint32_t SIZE = 1<<dim;
    
private:
    uint32_t width;
    uint32_t height;
    
public:
    uint32_t * pixels;

    /** 
     * The quadtree is stored in a heap-like fashion as a single array.
     * The child nodes of map[i] are map[4*i+1], ..., map[4*i+4].
     */
    union {
        uint8_t  map[N];
        uint32_t children[N/4];
    };

    /** Creates a new quadtree, to be used for rendering to the width * height * 32bit image buffer in pixels. 
     * It is assumed that the second row of pixels starts at pixels[width]. */
    quadtree(uint32_t width, uint32_t height, uint32_t * pixels);

    void set_face(uint32_t v, uint32_t color);
    
    /** Sets given node to 0 if all its children are zero. */
    void compute(uint32_t i);
    
    /** Initializes the quadtree such that all quadtree nodes within view are set to 1. */    
    void build();
    
private:
    /** Draws a single pixel in the provided pixel buffer. 
     * The pixel coordinates must be within bounds. */
    void pixel(uint32_t x, uint32_t y, uint32_t c);

    /** Sets a single value at given coordinates on the bottom level of the tree. (unused) 
     * Does not propagate this value through the rest of the tree. */
    void set(uint32_t x, uint32_t y);
    
    void build_fill(uint32_t i);
    void build_check(int w, int h, uint32_t i, int size);
};


#endif
