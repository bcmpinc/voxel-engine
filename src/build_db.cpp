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
#include <cstring>
#include <cassert>
#include <ctime>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#include "pointset.h"
#include "timing.h"
#include "octree.h"

// For outputing the elapsed time.
static Timer t;

struct human_filesize {
  uint64_t number;
  const char * suffix;
  human_filesize(uint64_t size) {
    if (size >= (10<<20)) { 
      number = size >> 20; 
      suffix = "Mi"; 
    } else if (size >= (10<<10)) { 
      number = size >> 10; 
      suffix = "ki"; 
    } else { 
      number = size;
      suffix = ""; 
    }
  }
};

/** Maximum allowed depth of octree
 * Note that the sorting procedure has a bound of 21 layers,
 * because 64 bits/3 bits = 21.
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
  octree &node = root[index];
  int n = __builtin_popcountl(node.bitmask);
  float r=0, g=0, b=0;
  for (int i=0; i<n; i++) {
    int v = node.is_pointer(i) ? average(root, node.child[i]) : node.color(i);
      r += (v&0xff0000)>>16;
      g += (v&0xff00)>>8;
      b += (v&0xff);
  }
  node.avgcolor = rgb(r/n,g/n,b/n);
  return node.avgcolor;
}

/*void replicate(octree* root, int index, uint32_t mask, uint32_t depth) {
    if (depth<=0) return;
    for (uint32_t i=0; i<8; i++) {
        if (i == (i&mask)) {
            if (~root[index].child[i]) replicate(root, root[index].child[i], mask, depth-1);
        } else {
            root.child[i]=root.child[i&mask];
        }
    }
}*/

struct arguments {
  const char * infile;
  const char * outfile;
  int repeat_mask;
  int repeat_depth;
};

arguments parse_arguments(int argc, char ** argv) {
  static const int ARG_INFILE = 1;
  static const int ARG_OUTFILE = 2;
  static const int ARG_REPEAT_MASK = 3;
  static const int ARG_REPEAT_DEPTH = 4;
  arguments r;
  r.repeat_mask = 7;
  r.repeat_depth = 0;

  if (argc != 3 && argc != 5) {
    fprintf(stderr,"Usage: %s input_file output_file [repeat_mask repeat_depth]\n", argv[0]);
    fprintf(stderr,"Converts a poinlist (*.vxl) into an octree (*.oc2).\n");
    exit(2);
  }

  // Determine the file names.
  r.infile  = argv[ARG_INFILE];
  r.outfile = argv[ARG_OUTFILE];
  time_t rawtime = std::time(NULL);
  std::tm * timeinfo = std::localtime(&rawtime);
  printf("[%10.0f] Conversion of pointfile %s into octree %s started at %d:%02d:%02d.\n", t.elapsed(), r.infile, r.outfile, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  // Determine repeat arguments
  if (argc == 5) {
    char * endptr = NULL;
    r.repeat_mask  = strtol(argv[ARG_REPEAT_MASK], &endptr, 10);
    if (errno) {perror("Could not parse mask"); exit(1);}
    assert(endptr);
    assert(endptr[0]==0);
    assert(r.repeat_mask>=0 && r.repeat_mask<8);
    r.repeat_depth = strtol(argv[ARG_REPEAT_DEPTH], &endptr, 10);
    if (errno) {perror("Could not parse depth"); exit(1);}
    assert(endptr);
    assert(endptr[0]==0);
    assert(r.repeat_depth>=0 && r.repeat_depth<16);
    int dirs = (0x01121223>>r.repeat_mask*4) & 3;
    printf("[%10.0f] Result cloned %d times at %d layers in %s%s%s direction(s).\n", t.elapsed(), 1<<dirs*r.repeat_depth, r.repeat_depth, r.repeat_mask&4?"":"X", r.repeat_mask&2?"":"Y", r.repeat_mask&1?"":"Z");
  }
  
  return r;
}

void hilbert_sort_points(const arguments &arg, pointset &in) {
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
        // TODO: branch into multiple threads at some point if cpu usage is high.
        std::sort(in.list, in.list+in.length, hilbert3d_compare);
        in.enable_write(false);
      } else {
        printf("[%10.0f] Cannot proceed as '%s' is read only.\n", t.elapsed(), arg.infile);
        exit(1);
      }
      break;
    }
    old = cur;
  }
}

/** Stores the number of nodes per layer and some additional information.
 * Note that bottom_layer < top_data_layer <= top_repeat_layer and
 * that the active layers range from bottom_layer to top_data_layer.
 */
struct layer_info {
  uint64_t nodecount[D];
  int top_repeat_layer;
  int top_data_layer;
  int bottom_layer;
};

layer_info count_nodes_per_layer(const arguments &arg, const pointset &in) {
  layer_info r;
  // Count nodes per layer
  // Used to determine file structure and size.
  // Layers are counted as well.
  printf("[%10.0f] Counting nodes per layer.\n", t.elapsed());
  int64_t maxnode=0;
  for (int j=0; j<D; j++) r.nodecount[j]=0;
  int64_t old = -1;
  for (uint64_t i=0; i<in.length; i++) {
    if (i && (i&0x3fffff)==0) {
      printf("[%10.0f] Counting ... %6.2f%%.\n", t.elapsed(), i*100.0/in.length);
    }
    point q = in.list[i];
    assert(q.c<0x1000000);    
    int64_t cur = morton3d(q.x, q.y, q.z);
    for (int j=0; j<D; j++) {
      if ((cur>>j*3)!=(old>>j*3)) {
        r.nodecount[j]++;
      }
    }
    old = cur;
    if (maxnode<cur)
      maxnode=cur;
  }
  
  // Determine top layer
  printf("[%10.0f] Counting layers (maxnode=0x%lx).\n", t.elapsed(), maxnode);
  r.top_data_layer = 0;
  while(maxnode>>r.top_data_layer*3) r.top_data_layer++;
  printf("[%10.0f] Found 1 leaf layer + %d data layers + %d repetition layers.\n", t.elapsed(), r.top_data_layer, arg.repeat_depth);
  assert(r.nodecount[r.top_data_layer]==1);
  r.top_repeat_layer = r.top_data_layer + arg.repeat_depth;
  assert(r.top_repeat_layer <= D);
  
  // Determine lower layer prunning. Nodes should have at least 2 childnodes on average.
  printf("[%10.0f] Determine lower layer pruning.\n", t.elapsed());
  r.bottom_layer=0;
  assert(r.nodecount[r.bottom_layer]>1);
  while(r.nodecount[r.bottom_layer]<r.nodecount[r.bottom_layer+1]*2) r.bottom_layer++;
  printf("[%10.0f] Lowest %d layers will be pruned.\n", t.elapsed(), r.bottom_layer);
    
  // Report on node counts per layer and determine file size.
  for (int i=0; i<=r.top_repeat_layer; i++) {
    if (i>r.bottom_layer) {
      printf("[%10.0f] At layer %2d: %8lu nodes.\n", t.elapsed(), i, r.nodecount[i]);
    } else if (i==r.bottom_layer) {
      printf("[%10.0f] At layer %2d: %8lu leaves.\n", t.elapsed(), i, r.nodecount[i]);
    } else {
      printf("[%10.0f] At layer %2d: %8lu pruned nodes.\n", t.elapsed(), i, r.nodecount[i]);
    }
  }
  // r.bottom_layer++;
  
  return r;
}

/** Describes the structure of the outputfile
 * Note that the layer_start and layer_end are the same type as octree::child 
 * and describe the position in the octree node array.
 */
struct file_info {
  uint32_t layer_start[D];
  uint32_t layer_end[D];
  uint64_t filesize;
};

/** Determine index offsets for each layer
 */
file_info compute_file_structure(const layer_info &layers) {
  file_info r;

  for (int j=0; j<D; j++) {r.layer_start[j]=0; r.layer_end[j]=0;}
  r.filesize = 0;
  // Repeated & top layers get room for the bitmask/color and 8 children.
  // Note: layers.nodecount[layers.top_data_layer]==1.
  for (int i=layers.top_repeat_layer; i>=layers.top_data_layer; i--) {
    r.layer_start[i] = r.layer_end[i+1];
    r.layer_end[i] = r.layer_start[i] + 9;
  }
  // Intermediate layers get room for that layers bitmasks/colors and for the pointers to the next layer.
  for (int i=layers.top_data_layer-1; i>layers.bottom_layer; i--) {
    r.layer_start[i] = r.layer_end[i+1];
    r.layer_end[i] = r.layer_start[i] + layers.nodecount[i] + layers.nodecount[i-1]; 
  } 
  // Leaf layer is stored in the parent layer.
  r.filesize = r.layer_end[layers.bottom_layer+1] * sizeof(octree);
  //for (int j=0; j<D; j++) {printf("Layer %d: %d-%d\n", j, r.layer_start[j], r.layer_end[j]);}
  return r;
}

void write_points(octree* root, const pointset &in, const layer_info &layers, const file_info &file) {
  // Read voxels and store them.
  printf("[%10.0f] Storing points.\n", t.elapsed());
  uint64_t bytes_written = 0;
  uint32_t location[D]; //< Writing location for data of each layer.
  for (uint32_t i=0; i<D; i++) {
    location[i] = file.layer_start[i];
  }
  // Create rootnode
  root[0].bitmask = 0;
  location[layers.top_repeat_layer]++;
  bytes_written += 4;
  // Process file.
  for (uint32_t i=0; i<in.length; i++) {
    // Periodically print some progress info every 4MiPoints.
    if (i && (i&0x3fffff)==0) {
      human_filesize bytes(bytes_written);
      printf("[%10.0f] Stored %6.2f%% points (%lu%sB).\n", t.elapsed(), i*100.0/in.length, bytes.number, bytes.suffix);
    }
    // Proces the next point.
    point p(in.list[i]);
    uint64_t val = morton3d(p.z, p.y, p.x);
    octree * cur = root;
    //printf("val=%15lx, p{x=%d,y=%d,z=%d,c=%6x}.\n", val, p.x, p.y, p.z, p.c);
    for (int depth = layers.top_repeat_layer-1; depth >= layers.bottom_layer; depth--) {
      // Extract the child index for the current layer based from the morton code.
      uint32_t index = (val >> depth*3)&7;
      uint32_t pos = cur->insert_index(index);
      //printf("i=%u, depth=%d, index=%d, pos=%d, location[depth]=%u, cur=%ld.\n", i, depth, index, pos, location[depth+1], cur-root);
      
      if (depth <= layers.bottom_layer) {
        if (cur->child[pos] == 0) {
          assert(location[depth+1]<file.layer_end[depth+1]);
          location[depth+1]++; // Create entry in this layer
          bytes_written += 4;
        }
        // Bottom layer stores child colors instead of child pointers.
        cur->set_color(pos, p.c);
        //printf("Created leaf (%ldB)\n", bytes_written);
      } else {
        // Check if we need to create a new node.
        if (cur->child[pos] == 0) {
          // Is there still sufficient bytes left?
          assert(location[depth+1]<file.layer_end[depth+1]);
          assert(location[depth]<file.layer_end[depth]);
          assert(bytes_written<file.filesize);
          // Assign bytes to the new node.
          location[depth+1]++; // Create entry in this layer
          location[depth]++; // Create node in lower layer
          bytes_written += 8;
          // Initialize new node.
          uint32_t next = location[depth];
          //printf("Created node %d (%ldB)\n", next, bytes_written);
          root[next].bitmask = 0;
          cur->child[pos] = next;
        }
        assert(cur->child[pos]<file.layer_end[depth]);
        cur = &root[cur->child[pos]];
      }
    }
  }
}

int main(int argc, char ** argv){ 
  arguments arg = parse_arguments(argc, argv);
  
  // Map input file to memory
  printf("[%10.0f] Opening '%s' read/write.\n", t.elapsed(), arg.infile);
  pointset in(arg.infile, true);
  
  hilbert_sort_points(arg, in);
  
  layer_info layers = count_nodes_per_layer(arg, in);
  file_info file = compute_file_structure(layers);
  
  // Prepare output file and map it to memory
  human_filesize size(file.filesize);
  printf("[%10.0f] Creating octree file (%lu%sB).\n", t.elapsed(), size.number, size.suffix);
  octree_file out(arg.outfile, file.filesize);
  
  write_points(out.root, in, layers, file);
  
  /*
  printf("[%10.0f] Computing average colors.\n", t.elapsed());
  average(root, 0);
  
  printf("[%10.0f] Replicating model.\n", t.elapsed());
  replicate(root, 0, repeat_mask, repeat_depth);
  */
  // Done with conversion, clean up.
  printf("[%10.0f] Done.\n", t.elapsed());
}

// kate: space-indent on; indent-width 2; mixedindent off; indent-mode cstyle; 

