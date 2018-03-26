/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2015  B.J. Conijn <bcmpinc@users.sourceforge.net>

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
#include <cstdint>

/** Scans for 0-holes in files. */
int main(int argc, char ** argv) {
    if (argc != 2) {
        printf("Usage: %s filename\n", argv[0]);
        exit(1);
    }
    FILE * f = fopen(argv[1], "rb");
    if (f == nullptr) {
        printf("Failed to open '%s'\n", argv[1]);
        exit(1);
    }
    int buffer;
    int start = 0;
    int pos = 0;
    bool empty=false;
    while(fread(&buffer, sizeof(buffer), 1, f)) {
        if (empty != (buffer==0)) {
            if (pos - start >= 8) {
                printf("%s segment from %d to %d.\n", empty?"null":"data", start, pos);
            }
            empty = (buffer==0);
            start = pos;
        }
        pos+=4;
    }
    if (pos - start >= 8) {
        printf("%s segment from %d to %d.\n", empty?"null":"data", start, pos);
    }
    fclose(f);
    return 0;
}
