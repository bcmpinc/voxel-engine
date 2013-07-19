/*
    Voxel-Engine - A CPU based sparse octree renderer.
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

#ifndef ART_H
#define ART_H
#include <stdint.h>
#include <glm/glm.hpp>

#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   768

void init_screen(const char * caption);
void flip_screen();

void draw_box();
void draw_cubemap(uint32_t texture);

uint32_t load_texture(const char* filename);
uint32_t load_cubemap(const char* format);

namespace frustum {
    // Compute frustum parameters.
    const int left   = -SCREEN_WIDTH/2;
    const int right  =  SCREEN_WIDTH/2;
    const int top    =  SCREEN_HEIGHT/2;
    const int bottom = -SCREEN_HEIGHT/2;
    const int near   =  SCREEN_HEIGHT; // I.e. 90 degree FOV.
    const int cubepos=  SCREEN_WIDTH; // > sqrt(3)*SCREEN_WIDTH > hypot(SCREEN_WIDTH,SCREEN_HEIGHT,SCREEN_HEIGHT) > max dist of view plane.
    const int far    =  SCREEN_WIDTH * 2; // > sqrt(3)*cubepos 
}

#endif
