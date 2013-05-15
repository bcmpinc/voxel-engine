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
    int cnt=1000000;

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

/** Draws a line. */
void line(double x1, double y1, double x2, double y2, int c) {
    if (x1<0) { 
        if (x2<0) return;
        y1 = (y2*(0-x1) + y1*(x2-0))/(x2-x1);
        x1 = 0;
    }
    if (x2<0) { 
        y2 = (y1*(0-x2) + y2*(x1-0))/(x1-x2);
        x2 = 0;
    }
    if (x1>SCREEN_WIDTH) { 
        if (x2>SCREEN_WIDTH) return;
        y1 = (y2*(SCREEN_WIDTH-x1) + y1*(x2-SCREEN_WIDTH))/(x2-x1);
        x1 = SCREEN_WIDTH;
    }
    if (x2>SCREEN_WIDTH) { 
        y2 = (y1*(SCREEN_WIDTH-x2) + y2*(x1-SCREEN_WIDTH))/(x1-x2);
        x2 = SCREEN_WIDTH;
    }
    
    if (y1<0) { 
        if (y2<0) return;
        x1 = (x2*(0-y1) + x1*(y2-0))/(y2-y1);
        y1 = 0;
    }
    if (y2<0) { 
        x2 = (x1*(0-y2) + x2*(y1-0))/(y1-y2);
        y2 = 0;
    }
    if (y1>SCREEN_HEIGHT) { 
        if (y2>SCREEN_HEIGHT) return;
        x1 = (x2*(SCREEN_HEIGHT-y1) + x1*(y2-SCREEN_HEIGHT))/(y2-y1);
        y1 = SCREEN_HEIGHT;
    }
    if (y2>SCREEN_HEIGHT) { 
        x2 = (x1*(SCREEN_HEIGHT-y2) + x2*(y1-SCREEN_HEIGHT))/(y1-y2);
        y2 = SCREEN_HEIGHT;
    }
    
    int d = (int)(1+max(abs(x1-x2),abs(y1-y2)));
    for (int i=0; i<=d; i++) {
        double x=(x1+(x2-x1)*i/d);
        double y=(y1+(y2-y1)*i/d);
        pix(x,y,1LL<<59,c);
    }
}

/** draws a 3d line. */
void line(glm::dvec3 va, glm::dvec3 vb, int c) {
    if (va.z<=1e-2 && vb.z<=1e-2) return;
    // Cut of part behind viewer.
    if (va.z<=1e-2) {
        double w = (1e-2-va.z)/(vb.z-va.z);
        va = vb*w + va*(1-w);
    }
    if (vb.z<=1e-2) {
        double w = (1e-2-vb.z)/(va.z-vb.z);
        vb = va*w + vb*(1-w);
    }
    
    int64_t pxa = SCREEN_WIDTH/2  + va.x*SCREEN_HEIGHT/va.z;
    int64_t pya = SCREEN_HEIGHT/2 - va.y*SCREEN_HEIGHT/va.z;
    int64_t pxb = SCREEN_WIDTH/2  + vb.x*SCREEN_HEIGHT/vb.z;
    int64_t pyb = SCREEN_HEIGHT/2 - vb.y*SCREEN_HEIGHT/vb.z;
    
    line(pxa,pya, pxb,pyb, c);
}

void draw_box() {
    glm::dvec3 vertices[8]={
        glm::dvec3(-1,-1,-1),
        glm::dvec3( 1,-1,-1),
        glm::dvec3(-1, 1,-1),
        glm::dvec3( 1, 1,-1),
        glm::dvec3(-1,-1, 1),
        glm::dvec3( 1,-1, 1),
        glm::dvec3(-1, 1, 1),
        glm::dvec3( 1, 1, 1),
    };
    for (int a=0; a<8; a++) {
        vertices[a] = orientation * vertices[a];
    }
    for (int a=0; a<8; a++) {
        for (int b=0; b<8; b++) {
            int c = a^b;
            glm::dvec3 va = vertices[a];
            glm::dvec3 vb = vertices[b];
            switch(c) {
                case 1:
                case 2:
                case 4:
                    line(va,vb, 0x000000);
                    break;
                case 3:
                    line(va,vb, 0x0000fe>>!(a&b));
                    break;
                case 5:
                    line(va,vb, 0x00fe00>>!(a&b));
                    break;
                case 6:
                    line(va,vb, 0xfe0000>>!(a&b));
                    break;
            }
        }
    }
}


struct Frustum {
    int x, y, s;
    glm::dvec3 p[4];
    void traverse() {
        if (x+s<=0) return;
        if (y+s<=0) return;
        if (x>=SCREEN_WIDTH) return;
        if (y>=SCREEN_HEIGHT) return;
        if (s<=1) {
            double ax=fabs(p[0].x);
            double ay=fabs(p[0].y);
            double az=fabs(p[0].z);
            
            if ((x&1)==1 && (y&1)==0 && p[0].x<0) return;
            if ((x&1)==0 && (y&1)==1 && p[0].y<0) return;
            if ((x&1)==1 && (y&1)==1 && p[0].z<0) return;
            
            if (ax>=ay && ax>=az) {
                pix(x,y,3LL<<58,p[0].x>0?0x7f0000:0x3f0000);
            } else if (ay>=ax && ay>=az) {
                pix(x,y,3LL<<58,p[0].y>0?0x007f00:0x003f00);
            } else if (az>=ax && az>=ay) {
                pix(x,y,3LL<<58,p[0].z>0?0x00007f:0x00003f);
            } else {
                pix(x,y,3LL<<58,0x808080);
            }

            return;
        }
        s>>=1;
        for (int i=0; i<4; i++) {
            Frustum f = {
                x+s*(i&1),y+s*(i/2),s,{
                    (p[0]+p[i])/2.,
                    (p[1]+p[i])/2.,
                    (p[2]+p[i])/2.,
                    (p[3]+p[i])/2.,
                }
            };
            f.traverse();
        }
    }
};


/** Draw anything on the screen. */
void draw() {
    for (int i = 0; i<(SCREEN_HEIGHT)*(SCREEN_WIDTH); i++) {
        zbuf[i] = 1LL<<60;
        pixs[i] = 0x8080b0;
    }
    
    Frustum f;
    f.x=SCREEN_WIDTH/2-512;
    f.y=SCREEN_HEIGHT/2-512;
    f.s=1024;
    glm::dmat3 io = glm::transpose(orientation);
    double d = 512.0/SCREEN_HEIGHT;
    f.p[0]=io*glm::dvec3(-d, d,1);
    f.p[1]=io*glm::dvec3( d, d,1);
    f.p[2]=io*glm::dvec3(-d,-d,1);
    f.p[3]=io*glm::dvec3( d,-d,1);
    f.traverse();
    
    line(orientation*(glm::dvec3(3<<17,4<<17,4<<17)-position),orientation*(glm::dvec3(5<<17,4<<17,4<<17)-position),0xff00ff);
    line(orientation*(glm::dvec3(4<<17,3<<17,4<<17)-position),orientation*(glm::dvec3(4<<17,5<<17,4<<17)-position),0xff00ff);
    line(orientation*(glm::dvec3(4<<17,4<<17,3<<17)-position),orientation*(glm::dvec3(4<<17,4<<17,5<<17)-position),0xff00ff);
    
    draw_box();
    
    M.draw(SCENE_SIZE-position.x,SCENE_SIZE-position.y,SCENE_SIZE-position.z,OCTREE_DEPTH);
    //holefill();
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
