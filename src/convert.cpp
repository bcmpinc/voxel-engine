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
#include <cstring>
#include <algorithm>
/*
 * Mouna Loa:
 * x: 22600000 - 22999999
 * y: 215100000 - 215999999
 * z: 373846 - 420008
 * intensity: 5 - 1536, avg: 21.084556
 * lines: 135833540
 */

void open_file(char * name) {
  // Determine the file names.
  int length=strlen(name)+5;
  char infile[length];
  char outfile[length];
  sprintf(infile, "input/%s.txt", name);
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
  int x1,x2,y1,y2,z1,z2;
  int clas, t1, t2, angle;
  int intensity,rets,retno, ptsrc;
  int eflf, dosf, ud, r,g,b;
  int minx=1e9,miny=1e9,minz=1e9;
  int maxx=0,  maxy=0,  maxz=0;
  int minint=1e9, maxint=0;
  int line=0;
  double int_mean=0;
  for(;;) {
    if(line%100000==0) fprintf(stderr,"line: %fM\n", line/1.e6);
    int ret = scanf("%d.%d,%d.%d,%d.%d,", &x1,&x2,&y1,&y2,&z1,&z2);
    if (ret < 6) break;
    x1*=100; x1+=x2; x1 -= 22600000;  
    y1*=100; y1+=y2; y1 -= 215100000; 
    z1*=100; z1+=z2; z1 -= 373846;    
    if(minx>x1) minx=x1; if(maxx<x1) maxx=x1;
    if(miny>y1) miny=y1; if(maxy<y1) maxy=y1;
    if(minz>z1) minz=z1; if(maxz<z1) maxz=z1;
    scanf("%d,%d.%d,%d,", &clas,&t1,&t2,&angle);
    t1*=1000000; t1+=t2;
    scanf("%d,%d,%d,%d,", &intensity, &rets, &retno, &ptsrc);
    if(minint>intensity) minint=intensity;
    if(maxint<intensity) maxint=intensity;
    int_mean += intensity;
    scanf("%d,%d,%d,%d,%d,%d", &eflf, &dosf, &ud, &r, &g, &b);
    printf("%d %d %d %6x\n", x1, z1, y1, 0x10101 * std::min(255, intensity*6));
    line++;
  }
  fprintf(stderr,"x: %d - %d\n", minx, maxx);
  fprintf(stderr,"y: %d - %d\n", miny, maxy);
  fprintf(stderr,"z: %d - %d\n", minz, maxz);  
  fprintf(stderr,"intensity: %d - %d, avg: %f\n", minint, maxint, int_mean/line);  
  fprintf(stderr,"lines: %d\n", line);
}

// kate: space-indent on; indent-width 2; mixedindent off; indent-mode cstyle; 
