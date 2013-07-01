#include <cstdio>
#include <cassert>
#include <list>

#include "art.h"
#include "timing.h"
#include "pointset.h"
#include "octree.h"

/** Initialize scene. */
octree * init_octree() {
    Timer t;
    octree * root = NULL;
    //load_voxel("vxl/sign.vxl",  6,           2,2);
    //load_voxel("vxl/mulch.vxl",     OCTREE_DEPTH-6,2,4);
    //load_voxel("vxl/tinplates.vxl", OCTREE_DEPTH-6,2,4);
    //load_voxel("vxl/wood.vxl",      OCTREE_DEPTH-5,2,4);
    //load_voxel("vxl/test.vxl",  OCTREE_DEPTH,2,6);
    //load_voxel("vxl/points.vxl",OCTREE_DEPTH,7,0,7);
    //load_voxel("vxl/tower.vxl", OCTREE_DEPTH,7,0,4);
    printf("Model loaded in %6.2fms.\n", t.elapsed());
    return root;
}

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle; 
