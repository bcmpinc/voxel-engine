cmake_minimum_required (VERSION 2.6)
project(voxel-engine)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -flto")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -fwhole-program -fuse-linker-plugin")

find_package(SDL)
find_package(PNG)
find_package(LibAV COMPONENTS avcodec avformat avutil swscale)
find_package(SDL_image)
# find_package(Lua51)
# find_package(Qt4)
# find_package(KDE4)

# The library containing the voxel rendering engine.
set(BUILDABLE_TARGETS engine)
add_library(engine
    src/timing.cpp
    src/pointset.cpp
    src/quadtree.cpp
    src/octree_file.cpp
    src/octree_draw.cpp
)

# Function to generate executable targets, based on their dependencies.
include(CMakeParseArguments)
function(add_target name) 
    cmake_parse_arguments(TARGET "NO_ENGINE" "" "SOURCE;REQUIRED;OPTIONAL" ${ARGN})
    string(TOUPPER "${TARGET_REQUIRED}" TARGET_REQUIRED)
    string(TOUPPER "${TARGET_OPTIONAL}" TARGET_OPTIONAL)
    set(REQUIRED_SATISFIED TRUE)
    set(MISSING "")
    foreach(LIB ${TARGET_REQUIRED})
        if (NOT ${LIB}_FOUND)
            set(REQUIRED_SATISFIED FALSE)
            set(MISSING "${MISSING} ${LIB}")
        endif ()
    endforeach()
    if (REQUIRED_SATISFIED)
        set(BUILDABLE_TARGETS "${BUILDABLE_TARGETS} ${name}" PARENT_SCOPE)
        add_executable(${name} ${TARGET_SOURCE})
        if (NOT TARGET_NO_ENGINE)
            # include the engine by default
            target_link_libraries(${name} engine)
        endif()
        foreach(LIB ${TARGET_REQUIRED})
            # We already know these libraries exist
            target_include_directories(${name} PUBLIC ${${LIB}_INCLUDE_DIR})
            target_link_libraries(${name} ${${LIB}_LIBRARY})
            set_property(TARGET ${name} APPEND_STRING PROPERTY COMPILE_FLAGS " -DFOUND_${LIB}=1 ")
        endforeach()
        foreach(LIB ${TARGET_OPTIONAL})
            if (${LIB}_FOUND)
                target_include_directories(${name} PUBLIC ${${LIB}_INCLUDE_DIR})
                target_link_libraries(${name} ${${LIB}_LIBRARY})
                set_property(TARGET ${name} APPEND_STRING PROPERTY COMPILE_FLAGS " -DFOUND_${LIB}=1 ")
            endif ()
        endforeach ()
    else ()
        message("Cannot build ${name}. Libraries missing:${MISSING}")
    endif ()
endfunction ()

# The targets
add_target(voxel SOURCE
    src/main.cpp
    # src/benchmark.cpp
    # src/main_capture.cpp
    src/events.cpp
    src/art_sdl.cpp
    REQUIRED SDL 
    OPTIONAL LibAV 
)

# add_target(voxel_capture SOURCE
#     src/main_capture.cpp
#     src/events.cpp
#     src/art_sdl.cpp
#     REQUIRED SDL LibAV 
# )

add_target(benchmark SOURCE
    src/benchmark.cpp
    src/events.cpp
    src/art_sdl.cpp
    REQUIRED SDL PNG
)

add_target(convert SOURCE src/convert.cpp)
add_target(convert2 SOURCE src/convert2.cpp)
add_target(ascii2bin SOURCE src/ascii2bin.cpp)
add_target(heightmap SOURCE src/heightmap.cpp REQUIRED SDL SDL_image)
add_target(build_db SOURCE src/build_db.cpp)
    
message(STATUS "Buildable Targets: ${BUILDABLE_TARGETS}")
