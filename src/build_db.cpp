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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#include "pointset.h"
#include "timing.h"
#include "octree.h"

/** Maximum allowed depth of octree
 * Note that the sorting procedure has a bound of 21 layers.
 */
static const int D = 21;

static const uint64_t B[] = {
  0xFFFF00000000FFFF, 
  0x00FF0000FF0000FF, 
  0xF00F00F00F00F00F, 
  0x30C30C30C30C30C3, 
  0x9249249249249249,
};
static const uint64_t S[] = {32, 16, 8, 4, 2};
    
uint64_t morton3d( uint64_t x, uint64_t y, uint64_t z ) {   
  // pack 3 32-bit indices into a 96-bit Morton code
  // except that the result is truncated to 64-bit.
  for (uint64_t i=0; i<5; i++) {
    x = (x | (x << S[i])) & B[i];
    y = (y | (y << S[i])) & B[i];
    z = (z | (z << S[i])) & B[i];
  }
  return x | (y<<1) | (z<<2);
}

uint64_t hilbert3d( const point & p ) {
  uint64_t val = morton3d( p.x,p.y,p.z );
  uint64_t start = 0;
  uint64_t end = 1; // can be 1,2,4
  uint64_t ret = 0;
  for (int64_t j=19; j>=0; j--) {
    uint64_t rg = ((val>>(3*j))&7) ^ start;
    uint64_t travel_shift = (0x30210 >> (start ^ end)*4)&3;
    uint64_t i = (((rg << 3) | rg) >> travel_shift ) & 7;
    i = (0x54672310 >> i*4) & 7;
    ret = (ret<<3) | i;
    uint64_t si = (0x64422000 >> i*4 ) & 7; // next lower even number, or 0
    uint64_t ei = (0x77755331 >> i*4 ) & 7; // next higher odd number, or 7
    uint64_t sg = ( si ^ (si>>1) ) << travel_shift;
    uint64_t eg = ( ei ^ (ei>>1) ) << travel_shift;
    end   = ( ( eg | ( eg >> 3 ) ) & 7 ) ^ start;
    start = ( ( sg | ( sg >> 3 ) ) & 7 ) ^ start;
  }
  return ret;
}
    
bool hilbert3d_compare( const point & p1,const point & p2 ) {
  uint64_t val1 = morton3d( p1.x,p1.y,p1.z );
  uint64_t val2 = morton3d( p2.x,p2.y,p2.z );
  uint64_t start = 0;
  uint64_t end = 1; // can be 1,2,4
  for (int64_t j=19; j>=0; j--) {
    uint64_t travel_shift = (0x30210 >> (start ^ end)*4)&3;
    uint64_t rg1 = ((val1>>(3*j))&7) ^ start;
    uint64_t rg2 = ((val2>>(3*j))&7) ^ start;
    uint64_t i1 = (((rg1 << 3) | rg1) >> travel_shift ) & 7;
    uint64_t i2 = (((rg2 << 3) | rg2) >> travel_shift ) & 7;
    i1 = (0x54672310 >> i1*4) & 7;
    i2 = (0x54672310 >> i2*4) & 7;
    if (i1<i2) return true;
    if (i1>i2) return false;
    uint64_t si = (0x64422000 >> i1*4 ) & 7; // next lower even number, or 0
    uint64_t ei = (0x77755331 >> i1*4 ) & 7; // next higher odd number, or 7
    uint64_t sg = ( si ^ (si>>1) ) << travel_shift;
    uint64_t eg = ( ei ^ (ei>>1) ) << travel_shift;
    end   = ( ( eg | ( eg >> 3 ) ) & 7 ) ^ start;
    start = ( ( sg | ( sg >> 3 ) ) & 7 ) ^ start;
  }
  return false;
}

#define CLAMP(x,l,u) (x<l?l:x>u?u:x)
uint32_t rgb(int32_t r, int32_t g, int32_t b) {
  return CLAMP(r,0,255)<<16|CLAMP(g,0,255)<<8|CLAMP(b,0,255);
}
uint32_t rgb(float r, float g, float b) {
  return rgb((int32_t)(r+0.5),(int32_t)(g+0.5),(int32_t)(b+0.5));
}

uint32_t average(octree* root, int index) {
  for (int i=0; i<8; i++) {
    if(~root[index].child[i]) {
      root[index].avgcolor[i] = average(root, root[index].child[i]);
    }
  }
  float r=0, g=0, b=0;
  int n=0;
  for (int i=0; i<8; i++) {
    if(root[index].avgcolor[i]>=0) {
      int v = root[index].avgcolor[i];
      r += (v&0xff0000)>>16;
      g += (v&0xff00)>>8;
      b += (v&0xff);
      n++;
    }
  }
  return rgb(r/n,g/n,b/n);
}

void replicate(octree* root, int index, uint32_t mask, uint32_t depth) {
    if (depth<=0) return;
    for (uint32_t i=0; i<8; i++) {
        if (i == (i&mask)) {
            if (~root[index].child[i]) replicate(root, root[index].child[i], mask, depth-1);
        } else {
            root[index].child[i] = root[index].child[i&mask];
            root[index].avgcolor[i] = root[index].avgcolor[i&mask];
        }
    }
}

void clear(octree& n) {
  for (int i=0; i<8; i++) {
    n.avgcolor[i]=-1;
    n.child[i]=~0u;
  }
}

int main(int argc, char ** argv){
  Timer t;
  if (argc != 2 && argc != 4) {
    fprintf(stderr,"Please specify the file to convert (without '.vxl') and optionally repeat mask & depth.\n");
    exit(2);
  }
  
  // Determine repeat arguments
  int repeat_mask=7;
  int repeat_depth=0;
  if (argc == 4) {
    char * endptr = NULL;
    repeat_mask  = strtol(argv[2], &endptr, 10);
    if (errno) {perror("Could not parse mask"); exit(1);}
    assert(endptr);
    assert(endptr[0]==0);
    assert(repeat_mask>=0 && repeat_mask<8);
    repeat_depth = strtol(argv[3], &endptr, 10);
    if (errno) {perror("Could not parse depth"); exit(1);}
    assert(endptr);
    assert(endptr[0]==0);
    assert(repeat_depth>=0 && repeat_depth<16);
    int dirs = (0x01121223>>repeat_mask*4) & 3;
    printf("[%10.0f] Result cloned %d times at %d layers in %s%s%s direction(s).\n", t.elapsed(), 1<<dirs*repeat_depth, repeat_depth, repeat_mask&4?"":"X", repeat_mask&2?"":"Y", repeat_mask&1?"":"Z");
  }

  // Determine the file names.
  char * name = argv[1];
  int length=strlen(name);
  char infile[length+9];
  char outfile[length+9];
  sprintf(infile, "vxl/%s.vxl", name);
  sprintf(outfile, "vxl/%s.oct", name);
  
  // Map input file to memory
  printf("[%10.0f] Opening '%s' read/write.\n", t.elapsed(), infile);
  pointset in(infile, true);

  // Check and possibly sort the data points.
  printf("[%10.0f] Checking if %d points are sorted.\n", t.elapsed(), in.length);
  int64_t old = 0;
  for (uint64_t i=0; i<in.length; i++) {
    if (i && (i&0x3fffff)==0) {
      printf("[%10.0f] Checking ... %6.2f%%.\n", t.elapsed(), i*100.0/in.length);
    }
    int64_t cur = hilbert3d(in.list[i]);
    if (old>cur) {
      printf("[%10.0f] Point %lu should precede previous point.\n", t.elapsed(), i);
      if (in.write) {
        printf("[%10.0f] Sorting points.\n", t.elapsed());
        in.enable_write(true);
        // TODO: replace with IO-efficient k-way quicksort, with inline hilbert curve computation.
        // TODO: branch into multiple threads at some point if meaningful.
        std::sort(in.list, in.list+in.length, hilbert3d_compare);
        in.enable_write(false);
      } else {
        printf("[%10.0f] Cannot proceed as '%s' is read only.\n", t.elapsed(), infile);
        exit(1);
      }
      break;
    }
    old = cur;
  }
  
  // Count nodes per layer
  // Used to determine file structure and size.
  // Layers are counted as well.
  printf("[%10.0f] Counting nodes per layer.\n", t.elapsed());
  uint64_t nodecount[D];
  int64_t maxnode=0;
  for (int j=0; j<D; j++) nodecount[j]=0;
  old = -1;
  for (uint64_t i=0; i<in.length; i++) {
    if (i && (i&0x3fffff)==0) {
      printf("[%10.0f] Counting ... %6.2f%%.\n", t.elapsed(), i*100.0/in.length);
    }
    point q = in.list[i];
    assert(q.c<0x1000000);    
    int64_t cur = morton3d(q.x, q.y, q.z);
    for (int j=0; j<D; j++) {
      if ((cur>>j*3)!=(old>>j*3)) {
        nodecount[j]++;
      }
    }
    old = cur;
    if (maxnode<cur)
      maxnode=cur;
  }
  printf("[%10.0f] Counting layers (maxnode=0x%lx).\n", t.elapsed(), maxnode);
  int layers=0;
  while(maxnode>>layers*3) layers++;
  printf("[%10.0f] Found 1 leaf layer + %d data layers + %d repetition layers.\n", t.elapsed(), layers, repeat_depth);
  assert(nodecount[layers]==1);
  layers+=repeat_depth;
  
  // Determine lower layer prunning
  printf("[%10.0f] Determine lower layer pruning.\n", t.elapsed());
  int bottom_layer=0;
  while(nodecount[bottom_layer]<nodecount[bottom_layer+1]*2) bottom_layer++;
  printf("[%10.0f] Lowest %d layers will be pruned.\n", t.elapsed(), bottom_layer);
  
  // Report on node counts per layer and determine file size.
  uint64_t nodesum = 0;
  for (int i=0; i<=layers; i++) {
    if (i>bottom_layer) {
      printf("[%10.0f] At layer %2d: %8lu nodes.\n", t.elapsed(), i, nodecount[i]);
      nodesum += nodecount[i];
    } else if (i==bottom_layer) {
      printf("[%10.0f] At layer %2d: %8lu leaves.\n", t.elapsed(), i, nodecount[i]);
    } else {
      printf("[%10.0f] At layer %2d: %8lu pruned nodes.\n", t.elapsed(), i, nodecount[i]);
    }
  }
  uint64_t filesize = nodesum*sizeof(octree);
  
  // Prepare output file and map it to memory
  printf("[%10.0f] Creating octree file with %lu nodes of %luB each (%luMiB).\n", t.elapsed(), nodesum, sizeof(octree), filesize>>20);
  octree_file out(outfile, filesize);
  octree* root = out.root;
  clear(root[0]);
  
  // Determine index offsets for each layer
  uint32_t offset[D], bounds[D];
  for (int j=0; j<D; j++) {offset[j]=0; bounds[j]=0;}
  offset[layers] = 0;
  for (int i=layers-1; i>=bottom_layer; i--) {
    offset[i] = offset[i+1] + nodecount[i+1]; 
    bounds[i] = offset[i] + nodecount[i];
  }  
  
  // Read voxels and store them.
  printf("[%10.0f] Storing points.\n", t.elapsed());
  uint32_t i;
  uint32_t nodes_created = 0;
  for (i=0; i<in.length; i++) {
    if (i && (i&0x3fffff)==0) printf("[%10.0f] Stored %6.2f%% points (%luMiB).\n", t.elapsed(), i*100.0/in.length, nodes_created*sizeof(octree)>>20);
    point p(in.list[i]);
    uint64_t val = morton3d(p.z, p.y, p.x);
    octree * cur = &root[0];
    //fprintf(stderr,"val=%15lx, p{x=%d,y=%d,x=%d,c=%6x.\n", val, p.x, p.y, p.z, p.c);
    for (int depth = layers-1; depth >= bottom_layer; depth--) {
      int idx = (val >> depth*3)&7;
      //fprintf(stderr,"i=%u, depth=%d, idx=%d, offset[depth]=%u, cur=%ld.\n", i, depth, idx, offset[depth], cur-root);
      if (depth<=bottom_layer) {
        cur->avgcolor[idx] = p.c;
      } else {
        if (~cur->child[idx]==0u) {
          assert(nodes_created<nodesum);
          assert(offset[depth]<bounds[depth]);
          nodes_created++;
          uint32_t next = offset[depth]++;
          //fprintf(stderr,"Created node %d (%d)\n", next, nodes_created);
          clear(root[next]);
          cur->child[idx] = next;
        }
        assert(cur->child[idx]<nodesum);
        cur = &root[cur->child[idx]];
      }
    }
  }
  printf("[%10.0f] Computing average colors.\n", t.elapsed());
  average(root, 0);
  
  printf("[%10.0f] Replicating model.\n", t.elapsed());
  replicate(root, 0, repeat_mask, repeat_depth);
  
  // Done with conversion, clean up.
  printf("[%10.0f] Done.\n", t.elapsed());
}

// kate: space-indent on; indent-width 2; mixedindent off; indent-mode cstyle; 

