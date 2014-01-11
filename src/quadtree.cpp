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

#include <cstring>
#include "quadtree.h"
#include "art.h"

static const unsigned int B[] = {0x00FF00FF, 0x0F0F0F0F, 0x33333333, 0x55555555};
static const unsigned int S[] = {8, 4, 2, 1};

/**
 * Sets a single value at given coordinates on the bottom level of the tree. (unused)
 */
void quadtree::set(int x, int y) {
    // Morton 2d encode
    for (int i=0; i<3; i++) {
        x = (x | (x << S[i])) & B[i];
        y = (y | (y << S[i])) & B[i];
    }
    // Set bit
    set_bit(M + (x | (y<<2)));
}

void quadtree::set_face(int node, int bit, int color) {
    map[node] &= ~(1<<bit);
    int pos = node*16+bit+1;
    pos -= M;
    int x = pos;
    int y = pos>>2;
    for (int i=2; i>=0; i--) {
        x &= B[i];
        y &= B[i];
        x = (x | (x >> S[i]));
        y = (y | (y >> S[i]));
    }
    x &= 0xffff;
    y &= 0xffff;
    pixel(x, y, color);
}    

/**
 * Resets the quadtree, such that it is 0 everywhere
 */
quadtree::quadtree() {
    memset(map,0,sizeof(map));
}

/** 
 * Sets given node to 0 if all its children are zero. 
 */
void quadtree::compute(int i) {
    if (i>0 && (map[i]==0)) unset_bit(i-1);
}

void quadtree::build_fill(int i) {
    set_bit(i);
    int n=1;
    while (i<(signed)M) {
        for (int j=0; j<n; j++) {
            assert(i+j<(signed)M);
            map[i+j]=~0;
        }
        i*=16;
        i++;
        n*=16;
    }
    
}
/* Recurse info
 * 
 * Order:
 * 0 1 2 3
 * 4 5 6 7
 * 8 9 A B
 * C D E F
 */
int DX[] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3};
int DY[] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3};
void quadtree::build_check(int width, int height, int i, int size) {
    // Check if entirely outside of frustum.
    if (width<=0 || height<=0) {
        if (i>=0) unset_bit(i);
        return;
    }
    // Check if partially out of frustum.
    if (i<(signed)L && (width<size || height<size)) {
        if (i>=0) set_bit(i);
        size/=4;
        for (int j=0; j<16; j++) {
            build_check(width-DX[j]*size,height-DY[j]*size,i*16+j+16,size);
        }
        return;
    }
    build_fill(i);
}

/**
 * Ensures that a node is non-zero if one of its children is nonzero.
 */
void quadtree::build(int width, int height) {
    build_check(width, height, -1, SIZE);
}

const unsigned int quadtree::CHILD_COUNT;
const unsigned int quadtree::LAYERS;
const unsigned int quadtree::N;
const unsigned int quadtree::M;
const unsigned int quadtree::L;
const unsigned int quadtree::SIZE;

    