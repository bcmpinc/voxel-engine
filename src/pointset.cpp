/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2013  B.J. Conijn <bcmpinc@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "pointset.h"

pointset::pointset(const char* filename, bool write) : write(write) {
    if (write) {
        fd = open(filename, O_RDWR | O_CREAT, 0644);
        if (fd == -1) write = false;
    } 
    if (!write) {
        fd = open(filename, O_RDONLY);
    }
    if (fd == -1) {perror("Could not open file"); exit(1);}
    size = lseek(fd, 0, SEEK_END);
    assert(size % sizeof(point) == 0);
    length = size / sizeof(point);
    list = (point*)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (list == MAP_FAILED) {perror("Could not map file to memory"); exit(1);} 
}

pointset::~pointset() {
    if (list!=MAP_FAILED)
        munmap(list, size);
    if (fd!=-1)
        close(fd);
}

/**
 * Enables write access for the mapped memory region. 
 * This is used to avoid file corruption due to invalid memory access.
 * Note that writing to the memory while it does not has read permission 
 * will cause a segfault.
 */
void pointset::enable_write(bool flag) {
    if (write) {
        int ret = mprotect(list, size, PROT_READ | (flag?PROT_WRITE:0));
        if (ret) {perror("Could not change read/write memory protection"); exit(1);}
    } else{
        fprintf(stderr, "Not opened in write mode");
    }
}

static const int point_buffer_size = 1<<16;
pointfile::pointfile(const char* filename) {
    fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd == -1) {perror("Could not open/create file"); exit(1);}
    buffer = new point[point_buffer_size];
    if (!buffer) {
        fprintf(stderr, "Could not allocate larger file buffer");
    }
    cnt = 0;
}

pointfile::~pointfile() {
    write(fd, buffer, cnt * sizeof(point));
    free(buffer);
    if (fd!=-1)
        close(fd);
}

void pointfile::add(const point& p) {
    buffer[cnt] = p;
    cnt++;
    if (cnt >= point_buffer_size) {
        write(fd, buffer, point_buffer_size * sizeof(point));
        cnt = 0;
    }
}
