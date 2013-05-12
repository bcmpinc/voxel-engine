// pointer to the pixels (32 bit)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
extern int * pixs;
extern glm::dmat3 orientation;
extern glm::dvec3 position;

void init();
void draw();

#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   768
