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
    uint32_t v = N + (x | (y<<1));
    children[v/4] &= ~(16<<(v&3));
}

void quadtree::draw(uint32_t v, uint32_t color) {
    // Uses 5-10 ms per frame.
    // children[v/4] &= ~(16<<(v&3)); // Moved to octree_draw.
    v -= N;
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
        memset(children, 0, sizeof(children));
}

void quadtree::build_fill(int i) {
    int n=1;
    while (i<N) {
        for (int j=0; j<n; j++) {
            children[i+j]=0xf0;
        }
        i++;
        i<<=2;
        n<<=2;
    }
    
}

bool quadtree::build_check(int w, int h, int i, int size) {
    if (i < N) {
        children[i]=0;
    }
    // Check if entirely outside of frustum.
    if (w<=0 || h<=0) {
        return false;
    }
    // Check if partially out of frustum.
    if (i<N && (w<size || h<size)) {
        size/=2;
        children[i] |= build_check(w,     h,     i*4+4,size) << 4;
        children[i] |= build_check(w-size,h,     i*4+5,size) << 5;
        children[i] |= build_check(w,     h-size,i*4+6,size) << 6;
        children[i] |= build_check(w-size,h-size,i*4+7,size) << 7;
        return children[i];
    }
    build_fill(i);
    return true;
}

void quadtree::build() {
    build_check(width, height, -1, SIZE);
}

const unsigned int quadtree::dim;
const unsigned int quadtree::SIZE;
const int quadtree::N;
const int quadtree::M;

    