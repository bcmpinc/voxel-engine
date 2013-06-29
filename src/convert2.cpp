#include <cstdio>
#include <cstring>
#include <algorithm>
/*
 * Tower:
 * x: -336.619 - 398.857
 * y: -408.512 - 283.904
 * z:  -42.858 - 276.855
 * lines: 152209633
 */

void open_file(char * name) {
  // Determine the file names.
  int length=strlen(name)+5;
  char infile[length];
  char outfile[length];
  sprintf(infile, "input/%s.xyz", name);
  sprintf(outfile, "vxl/%s.vxl", name);
  
  // Open the files.
  FILE * res;
  res = freopen( infile,  "r", stdin );
  if (res==NULL) {
    fprintf(stderr,"Failed to open '%s' for input.\n", infile);
    exit(2);
  }
  res = freopen( outfile, "w", stdout );
  if (res==NULL) {
    fprintf(stderr,"Failed to open '%s' for output.\n", outfile);
    exit(2);
  }
}

int main(int argc, char ** argv) {
  if (argc != 2) {
    fprintf(stderr,"Please specify the file to convert (without '.txt').\n");
    exit(2);
  }
  open_file(argv[1]);

  char header[1000];
  fgets(header,1000,stdin);
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
    printf("%d %d %d %6x\n", (int)(x+C), (int)(z+C), (int)(y+C), (b<<16)+(g<<8)+r);
    line++;
  }
  fprintf(stderr,"x: %d - %d\n", minx, maxx);
  fprintf(stderr,"y: %d - %d\n", miny, maxy);
  fprintf(stderr,"z: %d - %d\n", minz, maxz);  
  fprintf(stderr,"lines: %d\n", line);
}
