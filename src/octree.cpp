#include <cstdio>
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <list>
#include <SDL_video.h>

#include "art.h"
#include "events.h"
#include "quadtree.h"
#include "timing.h"

static const int OCTREE_DEPTH = 20;
static const int64_t SCENE_SIZE = 1 << OCTREE_DEPTH;

using std::max;
using std::min;

/** A node in an octree. */
struct octree {
    octree * c[8];
    int avgcolor[8];
    octree() : c{0,0,0,0,0,0,0,0}, avgcolor{-1,-1,-1,-1,-1,-1,-1,-1} {}
    void set(int x, int y, int z, int depth, int color);
    int average();
    void replicate(int mask=2, int depth=0);
};

struct octree_buffer {
    const int N = 65536;
    std::list<octree*> list;
    int i;
    int mc;
    octree_buffer() : i(N) {}
    ~octree_buffer() {
        for (octree* ptr : list) {
            delete[](ptr); 
        }
    }
    inline octree* allocate() {
        i++;
        if (i>=N) {
            list.push_back(new octree[N]);
            i=0;
            mc += N*sizeof(octree)>>20;
        }
        return list.back() + i;
    }
} octree_buffer;

void octree::set(int x, int y, int z, int depth, int color) {
    depth--;
    assert(depth>=0);
    assert(depth<30);
    int mask = 1 << depth;
    int idx = ((x&mask) * 4 + (y&mask) * 2 + (z&mask)) >> depth;
    assert(idx>=0);
    assert(idx<8);
    if (depth==0) {
        avgcolor[idx] = color;
    } else {
        if (c[idx]==NULL) c[idx] = octree_buffer.allocate();
        c[idx]->set(x,y,z, depth, color);
    }
}
int octree::average() {
    for (int i=0; i<8; i++) {
        if(c[i]) {
            avgcolor[i] = c[i]->average();
        }
    }
    float r=0, g=0, b=0;
    int n=0;
    for (int i=0; i<8; i++) {
        if(avgcolor[i]>=0) {
            int v = avgcolor[i];
            r += (v&0xff0000)>>16;
            g += (v&0xff00)>>8;
            b += (v&0xff);
            n++;
        }
    }
    return rgb(r/n,g/n,b/n);
}
void octree::replicate(int mask, int depth) {
    if (depth<=0) return;
    for (int i=0; i<8; i++) {
        if (i == (i&mask)) {
            if (c[i]) c[i]->replicate(mask, depth-1);
        } else {
            c[i] = c[i&mask];
            avgcolor[i] = avgcolor[i&mask];
        }
    }
}

/** Le scene. */
static octree * M;

/** Reads in the voxel. */
static void load_voxel(const char * filename, int depth, int rep_mask, int rep_depth, int ds=0) {
    // Open the file
    FILE * f = fopen(filename, "r");
    assert(f != NULL);
    int cnt=200000000;
    M = octree_buffer.allocate();

    // Read voxels and store them 
    int i;
    for (i=0; i<cnt; i++) {
        if (i%(1<<20)==0) printf("Loaded %dMi points (%dMiB)\n", i>>20, octree_buffer.mc);
        int x,y,z,c;
        int res = fscanf(f, "%d %d %d %x", &x, &y, &z, &c);
        if (res<4) break;
        c=((c&0xff)<<16)|(c&0xff00)|((c&0xff0000)>>16);
        M->set(x>>ds,y>>ds,z>>ds,depth-ds,c);
    }
    fclose(f);
    printf("Loaded %dMi points (%dMiB)\n", i>>20, octree_buffer.mc);
    M->average();
    M->replicate(rep_mask,rep_depth);
}

typedef quadtree<10> Q;
static Q cubemap[6];

/** Initialize scene. */
void init_octree () {
    Timer t;
    load_voxel("vxl/sign.vxl",  6,           2,2);
    //load_voxel("vxl/mulch.vxl", OCTREE_DEPTH,2,6);
    //load_voxel("vxl/test.vxl",  OCTREE_DEPTH,2,6);
    //load_voxel("vxl/points.vxl",OCTREE_DEPTH,7,0,7);
    //load_voxel("vxl/tower.vxl", OCTREE_DEPTH,7,0,4);
    printf("Model loaded in %6.2fms.\n", t.elapsed());

    // Reset the quadtrees
    for (int i=0; i<6; i++) cubemap[i].clear();
}

template<int DX, int DY, int C, int AX, int AY, int AZ>
struct SubFaceRenderer {
    static_assert(DX==1 || DX==-1, "Wrong DX");
    static_assert(DY==1 || DY==-1, "Wrong DY");
    static const int ONE = SCENE_SIZE;
    static void traverse(
        Q& f, unsigned int r, octree * s, int color,
        int x1, int x2, int x1p, int x2p, 
        int y1, int y2, int y1p, int y2p
    ){
        // occlusion
        if (x2-(1-DX)*x2p<=-ONE || ONE<=x1-(1+DX)*x1p) return;
        if (y2-(1-DY)*y2p<=-ONE || ONE<=y1-(1+DY)*y1p) return;
        
        // Recursion
        if (x2-x1 <= 2*ONE && y2-y1 <= 2*ONE) {
            // Traverse octree
            // x4 y2 z1
            int x3 = x1-x1p;
            int x4 = x2-x2p;
            int y3 = y1-y1p;
            int y4 = y2-y2p;
            if (s) {
                if (x3<x4 && y3<y4) {
                    if (s->avgcolor[C         ]>=0) traverse(f, r, s->c[C         ], s->avgcolor[C         ], 2*x3+DX*ONE,2*x4+DX*ONE,x1p,x2p, 2*y3+DY*ONE,2*y4+DY*ONE,y1p,y2p);
                    if (s->avgcolor[C^AX      ]>=0) traverse(f, r, s->c[C^AX      ], s->avgcolor[C^AX      ], 2*x3-DX*ONE,2*x4-DX*ONE,x1p,x2p, 2*y3+DY*ONE,2*y4+DY*ONE,y1p,y2p);
                    if (s->avgcolor[C   ^AY   ]>=0) traverse(f, r, s->c[C   ^AY   ], s->avgcolor[C   ^AY   ], 2*x3+DX*ONE,2*x4+DX*ONE,x1p,x2p, 2*y3-DY*ONE,2*y4-DY*ONE,y1p,y2p);
                    if (s->avgcolor[C^AX^AY   ]>=0) traverse(f, r, s->c[C^AX^AY   ], s->avgcolor[C^AX^AY   ], 2*x3-DX*ONE,2*x4-DX*ONE,x1p,x2p, 2*y3-DY*ONE,2*y4-DY*ONE,y1p,y2p);
                }
                if (s->avgcolor[C      ^AZ]>=0) traverse(f, r, s->c[C      ^AZ], s->avgcolor[C      ^AZ], 2*x1+DX*ONE,2*x2+DX*ONE,x1p,x2p, 2*y1+DY*ONE,2*y2+DY*ONE,y1p,y2p);
                if (s->avgcolor[C^AX   ^AZ]>=0) traverse(f, r, s->c[C^AX   ^AZ], s->avgcolor[C^AX   ^AZ], 2*x1-DX*ONE,2*x2-DX*ONE,x1p,x2p, 2*y1+DY*ONE,2*y2+DY*ONE,y1p,y2p);
                if (s->avgcolor[C   ^AY^AZ]>=0) traverse(f, r, s->c[C   ^AY^AZ], s->avgcolor[C   ^AY^AZ], 2*x1+DX*ONE,2*x2+DX*ONE,x1p,x2p, 2*y1-DY*ONE,2*y2-DY*ONE,y1p,y2p);
                if (s->avgcolor[C^AX^AY^AZ]>=0) traverse(f, r, s->c[C^AX^AY^AZ], s->avgcolor[C^AX^AY^AZ], 2*x1-DX*ONE,2*x2-DX*ONE,x1p,x2p, 2*y1-DY*ONE,2*y2-DY*ONE,y1p,y2p);
            } else {
                if (x3<x4 && y3<y4) {
                    // Skip nearest cube to avoid infinite recursion.
                    traverse(f, r, NULL, color, 2*x3-DX*ONE,2*x4-DX*ONE,x1p,x2p, 2*y3+DY*ONE,2*y4+DY*ONE,y1p,y2p);
                    traverse(f, r, NULL, color, 2*x3+DX*ONE,2*x4+DX*ONE,x1p,x2p, 2*y3-DY*ONE,2*y4-DY*ONE,y1p,y2p);
                    traverse(f, r, NULL, color, 2*x3-DX*ONE,2*x4-DX*ONE,x1p,x2p, 2*y3-DY*ONE,2*y4-DY*ONE,y1p,y2p);
                }
                traverse(f, r, NULL, color, 2*x1+DX*ONE,2*x2+DX*ONE,x1p,x2p, 2*y1+DY*ONE,2*y2+DY*ONE,y1p,y2p);
                traverse(f, r, NULL, color, 2*x1-DX*ONE,2*x2-DX*ONE,x1p,x2p, 2*y1+DY*ONE,2*y2+DY*ONE,y1p,y2p);
                traverse(f, r, NULL, color, 2*x1+DX*ONE,2*x2+DX*ONE,x1p,x2p, 2*y1-DY*ONE,2*y2-DY*ONE,y1p,y2p);
                traverse(f, r, NULL, color, 2*x1-DX*ONE,2*x2-DX*ONE,x1p,x2p, 2*y1-DY*ONE,2*y2-DY*ONE,y1p,y2p);
            }
        } else {
            int xm  = (x1 +x2 )/2; 
            int xmp = (x1p+x2p)/2; 
            int ym  = (y1 +y2 )/2; 
            int ymp = (y1p+y2p)/2; 
            if (r<Q::L) {
                // Traverse quadtree 
                if (f.map[r*4+4]) traverse(f, r*4+4, s, color, x1, xm, x1p, xmp, y1, ym, y1p, ymp); 
                if (f.map[r*4+5]) traverse(f, r*4+5, s, color, xm, x2, xmp, x2p, y1, ym, y1p, ymp); 
                if (f.map[r*4+6]) traverse(f, r*4+6, s, color, x1, xm, x1p, xmp, ym, y2, ymp, y2p); 
                if (f.map[r*4+7]) traverse(f, r*4+7, s, color, xm, x2, xmp, x2p, ym, y2, ymp, y2p); 
            } else {
                // Rendering
                if (f.map[r*4+4]) paint(f, r*4+4, color, x1, xm, x1p, xmp, y1, ym, y1p, ymp); 
                if (f.map[r*4+5]) paint(f, r*4+5, color, xm, x2, xmp, x2p, y1, ym, y1p, ymp); 
                if (f.map[r*4+6]) paint(f, r*4+6, color, x1, xm, x1p, xmp, ym, y2, ymp, y2p); 
                if (f.map[r*4+7]) paint(f, r*4+7, color, xm, x2, xmp, x2p, ym, y2, ymp, y2p); 
            }
            f.compute(r);
        }
    }
    
    static inline void paint(Q& f, unsigned int r, int color, int x1, int x2, int x1p, int x2p, int y1, int y2, int y1p, int y2p)  {
        if (x2-(1-DX)*x2p<=-ONE || ONE<=x1-(1+DX)*x1p) return;
        if (y2-(1-DY)*y2p<=-ONE || ONE<=y1-(1+DY)*y1p) return;
        f.face[r-Q::M] = color; 
        f.map[r] = 0;
    }
};

template<int C, int AX, int AY, int AZ>
struct FaceRenderer {
    static_assert(0<=C && C<8, "Invalid C");
    static_assert(AX==1 || AY==1 || AZ==1, "No z-axis.");
    static_assert(AX==2 || AY==2 || AZ==2, "No y-axis.");
    static_assert(AX==4 || AY==4 || AZ==4, "No x-axis.");
    static const int ONE = SCENE_SIZE;
    
    static void render(Q& f, int x, int y, int Q) {
        if (f.map[0]) SubFaceRenderer<-1,-1,C^AX^AY,AX,AY,AZ>::traverse(f, 0, M, 0, x-Q, x,-ONE, 0, y-Q, y,-ONE, 0);
        if (f.map[1]) SubFaceRenderer< 1,-1,C   ^AY,AX,AY,AZ>::traverse(f, 1, M, 0, x, x+Q, 0, ONE, y-Q, y,-ONE, 0);
        if (f.map[2]) SubFaceRenderer<-1, 1,C^AX   ,AX,AY,AZ>::traverse(f, 2, M, 0, x-Q, x,-ONE, 0, y, y+Q, 0, ONE);
        if (f.map[3]) SubFaceRenderer< 1, 1,C      ,AX,AY,AZ>::traverse(f, 3, M, 0, x, x+Q, 0, ONE, y, y+Q, 0, ONE);
    }
};

static void prepare_cubemap() {
    const int SIZE = Q::SIZE;
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
    for (int i=0; i<6; i++) {
        cubemap[i].build(0); 
        cubemap[i].build(1); 
        cubemap[i].build(2); 
        cubemap[i].build(3); 
    }
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
    int z = position.z;
    int W = SCENE_SIZE;

    Timer t1;
    prepare_cubemap();
    double d1 = t1.elapsed();
    
    /* x=4, y=2, z=1
     * 
     * 0 = neg-x, neg-y, neg-z
     * 1 = neg-x, neg-y, pos-z
     * ...
     */
    
    Timer t2;
    /* Z+ face
     * 
     *-W----W
     * 
     * +-z--+= y-(W-z)
     * |   /| 
     * y  / |
     * | .  |
     * |  \ |
     * +---\+
     *      \= y+(W-z)
     */
    FaceRenderer<0,4,2,1>::render(cubemap[1], x, y, W-z);

    /* Z- face
     * 
     *-W----W
     * 
     * +-z--+
     * \    |= y-(W+z)
     * y\   |
     * | .  |
     * |/   |
     * +----+= y+(W+z)
     *      
     */
    FaceRenderer<5,4,2,1>::render(cubemap[3],-x, y, W+z);
    
    // X+ face
    FaceRenderer<3,1,2,4>::render(cubemap[2],-z,-y, W-x);
    // X- face
    FaceRenderer<6,1,2,4>::render(cubemap[4], z,-y, W+x);

    // Y+ face
    FaceRenderer<0,4,1,2>::render(cubemap[0], x, z, W-y);    
    // Y- face
    FaceRenderer<3,4,1,2>::render(cubemap[5], x,-z, W+y);
    double d2 = t2.elapsed();

    Timer t3;
    draw_cubemap();
    double d3 = t3.elapsed();
    
    printf("%6.2f | %6.2f %6.2f %6.2f\n", t1.elapsed(), d1,d2,d3);
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
