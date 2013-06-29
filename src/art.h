#ifndef ART_H
#define ART_H
#include <glm/glm.hpp>

#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   768

void init_screen(const char * caption);
void flip_screen();

void pix(int64_t x, int64_t y, int c);
int rgb(int r, int g, int b);
int rgb(float r, float g, float b);
    
#endif
