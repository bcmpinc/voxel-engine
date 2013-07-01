#include <cstdio>
#include <cassert>
#include <algorithm>
#include <SDL_image.h>
#include "pointset.h"

SDL_PixelFormat fmt = {
  NULL,
  32,
  4,
  0,0,0,0,
  16,8,0,24,
  0xff0000, 0xff00, 0xff, 0xff000000,
  0,
  0
};

int main() {
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
  SDL_Surface* texture = SDL_ConvertSurface(IMG_Load("input/tinplates.jpg"), &fmt, SDL_SWSURFACE);
  SDL_Surface* height  = SDL_ConvertSurface(IMG_Load("input/tinplates-h.jpg"), &fmt, SDL_SWSURFACE);
  
  assert(texture);
  assert(height);
  pointfile out("vxl/tinplates.vxl");
  
  fprintf(stderr, "texture: %4dx%4d\n", texture->w, texture->h);
  fprintf(stderr, "height:  %4dx%4d\n", height->w, height->h);
  
  assert(texture->w==height->w);
  assert(texture->h==height->h);
  
  int w=texture->w;
  int h=texture->h;
  
  int * t_px = (int*)texture->pixels;
  int * h_px = (int*)height->pixels;
  
  static const int hmf = 4;
  
  for(int y=0;y<h;y++) {
    for(int x=0;x<w;x++) {
      int c[4]; 
      int z[4]; 
      for (int j=0; j<4; j++) {
        int i = (x+(j&1))%w+((y+j/2)%h)*w;
        c[j] = t_px[i] & 0xfcfcfc;
        z[j] = h_px[i] & 0xff;
      }
      for (int j=0; j<4; j++) {
          out.add(point(x*2,  ((z[0]               )<<2>>0>>hmf)+j, y*2,   (c[0]               )>>0));
          out.add(point(x*2+1,((z[0]+z[1]          )<<2>>1>>hmf)+j, y*2,   (c[0]+c[1]          )>>1));
          out.add(point(x*2,  ((z[0]+z[2]          )<<2>>1>>hmf)+j, y*2+1, (c[0]+c[2]          )>>1));
          out.add(point(x*2+1,((z[0]+z[1]+z[2]+z[3])<<2>>2>>hmf)+j, y*2+1, (c[0]+c[1]+c[2]+c[3])>>2));
      }
    }
  }
}
 
// kate: space-indent on; indent-width 2; mixedindent off; indent-mode cstyle; 
