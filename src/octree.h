#ifndef OCTREE_H
#define OCTREE_H
#include <cstdint>

/** A node in an octree. */
struct octree {
    uint32_t child[8];
    int32_t avgcolor[8];
};

struct octree_file {
    const bool write;
    uint32_t size;
    int32_t fd;
    octree * root;
    octree_file(const char * filename);
    octree_file(const char * filename, uint32_t size);
    ~octree_file();
};

void octree_draw(octree* root, uint32_t cubemap_texture);

static const uint32_t OCTREE_DEPTH = 20;
static const uint32_t SCENE_SIZE = 1 << OCTREE_DEPTH;

#endif
