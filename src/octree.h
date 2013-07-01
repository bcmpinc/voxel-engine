#ifndef OCTREE_H
#define OCTREE_H
#include <cstdint>

/** A node in an octree. */
struct octree {
    uint32_t child[8];
    int32_t avgcolor[8];
};

octree * init_octree();
void draw_octree(octree * root);

static const uint32_t OCTREE_DEPTH = 20;
static const uint32_t SCENE_SIZE = 1 << OCTREE_DEPTH;

#endif
