#ifndef VOXEL_QUADTREE_H
#define VOXEL_QUADTREE_H
#include <cstdint>
#include <cstring>

template <unsigned int dim>
struct quadtree {
    typedef uint_fast32_t type;
    static constexpr type B[] = {0x00FF00FF, 0x0F0F0F0F, 0x33333333, 0x55555555};
    static constexpr type S[] = {8, 4, 2, 1};
    static const type N = (4<<dim<<dim)/3;
    static const type M = N/4;
    static const type L = M/4;
    static const type SIZE = 1<<dim;
    
    /** 
     * The quadtree is stored in a heap-like fashion as a single array.
     * The child nodes of map[i] are map[4*i+1], ..., map[4*i+4].
     */
    type map[N];
    
    /**
     * Sets a single value at given coordinates on the bottom level of the tree.
     */
    void set(type x, type y, type value) {
        for (type i=0; i<4; i++) {
            x = (x | (x << S[i])) & B[i];
            y = (y | (y << S[i])) & B[i];
        }
        map[M + x + (y<<1)] = value;
    }
    
    /**
     * Resets the quadtree, such that it is 0 everywhere
     */
    void clear() {
        memset(map,0,sizeof(map));
    }
    
    /** 
     * Sets given node to 1 if one of its children is nonzero. 
     */
    void compute(type i) {
        map[i] = 
            map[4*i+1] ||
            map[4*i+2] ||
            map[4*i+3] ||
            map[4*i+4];
    }
    
    /**
     * Ensures that a node is non-zero if one of its children is nonzero.
     */
    void build(type i=0) {
        if (i<L) {
            build(4*i+1);
            build(4*i+2);
            build(4*i+3);
            build(4*i+4);
        }
        compute(i);
    }
};


#endif
