#ifndef OCTREE_H
#define OCTREE_H
#include <cstdint>

/** A node in an octree. 
 *
 * Indices are a bitwise or of the following values:
 * x=4, y=2, z=1.
 * 
 * Hence:
 * 0 = neg-x, neg-y, neg-z
 * 1 = neg-x, neg-y, pos-z
 * etc...
 * 
 */
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
    octree_file(octree_file &) = delete;
    octree_file& operator=(octree_file&) = delete;
    ~octree_file();
};

void octree_draw(octree_file* file, uint32_t cubemap_texture);

uint32_t prepare_cubemap();

static const uint32_t OCTREE_DEPTH = 20;
static const uint32_t SCENE_SIZE = 1 << OCTREE_DEPTH;

#endif
