// pointer to the pixels (32 bit)
extern int * pixs;
struct position {
    float x,y,z;
    float sphi, cphi;
    float srho, crho;  
};
extern position pos;
void init();
void draw();

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480
