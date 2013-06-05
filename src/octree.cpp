#include <cstdio>
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <SDL_video.h>

#include "art.h"
#include "events.h"

static const bool PRUNE_NODES = false;
static const int OCTREE_DEPTH = 20;
static const int64_t SCENE_SIZE = 1 << OCTREE_DEPTH;

using std::max;
using std::min;


/** A node in an octree. */
struct octree {
    octree * c[8];
    int avgcolor;
    bool leaf;
    octree() : c{0,0,0,0,0,0,0,0}, avgcolor(0) {   
    }
    ~octree() { 
        for (int i=0; i<8; i++) {
            if(c[i]==this) c[i] = NULL;
            else for (int j=i+1; j<8; j++)
                if(c[i]==c[j]) c[j] = NULL;
        }
        for (int i=0; i<8; i++) delete c[i]; 
    }
    void set(int x, int y, int z, int depth, int color) {
        if (depth==0) {
            avgcolor = color;   
        } else {
            depth--;
            assert(depth>=0);
            assert(depth<30);
            int mask = 1 << depth;
            int idx = ((x&mask) * 4 + (y&mask) * 2 + (z&mask)) >> depth;
            assert(idx>=0);
            assert(idx<8);
            if (c[idx]==NULL) c[idx] = new octree;
            c[idx]->set(x,y,z, depth, color);
        }
    }
    void average() {
        leaf=true;
        for (int i=0; i<8; i++) {
            if(c[i]) {
                c[i]->average();
                leaf = false;
            }
        }
        if (leaf) {
            for (int i=0; i<8; i++) {
                c[i]=this;
            }
            return;
        }
        float r=0, g=0, b=0;
        int n=0;
        for (int i=0; i<8; i++) {
            if(c[i]) {
                int v = c[i]->avgcolor;
                r += (v&0xff0000)>>16;
                g += (v&0xff00)>>8;
                b += (v&0xff);
                n++;
            }
        }
        if (n>1 || !PRUNE_NODES) {
            avgcolor = rgb(r/n,g/n,b/n);
        } else {
            // Prune single nodes.
            for (int i=0; i<8; i++) {
                if(c[i]) {
                    avgcolor = c[i]->avgcolor;
                    if (c[i]->leaf) {
                        delete c[i];
                        c[i] = NULL;
                    }
                }
            }            
        }
    }
    void replicate(int mask=2, int depth=0) {
        if (depth<=0) return;
        for (int i=0; i<8; i++) {
            if (i == (i&mask)) {
                if (c[i]) c[i]->replicate(mask, depth-1);
            } else {
                c[i] = c[i&mask];
            }
        }
    }
    void draw(int64_t x, int64_t y, int64_t z, int64_t scale) {
        scale--;
        int64_t sscale = 1<<scale;
        x-=sscale;
        y-=sscale;
        z-=sscale;
        if (
            !leaf &&
            abs(x) < (sscale*250) &&
            abs(y) < (sscale*250) &&
            abs(z) < (sscale*250) &&
            scale > 0 
        ) {
            for (int i=0; i<8; i++) {
                if(c[i]) {
                    c[i]->draw(
                        x+(((i&4)>>2)<<scale),
                        y+(((i&2)>>1)<<scale),
                        z+(((i&1)>>0)<<scale), 
                    scale);
                }
            }
        } else {
            glm::dvec3 n = orientation * glm::dvec3(x,y,z);
            if (n.z>1e-10) {
                int64_t px = SCREEN_WIDTH/2  + n.x*SCREEN_HEIGHT/n.z;
                int64_t py = SCREEN_HEIGHT/2 - n.y*SCREEN_HEIGHT/n.z;
                pix(px, py, avgcolor);
                /*pix(px+1, py+1, mz, avgcolor);
                pix(px-1, py+1, mz, avgcolor);
                pix(px-1, py-1, mz, avgcolor);
                pix(px+1, py-1, mz, avgcolor);
                pos.points_rendered++;*/
            }
        }
    }
};

/** Le scene. */
static octree M;

/** Reads in the voxel. */
static void load_voxel(const char * filename) {
    // Open the file
    FILE * f = fopen(filename, "r");
    assert(f != NULL);
    int cnt=2000000;

    // Read voxels and store them 
    for (int i=0; i<cnt; i++) {
        int x,y,z,c;
        int res = fscanf(f, "%d %d %d %x", &x, &y, &z, &c);
        if (res<4) break;
        c=((c&0xff)<<16)|(c&0xff00)|((c&0xff0000)>>16)|0xff000000;
        M.set(x,y,z,6,c);
    }
    fclose(f);
}

int * rendered[10];

/** Initialize scene. */
void init_octree () {
    load_voxel("vxl/sign.vxl");
    //load_voxel("vxl/mulch.vxl");
    //load_voxel("vxl/test.vxl");
    //load_voxel("vxl/points.vxl");
    M.average();
    M.replicate(2,2);
    
    for (int i=0; i<10; i++) {
        rendered[i] = new int[1<<(2*i)];
    }
}

#define ONE SCENE_SIZE
void traverse_zpp(
    SDL_Surface * f, int rd, int rx, int ry, octree * s, 
    int x1, int x2, int x1p, int x2p, 
    int y1, int y2, int y1p, int y2p,
    int d
){
    assert(rx>=0);
    assert(ry>=0);
    assert(rx<1<<rd);
    assert(ry<1<<rd);
     //((uint32_t*)f->pixels)[512+(rx<<(9-rd))+1024*(ry<<(9-rd))] = rd*0x1c1c1c;
        
    // occlusion
    if (s==NULL) return;
    if (rendered[rd][rx+(ry<<rd)]) return;
    if (x2<-ONE || ONE<x1-2*x1p) return;
    if (y2<-ONE || ONE<y1-2*y1p) return;
    if (x2<x1) return;
    if (y2<y1) return;
    
    // rendering
    if (rd==9) {
        ((uint32_t*)f->pixels)[512+rx+1024*(511-ry)] = s->avgcolor;
        rendered[9][rx+512*ry] = 1;
        return;
    }
    
    // Recursion
    if (x2-x1 <= 2*ONE && y2-y1 <= 2*ONE && d < 32) {
        // Traverse octree
        // x4 y2 z1
        traverse_zpp(f, rd, rx, ry, s->c[0], 2*(x1-x1p)+ONE,2*(x2-x2p)+ONE,x1p,x2p, 2*(y1-y1p)+ONE,2*(y2-y2p)+ONE,y1p,y2p,d+1);
        traverse_zpp(f, rd, rx, ry, s->c[2], 2*(x1-x1p)+ONE,2*(x2-x2p)+ONE,x1p,x2p, 2*(y1-y1p)-ONE,2*(y2-y2p)-ONE,y1p,y2p,d+1);
        traverse_zpp(f, rd, rx, ry, s->c[4], 2*(x1-x1p)-ONE,2*(x2-x2p)-ONE,x1p,x2p, 2*(y1-y1p)+ONE,2*(y2-y2p)+ONE,y1p,y2p,d+1);
        traverse_zpp(f, rd, rx, ry, s->c[6], 2*(x1-x1p)-ONE,2*(x2-x2p)-ONE,x1p,x2p, 2*(y1-y1p)-ONE,2*(y2-y2p)-ONE,y1p,y2p,d+1);
        traverse_zpp(f, rd, rx, ry, s->c[1], 2*x1+ONE,2*x2+ONE,x1p,x2p, 2*y1+ONE,2*y2+ONE,y1p,y2p,d+1);
        traverse_zpp(f, rd, rx, ry, s->c[3], 2*x1+ONE,2*x2+ONE,x1p,x2p, 2*y1-ONE,2*y2-ONE,y1p,y2p,d+1);
        traverse_zpp(f, rd, rx, ry, s->c[5], 2*x1-ONE,2*x2-ONE,x1p,x2p, 2*y1+ONE,2*y2+ONE,y1p,y2p,d+1);
        traverse_zpp(f, rd, rx, ry, s->c[7], 2*x1-ONE,2*x2-ONE,x1p,x2p, 2*y1-ONE,2*y2-ONE,y1p,y2p,d+1);
    } else {
        // Traverse quadtree
        int xm = (x1+x2)/2;
        int xmp = (x1p+x2p)/2;
        int ym = (y1+y2)/2;
        int ymp = (y1p+y2p)/2;
        traverse_zpp(f, rd+1, rx*2  , ry*2  , s, x1, xm, x1p, xmp, y1, ym, y1p, ymp, d );
        traverse_zpp(f, rd+1, rx*2+1, ry*2  , s, xm, x2, xmp, x2p, y1, ym, y1p, ymp, d );
        traverse_zpp(f, rd+1, rx*2  , ry*2+1, s, x1, xm, x1p, xmp, ym, y2, ymp, y2p, d );
        traverse_zpp(f, rd+1, rx*2+1, ry*2+1, s, xm, x2, xmp, x2p, ym, y2, ymp, y2p, d );
        rendered[rd][rx+(ry<<rd)] = 
            rendered[rd+1][2*rx  +((2*ry  )<<(rd+1))] &&
            rendered[rd+1][2*rx+1+((2*ry  )<<(rd+1))] && 
            rendered[rd+1][2*rx  +((2*ry+1)<<(rd+1))] &&
            rendered[rd+1][2*rx+1+((2*ry+1)<<(rd+1))];
    }
}

void traverse_znp(
    SDL_Surface * f, int rd, int rx, int ry, octree * s, 
    int x1, int x2, int x1p, int x2p, 
    int y1, int y2, int y1p, int y2p,
    int d
){
    assert(rx>=0);
    assert(ry>=0);
    assert(rx<1<<rd);
    assert(ry<1<<rd);
     //((uint32_t*)f->pixels)[512+(rx<<(9-rd))+1024*(ry<<(9-rd))] = rd*0x1c1c1c;
        
    // occlusion
    if (s==NULL) return;
    if (rendered[rd][rx+(ry<<rd)]) return;
    if (x2-2*x2p<-ONE || ONE<x1) return;
    if (y2<-ONE || ONE<y1-2*y1p) return;
    if (x2<x1) return;
    if (y2<y1) return;
    
    // rendering
    if (rd==9) {
        ((uint32_t*)f->pixels)[rx+1024*(511-ry)] = s->avgcolor;
        rendered[9][rx+512*ry] = 1;
        return;
    }
    
    // Recursion
    if (x2-x1 <= 2*ONE && y2-y1 <= 2*ONE && d < 32) {
        // Traverse octree
        // x4 y2 z1
        traverse_znp(f, rd, rx, ry, s->c[4], 2*(x1-x1p)-ONE,2*(x2-x2p)-ONE,x1p,x2p, 2*(y1-y1p)+ONE,2*(y2-y2p)+ONE,y1p,y2p,d+1);
        traverse_znp(f, rd, rx, ry, s->c[6], 2*(x1-x1p)-ONE,2*(x2-x2p)-ONE,x1p,x2p, 2*(y1-y1p)-ONE,2*(y2-y2p)-ONE,y1p,y2p,d+1);
        traverse_znp(f, rd, rx, ry, s->c[0], 2*(x1-x1p)+ONE,2*(x2-x2p)+ONE,x1p,x2p, 2*(y1-y1p)+ONE,2*(y2-y2p)+ONE,y1p,y2p,d+1);
        traverse_znp(f, rd, rx, ry, s->c[2], 2*(x1-x1p)+ONE,2*(x2-x2p)+ONE,x1p,x2p, 2*(y1-y1p)-ONE,2*(y2-y2p)-ONE,y1p,y2p,d+1);
        traverse_znp(f, rd, rx, ry, s->c[5], 2*x1-ONE,2*x2-ONE,x1p,x2p, 2*y1+ONE,2*y2+ONE,y1p,y2p,d+1);
        traverse_znp(f, rd, rx, ry, s->c[7], 2*x1-ONE,2*x2-ONE,x1p,x2p, 2*y1-ONE,2*y2-ONE,y1p,y2p,d+1);
        traverse_znp(f, rd, rx, ry, s->c[1], 2*x1+ONE,2*x2+ONE,x1p,x2p, 2*y1+ONE,2*y2+ONE,y1p,y2p,d+1);
        traverse_znp(f, rd, rx, ry, s->c[3], 2*x1+ONE,2*x2+ONE,x1p,x2p, 2*y1-ONE,2*y2-ONE,y1p,y2p,d+1);
    } else {
        // Traverse quadtree
        int xm = (x1+x2)/2;
        int xmp = (x1p+x2p)/2;
        int ym = (y1+y2)/2;
        int ymp = (y1p+y2p)/2;
        traverse_znp(f, rd+1, rx*2  , ry*2  , s, x1, xm, x1p, xmp, y1, ym, y1p, ymp, d );
        traverse_znp(f, rd+1, rx*2+1, ry*2  , s, xm, x2, xmp, x2p, y1, ym, y1p, ymp, d );
        traverse_znp(f, rd+1, rx*2  , ry*2+1, s, x1, xm, x1p, xmp, ym, y2, ymp, y2p, d );
        traverse_znp(f, rd+1, rx*2+1, ry*2+1, s, xm, x2, xmp, x2p, ym, y2, ymp, y2p, d );
        rendered[rd][rx+(ry<<rd)] = 
            rendered[rd+1][2*rx  +((2*ry  )<<(rd+1))] &&
            rendered[rd+1][2*rx+1+((2*ry  )<<(rd+1))] && 
            rendered[rd+1][2*rx  +((2*ry+1)<<(rd+1))] &&
            rendered[rd+1][2*rx+1+((2*ry+1)<<(rd+1))];
    }
}

void traverse_zpn(
    SDL_Surface * f, int rd, int rx, int ry, octree * s, 
    int x1, int x2, int x1p, int x2p, 
    int y1, int y2, int y1p, int y2p,
    int d
){
    assert(rx>=0);
    assert(ry>=0);
    assert(rx<1<<rd);
    assert(ry<1<<rd);
     //((uint32_t*)f->pixels)[512+(rx<<(9-rd))+1024*(ry<<(9-rd))] = rd*0x1c1c1c;
        
    // occlusion
    if (s==NULL) return;
    if (rendered[rd][rx+(ry<<rd)]) return;
    if (x2<-ONE || ONE<x1-2*x1p) return;
    if (y2-2*y2p<-ONE || ONE<y1) return;
    if (x2<x1) return;
    if (y2<y1) return;
    
    // rendering
    if (rd==9) {
        ((uint32_t*)f->pixels)[512+rx+1024*(1023-ry)] = s->avgcolor;
        rendered[9][rx+512*ry] = 1;
        return;
    }
    
    // Recursion
    if (x2-x1 <= 2*ONE && y2-y1 <= 2*ONE && d < 32) {
        // Traverse octree
        // x4 y2 z1
        traverse_zpn(f, rd, rx, ry, s->c[2], 2*(x1-x1p)+ONE,2*(x2-x2p)+ONE,x1p,x2p, 2*(y1-y1p)-ONE,2*(y2-y2p)-ONE,y1p,y2p,d+1);
        traverse_zpn(f, rd, rx, ry, s->c[6], 2*(x1-x1p)-ONE,2*(x2-x2p)-ONE,x1p,x2p, 2*(y1-y1p)-ONE,2*(y2-y2p)-ONE,y1p,y2p,d+1);
        traverse_zpn(f, rd, rx, ry, s->c[0], 2*(x1-x1p)+ONE,2*(x2-x2p)+ONE,x1p,x2p, 2*(y1-y1p)+ONE,2*(y2-y2p)+ONE,y1p,y2p,d+1);
        traverse_zpn(f, rd, rx, ry, s->c[4], 2*(x1-x1p)-ONE,2*(x2-x2p)-ONE,x1p,x2p, 2*(y1-y1p)+ONE,2*(y2-y2p)+ONE,y1p,y2p,d+1);
        traverse_zpn(f, rd, rx, ry, s->c[3], 2*x1+ONE,2*x2+ONE,x1p,x2p, 2*y1-ONE,2*y2-ONE,y1p,y2p,d+1);
        traverse_zpn(f, rd, rx, ry, s->c[7], 2*x1-ONE,2*x2-ONE,x1p,x2p, 2*y1-ONE,2*y2-ONE,y1p,y2p,d+1);
        traverse_zpn(f, rd, rx, ry, s->c[1], 2*x1+ONE,2*x2+ONE,x1p,x2p, 2*y1+ONE,2*y2+ONE,y1p,y2p,d+1);
        traverse_zpn(f, rd, rx, ry, s->c[5], 2*x1-ONE,2*x2-ONE,x1p,x2p, 2*y1+ONE,2*y2+ONE,y1p,y2p,d+1);
    } else {
        // Traverse quadtree
        int xm = (x1+x2)/2;
        int xmp = (x1p+x2p)/2;
        int ym = (y1+y2)/2;
        int ymp = (y1p+y2p)/2;
        traverse_zpn(f, rd+1, rx*2  , ry*2  , s, x1, xm, x1p, xmp, y1, ym, y1p, ymp, d );
        traverse_zpn(f, rd+1, rx*2+1, ry*2  , s, xm, x2, xmp, x2p, y1, ym, y1p, ymp, d );
        traverse_zpn(f, rd+1, rx*2  , ry*2+1, s, x1, xm, x1p, xmp, ym, y2, ymp, y2p, d );
        traverse_zpn(f, rd+1, rx*2+1, ry*2+1, s, xm, x2, xmp, x2p, ym, y2, ymp, y2p, d );
        rendered[rd][rx+(ry<<rd)] = 
            rendered[rd+1][2*rx  +((2*ry  )<<(rd+1))] &&
            rendered[rd+1][2*rx+1+((2*ry  )<<(rd+1))] && 
            rendered[rd+1][2*rx  +((2*ry+1)<<(rd+1))] &&
            rendered[rd+1][2*rx+1+((2*ry+1)<<(rd+1))];
    }
}

void traverse_znn(
    SDL_Surface * f, int rd, int rx, int ry, octree * s, 
    int x1, int x2, int x1p, int x2p, 
    int y1, int y2, int y1p, int y2p,
    int d
){
    assert(rx>=0);
    assert(ry>=0);
    assert(rx<1<<rd);
    assert(ry<1<<rd);
     //((uint32_t*)f->pixels)[512+(rx<<(9-rd))+1024*(ry<<(9-rd))] = rd*0x1c1c1c;
        
    // occlusion
    if (s==NULL) return;
    if (rendered[rd][rx+(ry<<rd)]) return;
    if (x2-2*x2p<-ONE || ONE<x1) return;
    if (y2-2*y2p<-ONE || ONE<y1) return;
    if (x2<x1) return;
    if (y2<y1) return;
    
    // rendering
    if (rd==9) {
        ((uint32_t*)f->pixels)[rx+1024*(1023-ry)] = s->avgcolor;
        rendered[9][rx+512*ry] = 1;
        return;
    }
    
    // Recursion
    if (x2-x1 <= 2*ONE && y2-y1 <= 2*ONE && d < 32) {
        // Traverse octree
        // x4 y2 z1
        traverse_znn(f, rd, rx, ry, s->c[6], 2*(x1-x1p)-ONE,2*(x2-x2p)-ONE,x1p,x2p, 2*(y1-y1p)-ONE,2*(y2-y2p)-ONE,y1p,y2p,d+1);
        traverse_znn(f, rd, rx, ry, s->c[4], 2*(x1-x1p)-ONE,2*(x2-x2p)-ONE,x1p,x2p, 2*(y1-y1p)+ONE,2*(y2-y2p)+ONE,y1p,y2p,d+1);
        traverse_znn(f, rd, rx, ry, s->c[2], 2*(x1-x1p)+ONE,2*(x2-x2p)+ONE,x1p,x2p, 2*(y1-y1p)-ONE,2*(y2-y2p)-ONE,y1p,y2p,d+1);
        traverse_znn(f, rd, rx, ry, s->c[0], 2*(x1-x1p)+ONE,2*(x2-x2p)+ONE,x1p,x2p, 2*(y1-y1p)+ONE,2*(y2-y2p)+ONE,y1p,y2p,d+1);
        traverse_znn(f, rd, rx, ry, s->c[7], 2*x1-ONE,2*x2-ONE,x1p,x2p, 2*y1-ONE,2*y2-ONE,y1p,y2p,d+1);
        traverse_znn(f, rd, rx, ry, s->c[5], 2*x1-ONE,2*x2-ONE,x1p,x2p, 2*y1+ONE,2*y2+ONE,y1p,y2p,d+1);
        traverse_znn(f, rd, rx, ry, s->c[3], 2*x1+ONE,2*x2+ONE,x1p,x2p, 2*y1-ONE,2*y2-ONE,y1p,y2p,d+1);
        traverse_znn(f, rd, rx, ry, s->c[1], 2*x1+ONE,2*x2+ONE,x1p,x2p, 2*y1+ONE,2*y2+ONE,y1p,y2p,d+1);
    } else {
        // Traverse quadtree
        int xm = (x1+x2)/2;
        int xmp = (x1p+x2p)/2;
        int ym = (y1+y2)/2;
        int ymp = (y1p+y2p)/2;
        traverse_znn(f, rd+1, rx*2  , ry*2  , s, x1, xm, x1p, xmp, y1, ym, y1p, ymp, d );
        traverse_znn(f, rd+1, rx*2+1, ry*2  , s, xm, x2, xmp, x2p, y1, ym, y1p, ymp, d );
        traverse_znn(f, rd+1, rx*2  , ry*2+1, s, x1, xm, x1p, xmp, ym, y2, ymp, y2p, d );
        traverse_znn(f, rd+1, rx*2+1, ry*2+1, s, xm, x2, xmp, x2p, ym, y2, ymp, y2p, d );
        rendered[rd][rx+(ry<<rd)] = 
            rendered[rd+1][2*rx  +((2*ry  )<<(rd+1))] &&
            rendered[rd+1][2*rx+1+((2*ry  )<<(rd+1))] && 
            rendered[rd+1][2*rx  +((2*ry+1)<<(rd+1))] &&
            rendered[rd+1][2*rx+1+((2*ry+1)<<(rd+1))];
    }
}

/** Draw anything on the screen. */
void draw_octree(SDL_Surface ** cubemap) {
    int x = position.x;
    int y = position.y;
    int W = SCENE_SIZE/2 - position.z;
    for (int i=0; i<10; i++) {
        memset(rendered[i],0,sizeof(int)<<(2*i));
    }
    traverse_zpp(cubemap[1], 0, 0, 0, &M, x, x+W, 0, ONE, y, y+W, 0, ONE, 0);
    for (int i=0; i<10; i++) {
        memset(rendered[i],0,sizeof(int)<<(2*i));
    }
    traverse_znp(cubemap[1], 0, 0, 0, &M, x-W, x,-ONE, 0, y, y+W, 0, ONE, 0);
    for (int i=0; i<10; i++) {
        memset(rendered[i],0,sizeof(int)<<(2*i));
    }
    traverse_zpn(cubemap[1], 0, 0, 0, &M, x, x+W, 0, ONE, y-W, y,-ONE, 0, 0);
    for (int i=0; i<10; i++) {
        memset(rendered[i],0,sizeof(int)<<(2*i));
    }
    traverse_znn(cubemap[1], 0, 0, 0, &M, x-W, x,-ONE, 0, y-W, y,-ONE, 0, 0);
}

void draw_octree() {
    M.draw(SCENE_SIZE/2-position.x,SCENE_SIZE/2-position.y,SCENE_SIZE/2-position.z,OCTREE_DEPTH);
}
// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
