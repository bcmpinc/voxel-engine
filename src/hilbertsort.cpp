#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <algorithm>

#include "pointset.h"

static constexpr uint64_t B[] = {
  0xFFFF00000000FFFF, 
  0x00FF0000FF0000FF, 
  0xF00F00F00F00F00F, 
  0x30C30C30C30C30C3, 
  0x9249249249249249,
};
static constexpr uint64_t S[] = {32, 16, 8, 4, 2};
    
uint64_t morton3d( uint64_t x, uint64_t y, uint64_t z) {   
  // pack 3 32-bit indices into a 96-bit Morton code
  // except that the result is truncated to 64-bit.
  for (uint64_t i=0; i<5; i++) {
    x = (x | (x << S[i])) & B[i];
    y = (y | (y << S[i])) & B[i];
    z = (z | (z << S[i])) & B[i];
  }
  return x | (y<<1) | (z<<2);
}
    
uint64_t hilbert3d( const uint64_t x, const uint64_t y, const uint64_t z ) {
  uint64_t val = morton3d( x,y,z );
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

int main(int argc, char ** argv){
  /*
  if (argc != 2) {
    fprintf(stderr,"Please specify the file to convert (without '.vxl').\n");
    exit(2);
  }
    
  pointset p(argv[1], true);
  std::sort(p.list,
  */
  int N = 8;
  for (int z=0; z<N; z++) {
    for (int y=0; y<N; y++) {
      for (int x=0; x<N; x++) {
        uint64_t v = hilbert3d(x,y,z);
        printf("%3lu ",v);
        int i=x;
        while(i%2==1) {
          printf(" ");
          i/=2;
        }
      }
      printf("\n");
      int i=y;
      while(i%2==1) {
        printf("\n");
        i/=2;
      }
    }
    printf("--------------------------------\n");
  }
}

// kate: space-indent on; indent-width 2; mixedindent off; indent-mode cstyle; 

