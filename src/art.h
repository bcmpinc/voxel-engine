#ifndef ART_H
#define ART_H
#include <cstdint>
#include <glm/glm.hpp>

#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   768

void init_screen(const char * caption);
void flip_screen();

void draw_box();
void draw_cubemap(uint32_t texture, int face);

void load_texture(uint32_t id, const char * filename);

namespace frustum {
    // Compute frustum parameters.
    const int left   = -SCREEN_WIDTH;
    const int right  =  SCREEN_WIDTH;
    const int top    =  SCREEN_HEIGHT;
    const int bottom = -SCREEN_HEIGHT;
    const int near   =  SCREEN_HEIGHT; // I.e. 90 degree FOV.
    const int cubepos=  SCREEN_WIDTH * 2; // > sqrt(3)*SCREEN_WIDTH > hypot(SCREEN_WIDTH,SCREEN_HEIGHT,SCREEN_HEIGHT) > max dist of view plane.
    const int far    =  SCREEN_WIDTH * 4; // > sqrt(3)*cubepos 
    const int slack  =  0;
}

#endif
