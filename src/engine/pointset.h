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

#ifndef POINTSET_H
#define POINTSET_H
#include <stdint.h>

struct point {
    uint32_t x,y,z,c;
    point() {}
    point(uint32_t x, uint32_t y, uint32_t z, uint32_t c) : x(x),y(y),z(z),c(c) {}
};

/**
 * Opens a pointset file for reading.
 * Can also be opened in write mode for transforming or sorting the points.
 * Write access must be enabled before the data can be modified.
 * Points cannot be added or removed.
 */
struct pointset {
    bool write;
    uint32_t size; /// Number of bytes in the pointfile.
    uint32_t length; /// Number of points in the pointfile.
    int32_t fd;
    point * list;
    pointset(const char* filename, bool write=false);
    ~pointset();
    void enable_write(bool flag);
};

/**
 * Opens a file for writing out points.
 */
struct pointfile {
    int32_t fd;
    point * buffer;
    int cnt;
    pointfile(const char* filename);
    ~pointfile();
    void add(const point &p);
};

#endif
