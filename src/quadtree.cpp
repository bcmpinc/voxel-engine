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

static const unsigned int B[] = {0x00FF00FF, 0x0F0F0F0F, 0x33333333, 0x55555555};
static const unsigned int S[] = {8, 4, 2, 1};

/**
 * Sets a single value at given coordinates on the bottom level of the tree.
 */
void quadtree::set(int x, int y) {
    for (int i=0; i<4; i++) {
        x = (x | (x << S[i])) & B[i];
        y = (y | (y << S[i])) & B[i];
    }
    map[M + (x | (y<<1))] = 1;
    face[x | (y<<1)] = 0x3f7fff;
}

void quadtree::set_face(int v, int color) {
    map[v] = 0;
    v -= M;
    int x = v;
    int y = v>>1;
    for (int i=3; i>=0; i--) {
        x &= B[i];
        y &= B[i];
        x = (x | (x >> S[i]));
        y = (y | (y >> S[i]));
    }
    x &= 0xffff;
    y &= 0xffff;
    face[x + y*SIZE] = color;
}    

/**
 * Resets the quadtree, such that it is 0 everywhere
 */
quadtree::quadtree() {
    memset(map,0,sizeof(map));
}

/** 
 * Sets given node to 1 if one of its children is nonzero. 
 */
void quadtree::compute(unsigned int i) {
    if (children[i+1]==0) map[i] = 0;
}

void quadtree::build_fill(unsigned int i) {
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

void quadtree::build_check(glm::dvec3 * normals, unsigned int i, int x1, int x2, int y1, int y2) {
    // Check if entirely outside of frustum.
    for (int j=0; j<4; j++) {
        if (
            glm::dot(glm::dvec3(x1,y1,SIZE), normals[j])<0 &&
            glm::dot(glm::dvec3(x2,y1,SIZE), normals[j])<0 &&
            glm::dot(glm::dvec3(x1,y2,SIZE), normals[j])<0 &&
            glm::dot(glm::dvec3(x2,y2,SIZE), normals[j])<0
        ) {
            map[i]=0;
            return;
        }
    }
    // Check if partially out of frustum.
    if (i<L) {
        for (int j=0; j<4; j++) {
            if (
                glm::dot(glm::dvec3(x1,y1,SIZE), normals[j])<0 ||
                glm::dot(glm::dvec3(x2,y1,SIZE), normals[j])<0 ||
                glm::dot(glm::dvec3(x1,y2,SIZE), normals[j])<0 ||
                glm::dot(glm::dvec3(x2,y2,SIZE), normals[j])<0
            ) {
                int xm = (x1+x2)/2;
                int ym = (y1+y2)/2;
                map[i]=1;
                build_check(normals,i*4+4,x1,xm,y1,ym);
                build_check(normals,i*4+5,xm,x2,y1,ym);
                build_check(normals,i*4+6,x1,xm,ym,y2);
                build_check(normals,i*4+7,xm,x2,ym,y2);
                return;
            }
        }
    }
    build_fill(i);
}

/**
 * Ensures that a node is non-zero if one of its children is nonzero.
 */
void quadtree::build(glm::dvec3 * normals) {
    build_check(normals,0,-SIZE,0,-SIZE,0);
    build_check(normals,1, 0,SIZE,-SIZE,0);
    build_check(normals,2,-SIZE,0, 0,SIZE);
    build_check(normals,3, 0,SIZE, 0,SIZE);
}

void quadtree::clear_face() {
    memset(face,0xc0,sizeof(face));
}

const unsigned int quadtree::dim;
const unsigned int quadtree::N;
const unsigned int quadtree::M;
const unsigned int quadtree::L;
const unsigned int quadtree::SIZE;

    