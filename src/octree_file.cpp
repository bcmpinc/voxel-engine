#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "octree.h"

octree_file::octree_file(const char* filename) : write(false) {
    fd = open(filename, O_RDONLY);
    if (fd == -1) {perror("Could not open file"); exit(1);}
    size = lseek(fd, 0, SEEK_END);
    assert(size % sizeof(octree) == 0);
    root = (octree*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (root == MAP_FAILED) {perror("Could not map file to memory"); exit(1);} 
}

octree_file::octree_file(const char* filename, uint32_t size) : write(true), size(size) {
    fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {perror("Could not open/creat file"); exit(1);}
    int ret = ftruncate(fd, size);
    if (ret) {perror("Could not reserve diskspace"); exit(1);}
    assert(size % sizeof(octree) == 0);
    root = (octree*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (root == MAP_FAILED) {perror("Could not map file to memory"); exit(1);} 
}

octree_file::~octree_file() {
    if (root!=MAP_FAILED)
        munmap(root, size);
    if (fd!=-1)
        close(fd);
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
