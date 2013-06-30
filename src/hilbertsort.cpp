#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <algorithm>

#include "pointset.h"
#include "timing.h"

static constexpr uint64_t B[] = {
  0xFFFF00000000FFFF, 
  0x00FF0000FF0000FF, 
  0xF00F00F00F00F00F, 
  0x30C30C30C30C30C3, 
  0x9249249249249249,
};
static constexpr uint64_t S[] = {32, 16, 8, 4, 2};
    
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
    
bool hilbert3d( const point & p1,const point & p2 ) {
  uint64_t val1 = morton3d( p1.x,p1.y,p1.z );
  uint64_t val2 = morton3d( p1.x,p1.y,p1.z );
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

int main(int argc, char ** argv){
  Timer t;
  if (argc != 2) {
    fprintf(stderr,"Please specify the file to convert (without '.vxl').\n");
    exit(2);
  }

  // Determine the file names.
  char * name = argv[1];
  int length=strlen(name);
  char infile[length+9];
  char outfile[length+9];
  sprintf(infile, "vxl/%s.vxl", name);
  sprintf(outfile, "vxl/%s.oct", name);
  
  printf("[%10.0f] Sorting points\n", t.elapsed());
  pointset p(infile, true);
  std::stable_sort(p.list, p.list+p.length, hilbert3d);
  printf("[%10.0f] Points sorted\n", t.elapsed());

}

// kate: space-indent on; indent-width 2; mixedindent off; indent-mode cstyle; 

