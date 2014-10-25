/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2013,2014  B.J. Conijn <bcmpinc@users.sourceforge.net>

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

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "octree.h"

#define static_assert(test, message) typedef char static_assert__##message[(test)?1:-1]
static_assert(sizeof(octree)==4,octree_wrong_size);

char * octree_available = NULL;

octree_file::octree_file(const char* filename) : write(false) {
  fd = open(filename, O_RDONLY);
  if (fd == -1) {perror("Could not open file"); exit(1);}
  size = lseek(fd, 0, SEEK_END);
  assert(size % sizeof(octree) == 0);
  // It is unclear whether using MAP_PRIVATE or MAP_SHARED for mmap makes any difference.
  root = (octree*)mmap(NULL, size, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, fd, 0);
  if (root == MAP_FAILED) {perror("Could not map octree file to memory for reading"); exit(1);} 
}

octree_file::octree_file(const char* filename, uint32_t size) : write(true), size(size) {
  fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {perror("Could not open/creat file"); exit(1);}
  int ret = ftruncate(fd, size);
  if (ret) {perror("Could not reserve diskspace"); exit(1);}
  assert(size % sizeof(octree) == 0);
  // This requires MAP_SHARED for mmap as changes must be written to disk
  root = (octree*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (root == MAP_FAILED) {perror("Could not map octree file to memory for writing"); exit(1);} 
}

octree_file::~octree_file() {
  if (root!=MAP_FAILED)
    munmap(root, size);
  if (fd!=-1)
    close(fd);
}

// kate: space-indent on; indent-width 2; mixedindent off; indent-mode cstyle; 
