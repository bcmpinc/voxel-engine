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

#ifndef SSAO_H
#define SSAO_H

#include <cstdint>

struct surface;

static const int SSAO_SIZE = 8;
static const int SSAO_PROBES_PER_PIXEL = 8;

struct ssao {
    struct probe {
        int dx, dy;
        int offset;
        int64_t z;
        int64_t range;
    };
    const double projection;
    const int radius;
    const int stride;
    
    int lightmap[SSAO_PROBES_PER_PIXEL][256];
    probe probes[SSAO_SIZE*SSAO_SIZE*SSAO_PROBES_PER_PIXEL];
    
    ssao(int radius, double projection, int stride);
    uint32_t modulate(uint32_t color, int light);
    void apply(const surface &target);
};

#endif
