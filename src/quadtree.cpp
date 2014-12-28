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

#include <cassert>
#include <cstring>
#include "quadtree.h"

static const uint32_t B[] = {0x00FF00FF, 0x0F0F0F0F, 0x33333333, 0x55555555};
static const uint32_t S[] = {8, 4, 2, 1};

void quadtree::set(uint32_t x, uint32_t y) {
    for (int i=0; i<4; i++) {
        x = (x | (x << S[i])) & B[i];
        y = (y | (y << S[i])) & B[i];
    }
    map[M + (x | (y<<1))] = 1;
}

void quadtree::set_face(uint32_t v, uint32_t color) {
    // Uses 5-10 ms per frame.
    map[v] = 0;
    v -= M;
    uint32_t x = v;
    uint32_t y = v>>1;
    for (int i=3; i>=0; i--) {
        x &= B[i];
        y &= B[i];
        x = (x | (x >> S[i]));
        y = (y | (y >> S[i]));
    }
    x &= 0xffff;
    y &= 0xffff;
    pixel(x, y, color);
}

void quadtree::pixel(uint32_t x, uint32_t y, uint32_t c) {
    assert(x<width && y<height);
    int64_t i = x+y*width;
    pixels[i] = c;
}

quadtree::quadtree(uint32_t width, uint32_t height, uint32_t* pixels) 
    : width(width), height(height), pixels(pixels) {
    memset(map,0,sizeof(map));
}

void quadtree::compute(uint32_t i) {
    map[i] = children[i+1] > 0;
}

void quadtree::build_fill(uint32_t i) {
    int n=1;
    while (i<N) {
        for (int j=0; j<n; j++) {
            map[i+j]=1;
        }
        i++;
        i<<=2;
        n<<=2;
    }
    
}

void quadtree::build_check(int w, int h, unsigned int i, int size) {
    // Check if entirely outside of frustum.
    if (w<=0 || h<=0) {
        map[i]=0;
        return;
    }
    // Check if partially out of frustum.
    if (i<L && (w<size || h<size)) {
        map[i]=1;
        size/=2;
        build_check(w,     h,     i*4+4,size);
        build_check(w-size,h,     i*4+5,size);
        build_check(w,     h-size,i*4+6,size);
        build_check(w-size,h-size,i*4+7,size);
        return;
    }
    build_fill(i);
}

void quadtree::build() {
    int size = SIZE/2;
    build_check(width,      height,      0, size);
    build_check(width-size, height,      1, size);
    build_check(width,      height-size, 2, size);
    build_check(width-size, height-size, 3, size);
}

const unsigned int quadtree::dim;
const unsigned int quadtree::N;
const unsigned int quadtree::M;
const unsigned int quadtree::L;
const unsigned int quadtree::SIZE;

    