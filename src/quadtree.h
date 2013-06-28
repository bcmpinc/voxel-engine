#ifndef VOXEL_QUADTREE_H
#define VOXEL_QUADTREE_H
#include <cstdint>
#include <cstring>

namespace quadtree_internal {
    typedef uint_fast32_t type;
    static constexpr type B[] = {0x00FF00FF, 0x0F0F0F0F, 0x33333333, 0x55555555};
    static constexpr type S[] = {8, 4, 2, 1};
}

template <unsigned int dim>
struct quadtree {
    typedef uint_fast32_t type;
    
    static const type N = (4<<dim<<dim)/3-1;
    static const type M = N/4-1;
    static const type L = M/4-1;
    static const type SIZE = 1<<dim;
    
    /** 
     * The quadtree is stored in a heap-like fashion as a single array.
     * The child nodes of map[i] are map[4*i+1], ..., map[4*i+4].
     */
    char map[N];
    uint32_t face[SIZE*SIZE];
    
    /**
     * Sets a single value at given coordinates on the bottom level of the tree.
     */
    void set(type x, type y) {
        for (type i=0; i<4; i++) {
            x = (x | (x << quadtree_internal::S[i])) & quadtree_internal::B[i];
            y = (y | (y << quadtree_internal::S[i])) & quadtree_internal::B[i];
        }
        map[M + x + (y<<1)] = 1;
    }

    uint32_t get_face(type x, type y) {
        for (type i=0; i<4; i++) {
            x = (x | (x << quadtree_internal::S[i])) & quadtree_internal::B[i];
            y = (y | (y << quadtree_internal::S[i])) & quadtree_internal::B[i];
        }
        return face[x + (y<<1)];
    }    
    
    /**
     * Resets the quadtree, such that it is 0 everywhere
     */
    void clear() {
        memset(map,0,sizeof(map));
        memset(face,0x7f,sizeof(face));
    }
    
    /** 
     * Sets given node to 1 if one of its children is nonzero. 
     */
    void compute(type i) {
        map[i] = ((int32_t*)map)[i+1]!=0;
        /*map[i] = map[4*i+4] || 
                 map[4*i+5] || 
                 map[4*i+6] || 
                 map[4*i+7];*/
    }
    
    /**
     * Ensures that a node is non-zero if one of its children is nonzero.
     */
    void build(type i) {
        if (i<L) {
            build(4*i+4);
            build(4*i+5);
            build(4*i+6);
            build(4*i+7);
        }
        compute(i);
    }
};


#endif
