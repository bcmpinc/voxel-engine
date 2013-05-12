// pointer to the pixels (32 bit)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
extern int * pixs;
extern glm::dmat4 view;

void init();
void draw();

#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   768
