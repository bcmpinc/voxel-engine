/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2015  B.J. Conijn <bcmpinc@users.sourceforge.net>

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

#ifndef SURFACE_H
#define SURFACE_H
#include <stdint.h>

struct surface {
    uint32_t * data;
    uint32_t width;
    uint32_t height;
    surface() : data(nullptr), width(-1), height(-1) {}
    surface(uint32_t * data, uint32_t width, uint32_t height) : data(data), width(width), height(height) {}
    void export_png(const char * filename);
    void pixel(uint32_t x, uint32_t y, uint32_t c);
};

#endif
