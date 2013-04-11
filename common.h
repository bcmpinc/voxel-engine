// pointer to the pixels (32 bit)
#include <cstdint>
extern int * pixs;
struct position {
    int64_t x,y,z;
    int64_t sphi, cphi;
    int64_t srho, crho;  
    uint64_t points_rendered;
};
extern position pos;
void init();
void draw();

#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   768
