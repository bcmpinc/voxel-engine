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

#ifndef OCTREE_H
#define OCTREE_H
#include <stdint.h>
#include <glm/glm.hpp>
#include "surface.h"

static inline int popcount(int v) {
    return __builtin_popcount(v);
}

/** A node in an octree. 
 *
 * Indices are a bitwise or of the following values:
 * x=4, y=2, z=1.
 * 
 * Hence:
 * 0 = neg-x, neg-y, neg-z
 * 1 = neg-x, neg-y, pos-z
 * etc...
 * 
 */
struct octree {
    uint32_t avgcolor:24;
    uint32_t bitmask : 8;
    uint32_t child[0];
    /** Checks if a given position in the child array is a pointer. */
    bool is_pointer(int pos) const { return child[pos] < 0xff000000u; }
    /** Returns the color of a given position in the child array (assuming it is not a pointer). */
    uint32_t color(int pos) const { return child[pos] & 0x00ffffffu; }
    /** Converts an index (0-7) into a position in the child array, assuming it is in the child array. */
    uint32_t position(int index) const { return popcount(bitmask & ((1<<index) - 1)); }
    /** Returns the length of the child array. */
    uint32_t size() const { return popcount(bitmask); }
    /** Checks whether a certain index is in the child array. */
    bool has_index(int index) const { return bitmask & (1<<index); }
    /** Makes room for the given index in the child array, and returns its position. */
    uint32_t insert_index(int index) {
        uint32_t pos = position(index);
        if (has_index(index)) return pos;
        uint32_t child_length = size();
        for (uint32_t i = child_length; i > pos; i--) {
            child[i] = child[i-1];
        }
        bitmask |= (1<<index);
        child[pos] = 0;
        return pos;
    }
    void set_color(int pos, uint32_t color) { child[pos] = (color | 0xff000000u); }
};

struct octree_file {
    const bool write;
    uint32_t size;
    int32_t fd;
    octree * root;
    /** Maps the given octree file to memory for reading and rendering. */
    octree_file(const char * filename);
    /** Creates an octree file with the given name and size for writing. */
    octree_file(const char * filename, uint32_t size);
    ~octree_file();
private:
    octree_file(octree_file &);
    octree_file& operator=(octree_file&);
};

struct view_pane {
    double left, right, top, bottom;
};

void octree_draw(octree_file* file, surface surf, view_pane view, glm::dvec3 position, glm::dmat3 orientation);

#endif
