#include <stdexcept>
#include <cassert>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "pointset.h"

pointset::pointset(const char* filename, bool write) : write(write) {
    if (write) {
        fd = open(filename, O_RDWR | O_CREAT, 0644);
    } else {
        fd = open(filename, O_RDONLY);
    }
    if (fd == -1) throw std::runtime_error("Could not open file");
    size = lseek(fd, 0, SEEK_END);
    assert(size % sizeof(point) == 0);
    length = size / sizeof(point);
    list = (point*)mmap(NULL, size, PROT_READ | (write?PROT_WRITE:0), MAP_SHARED, fd, 0);
    if (list == MAP_FAILED) throw std::runtime_error("Could not map file to memory");
}

pointset::~pointset() {
    if (list!=MAP_FAILED)
        munmap(list, size);
    if (fd!=-1)
        close(fd);
}

pointfile::pointfile(const char* filename) {
    fd = open(filename, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) throw std::runtime_error("Could not open/create file");
    ftruncate(fd, 0);
}

pointfile::~pointfile() {
    if (fd!=-1)
        close(fd);
}

void pointfile::add(const point& p) {
    write(fd, &p, sizeof(p));
}
