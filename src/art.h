#ifndef ART_H
#define ART_H
#include <cstdint>
#include <glm/glm.hpp>

#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   768

void init_screen(const char * caption);
void flip_screen();

void draw_box();
void draw_cubemap(uint32_t texture);

uint32_t load_texture(const char* filename);
uint32_t load_cubemap(const char* format);

namespace frustum {
    // Compute frustum parameters.
    const int left   = -SCREEN_WIDTH/2;
    const int right  =  SCREEN_WIDTH/2;
    const int top    =  SCREEN_HEIGHT/2;
    const int bottom = -SCREEN_HEIGHT/2;
    const int near   =  SCREEN_HEIGHT; // I.e. 90 degree FOV.
    const int cubepos=  SCREEN_WIDTH; // > sqrt(3)*SCREEN_WIDTH > hypot(SCREEN_WIDTH,SCREEN_HEIGHT,SCREEN_HEIGHT) > max dist of view plane.
    const int far    =  SCREEN_WIDTH * 2; // > sqrt(3)*cubepos 
}

#endif
