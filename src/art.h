#ifndef ART_H
#define ART_H
#include <cstdint>
#include <glm/glm.hpp>

#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   768

void init_screen(const char * caption);
void flip_screen();

void pix(uint32_t x, uint32_t y, uint32_t c);
uint32_t rgb(uint32_t r, uint32_t g, uint32_t b);
uint32_t rgb(float r, float g, float b);
void draw_box();

#endif
