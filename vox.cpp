#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <SDL/SDL.h>
#include "timing.h"

using namespace std;

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480
#define SCALE           100

// The screen surface
SDL_Surface *screen = NULL;

// pointer to the pixels (32 bit)
int * pixs;

// Quiting?
bool quit = false;

// Buttons
class button {
    public:
    enum {
        W, A, S, D,
        
        STATES
    };
};
bool button_state[button::STATES];
bool mousemove=false;
bool moves=true;

// Position
const float rotatespeed = 360, movespeed = 20;  
float px=0, py=0, pz=-16;
float phi, rho;
const float pid180=3.1415926535/180;

float sphi;
float cphi;
float srho;
float crho;


// checks user input
void pollevent() {
    SDL_Event event;
  
    /* Check for events */
    while (SDL_PollEvent (&event)) {
        switch (event.type) {
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            bool state = (event.type == SDL_KEYDOWN);
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    quit = true;
                    break;
                case SDLK_w:
                    button_state[button::W] = state;
                    break;
                case SDLK_a:
                    button_state[button::A] = state;
                    break;
                case SDLK_s:
                    button_state[button::S] = state;
                    break;
                case SDLK_d:
                    button_state[button::D] = state;
                    break;
                case SDLK_SPACE:
                    break;
                default:
                    break;
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            SDL_ShowCursor(mousemove);
            mousemove=!mousemove;
            SDL_WM_GrabInput(mousemove?SDL_GRAB_ON:SDL_GRAB_OFF);
            break;
        }
        case SDL_MOUSEMOTION: {
            if (mousemove) {
                phi -= event.motion.xrel*0.3;
                rho -= event.motion.yrel*0.3;
                moves=true;
            }
            break;
        }
        case SDL_QUIT:
            quit = true;
            break;
        default:
            break;
        }
    }
}

inline void pix(int x, int y, int c) {
  if (x>=0 && y>=0 && x<SCREEN_WIDTH && y<SCREEN_HEIGHT)
    pixs[x+y*SCREEN_WIDTH] = c;
}
inline void pix(double x, double y, int c) {
    pix((int)(x+0.5),(int)(y+0.5), c);
}
#define CLAMP(x) (x<0?0:x>255?255:x)
inline int rgb(int r, int g, int b) {
    return CLAMP(r)<<16|CLAMP(g)<<8|CLAMP(b);
}
inline int rgb(double r, double g, double b) {
    return rgb((int)(r*255+.5),(int)(g*255+.5),(int)(b*255+.5));
}

/** Draws a line. */
void line(double x1, double y1, double x2, double y2, int c) {
    int d = (int)(1+max(abs(x1-x2),abs(y1-y2)));
    for (int i=0; i<=d; i++) {
        double x=(x1+(x2-x1)*i/d);
        double y=(y1+(y2-y1)*i/d);
        pix(x,y,c);
    }
}

/** Fills the screen with a nice rainbow spiral. */
void spirals() {
    int N = 6;
    int D = 10;
    int A[] = { 0x000001, 0x000101, 0x000100, 0x010100, 0x010000, 0x010001, };
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            int cx=x-SCREEN_WIDTH/2;
            int cy=y-SCREEN_HEIGHT/2;
            double d = hypot(cx,cy);
            double z = d + N*D*(atan2(cx,cy)/M_PI/2+.5);
            int n = (int)(fmod(z,N*D)/D);
            z = 2.5-fabs(fmod(z,D)-D/2)/2;
            d = d/320;
            if (d>1) d=1;
            z*=d;
            double q=z-1;
            if (z>1) z=1;
            if (z<0) z=0;
            if (q>1) q=1;
            if (q<0) q=0;
            int w = (int)(z*255+.5);
            int v = (int)(q*255+.5);
            pix(x,y,w*A[n]+v*A[(n+3)%N]);
        }
    }
}

/** A node in an octree. */
struct octtree {
    octtree * c[8];
    int avgcolor;
    octtree() : c{0,0,0,0,0,0,0,0}, avgcolor(0) {   
    }
    ~octtree() { for (int i=0; i<8; i++) delete c[i]; }
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
            if (c[idx]==NULL) c[idx] = new octtree;
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
            if (scale > 0.02) {
                for (int i=0; i<8; i++) {
                    draw(x+(i&4?scale:-scale),y+(i&2?scale:-scale),z+(i&1?scale:-scale), scale);
                }
            } else {
                double nx =   x*cphi + z*sphi;
                double nz = - x*sphi + z*cphi;
                double ny =   y*crho -nz*srho;
                        z =   y*srho +nz*crho;
                if (z>0)
                    pix(nx*SCREEN_HEIGHT/z+SCREEN_WIDTH/2,-ny*SCREEN_HEIGHT/z+SCREEN_HEIGHT/2, avgcolor);
            }
        }
    }
};

/** Le scene. */
octtree M;

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

/*
void raster() {
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        float cy=y-SCREEN_HEIGHT/2;
        cy/=SCREEN_WIDTH/2;
        for (int x=0; x<SCREEN_WIDTH; x++) {
            float cx=x-SCREEN_WIDTH/2;
            cx/=SCREEN_WIDTH/2;
            int c = ray(px,py,pz,
                        (-crho+srho*cy)*sphi+cphi*cx,
                        (+srho-crho*cy),
                        (+crho+srho*cy)*cphi+sphi*cx);
            pix(x,y,c);
        }
    }
}
*/

// Draw anything on the screen
void draw() {
    Timer t;
    if (moves) {
        SDL_FillRect(screen,NULL, 0x000000);
        sphi = sin(phi*pid180);
        cphi = cos(phi*pid180);
        srho = sin(rho*pid180);
        crho = cos(rho*pid180);
        M.draw(-px,-py,-pz,8);
        SDL_Flip (screen);
        printf("%4.2f\n", t.elapsed());
    }
    if (t.elapsed()<=40) {
        SDL_Delay((int)(50-t.elapsed()));
    }
}

void init () {
    load_voxel("sign.vxl");
    //spirals();
}

void sim() {
    const double dt = 1.0/60.0;
    moves=false;
    float dist=movespeed * dt;

    float sphi = sin(phi*pid180);
    float cphi = cos(phi*pid180);
    float srho = sin(rho*pid180);
    float crho = cos(rho*pid180);
    
    if (button_state[button::W]) {
        px -= crho * sphi * dist;
        py += srho * dist;
        pz += crho * cphi * dist;
        moves=true;
    }
    if (button_state[button::S]) {
        px += crho * sphi * dist;
        py -= srho * dist;
        pz -= crho * cphi * dist;
        moves=true;
    }
    if (button_state[button::A]) {
        px -= cphi * dist;
        pz -= sphi * dist;
        moves=true;
    }
    if (button_state[button::D]) {
        px += cphi * dist;
        pz += sphi * dist;
        moves=true;
    }
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {

    // Initialize SDL 
    if (SDL_Init (SDL_INIT_VIDEO) < 0) {
        fprintf (stderr, "Couldn't initialize SDL: %s\n", SDL_GetError ());
        exit (2);
    }
    atexit (SDL_Quit);

    // Set 32-bits video mode (eventually emulated)
    screen = SDL_SetVideoMode (SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);
    if (screen == NULL) {
        fprintf (stderr, "Couldn't set video mode: %s\n", SDL_GetError ());
        exit (3);
    }
    SDL_WM_SetCaption ("Voxel renderer", NULL);

    // set the pixel pointer
    pixs=(int*)screen->pixels;
    
    init();

    // mainloop
    while (!quit) {
        draw();
        sim();
        pollevent();
    }
    
    return 0;
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
