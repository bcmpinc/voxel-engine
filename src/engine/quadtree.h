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
#include "surface.h"

struct quadtree {
public:
    /** The number of levels in the quadtree.
     * This should be the lowest number such that width and height are at most (1<<dim). 
     */
    static const uint32_t dim = 10;
    static const uint32_t SIZE = 1<<dim;
    static const int N = (1<<dim<<dim)/3-1;
    static const int M = N/4-1;
    
    surface surf;

    /** children[-1] */
    uint32_t rootnode;
    
    /** 
     * The quadtree is stored in a heap-like fashion as a single array.
     * The child nodes of map[i] are map[4*i+4], ..., map[4*i+7].
     */
    uint32_t children[N];

    /** Creates a new quadtree, to be used for rendering to the width * height * 32bit image buffer in pixels. 
     * It is assumed that the second row of pixels starts at pixels[width]. */
    quadtree();
    quadtree(surface surf);

    /** Draws the pixel associated with the given leafnode. */
    void draw(uint32_t v, uint32_t color, uint32_t depth);
    
    /** Initializes the quadtree such that all quadtree nodes within view are set to 1. */    
    void build();
    
private:
    /** Sets a single value at given coordinates on the bottom level of the tree. (unused) 
     * Does not propagate this value through the rest of the tree. */
    void set(uint32_t x, uint32_t y);
    
    void build_fill(int i);
    bool build_check(int w, int h, int i, int size);
};


#endif
