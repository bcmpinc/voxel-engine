#ifndef ART_H
#define ART_H
#include <glm/glm.hpp>

#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   768

void init_screen(const char * caption);
void clear_screen(int c);
void flip_screen();

void pix(int64_t x, int64_t y, int64_t z, int c);
int rgb(int r, int g, int b);
int rgb(float r, float g, float b);
void holefill();
void line(double x1, double y1, double x2, double y2, int c);
void line(glm::dvec3 va, glm::dvec3 vb, int c);
void draw_box();
void draw_axis();
void draw_cube();

#endif
