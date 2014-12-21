cmake_minimum_required (VERSION 2.6)
project(voxel-engine)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -flto")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -fwhole-program -fuse-linker-plugin")

find_package(GLM REQUIRED)
find_package(SDL)
find_package(PNG)
find_package(LibAV COMPONENTS avcodec avformat avutil swscale) # Actually we are using ffmpeg here.
find_package(SDL_image)
# find_package(Lua51)
# find_package(Qt4)
# find_package(KDE4)

# The library containing the voxel rendering engine.
set(BUILDABLE_TARGETS engine)
include_directories(${GLM_INCLUDE_DIR})
add_library(engine
    src/timing.cpp
    src/pointset.cpp
    src/quadtree.cpp
    src/octree_file.cpp
    src/octree_draw.cpp
)

function(require_lib name lib)
    # Find Package scripts use inconsistent naming :(
    if (${lib}_INCLUDE_DIR)
        target_include_directories(${name} PUBLIC ${${lib}_INCLUDE_DIR})
    elseif (${lib}_INCLUDE_DIRS)
        target_include_directories(${name} PUBLIC ${${lib}_INCLUDE_DIRS})
    else ()
        message("${name} - Warning: No include dirs for ${lib}")
    endif ()
    if (${lib}_LIBRARY)
        target_link_libraries(${name} ${${lib}_LIBRARY})
    elseif (${lib}_LIBRARIES)
        target_link_libraries(${name} ${${lib}_LIBRARIES})
    else ()
        message("${name} - Warning: No libraries for ${lib}")
    endif ()
    set_property(TARGET ${name} APPEND_STRING PROPERTY COMPILE_FLAGS " -DFOUND_${lib}=1 ")
endfunction()


# Function to generate executable targets, based on their dependencies.
include(CMakeParseArguments)
function(add_target name) 
    cmake_parse_arguments(TARGET "NO_ENGINE;LIBRARY" "" "SOURCE;REQUIRED;OPTIONAL" ${ARGN})
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
        if (TARGET_LIBRARY)
            add_library(${name} ${TARGET_SOURCE})
        else ()
            add_executable(${name} ${TARGET_SOURCE})
        endif ()
        if (NOT TARGET_NO_ENGINE)
            # include the engine by default
            target_link_libraries(${name} engine)
        endif()
        foreach(LIB ${TARGET_REQUIRED})
            # We already know these libraries exist
            require_lib(${name} ${LIB})
        endforeach()
        foreach(LIB ${TARGET_OPTIONAL})
            if (${LIB}_FOUND)
                require_lib(${name} ${LIB})
            endif ()
        endforeach ()
    else ()
        message("Cannot build ${name}. Libraries missing:${MISSING}")
    endif ()
endfunction ()

# The targets
add_target(voxel SOURCE
    src/main.cpp
    src/events.cpp
    src/art_sdl.cpp
    src/capture.cpp
    REQUIRED SDL 
    OPTIONAL LibAV 
)

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
