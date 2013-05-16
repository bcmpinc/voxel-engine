#ifndef EVENTS_H
#define EVENTS_H
#include <glm/glm.hpp>

void handle_events();
void next_frame(int elapsed);

extern bool quit;
extern bool moves;
extern glm::dmat3 orientation;
extern glm::dvec3 position;

#endif
