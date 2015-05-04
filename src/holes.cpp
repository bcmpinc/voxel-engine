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
    return 0;
}
