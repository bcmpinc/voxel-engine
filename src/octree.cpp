#include <cstdio>
#include <cassert>
#include <algorithm>
#include <cstdint>

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
            glm::dvec3 n = orientation * glm::dvec3(x,y,z);
            if (n.z>1e-10) {
                int64_t px = SCREEN_WIDTH/2  + n.x*SCREEN_HEIGHT/n.z;
                int64_t py = SCREEN_HEIGHT/2 - n.y*SCREEN_HEIGHT/n.z;
                pix(px, py, n.z, avgcolor);
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
        M.set(x,y,z,OCTREE_DEPTH,c);
    }
    fclose(f);
}

struct Quadtree {
    uint64_t depth;
    Quadtree* c[4];
    Quadtree() : depth(1ULL<<60), c{0,0,0,0} {}
    ~Quadtree() {
        for (int i=0; i<4; i++) {
            delete c[i];
        }
    }
    void init(int x, int y, int s) {
        if (x+s<=0 || y+s<=0 || x>=SCREEN_WIDTH || y>=SCREEN_HEIGHT) {
            depth=-1;
            return;
        }
        s>>=1;
        if (s) {
            for (int i=0; i<4; i++) {
                c[i] = new Quadtree();
                c[i]->init(x+s*(i&1),y+s*(i/2),s);
            }
        }
    }
};

static Quadtree Q;

/** Initialize scene. */
void init_octree () {
    //load_voxel("vxl/sign.vxl");
    load_voxel("vxl/mulch.vxl");
    //load_voxel("vxl/points.vxl");
    M.average();
    M.replicate(2,6);
    
    Q.init(SCREEN_WIDTH/2-512,SCREEN_HEIGHT/2-512,1024);
}

/** Draw anything on the screen. */
void draw_octree() {
    M.draw(SCENE_SIZE-position.x,SCENE_SIZE-position.y,SCENE_SIZE-position.z,OCTREE_DEPTH);
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
