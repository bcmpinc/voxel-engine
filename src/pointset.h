#ifndef POINTSET_H
#define POINTSET_H
#include <cstdint>

struct point {
    uint32_t x,y,z,c;
    point() {}
    point(uint32_t x, uint32_t y, uint32_t z, uint32_t c) : x(x),y(y),z(z),c(c) {}
};

/**
 * Opens a pointset file for reading.
 * Can also be opened in write mode for transforming or sorting the points.
 * Write access must be enabled before the data can be modified.
 * Points cannot be added or removed.
 */
struct pointset {
    bool write;
    uint32_t size;
    uint32_t length;
    int32_t fd;
    point * list;
    pointset(const char* filename, bool write);
    ~pointset();
    void enable_write(bool flag);
};

/**
 * Opens a file for writing out points.
 */
struct pointfile {
    int32_t fd;
    point * buffer;
    int cnt;
    pointfile(const char* filename);
    ~pointfile();
    void add(const point &p);
};

#endif
