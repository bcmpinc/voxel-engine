#include <cstdio>
#include <cassert>
#include <algorithm>
#include <SDL_image.h>

SDL_PixelFormat fmt = {
  NULL,
  32,
  4,
  0,0,0,0,
  0,8,16,24,
  0xff, 0xff00, 0xff0000, 0xff000000,
  0,
  0
};

int main() {
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
  SDL_Surface* texture = SDL_ConvertSurface(IMG_Load("input/mulch-tiled.jpeg"), &fmt, SDL_SWSURFACE);
  SDL_Surface* height  = SDL_ConvertSurface(IMG_Load("input/mulch-heightmap.png"), &fmt, SDL_SWSURFACE);
  
  assert(texture);
  assert(height);
  
  fprintf(stderr, "texture: %4dx%4d\n", texture->w, texture->h);
  fprintf(stderr, "height:  %4dx%4d\n", height->w, height->h);
  
  assert(texture->w==height->w);
  assert(texture->h==height->h);
  
  int w=texture->w;
  int h=texture->h;
  
  int * t_px = (int*)texture->pixels;
  int * h_px = (int*)height->pixels;
  
  for(int y=0;y<h;y++) {
    for(int x=0;x<w;x++) {
      int i = x+y*w;
      int z = h_px[i]&0xff;
      printf("%d %d %d %06x\n", x*16  , z*16, y*16  , t_px[i] & 0xffffff);
      //printf("%d %d %d %06x\n", x*16+8, z, y*16  , t_px[i] & 0xffffff);
      //printf("%d %d %d %06x\n", x*16  , z, y*16+8, t_px[i] & 0xffffff);
      //printf("%d %d %d %06x\n", x*16+8, z, y*16+8, t_px[i] & 0xffffff);
    }
  }
}
 
