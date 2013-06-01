#include <SDL.h>
#include <SDL_image.h>

#include "events.h"
#include "art.h"

using glm::min;
using glm::max;

namespace {
  // The screen surface
  SDL_Surface *screen = NULL;
  
  // pointer to the pixels (32 bit)
  int * pixs;
  int64_t * zbuf;
}

void init_screen(const char * caption) {
    // Initialize SDL 
    if (SDL_Init (SDL_INIT_VIDEO) < 0) {
        fprintf (stderr, "Couldn't initialize SDL: %s\n", SDL_GetError ());
        exit (2);
    }
    atexit (SDL_Quit);
#if defined _WIN32 || defined _WIN64
    freopen( "CON", "w", stdout );
    freopen( "CON", "w", stderr );
#endif

    // Set 32-bits video mode (eventually emulated)
    screen = SDL_SetVideoMode (SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);
    if (screen == NULL) {
        fprintf (stderr, "Couldn't set video mode: %s\n", SDL_GetError ());
        exit (3);
    }
    SDL_WM_SetCaption (caption, NULL);

    // set the pixel pointer
    pixs=(int*)screen->pixels;  
    zbuf = new int64_t[(SCREEN_HEIGHT)*(SCREEN_WIDTH)];
}

void clear_screen(int c){
    for (int i = 0; i<(SCREEN_HEIGHT)*(SCREEN_WIDTH); i++) {
        zbuf[i] = 1LL<<60;
        pixs[i] = c;
    }  
}

void flip_screen() {
    SDL_Flip (screen);
}

void pix(int64_t x, int64_t y, int64_t z, int c) {
    if (x>=0 && y>=0 && x<SCREEN_WIDTH && y<SCREEN_HEIGHT) {
        int64_t i = x+y*(SCREEN_WIDTH);
        if (zbuf[i]>z) {
            pixs[i] = c;
            zbuf[i] = z;
            //pos.points_rendered++;
        }
    }
}
#define CLAMP(x,l,u) (x<l?l:x>u?u:x)
int rgb(int r, int g, int b) {
    return CLAMP(r,0,255)<<16|CLAMP(g,0,255)<<8|CLAMP(b,0,255);
}
int rgb(float r, float g, float b) {
    return rgb((int)(r+0.5),(int)(g+0.5),(int)(b+0.5));
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

void draw_axis() {
    line(orientation*(glm::dvec3(3<<17,4<<17,4<<17)-position),orientation*(glm::dvec3(5<<17,4<<17,4<<17)-position),0xff00ff);
    line(orientation*(glm::dvec3(4<<17,3<<17,4<<17)-position),orientation*(glm::dvec3(4<<17,5<<17,4<<17)-position),0xff00ff);
    line(orientation*(glm::dvec3(4<<17,4<<17,3<<17)-position),orientation*(glm::dvec3(4<<17,4<<17,5<<17)-position),0xff00ff);
}

void draw_cube() {
    glm::dmat3 io = glm::transpose(orientation);
    double d = 1.0/SCREEN_HEIGHT;
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            glm::dvec3 p( (x-SCREEN_WIDTH/2)*d, (SCREEN_HEIGHT/2-y)*d, 1 );
            p = io * p;
            double ax=fabs(p.x);
            double ay=fabs(p.y);
            double az=fabs(p.z);
        
            if ((x&1)==0 && (y&1)==0) continue;
            if ((x&1)==1 && (y&1)==0 && p.x<0) continue;
            if ((x&1)==0 && (y&1)==1 && p.y<0) continue;
            if ((x&1)==1 && (y&1)==1 && p.z<0) continue;
        
            if (ax>=ay && ax>=az) {
                pix(x,y,3LL<<58,p.x>0?0x7f0000:0x3f0000);
            } else if (ay>=ax && ay>=az) {
                pix(x,y,3LL<<58,p.y>0?0x007f00:0x003f00);
            } else if (az>=ax && az>=ay) {
                pix(x,y,3LL<<58,p.z>0?0x00007f:0x00003f);
            } else {
                pix(x,y,3LL<<58,0x808080);
            }
        }
    }
}

void draw_cubemap(struct SDL_Surface ** cubemap) {
    glm::dmat3 io = glm::transpose(orientation);
    double d = 1.0/SCREEN_HEIGHT;
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            glm::dvec3 p( (x-SCREEN_WIDTH/2)*d, (SCREEN_HEIGHT/2-y)*d, 1 );
            p = io * p;
            double ax=fabs(p.x);
            double ay=fabs(p.y);
            double az=fabs(p.z);
        
            if (ax>=ay && ax>=az) {
                if (p.x>0) {
                    SDL_Surface * s = cubemap[2];
                    if (s) {
                        int fx = s->w*(-p.z/ax/2+0.5);
                        int fy = s->h*(-p.y/ax/2+0.5);
                        fx = CLAMP(fx, 0, s->w);
                        fy = CLAMP(fy, 0, s->h);
                        pix(x,y,7LL<<57, ((unsigned int*)s->pixels)[fx+fy*s->w]);
                    }
                } else {
                    SDL_Surface * s = cubemap[4];
                    if (s) {
                        int fx = s->w*(p.z/ax/2+0.5);
                        int fy = s->h*(-p.y/ax/2+0.5);
                        fx = CLAMP(fx, 0, s->w);
                        fy = CLAMP(fy, 0, s->h);
                        pix(x,y,7LL<<57, ((unsigned int*)s->pixels)[fx+fy*s->w]);
                    }
                }
            } else if (ay>=ax && ay>=az) {
                if (p.y>0) {
                    SDL_Surface * s = cubemap[0];
                    if (s) {
                        int fx = s->w*(p.x/ay/2+0.5);
                        int fy = s->h*(p.z/ay/2+0.5);
                        fx = CLAMP(fx, 0, s->w);
                        fy = CLAMP(fy, 0, s->h);
                        pix(x,y,7LL<<57, ((unsigned int*)s->pixels)[fx+fy*s->w]);
                    }
                } else {
                    SDL_Surface * s = cubemap[5];
                    if (s) {
                        int fx = s->w*(p.x/ay/2+0.5);
                        int fy = s->h*(-p.z/ay/2+0.5);
                        fx = CLAMP(fx, 0, s->w);
                        fy = CLAMP(fy, 0, s->h);
                        pix(x,y,7LL<<57, ((unsigned int*)s->pixels)[fx+fy*s->w]);
                    }                    
                }
            } else if (az>=ax && az>=ay) {
                if (p.z>0) {
                    SDL_Surface * s = cubemap[1];
                    if (s) {
                        int fx = s->w*(p.x/az/2+0.5);
                        int fy = s->h*(-p.y/az/2+0.5);
                        fx = CLAMP(fx, 0, s->w);
                        fy = CLAMP(fy, 0, s->h);
                        pix(x,y,7LL<<57, ((unsigned int*)s->pixels)[fx+fy*s->w]);
                    }
                } else {
                    SDL_Surface * s = cubemap[3];
                    if (s) {
                        int fx = s->w*(-p.x/az/2+0.5);
                        int fy = s->h*(-p.y/az/2+0.5);
                        fx = CLAMP(fx, 0, s->w);
                        fy = CLAMP(fy, 0, s->h);
                        pix(x,y,7LL<<57, ((unsigned int*)s->pixels)[fx+fy*s->w]);
                    }
                }
            }
        }
    }
}


