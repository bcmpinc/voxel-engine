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

#include <cstdio>
#include <algorithm>
#include <GL/gl.h>

#include "art.h"
#include "events.h"
#include "quadtree.h"
#include "timing.h"
#include "octree.h"

#define static_assert(test, message) typedef char static_assert__##message[(test)?1:-1]

using std::max;
using std::min;

namespace {
    quadtree face;
    octree * root;
}

template<int D>
struct occlusion {
    static_assert(D==1 || D==-1, Wrong_D);
    static const int ONE = SCENE_SIZE;
    bool t1,t2,t3,t4;
    occlusion(int v, int vp, int d, int dp);
};

template<>
occlusion<1>::occlusion(int v, int vp, int d, int dp) : t1(true),t2(true),t3(true),t4(true) {
    if (0 <= v-vp) {
        t3=false;
        if (0 <= v-2*vp) {
            t1=false;
            if (ONE <= v-vp) {
                t4=false; 
            }
        } 
    } else if (v-vp+d-dp <= 0) {
        t2=false;
        if (v+d <= 0) {
            t4=false;
            if (v-vp+d-dp <= -ONE) {
                t1=false;
            }
        }
    }        
}

template<>
occlusion<-1>::occlusion(int v, int vp, int d, int dp) : t1(true),t2(true),t3(true),t4(true) {
    if (v-vp+d-dp <= 0) {
        t3=false;
        if (v+d-2*(vp+dp) <= 0) {
            t1=false;
            if (v-vp+d-dp <= -ONE) {
                t4=false; 
            }
        } 
    } else if (0 <= v-vp) {
        t2=false;
        if (0 <= v) {
            t4=false;
            if (ONE <= v-vp) {
                t1=false;
            }
        }
    }        
}


template<int DX, int DY, int C, int AX, int AY, int AZ>
struct SubFaceRenderer {
    static_assert(DX==1 || DX==-1, Wrong_DX);
    static_assert(DY==1 || DY==-1, Wrong_DY);
    static const int ONE = SCENE_SIZE;
    /** Returns true if quadtree node is rendered 
     * Function is assumed to be called only if quadtree node is not yet fully rendered.
     */
    static bool traverse(
        unsigned int r, uint32_t index, uint32_t color,
        int x,  int y,  int d,
        int xp, int yp, int dp
    ){
        // occlusion
        if (x+d-(1-DX)*(xp+dp)<=-ONE || ONE<=x-(1+DX)*xp) abort();
        if (y+d-(1-DY)*(yp+dp)<=-ONE || ONE<=y-(1+DY)*yp) abort();
        
        // Recursion
        if (d <= 2*ONE) {
            // Traverse octree
            
            // occlusion
            occlusion<DX> occ_x(x,xp,d,dp);
            occlusion<DY> occ_y(y,yp,d,dp);

            int xn = (x-xp)*2; // x3, x4=xn+dn
            int yn = (y-yp)*2; 
            int dn = (d-dp)*2;
            x*=2;
            y*=2;
            d*=2;
                        
            // Recursive calls
            if (~index) {
                octree &s = root[index];
                if (dn>0) {
                    if (occ_x.t1 && occ_y.t1 && s.avgcolor[C         ]>=0 && traverse(r, s.child[C         ], s.avgcolor[C         ], xn+DX*ONE,yn+DY*ONE,dn,xp,yp,dp)) return true;
                    if (occ_x.t2 && occ_y.t1 && s.avgcolor[C^AX      ]>=0 && traverse(r, s.child[C^AX      ], s.avgcolor[C^AX      ], xn-DX*ONE,yn+DY*ONE,dn,xp,yp,dp)) return true;
                    if (occ_x.t1 && occ_y.t2 && s.avgcolor[C   ^AY   ]>=0 && traverse(r, s.child[C   ^AY   ], s.avgcolor[C   ^AY   ], xn+DX*ONE,yn-DY*ONE,dn,xp,yp,dp)) return true;
                    if (occ_x.t2 && occ_y.t2 && s.avgcolor[C^AX^AY   ]>=0 && traverse(r, s.child[C^AX^AY   ], s.avgcolor[C^AX^AY   ], xn-DX*ONE,yn-DY*ONE,dn,xp,yp,dp)) return true;
                }
                if (occ_x.t3 && occ_y.t3 && s.avgcolor[C      ^AZ]>=0 && traverse(r, s.child[C      ^AZ], s.avgcolor[C      ^AZ], x+DX*ONE,y+DY*ONE,d,xp,yp,dp)) return true;
                if (occ_x.t4 && occ_y.t3 && s.avgcolor[C^AX   ^AZ]>=0 && traverse(r, s.child[C^AX   ^AZ], s.avgcolor[C^AX   ^AZ], x-DX*ONE,y+DY*ONE,d,xp,yp,dp)) return true;
                if (occ_x.t3 && occ_y.t4 && s.avgcolor[C   ^AY^AZ]>=0 && traverse(r, s.child[C   ^AY^AZ], s.avgcolor[C   ^AY^AZ], x+DX*ONE,y-DY*ONE,d,xp,yp,dp)) return true;
                if (occ_x.t4 && occ_y.t4 && s.avgcolor[C^AX^AY^AZ]>=0 && traverse(r, s.child[C^AX^AY^AZ], s.avgcolor[C^AX^AY^AZ], x-DX*ONE,y-DY*ONE,d,xp,yp,dp)) return true;
            } else {
                if (dn>0) {
                    // Skip nearest cube to avoid infinite recursion.
                    if (occ_x.t2 && occ_y.t1 && traverse(r, ~0u, color, xn-DX*ONE,yn+DY*ONE,dn,xp,yp,dp)) return true;
                    if (occ_x.t1 && occ_y.t2 && traverse(r, ~0u, color, xn+DX*ONE,yn-DY*ONE,dn,xp,yp,dp)) return true;
                    if (occ_x.t2 && occ_y.t2 && traverse(r, ~0u, color, xn-DX*ONE,yn-DY*ONE,dn,xp,yp,dp)) return true;
                }
                if (occ_x.t3 && occ_y.t3 && traverse(r, ~0u, color, x+DX*ONE,y+DY*ONE,d,xp,yp,dp)) return true;
                if (occ_x.t4 && occ_y.t3 && traverse(r, ~0u, color, x-DX*ONE,y+DY*ONE,d,xp,yp,dp)) return true;
                if (occ_x.t3 && occ_y.t4 && traverse(r, ~0u, color, x+DX*ONE,y-DY*ONE,d,xp,yp,dp)) return true;
                if (occ_x.t4 && occ_y.t4 && traverse(r, ~0u, color, x-DX*ONE,y-DY*ONE,d,xp,yp,dp)) return true;
            }
            return false;
        } else {
            assert((d&1)==0);
            assert((dp&1)==0);
            d/=2;
            dp/=2;
            int xm  = x  + d; 
            int xmp = xp + dp; 
            int ym  = y  + d; 
            int ymp = yp + dp; 
            if (r<quadtree::L) {
                // Traverse quadtree 
                if (face.map[r*4+4] && xm-(1-DX)*xmp>-ONE && ym-(1-DY)*ymp>-ONE) traverse(r*4+4, index, color, x,  y,  d, xp,  yp,  dp); 
                if (face.map[r*4+5] && ONE>xm-(1+DX)*xmp  && ym-(1-DY)*ymp>-ONE) traverse(r*4+5, index, color, xm, y,  d, xmp, yp,  dp); 
                if (face.map[r*4+6] && xm-(1-DX)*xmp>-ONE && ONE>ym-(1+DY)*ymp ) traverse(r*4+6, index, color, x,  ym, d, xp,  ymp, dp); 
                if (face.map[r*4+7] && ONE>xm-(1+DX)*xmp  && ONE>ym-(1+DY)*ymp ) traverse(r*4+7, index, color, xm, ym, d, xmp, ymp, dp); 
            } else {
                // Rendering
                if (face.map[r*4+4] && xm-(1-DX)*xmp>-ONE && ym-(1-DY)*ymp>-ONE) face.set_face(r*4+4, color); 
                if (face.map[r*4+5] && ONE>xm-(1+DX)*xmp  && ym-(1-DY)*ymp>-ONE) face.set_face(r*4+5, color); 
                if (face.map[r*4+6] && xm-(1-DX)*xmp>-ONE && ONE>ym-(1+DY)*ymp ) face.set_face(r*4+6, color); 
                if (face.map[r*4+7] && ONE>xm-(1+DX)*xmp  && ONE>ym-(1+DY)*ymp ) face.set_face(r*4+7, color); 
            }
            face.compute(r);
            return !face.map[r];
        }
    }
};

template<int C, int AX, int AY, int AZ>
struct FaceRenderer {
    static_assert(0<=C && C<8, Invalid_C);
    static_assert(AX==1 || AY==1 || AZ==1, No_Z_axis);
    static_assert(AX==2 || AY==2 || AZ==2, No_Y_axis);
    static_assert(AX==4 || AY==4 || AZ==4, No_X_axis);
    static const int ONE = SCENE_SIZE;
    
    /**
     * Renders the scene to a single face of the cubemap.
     * 
     * The (x,y) coordinate is the position of the eye projected on the cubemap face. 
     * The value Q is the distance between the eye and the side of the octree corresponding to the face being rendered to.
     */
    static void render(int x, int y, int Q) {
        //                                                                          x    y    d  xp    yp    dp
        if (face.map[0]) SubFaceRenderer<-1,-1,C^AX^AY,AX,AY,AZ>::traverse(0, 0, 0, x-Q, y-Q, Q, -ONE, -ONE, ONE);
        if (face.map[1]) SubFaceRenderer< 1,-1,C   ^AY,AX,AY,AZ>::traverse(1, 0, 0, x,   y-Q, Q, 0,    -ONE, ONE);
        if (face.map[2]) SubFaceRenderer<-1, 1,C^AX   ,AX,AY,AZ>::traverse(2, 0, 0, x-Q, y,   Q, -ONE, 0,    ONE);
        if (face.map[3]) SubFaceRenderer< 1, 1,C      ,AX,AY,AZ>::traverse(3, 0, 0, x,   y,   Q, 0,    0,    ONE);
    }
};

struct FaceRendererProxy {
    int x, y, Q;
    void (*render)(int x, int y, int Q);
};

    
static GLuint cubetargets[6] = {
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
};

uint32_t prepare_cubemap() {
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    for (int i=0; i<6; i++) {
        glTexImage2D( cubetargets[i], 0, 4, quadtree::SIZE, quadtree::SIZE, 0, GL_BGRA, GL_UNSIGNED_BYTE, face.face);
    }
    return id;
};

/** Render the octree to the OpenGL cubemap texture. 
 */
void octree_draw(octree_file * file, uint32_t cubemap_texture) {
    Timer t_global;
    
    int x = position.x;
    int y = position.y;
    int z = position.z;
    int W = SCENE_SIZE;

    double timer_prepare=0;
    double timer_query=0;
    double timer_transfer=0;
    
    root = file->root;
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);
    
    // Create list that tells what template function must be called with which parameters for which face.
    FaceRendererProxy proxy[] = {
        { x,-z, W-y, FaceRenderer<1,4,1,2>::render}, // Y+
        { x, y, W-z, FaceRenderer<0,4,2,1>::render}, // Z+
        {-z, y, W-x, FaceRenderer<1,1,2,4>::render}, // X+
        {-x, y, W+z, FaceRenderer<5,4,2,1>::render}, // Z-
        { z, y, W+x, FaceRenderer<4,1,2,4>::render}, // X-
        { x, z, W+y, FaceRenderer<2,4,1,2>::render}, // Y-
    };

    // The orientation matrix is (asumed to be) orthogonal, and therefore can be inversed by transposition.
    glm::dmat3 inverse_orientation = glm::transpose(orientation);
    
    // Compute normals of the 4 planes of the view piramid.
    glm::dvec3 normals[4] = {
        inverse_orientation*glm::dvec3( frustum::near, 0, -frustum::left  ),
        inverse_orientation*glm::dvec3(-frustum::near, 0,  frustum::right ),
        inverse_orientation*glm::dvec3(0,  frustum::near, -frustum::bottom),
        inverse_orientation*glm::dvec3(0, -frustum::near,  frustum::top   ),
    };

    // Render the scene to each of the 6 cubemap faces.
    char rendered[]="......";
    for (int i=0; i<6; i++) {
        Timer t_prepare;
        
        // Compute the view piramid for the current face.
        glm::dvec3 face_normals[4];
        for (int j=0; j<4; j++) {
            glm::dvec3 v = normals[j];
            switch (i) {
                case 0: face_normals[j] = glm::dvec3( v.x,-v.z, v.y); break;
                case 1: face_normals[j] = v; break;
                case 2: face_normals[j] = glm::dvec3(-v.z, v.y, v.x); break;
                case 3: face_normals[j] = glm::dvec3(-v.x, v.y,-v.z); break;
                case 4: face_normals[j] = glm::dvec3( v.z, v.y,-v.x); break;
                case 5: face_normals[j] = glm::dvec3( v.x, v.z,-v.y); break;
            }
        }
        
        // Prepare the occlusion quadtree
        face.build(face_normals);
        
        timer_prepare += t_prepare.elapsed();
        
        // Check if the face is used.
        if (face.children[0]==0) continue;
        
        Timer t_query;
        
        // Clear the previous data from the face.
        face.clear_face();
        
        // Do the actual rendering of the scene to the face (i.e. execute the query).
        proxy[i].render(proxy[i].x, proxy[i].y, proxy[i].Q);
        
        timer_query += t_query.elapsed();

        Timer t_transfer;
        
        // Send the image data to OpenGL.
        glTexImage2D( cubetargets[i], 0, 4, quadtree::SIZE, quadtree::SIZE, 0, GL_BGRA, GL_UNSIGNED_BYTE, face.face);
        
        timer_transfer += t_transfer.elapsed();
        
        rendered[i] = '0'+i;
    }
        
    printf("%6.2f | Prepare:%4.2f Query:%7.2f Transfer:%5.2f %s\n", t_global.elapsed(), timer_prepare, timer_query, timer_transfer, rendered);
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
