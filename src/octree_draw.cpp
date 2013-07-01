#include <cstdio>
#include <algorithm>

#include "art.h"
#include "events.h"
#include "quadtree.h"
#include "timing.h"
#include "octree.h"

namespace frustum {
    // Compute frustum parameters.
    static const int left   = -SCREEN_WIDTH;
    static const int right  =  SCREEN_WIDTH;
    static const int top    =  SCREEN_HEIGHT;
    static const int bottom = -SCREEN_HEIGHT;
    //static const int near   =  SCREEN_HEIGHT; // I.e. 90 degree FOV.
    static const int near   =  SCREEN_HEIGHT*2;
    static const int cubepos=  SCREEN_WIDTH * 2; // > sqrt(3)*SCREEN_WIDTH > hypot(SCREEN_WIDTH,SCREEN_HEIGHT,SCREEN_HEIGHT) > max dist of view plane.
    static const int far    =  SCREEN_WIDTH * 4; // > sqrt(3)*cubepos 
}

using std::max;
using std::min;

typedef quadtree<10> Q;
static Q cubemap[6];

template<int DX, int DY, int C, int AX, int AY, int AZ>
struct SubFaceRenderer {
    static_assert(DX==1 || DX==-1, "Wrong DX");
    static_assert(DY==1 || DY==-1, "Wrong DY");
    static const int ONE = SCENE_SIZE;
    /** Returns true if quadtree node is rendered 
     * Function is assumed to be called only if quadtree node is not yet fully rendered.
     */
    static bool traverse(
        octree* root, Q& f, unsigned int r, uint32_t index, uint32_t color,
        int x1, int x2, int x1p, int x2p, 
        int y1, int y2, int y1p, int y2p
    ){
        // occlusion
        if (x2-(1-DX)*x2p<=-ONE || ONE<=x1-(1+DX)*x1p) return false;
        if (y2-(1-DY)*y2p<=-ONE || ONE<=y1-(1+DY)*y1p) return false;
                
        // Recursion
        if (x2-x1 <= 2*ONE && y2-y1 <= 2*ONE) {
            // Traverse octree
            // x4 y2 z1
            int x3 = x1-x1p;
            int x4 = x2-x2p;
            int y3 = y1-y1p;
            int y4 = y2-y2p;
            if (~index) {
                octree &s = root[index];
                if (x3<x4 && y3<y4) {
                    if (s.avgcolor[C         ]>=0 && traverse(root, f, r, s.child[C         ], s.avgcolor[C         ], 2*x3+DX*ONE,2*x4+DX*ONE,x1p,x2p, 2*y3+DY*ONE,2*y4+DY*ONE,y1p,y2p)) return true;
                    if (s.avgcolor[C^AX      ]>=0 && traverse(root, f, r, s.child[C^AX      ], s.avgcolor[C^AX      ], 2*x3-DX*ONE,2*x4-DX*ONE,x1p,x2p, 2*y3+DY*ONE,2*y4+DY*ONE,y1p,y2p)) return true;
                    if (s.avgcolor[C   ^AY   ]>=0 && traverse(root, f, r, s.child[C   ^AY   ], s.avgcolor[C   ^AY   ], 2*x3+DX*ONE,2*x4+DX*ONE,x1p,x2p, 2*y3-DY*ONE,2*y4-DY*ONE,y1p,y2p)) return true;
                    if (s.avgcolor[C^AX^AY   ]>=0 && traverse(root, f, r, s.child[C^AX^AY   ], s.avgcolor[C^AX^AY   ], 2*x3-DX*ONE,2*x4-DX*ONE,x1p,x2p, 2*y3-DY*ONE,2*y4-DY*ONE,y1p,y2p)) return true;
                }
                if (s.avgcolor[C      ^AZ]>=0 && traverse(root, f, r, s.child[C      ^AZ], s.avgcolor[C      ^AZ], 2*x1+DX*ONE,2*x2+DX*ONE,x1p,x2p, 2*y1+DY*ONE,2*y2+DY*ONE,y1p,y2p)) return true;
                if (s.avgcolor[C^AX   ^AZ]>=0 && traverse(root, f, r, s.child[C^AX   ^AZ], s.avgcolor[C^AX   ^AZ], 2*x1-DX*ONE,2*x2-DX*ONE,x1p,x2p, 2*y1+DY*ONE,2*y2+DY*ONE,y1p,y2p)) return true;
                if (s.avgcolor[C   ^AY^AZ]>=0 && traverse(root, f, r, s.child[C   ^AY^AZ], s.avgcolor[C   ^AY^AZ], 2*x1+DX*ONE,2*x2+DX*ONE,x1p,x2p, 2*y1-DY*ONE,2*y2-DY*ONE,y1p,y2p)) return true;
                if (s.avgcolor[C^AX^AY^AZ]>=0 && traverse(root, f, r, s.child[C^AX^AY^AZ], s.avgcolor[C^AX^AY^AZ], 2*x1-DX*ONE,2*x2-DX*ONE,x1p,x2p, 2*y1-DY*ONE,2*y2-DY*ONE,y1p,y2p)) return true;
            } else {
                if (x3<x4 && y3<y4) {
                    // Skip nearest cube to avoid infinite recursion.
                    if (traverse(root, f, r, ~0u, color, 2*x3-DX*ONE,2*x4-DX*ONE,x1p,x2p, 2*y3+DY*ONE,2*y4+DY*ONE,y1p,y2p)) return true;
                    if (traverse(root, f, r, ~0u, color, 2*x3+DX*ONE,2*x4+DX*ONE,x1p,x2p, 2*y3-DY*ONE,2*y4-DY*ONE,y1p,y2p)) return true;
                    if (traverse(root, f, r, ~0u, color, 2*x3-DX*ONE,2*x4-DX*ONE,x1p,x2p, 2*y3-DY*ONE,2*y4-DY*ONE,y1p,y2p)) return true;
                }
                if (traverse(root, f, r, ~0u, color, 2*x1+DX*ONE,2*x2+DX*ONE,x1p,x2p, 2*y1+DY*ONE,2*y2+DY*ONE,y1p,y2p)) return true;
                if (traverse(root, f, r, ~0u, color, 2*x1-DX*ONE,2*x2-DX*ONE,x1p,x2p, 2*y1+DY*ONE,2*y2+DY*ONE,y1p,y2p)) return true;
                if (traverse(root, f, r, ~0u, color, 2*x1+DX*ONE,2*x2+DX*ONE,x1p,x2p, 2*y1-DY*ONE,2*y2-DY*ONE,y1p,y2p)) return true;
                if (traverse(root, f, r, ~0u, color, 2*x1-DX*ONE,2*x2-DX*ONE,x1p,x2p, 2*y1-DY*ONE,2*y2-DY*ONE,y1p,y2p)) return true;
            }
            return false;
        } else {
            int xm  = (x1 +x2 )/2; 
            int xmp = (x1p+x2p)/2; 
            int ym  = (y1 +y2 )/2; 
            int ymp = (y1p+y2p)/2; 
            if (r<Q::L) {
                // Traverse quadtree 
                if (f.map[r*4+4]) traverse(root, f, r*4+4, index, color, x1, xm, x1p, xmp, y1, ym, y1p, ymp); 
                if (f.map[r*4+5]) traverse(root, f, r*4+5, index, color, xm, x2, xmp, x2p, y1, ym, y1p, ymp); 
                if (f.map[r*4+6]) traverse(root, f, r*4+6, index, color, x1, xm, x1p, xmp, ym, y2, ymp, y2p); 
                if (f.map[r*4+7]) traverse(root, f, r*4+7, index, color, xm, x2, xmp, x2p, ym, y2, ymp, y2p); 
            } else {
                // Rendering
                if (f.map[r*4+4]) paint(f, r*4+4, color, x1, xm, x1p, xmp, y1, ym, y1p, ymp); 
                if (f.map[r*4+5]) paint(f, r*4+5, color, xm, x2, xmp, x2p, y1, ym, y1p, ymp); 
                if (f.map[r*4+6]) paint(f, r*4+6, color, x1, xm, x1p, xmp, ym, y2, ymp, y2p); 
                if (f.map[r*4+7]) paint(f, r*4+7, color, xm, x2, xmp, x2p, ym, y2, ymp, y2p); 
            }
            f.compute(r);
            return !f.map[r];
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
    
    static void render(Q& f, octree * root, int x, int y, int Q) {
        if (f.map[0]) SubFaceRenderer<-1,-1,C^AX^AY,AX,AY,AZ>::traverse(root, f, 0, 0, 0, x-Q, x,-ONE, 0, y-Q, y,-ONE, 0);
        if (f.map[1]) SubFaceRenderer< 1,-1,C   ^AY,AX,AY,AZ>::traverse(root, f, 1, 0, 0, x, x+Q, 0, ONE, y-Q, y,-ONE, 0);
        if (f.map[2]) SubFaceRenderer<-1, 1,C^AX   ,AX,AY,AZ>::traverse(root, f, 2, 0, 0, x-Q, x,-ONE, 0, y, y+Q, 0, ONE);
        if (f.map[3]) SubFaceRenderer< 1, 1,C      ,AX,AY,AZ>::traverse(root, f, 3, 0, 0, x, x+Q, 0, ONE, y, y+Q, 0, ONE);
    }
};

static void prepare_cubemap() {
    // The orientation matrix is (asumed to be) orthogonal, and therefore can be inversed by transposition.
    glm::dmat3 inverse_orientation = glm::transpose(orientation);
    // Compute normals of the 4 planes of the view piramid.
    glm::dvec3 normals[4] = {
        inverse_orientation*glm::normalize(glm::dvec3( frustum::near, 0, -frustum::left  )),
        inverse_orientation*glm::normalize(glm::dvec3(-frustum::near, 0,  frustum::right )),
        inverse_orientation*glm::normalize(glm::dvec3(0,  frustum::near, -frustum::bottom)),
        inverse_orientation*glm::normalize(glm::dvec3(0, -frustum::near,  frustum::top   )),
    };
    
    // build the non-leaf layers of the quadtree
    for (int i=0; i<6; i++) {
        glm::dvec3 face_normals[4];
        for (int j=0; j<4; j++) {
            glm::dvec3 v = normals[j];
            switch (i) {
                case 0: face_normals[j] = glm::dvec3(v.x,-v.z,v.y); break;
                case 1: face_normals[j] = v; break;
                case 2: face_normals[j] = glm::dvec3(-v.z,v.y,v.x); break;
                case 3: face_normals[j] = glm::dvec3(-v.x,v.y,-v.z); break;
                case 4: face_normals[j] = glm::dvec3(v.z,v.y,-v.x); break;
                case 5: face_normals[j] = glm::dvec3(v.x,v.z,-v.y); break;
            }
        }
        cubemap[i].build(face_normals);
        
        memset(cubemap[i].face,0xcc,sizeof(cubemap[i].face));
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
void octree_draw(octree * root) {
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
    FaceRenderer<0,4,2,1>::render(cubemap[1], root, x, y, W-z);

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
    FaceRenderer<5,4,2,1>::render(cubemap[3], root,-x, y, W+z);
    
    // X+ face
    FaceRenderer<3,1,2,4>::render(cubemap[2], root,-z,-y, W-x);
    // X- face
    FaceRenderer<6,1,2,4>::render(cubemap[4], root, z,-y, W+x);

    // Y+ face
    FaceRenderer<0,4,1,2>::render(cubemap[0], root, x, z, W-y);    
    // Y- face
    FaceRenderer<3,4,1,2>::render(cubemap[5], root, x,-z, W+y);
    double d2 = t2.elapsed();

    Timer t3;
    draw_cubemap();
    double d3 = t3.elapsed();
    
    printf("%6.2f | %6.2f %6.2f %6.2f\n", t1.elapsed(), d1,d2,d3);
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
