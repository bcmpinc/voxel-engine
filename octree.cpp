#include <cstdio>
#include <cassert>
#include <algorithm>
#include <cstdint>
#include "common.h"

static const bool PRUNE_NODES = false;
static const int OCTREE_DEPTH = 20;
static const int64_t SCENE_SIZE = 1 << OCTREE_DEPTH;

using std::max;
using std::min;

int64_t * zbuf;

inline void pix(int64_t x, int64_t y, int64_t z, int c) {
    if (x>=0 && y>=0 && x<SCREEN_WIDTH && y<SCREEN_HEIGHT) {
        int64_t i = x+y*(SCREEN_WIDTH);
        if (zbuf[i]>z) {
            pixs[i] = c;
            zbuf[i] = z;
            //pos.points_rendered++;
        }
    }
}
#define CLAMP(x) (x<0?0:x>255?255:x)
inline int rgb(int r, int g, int b) {
    return CLAMP(r)<<16|CLAMP(g)<<8|CLAMP(b);
}
inline int rgb(float r, float g, float b) {
    return rgb((int)(r+0.5),(int)(g+0.5),(int)(b+0.5));
}


/** A node in an octree. */
struct octree {
    octree * c[8];
    int avgcolor;
    bool leaf;
    octree() : c{0,0,0,0,0,0,0,0}, avgcolor(0) {   
    }
    ~octree() { 
        for (int i=0; i<8; i++)
            for (int j=i+1; j<8; j++)
                if(c[i]==c[j]) c[j] = NULL;
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
        if (leaf) return;
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
            abs(x) < (sscale*350) &&
            abs(y) < (sscale*350) &&
            abs(z) < (sscale*350) &&
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
            int64_t nx = (  x*pos.cphi + z*pos.sphi) >> 16;
            int64_t nz = (- x*pos.sphi + z*pos.cphi) >> 16;
            int64_t ny = (  y*pos.crho -nz*pos.srho) >> 16;
            int64_t mz = (  y*pos.srho +nz*pos.crho) >> 16;
            if (mz>1e-10) {
                int64_t px = SCREEN_WIDTH/2  + nx*SCREEN_HEIGHT/mz;
                int64_t py = SCREEN_HEIGHT/2 - ny*SCREEN_HEIGHT/mz;
                pix(px, py, mz, avgcolor);
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
    int cnt=5000000;

    // Read voxels and store them 
    for (int i=0; i<cnt; i++) {
        int x,y,z,c;
        int res = fscanf(f, "%d %d %d %x", &x, &y, &z, &c);
        if (res<4) break;
        c=((c&0xff)<<16)|(c&0xff00)|((c&0xff0000)>>16)|0xff000000;
        M.set(x,y,z,OCTREE_DEPTH,c);
    }
    fclose(f);
}

/** Initialize scene. */
void init () {
    //load_voxel("sign.vxl");
    //load_voxel("mulch.vxl");
    load_voxel("points.vxl");
    M.average();
    //M.replicate(2,6);
    zbuf = new int64_t[(SCREEN_HEIGHT)*(SCREEN_WIDTH)];
}

void holefill() {
    static const int W1=SCREEN_WIDTH;
    static const int W2=SCREEN_WIDTH*2;
    static const int W3=SCREEN_WIDTH*3;
    // #..#
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH-3; x++) {
            int i = x+y*SCREEN_WIDTH;
            int64_t depth = min(zbuf[i], zbuf[i+3]);
            depth += depth>>8;
            if (
                zbuf[i  ]<depth &&
                zbuf[i+1]>depth && 
                zbuf[i+2]>depth &&
                zbuf[i+3]<depth
            ) {
                zbuf[i+1] = zbuf[i];
                pixs[i+1] = pixs[i];
                zbuf[i+2] = zbuf[i+3];
                pixs[i+2] = pixs[i+3];
            }
        }
    }
    
    
    // #
    // .
    // #
    for (int y=0; y<SCREEN_HEIGHT-2; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            int i = x+y*SCREEN_WIDTH;
            int64_t depth = min(zbuf[i], zbuf[i+W2]);
            depth += depth>>8;
            if (
                zbuf[i  ]<depth &&
                zbuf[i+W1]>depth && 
                zbuf[i+W2]<depth
            ) {
                zbuf[i+W1] = (zbuf[i]+zbuf[i+W2])/2;
                pixs[i+W1] = ((pixs[i]&0xfefefe)+(pixs[i+W2]&0xfefefe))/2;
            }
        }
    }
    
    
    // #
    // .
    // .
    // #
    for (int y=0; y<SCREEN_HEIGHT-3; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            int i = x+y*SCREEN_WIDTH;
            int64_t depth = min(zbuf[i], zbuf[i+W3]);
            depth += depth>>8;
            if (
                zbuf[i  ]<depth &&
                zbuf[i+W1]>depth && 
                zbuf[i+W2]>depth &&
                zbuf[i+W3]<depth
            ) {
                zbuf[i+W1] = zbuf[i];
                pixs[i+W1] = pixs[i];
                zbuf[i+W2] = zbuf[i+W3];
                pixs[i+W2] = pixs[i+W3];
            }
        }
    }
    
    
    // #.#
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH-2; x++) {
            int i = x+y*SCREEN_WIDTH;
            int64_t depth = min(zbuf[i], zbuf[i+2]);
            depth += depth>>8;
            if (
                zbuf[i  ]<depth &&
                zbuf[i+1]>depth && 
                zbuf[i+2]<depth
            ) {
                zbuf[i+1] = (zbuf[i]+zbuf[i+2])/2;
                pixs[i+1] = ((pixs[i]&0xfefefe)+(pixs[i+2]&0xfefefe))/2;
            }
        }
    }
}

/** Draw anything on the screen. */
void draw() {
    for (int i = 0; i<(SCREEN_HEIGHT)*(SCREEN_WIDTH); i++) {
      zbuf[i] = 1LL<<60;
      pixs[i] = 0x8080b0;
    }
    pos.points_rendered = 0;
    
    M.draw(SCENE_SIZE-pos.x,SCENE_SIZE-pos.y,SCENE_SIZE-pos.z,OCTREE_DEPTH);
    holefill();
}

