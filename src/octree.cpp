#include <cstdio>
#include <list>

#include "art.h"
#include "timing.h"
#include "octree.h"

using std::max;
using std::min;

struct octree_buffer {
    const int N = 65536;
    std::list<octree*> list;
    int i;
    int mc;
    octree_buffer() : i(N) {}
    ~octree_buffer() {
        for (octree* ptr : list) {
            delete[](ptr); 
        }
    }
    inline octree* allocate() {
        i++;
        if (i>=N) {
            list.push_back(new octree[N]);
            i=0;
            mc += N*sizeof(octree)>>20;
        }
        return list.back() + i;
    }
} octree_buffer;

void octree::set(int x, int y, int z, int depth, int color) {
    depth--;
    assert(depth>=0);
    assert(depth<30);
    int mask = 1 << depth;
    int idx = ((x&mask) * 4 + (y&mask) * 2 + (z&mask)) >> depth;
    assert(idx>=0);
    assert(idx<8);
    if (depth==0) {
        avgcolor[idx] = color;
    } else {
        if (c[idx]==NULL) c[idx] = octree_buffer.allocate();
        c[idx]->set(x,y,z, depth, color);
    }
}
int octree::average() {
    for (int i=0; i<8; i++) {
        if(c[i]) {
            avgcolor[i] = c[i]->average();
        }
    }
    float r=0, g=0, b=0;
    int n=0;
    for (int i=0; i<8; i++) {
        if(avgcolor[i]>=0) {
            int v = avgcolor[i];
            r += (v&0xff0000)>>16;
            g += (v&0xff00)>>8;
            b += (v&0xff);
            n++;
        }
    }
    return rgb(r/n,g/n,b/n);
}
void octree::replicate(int mask, int depth) {
    if (depth<=0) return;
    for (int i=0; i<8; i++) {
        if (i == (i&mask)) {
            if (c[i]) c[i]->replicate(mask, depth-1);
        } else {
            c[i] = c[i&mask];
            avgcolor[i] = avgcolor[i&mask];
        }
    }
}

/** Reads in the voxel. */
static octree * load_voxel(const char * filename, int depth, int rep_mask, int rep_depth, int ds=0) {
    // Open the file
    FILE * f = fopen(filename, "r");
    assert(f != NULL);
    int cnt=200000000;
    octree * root = octree_buffer.allocate();

    // Read voxels and store them 
    int i;
    for (i=0; i<cnt; i++) {
        if (i%(1<<20)==0) printf("Loaded %dMi points (%dMiB)\n", i>>20, octree_buffer.mc);
        int x,y,z,c;
        int res = fscanf(f, "%d %d %d %x", &x, &y, &z, &c);
        if (res<4) break;
        c = ((c&0xff)<<16)|(c&0xff00)|((c&0xff0000)>>16);
        root->set(x>>ds,y>>ds,z>>ds,depth-ds,c);
    }
    fclose(f);
    printf("Loaded %dMi points (%dMiB)\n", i>>20, octree_buffer.mc);
    root->average();
    root->replicate(rep_mask,rep_depth);
    return root;
}

/** Initialize scene. */
octree * init_octree() {
    Timer t;
    octree * root =
    load_voxel("vxl/sign.vxl",  6,           2,2);
    //load_voxel("vxl/mulch.vxl", OCTREE_DEPTH,2,6);
    //load_voxel("vxl/test.vxl",  OCTREE_DEPTH,2,6);
    //load_voxel("vxl/points.vxl",OCTREE_DEPTH,7,0,7);
    //load_voxel("vxl/tower.vxl", OCTREE_DEPTH,7,0,4);
    printf("Model loaded in %6.2fms.\n", t.elapsed());
    return root;
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
