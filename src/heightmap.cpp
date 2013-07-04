/*
    Voxel - A CPU based sparse octree renderer.
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
#include <cassert>
#include <algorithm>
#include <SDL_image.h>
#include <unistd.h>
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

bool checkfile(char * buffer, const char * format, const char * name) __attribute__ ((format (printf, 2, 0)));
bool checkfile(char * buffer, const char * format, const char * name) {
  sprintf(buffer, format, name);
  return !access(buffer, F_OK);
}

uint32_t sample(SDL_Surface* s, int x, int y) {
  return ((int*)s->pixels)[
    (x+s->w)%s->w +
    (y+s->h)%s->h * s->w
  ];
}

int subsample(int c1, int c2, int c3, int c4, int x, int y) {
  static const int SUB = 2;
  static const int P = 1<<SUB;
  return ((c1*(P-x)+c2*x)*(P-y) + (c3*(P-x)+c4*x)*y) >> SUB >> SUB;
}

uint32_t subsample_color(SDL_Surface* s, int x, int y) {
  static const int SUB = 2;
  static const int MASK = (1<<SUB)-1;
  static const int M1 = 0xff0000;
  static const int M2 = 0x00ff00;
  static const int M3 = 0x0000ff;
  
  int x1 = x>>SUB, x2 = x&MASK;
  int y1 = y>>SUB, y2 = y&MASK;
  uint32_t c1 = sample(s,x1,  y1);
  uint32_t c2 = sample(s,x1+1,y1);
  uint32_t c3 = sample(s,x1,  y1+1);
  uint32_t c4 = sample(s,x1+1,y1+1);
  
  uint32_t r =
    (subsample(c1&M1,c2&M1,c3&M1,c4&M1,x2,y2)&M1) |
    (subsample(c1&M2,c2&M2,c3&M2,c4&M2,x2,y2)&M2) |
    (subsample(c1&M3,c2&M3,c3&M3,c4&M3,x2,y2)&M3);
  assert(x2 || y2 || r==(c1&0xffffff));
  return r;
}

uint32_t subsample_height(SDL_Surface* s, int x, int y) {
  static const int SUB = 2;
  static const int P = 1<<SUB;
  static const int MASK = P-1;
  static const int M = 0xff;
  
  int x1 = x>>SUB, x2 = x&MASK;
  int y1 = y>>SUB, y2 = y&MASK;
  uint32_t c1 = sample(s,x1,  y1)&M;
  uint32_t c2 = sample(s,x1+1,y1)&M;
  uint32_t c3 = sample(s,x1,  y1+1)&M;
  uint32_t c4 = sample(s,x1+1,y1+1)&M;
  
  uint32_t r = (c1*(P-x2)+c2*x2)*(P-y2) + (c3*(P-x2)+c4*x2)*y2;
  assert(x2 || y2 || (r>>4==c1));
  return r;
}

int main(int argc, const char ** argv) {
  if (argc != 3) {
    fprintf(stderr,"Please specify the file to convert (without 'input/', '-h', '.png' or '.jpg'), followed by the height reduction power.\n");
    exit(2);
  }

  // Determine height reduction power.
  char * endptr = NULL;
  const int hrp = strtol(argv[2], &endptr, 10);
  if (errno) {perror("Could not parse height reduction power"); exit(1);}
  assert(endptr);
  assert(endptr[0]==0);
  assert(hrp>=0 && hrp<12);

  // Determine the file names.
  const char * name = argv[1];
  int length=strlen(name);
  char infile[length+12];
  char infileh[length+14];
  char outfile[length+9];
  
  if (not(
    checkfile(infile, "input/%s.png", name) ||
    checkfile(infile, "input/%s.jpg", name) ||
    checkfile(infile, "input/%s.jpeg", name)
  )) {
    fprintf(stderr,"Failed to open texture.\n");
    exit(1);
  }
  if (not(
    checkfile(infileh, "input/%s-h.png", name) ||
    checkfile(infileh, "input/%s-h.jpg", name) ||
    checkfile(infileh, "input/%s-h.jpeg", name)
  )) {
    fprintf(stderr,"Failed to open heightmap.\n");
    exit(1);
  }
  sprintf(outfile, "vxl/%s.vxl", name);
  
  // Loading images
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
  SDL_Surface* texture = SDL_ConvertSurface(IMG_Load(infile), &fmt, SDL_SWSURFACE);
  SDL_Surface* height  = SDL_ConvertSurface(IMG_Load(infileh), &fmt, SDL_SWSURFACE);
  assert(texture);
  assert(height);

  fprintf(stderr, "texture: %4dx%4d  %s\n", texture->w, texture->h, infile);
  fprintf(stderr, "height:  %4dx%4d  %s\n", height->w, height->h, infileh);  
  
  // Preparing
  assert(texture->w==height->w);
  assert(texture->h==height->h);
  int w=texture->w;
  int h=texture->h;
  
  // Write output
  pointfile out(outfile);
  int points = 0;
  int maxh = 0;
  const int ds = 2;
  for(int y=0;y<h*4;y+=ds) {
    for(int x=0;x<w*4;x+=ds) {
      int c = subsample_color(texture, x, y);
      int z = subsample_height(height, x, y)>>hrp;
      out.add(point(x/ds,z,y/ds,c));
      maxh = std::max(maxh, z);
      points++;
      int n = std::min(std::min(std::min((
          subsample_height(height, x-ds, y)>>hrp),
          subsample_height(height, x+ds, y)>>hrp),
          subsample_height(height, x, y-ds)>>hrp),
          subsample_height(height, x, y+ds)>>hrp);
      for (int i=n+1; i<z; i++) {
        out.add(point(x/ds,i,y/ds,c));
        points++;
      }
    }
  }
  fprintf(stderr, "wrote: %dMi points\n", points>>20);
  fprintf(stderr, "maximum height: %d\n", maxh);
}
 
// kate: space-indent on; indent-width 2; mixedindent off; indent-mode cstyle; 
