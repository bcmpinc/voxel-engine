#include <cstdio>
#include <cassert>
#include <algorithm>
#include "common.h"

static const bool PRUNE_NODES = false;

using std::max;
using std::min;

float * z_buf;
int * pixs2;

inline void pix(int x, int y, float z, int c) {
    int i = x+(y+1)*(SCREEN_WIDTH+2)+1;
    if (x>=-1 && y>=-1 && x<SCREEN_WIDTH+1 && y<SCREEN_HEIGHT+1 && z_buf[i]>z) {
        pixs2[i] = c;
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
inline int rgb(float r, float g, float b) {
    return rgb((int)(r+0.5),(int)(g+0.5),(int)(b+0.5));
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
        bool leaf=true;
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
                    for (int j=0; j<8; j++) {
                        if(c[i]->c[j]) goto non_leaf;
                    }
                    delete c[i];
                    c[i] = NULL;
                    non_leaf:;
                }
            }            
        }
    }
    void draw(float x, float y, float z, float scale) {
        scale/=2;
        bool leaf=true;
        if (
//           x*x+y*y+z*z < 4e6*scale*scale
            fabs(x) < 1000*scale &&
            fabs(y) < 1000*scale &&
            fabs(z) < 1000*scale
        ) {
            for (int i=0; i<8; i++) {
                if(c[i]) {
                    c[i]->draw(x+(i&4?scale:-scale),y+(i&2?scale:-scale),z+(i&1?scale:-scale), scale);
                    leaf = false;
                }
            }
        }
        if (leaf) {
            float nx =   x*pos.cphi + z*pos.sphi;
            float nz = - x*pos.sphi + z*pos.cphi;
            float ny =   y*pos.crho -nz*pos.srho;
                   z =   y*pos.srho +nz*pos.crho;
            if (z>1e-10) {
                int px = nx*SCREEN_HEIGHT/z+SCREEN_WIDTH/2;
                int py = -ny*SCREEN_HEIGHT/z+SCREEN_HEIGHT/2;
                pix(px, py, z, avgcolor);
                pix(px+1, py, z, avgcolor);
                pix(px, py+1, z, avgcolor);
                pix(px-1, py, z, avgcolor);
                pix(px, py-1, z, avgcolor);
            }
        }
    }
};

/** Le scene. */
static octree M;

/** Reads in the voxel. */
static void load_voxel(const char * filename,int scale=0) {
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
        M.set(x<<scale,y<<scale,z<<scale,20,c);
    }
    fclose(f);
}

/** Initialize scene. */
void init () {
    //load_voxel("sign.vxl",16);
    load_voxel("points.vxl");
    M.average();
    z_buf = new float[(SCREEN_HEIGHT+2)*(SCREEN_WIDTH+2)];
    pixs2 = new int[(SCREEN_HEIGHT+2)*(SCREEN_WIDTH+2)];
}

void holefill() {
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            int i = x+1+(y+1)*(SCREEN_WIDTH+2);
            float depth = min(z_buf[i],min(max(z_buf[i-1],z_buf[i+1]),max(z_buf[i-SCREEN_WIDTH-2],z_buf[i+SCREEN_WIDTH+2])));
            if (z_buf[i]<depth*1.05) {
                pixs[x+y*SCREEN_WIDTH] = pixs2[i];
            } else {
                float r=0,g=0,b=0,n=0;
                for (int y2=y;y2<=y+2;y2++) {
                    for (int x2=x;x2<=x+2;x2++) {
                        int j = x2 + y2*(SCREEN_WIDTH+2);
                        if (depth*0.95<z_buf[j] && z_buf[j]<depth*1.05) {
                            n++;
                            int v = pixs2[j];
                            r += (v&0xff0000>>16);
                            g += (v&0xff00>>8);
                            b += (v&0xff);
                            n++;
                        }
                    }
                }
                pixs[x+y*SCREEN_WIDTH] = rgb(r/n,g/n,b/n);
            }
        }
    }
}

/** Draw anything on the screen. */
void draw() {
    for (int i = 0; i<(SCREEN_HEIGHT+2)*(SCREEN_WIDTH+2); i++) {
      z_buf[i] = 1e30f;
      pixs2[i] = 0x8080b0;
    }
    M.draw(-pos.x,-pos.y,-pos.z,8);
    holefill();
}

