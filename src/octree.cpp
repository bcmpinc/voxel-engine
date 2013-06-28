#include <cstdio>
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <SDL_video.h>

#include "art.h"
#include "events.h"
#include "quadtree.h"

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
        M.set(x,y,z,OCTREE_DEPTH,c);
    }
    fclose(f);
}

typedef quadtree<9> Q;
static Q cubemap[6];

/** Initialize scene. */
void init_octree () {
    //load_voxel("vxl/sign.vxl");
    //load_voxel("vxl/mulch.vxl");
    load_voxel("vxl/test.vxl");
    //load_voxel("vxl/points.vxl");
    M.average();
    M.replicate(2,6);
    
}

#define TRAVERSE_QUADTREE(traverse) \
        /* Traverse quadtree */ \
        int xm = (x1+x2)/2; \
        int xmp = (x1p+x2p)/2; \
        int ym = (y1+y2)/2; \
        int ymp = (y1p+y2p)/2; \
        traverse(f, r*4+1, s, x1, xm, x1p, xmp, y1, ym, y1p, ymp, d ); \
        traverse(f, r*4+2, s, xm, x2, xmp, x2p, y1, ym, y1p, ymp, d ); \
        traverse(f, r*4+3, s, x1, xm, x1p, xmp, ym, y2, ymp, y2p, d ); \
        traverse(f, r*4+4, s, xm, x2, xmp, x2p, ym, y2, ymp, y2p, d ); \
        f.compute(r);


#define ONE SCENE_SIZE
template<int DX, int DY, int C0, int C1, int C2, int C3, int C4, int C5, int C6, int C7>
void traverse(
    Q& f, unsigned int r, octree * s, 
    int x1, int x2, int x1p, int x2p, 
    int y1, int y2, int y1p, int y2p,
    int d
){
    static_assert(DX==1 || DX==-1, "Wrong DX");
    static_assert(DY==1 || DY==-1, "Wrong DY");
    assert(r<Q::N);
     //((uint32_t*)f->pixels)[512+(rx<<(9-rd))+1024*(ry<<(9-rd))] = rd*0x1c1c1c;
        
    // occlusion
    if (s==NULL) return;
    if (f.map[r]==0) return;
    if (x2-(1-DX)*x2p<-ONE || ONE<x1-(1+DX)*x1p) return;
    if (y2-(1-DY)*y2p<-ONE || ONE<y1-(1+DY)*y1p) return;
    if (x2<x1) return;
    if (y2<y1) return;
    
    // rendering
    if (r>=Q::M) {
        f.face[r-Q::M] = s->avgcolor;
        f.map[r] = 0;
        return;
    }
    
    // Recursion
    if (x2-x1 <= 2*ONE && y2-y1 <= 2*ONE && d < 20) {
        // Traverse octree
        // x4 y2 z1
        traverse<DX,DY,C0,C1,C2,C3,C4,C5,C6,C7>(f, r, s->c[C0], 2*(x1-x1p)+DX*ONE,2*(x2-x2p)+DX*ONE,x1p,x2p, 2*(y1-y1p)+DY*ONE,2*(y2-y2p)+DY*ONE,y1p,y2p,d+1);
        traverse<DX,DY,C0,C1,C2,C3,C4,C5,C6,C7>(f, r, s->c[C1], 2*(x1-x1p)+DX*ONE,2*(x2-x2p)+DX*ONE,x1p,x2p, 2*(y1-y1p)-DY*ONE,2*(y2-y2p)-DY*ONE,y1p,y2p,d+1);
        traverse<DX,DY,C0,C1,C2,C3,C4,C5,C6,C7>(f, r, s->c[C2], 2*(x1-x1p)-DX*ONE,2*(x2-x2p)-DX*ONE,x1p,x2p, 2*(y1-y1p)+DY*ONE,2*(y2-y2p)+DY*ONE,y1p,y2p,d+1);
        traverse<DX,DY,C0,C1,C2,C3,C4,C5,C6,C7>(f, r, s->c[C3], 2*(x1-x1p)-DX*ONE,2*(x2-x2p)-DX*ONE,x1p,x2p, 2*(y1-y1p)-DY*ONE,2*(y2-y2p)-DY*ONE,y1p,y2p,d+1);
        traverse<DX,DY,C0,C1,C2,C3,C4,C5,C6,C7>(f, r, s->c[C4], 2*x1+DX*ONE,2*x2+DX*ONE,x1p,x2p, 2*y1+DY*ONE,2*y2+DY*ONE,y1p,y2p,d+1);
        traverse<DX,DY,C0,C1,C2,C3,C4,C5,C6,C7>(f, r, s->c[C5], 2*x1+DX*ONE,2*x2+DX*ONE,x1p,x2p, 2*y1-DY*ONE,2*y2-DY*ONE,y1p,y2p,d+1);
        traverse<DX,DY,C0,C1,C2,C3,C4,C5,C6,C7>(f, r, s->c[C6], 2*x1-DX*ONE,2*x2-DX*ONE,x1p,x2p, 2*y1+DY*ONE,2*y2+DY*ONE,y1p,y2p,d+1);
        traverse<DX,DY,C0,C1,C2,C3,C4,C5,C6,C7>(f, r, s->c[C7], 2*x1-DX*ONE,2*x2-DX*ONE,x1p,x2p, 2*y1-DY*ONE,2*y2-DY*ONE,y1p,y2p,d+1);
    } else {
        TRAVERSE_QUADTREE((traverse<DX,DY,C0,C1,C2,C3,C4,C5,C6,C7>))
    }
}

static void prepare_cubemap() {
    const int SIZE = Q::SIZE;
    // Reset the quadtrees
    for (int i=0; i<6; i++) cubemap[i].clear();
    // The orientation matrix is (asumed to be) orthogonal, and therefore can be inversed by transposition.
    glm::dmat3 inverse_orientation = glm::transpose(orientation);
    double fov = 1.0/SCREEN_HEIGHT;
    // Fill the leaf-layer of the quadtrees with wether they have a pixel location on screen.
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            glm::dvec3 p( (x-SCREEN_WIDTH/2)*fov, (SCREEN_HEIGHT/2-y)*fov, 1 );
            p = inverse_orientation * p;
            double ax=fabs(p.x);
            double ay=fabs(p.y);
            double az=fabs(p.z);
        
            if (ax>=ay && ax>=az) {
                if (p.x>0) {
                    int fx = SIZE*(-p.z/ax/2+0.5);
                    int fy = SIZE*(-p.y/ax/2+0.5);
                    cubemap[2].set(fx,fy);
                } else {
                    int fx = SIZE*(p.z/ax/2+0.5);
                    int fy = SIZE*(-p.y/ax/2+0.5);
                    cubemap[4].set(fx,fy);
                }
            } else if (ay>=ax && ay>=az) {
                if (p.y>0) {
                    int fx = SIZE*(p.x/ay/2+0.5);
                    int fy = SIZE*(p.z/ay/2+0.5);
                    cubemap[0].set(fx,fy);
                } else {
                    int fx = SIZE*(p.x/ay/2+0.5);
                    int fy = SIZE*(-p.z/ay/2+0.5);
                    cubemap[5].set(fx,fy);
                }
            } else if (az>=ax && az>=ay) {
                if (p.z>0) {
                    int fx = SIZE*(p.x/az/2+0.5);
                    int fy = SIZE*(p.y/az/2+0.5);
                    cubemap[1].set(fx,fy);
                } else {
                    int fx = SIZE*(-p.x/az/2+0.5);
                    int fy = SIZE*(p.y/az/2+0.5);
                    cubemap[3].set(fx,fy);
                }
            }
        }
    }
    // build the non-leaf layers of the quadtree
    for (int i=0; i<6; i++) cubemap[i].build(); 
}

static void draw_cubemap() {
    const int SIZE = Q::SIZE;
    // The orientation matrix is (asumed to be) orthogonal, and therefore can be inversed by transposition.
    glm::dmat3 inverse_orientation = glm::transpose(orientation);
    double fov = 1.0/SCREEN_HEIGHT;
    // render the faces of the cubemap on screen.
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            glm::dvec3 p( (x-SCREEN_WIDTH/2)*fov, (SCREEN_HEIGHT/2-y)*fov, 1 );
            p = inverse_orientation * p;
            double ax=fabs(p.x);
            double ay=fabs(p.y);
            double az=fabs(p.z);
        
            if (ax>=ay && ax>=az) {
                if (p.x>0) {
                    int fx = SIZE*(-p.z/ax/2+0.5);
                    int fy = SIZE*(-p.y/ax/2+0.5);
                    pix(x, y, cubemap[2].get_face(fx,fy));
                } else {
                    int fx = SIZE*(p.z/ax/2+0.5);
                    int fy = SIZE*(-p.y/ax/2+0.5);
                    pix(x, y, cubemap[4].get_face(fx,fy));
                }
            } else if (ay>=ax && ay>=az) {
                if (p.y>0) {
                    int fx = SIZE*(p.x/ay/2+0.5);
                    int fy = SIZE*(p.z/ay/2+0.5);
                    pix(x, y, cubemap[0].get_face(fx,fy));
                } else {
                    int fx = SIZE*(p.x/ay/2+0.5);
                    int fy = SIZE*(-p.z/ay/2+0.5);
                    pix(x, y, cubemap[5].get_face(fx,fy));
                }
            } else if (az>=ax && az>=ay) {
                if (p.z>0) {
                    int fx = SIZE*(p.x/az/2+0.5);
                    int fy = SIZE*(p.y/az/2+0.5);
                    pix(x, y, cubemap[1].get_face(fx,fy));
                } else {
                    int fx = SIZE*(-p.x/az/2+0.5);
                    int fy = SIZE*(p.y/az/2+0.5);
                    pix(x, y, cubemap[3].get_face(fx,fy));
                }
            }
        }
    }
}

/** Draw anything on the screen. */
void draw_octree() {
    int x = position.x;
    int y = position.y;
    int W = SCENE_SIZE/2 - position.z;
    prepare_cubemap();
    traverse<-1,-1,6,4,2,0,7,5,3,1>(cubemap[1], 1, &M, x-W, x,-ONE, 0, y-W, y,-ONE, 0, 0);
    traverse< 1,-1,2,0,6,4,3,1,7,5>(cubemap[1], 2, &M, x, x+W, 0, ONE, y-W, y,-ONE, 0, 0);
    traverse<-1, 1,4,6,0,2,5,7,1,3>(cubemap[1], 3, &M, x-W, x,-ONE, 0, y, y+W, 0, ONE, 0);
    traverse< 1, 1,0,2,4,6,1,3,5,7>(cubemap[1], 4, &M, x, x+W, 0, ONE, y, y+W, 0, ONE, 0);
    draw_cubemap();
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
