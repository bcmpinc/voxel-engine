#include <cstdio>
#include <cstdlib>

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
    while(!feof(f)) {
        
    }
    return 0;
}
