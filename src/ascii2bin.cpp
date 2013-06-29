#include <cstdio>
#include <cstring>
#include <algorithm>
#include <unistd.h>

#include "pointset.h"

/* Accepts files with lines of the format:
 * x y z color
 * And converts them to binary format.
 */

int main(int argc, char ** argv) {
  if (argc != 2) {
    fprintf(stderr,"Please specify the file to convert (without '.txt').\n");
    exit(2);
  }
  char * name = argv[1];

  // Determine the file names.
  int length=strlen(name);
  char infile[length+13];
  char outfile[length+9];
  sprintf(infile, "vxl/%s.vxl.txt", name);
  sprintf(outfile, "vxl/%s.vxl", name);
  
  if (access( infile, F_OK )) {
      int r = rename(outfile, infile);
      if (r) {
        fprintf(stderr,"Failed to rename '%s' to '%s'.\n", outfile, infile);
        exit(2);      
      }
  }
  
  // Open the files.
  FILE * res;
  res = freopen( infile,  "r", stdin );
  if (res==NULL) {
    fprintf(stderr,"Failed to open '%s' for input.\n", infile);
    exit(2);
  }
  
  pointfile out(outfile);

  point p;
  int line=0;
  for(;;) {
    if(line%(1<<20)==0) fprintf(stderr,"line: %3dMi\n", line>>20);
    int ret = scanf("%u %u %u %x\n", &p.x, &p.y, &p.z, &p.c);
    if (ret < 4) break;
    p.c = ((p.c&0xff)<<16)|(p.c&0xff00)|((p.c&0xff0000)>>16);
    out.add(p);
    line++;
  }
  fprintf(stderr,"lines: %d\n", line);
}
