#ifndef OCTREE_H
#define OCTREE_H
#include <cstdint>

/** A node in an octree. */
struct octree {
    octree * c[8];
    int32_t avgcolor[8];
    octree() : c{0,0,0,0,0,0,0,0}, avgcolor{-1,-1,-1,-1,-1,-1,-1,-1} {}
    void set(uint32_t x, uint32_t y, uint32_t z, uint32_t depth, uint32_t color);
    uint32_t average();
    void replicate(uint32_t mask=2, uint32_t depth=0);
};

octree * init_octree();
void draw_octree(octree * root);

static const uint32_t OCTREE_DEPTH = 20;
static const uint32_t SCENE_SIZE = 1 << OCTREE_DEPTH;

#endif
