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

#ifndef SURFACE_H
#define SURFACE_H
#include <stdint.h>

struct surface {
    uint32_t * refs;
    uint32_t * data;
    uint32_t * depth;
    uint32_t width;
    uint32_t height;
    
    /** Create a null-surface. */
    surface();
    
    surface(const surface &src);
    
    /** Create and allocate a surface with given sizes. 
     * \param depth Also allocate a depth buffer. */
    surface(uint32_t width, uint32_t height, bool depth = false);
    
    /** Create a surface that uses the provided external buffers.
     * The surface must be destroyed prior to deleting the buffers. */
    surface(uint32_t width, uint32_t height, uint32_t * data, uint32_t * depth = nullptr);
    
    /** Destroys the surface.
     * Any memory allocated by the surface will be released. */
    ~surface();

    surface& operator=(const surface &src);

    void export_png(const char * filename);
    
    /** Set the given pixel to the given color. 
     * The pixel coordinates must be within bounds. */
    void pixel(uint32_t x, uint32_t y, uint32_t c);

    /** Set the given pixel to the given color and depth. 
     * The pixel coordinates must be within bounds. 
     * The depth argument is ignored if this surface has no depth buffer. */
    void pixel(uint32_t x, uint32_t y, uint32_t c, uint32_t depth);

    /** Fill the surface with the given color. */
    void clear(uint32_t c);
    
    /** Create an uninitialized buffer that is n times as big in both dimensions as this surface.
     * \param depth Also allocate a depth buffer. */
    surface scale(int n, bool depth = false);
    
    /** Copy the source onto this surface. 
     * The sizes of the surfaces must match, or source must be 2x or 4x as big as this surface. */
    void copy(surface source);
};

#endif
