cmake_minimum_required (VERSION 2.6)
project(voxel-engine)
option(ENABLE_CAPTURE "Support the -capture switch if ffmpeg is available")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Wextra -march=native")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Wextra -march=nocona") # For testing without SSE4.1
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -fwhole-program -fuse-linker-plugin")
set(CMAKE_AR "gcc-ar")
set(CMAKE_NM "gcc-nm")
set(CMAKE_RANLIB "gcc-ranlib")

find_package(GLM REQUIRED)
find_package(SDL2)
find_package(SDL2_image)
find_package(PNG)
if (ENABLE_CAPTURE)
    find_package(LibAV COMPONENTS avcodec avformat avutil swscale) # Actually we are using ffmpeg here.
endif()

# find_package(Lua51)
# find_package(Qt4)
# find_package(KDE4)

include(AddTarget)

# The library containing the voxel rendering engine.
add_target(engine LIBRARY SOURCE
    src/engine/octree.h
    src/engine/octree_file.cpp
    src/engine/octree_draw.cpp
    src/engine/pointset.h
    src/engine/pointset.cpp
    src/engine/quadtree.h
    src/engine/quadtree.cpp
    src/engine/surface.h
    src/engine/surface.cpp
    src/engine/timing.h
    src/engine/timing.cpp
    HEADERS src/engine
    REQUIRED GLM
    OPTIONAL PNG
)

# The targets
add_target(voxel SOURCE
    src/main.cpp
    src/events.cpp
    src/art_sdl.cpp
    src/capture.cpp
    src/ssao.cpp
    REQUIRED SDL2 engine
    OPTIONAL LibAV 
)

add_target(benchmark SOURCE
    src/benchmark.cpp
    src/art_sdl.cpp
    src/ssao.cpp
    REQUIRED SDL2 engine
)

add_target(convert   SOURCE src/convert.cpp   REQUIRED engine)
add_target(convert2  SOURCE src/convert2.cpp  REQUIRED engine)
add_target(ascii2bin SOURCE src/ascii2bin.cpp REQUIRED engine)
# add_target(heightmap SOURCE src/heightmap.cpp REQUIRED engine SDL2 SDL2_image) # Not yet ported to SDL2.
add_target(build_db  SOURCE src/build_db.cpp  REQUIRED engine)

add_target(holes     SOURCE src/holes.cpp)
    
message(STATUS "Buildable Targets: ${BUILDABLE_TARGETS}")
