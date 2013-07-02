#include <cstdio>
#include <cstring>
#include <algorithm>

#include "pointset.h"

/*
 * Tower:
 * x: -336.619 - 398.857
 * y: -408.512 - 283.904
 * z:  -42.858 - 276.855
 * lines: 152209633
 */

int main(int argc, char ** argv) {
  if (argc != 2) {
    fprintf(stderr,"Please specify the file to convert (without '.xyz').\n");
    exit(2);
  }
  // Determine the file names.
  char * name = argv[1];
  int length=strlen(name);
  char infile[length+11];
  char outfile[length+9];
  sprintf(infile, "input/%s.xyz", name);
  sprintf(outfile, "vxl/%s.vxl", name);
    
  // Open the files.
  FILE * res;
  res = freopen( infile,  "r", stdin );
  if (res==NULL) {
    fprintf(stderr,"Failed to open '%s' for input.\n", infile);
    exit(2);
  }
  pointfile out(outfile);

  // Do the conversion
  double x,y,z;
  int r,g,b;
  int minx= 1e9,miny= 1e9,minz= 1e9;
  int maxx=-1e9,maxy=-1e9,maxz=-1e9;
  int line=0;
  const int C = 1<<19;
  for(;;) {
    if(line%(1<<20)==0) fprintf(stderr,"line: %3dMi\n", line>>20);
    int ret = scanf("%lf %lf %lf %d %d %d\n", &x,&y,&z, &r,&g,&b);
    if (ret < 6) break;
    x*=1000;
    y*=1000;
    z*=1000;
    if(minx>x) minx=x; if(maxx<x) maxx=x;
    if(miny>y) miny=y; if(maxy<y) maxy=y;
    if(minz>z) minz=z; if(maxz<z) maxz=z;
    out.add(point((int)(x+C), (int)(z+C), (int)(y+C), (r<<16)+(g<<8)+b));
    line++;
  }
  fprintf(stderr,"x: %d - %d\n", minx, maxx);
  fprintf(stderr,"y: %d - %d\n", miny, maxy);
  fprintf(stderr,"z: %d - %d\n", minz, maxz);  
  fprintf(stderr,"lines: %d\n", line);
}

// kate: space-indent on; indent-width 2; mixedindent off; indent-mode cstyle; 
