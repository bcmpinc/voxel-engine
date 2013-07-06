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

#ifndef OCTREE_H
#define OCTREE_H
#include <stdint.h>

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
    uint32_t child[8];
    int32_t avgcolor[8];
};

struct octree_file {
    const bool write;
    uint32_t size;
    int32_t fd;
    octree * root;
    octree_file(const char * filename);
    octree_file(const char * filename, uint32_t size);
    ~octree_file();
private:
    octree_file(octree_file &);
    octree_file& operator=(octree_file&);
};

void octree_draw(octree_file* file, uint32_t cubemap_texture);

uint32_t prepare_cubemap();

static const uint32_t OCTREE_DEPTH = 20;
static const uint32_t SCENE_SIZE = 1 << OCTREE_DEPTH;

#endif
