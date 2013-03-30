#include <cstdio>
#include <cassert>
#include "common.h"

float * z_buf;
int * pixs2;

inline void pix(int x, int y, float z, int c) {
    int i = x+y*SCREEN_WIDTH;
    if (x>=0 && y>=0 && x<SCREEN_WIDTH && y<SCREEN_HEIGHT && z_buf[i]>z) {
        pixs[i] = c;
        z_buf[i] = z;
    }
}
inline void pix(float x, float y, float z, int c) {
    pix((int)x,(int)y, z, c);
}
#define CLAMP(x) (x<0?0:x>255?255:x)
inline int rgb(int r, int g, int b) {
    return CLAMP(r)<<16|CLAMP(g)<<8|CLAMP(b);
}

/** A node in an octree. */
struct octree {
    octree * c[8];
    int avgcolor;
    octree() : c{0,0,0,0,0,0,0,0}, avgcolor(0) {   
    }
    ~octree() { for (int i=0; i<8; i++) delete c[i]; }
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
        for (int i=0; i<8; i++) {
            if(c[i]) c[i]->average();
        }
        int r=0, g=0, b=0, n=0;
        for (int i=0; i<8; i++) {
            if(c[i]) {
                int v = c[i]->avgcolor;
                r += (v&0xff0000>>16);
                g += (v&0xff00>>8);
                b += (v&0xff);
                n++;
            }
        }
        r += n/2;
        g += n/2;
        b += n/2;
        avgcolor = rgb(r,g,b);
    }
    void draw(float x, float y, float z, float scale) {
        scale/=2;
        bool leaf=true;
        for (int i=0; i<8; i++) {
            if(c[i]) {
                c[i]->draw(x+(i&4?scale:-scale),y+(i&2?scale:-scale),z+(i&1?scale:-scale), scale);
                leaf = false;
            }
        }
        if (leaf) {
            if (scale > 0.05) {
                for (int i=0; i<8; i++) {
                    draw(x+(i&4?scale:-scale),y+(i&2?scale:-scale),z+(i&1?scale:-scale), scale);
                }
            } else {
                float nx =   x*pos.cphi + z*pos.sphi;
                float nz = - x*pos.sphi + z*pos.cphi;
                float ny =   y*pos.crho -nz*pos.srho;
                       z =   y*pos.srho +nz*pos.crho;
                if (z>0)
                    pix(nx*SCREEN_HEIGHT/z+SCREEN_WIDTH/2,-ny*SCREEN_HEIGHT/z+SCREEN_HEIGHT/2, z, avgcolor);
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
    int cnt;
    fscanf(f, "%d", &cnt);

    // Read voxels and store them 
    for (int i=0; i<cnt; i++) {
        int x,y,z,c;
        fscanf(f, "%d %d %d %x", &x, &y, &z, &c);
        c=((c&0xff)<<16)|(c&0xff00)|((c&0xff0000)>>16)|0xff000000;
        M.set(x,y,z,4,c);
    }
    fclose(f);
}

/** Initialize scene. */
void init () {
    load_voxel("sign.vxl");
    z_buf = new float[SCREEN_HEIGHT*SCREEN_WIDTH];
    pixs2 = new int[SCREEN_HEIGHT*SCREEN_WIDTH];
}

/** Draw anything on the screen. */
void draw() {
    for (int i = 0; i<SCREEN_HEIGHT*SCREEN_WIDTH; i++) z_buf[i] = 1e30f;
    M.draw(-pos.x,-pos.y,-pos.z,8);
}

