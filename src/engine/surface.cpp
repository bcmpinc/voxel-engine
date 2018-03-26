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

#ifdef IN_IDE_PARSER
# define FOUND_PNG
#endif

#include <algorithm>
#include <cassert>
#include "surface.h"

surface::surface() : refs(nullptr), data(nullptr), depth(nullptr), width(0), height(0) {}

surface::surface(const surface& src) : refs(src.refs), data(src.data), depth(src.depth), width(src.width), height(src.height) {
    if (refs) {
        ++*refs; // Increment ref-counter
    }
}

surface::surface(uint32_t width, uint32_t height, bool depth) : refs(new uint32_t(1)), data(new uint32_t[width*height]), depth(depth?new uint32_t[width*height]:nullptr), width(width), height(height) {
    assert(width > 0);
    assert(height > 0);
}

surface::surface(uint32_t width, uint32_t height, uint32_t * data, uint32_t * depth) : refs(nullptr), data(data), depth(depth), width(width), height(height) {}

surface::~surface() {
    if (refs && --*refs == 0) {
        delete refs;
        delete[] data;
        delete depth;
    }
}

surface& surface::operator=(const surface& src) {
    if (refs && refs == src.refs) return *this; // Copying self.
    this->~surface(); // Release current
    refs = src.refs;
    data = src.data;
    depth = src.depth;
    width = src.width;
    height = src.height;
    if (refs) {
        ++*refs; // Increment ref-counter
    }
    return *this;
}


#ifdef FOUND_PNG
# include <png.h>
void surface::export_png(const char * out) {
    png_bytep row_pointers[height];
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (png_ptr && info_ptr && !setjmp(png_jmpbuf(png_ptr))) {
        for (uint32_t i=0; i<width*height; i++)
            data[i] = 0xff000000 | ((data[i]&0xff0000)>>16) | (data[i]&0xff00) | ((data[i]&0xff)<<16);
        FILE * fp = fopen(out,"wb");
        png_init_io (png_ptr, fp);
        png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        for (uint32_t i = 0; i < height; i++) row_pointers[i] = (png_bytep)(data+i*width);
        png_set_rows(png_ptr, info_ptr, row_pointers);
        png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, 0);
        fclose(fp);
    }
    png_destroy_write_struct(&png_ptr, &info_ptr);
}
#else
void export_png(const char * out) {}
#endif

void surface::pixel(uint32_t x, uint32_t y, uint32_t c) {
    assert(x<width && y<height);
    int64_t i = x+y*(width);
    data[i] = c;
}

void surface::clear(uint32_t c) {
    std::fill_n(data, width*height, c);
    if (depth) {
        std::fill_n(depth, width*height, ~0u);
    }
}

surface surface::scale(int n, bool depth) {
    return surface(width*n, height*n, depth);
}

void surface::copy(surface source) {
    assert(data);
    assert(source.data);
    if (width == source.width) {
        assert(height == source.height);
        std::copy_n(source.data, width*height, data);
    } else if (2*width == source.width) {
        assert(2*height == source.height);
        for (unsigned int y=0; y<height; y++) {
            for (unsigned int x=0; x<width; x++) {
                uint64_t c = 0x0200020002;
                c += (source.data[x*2+0+(y*2+0)*width*2] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*2+1+(y*2+0)*width*2] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*2+0+(y*2+1)*width*2] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*2+1+(y*2+1)*width*2] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c >>= 2;
                c &= 0x0000ff0000ff00ff;
                c |= c >> 32;
                c &= 0xffffff;
                pixel(x,y,c);
            }
        }
    } else if (4*width == source.width) {
        assert(4*height == source.height);
        for (unsigned int y=0; y<height; y++) {
            for (unsigned int x=0; x<width; x++) {
                uint64_t c = 0x0200020002;
                c += (source.data[x*4+0+(y*4+0)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+1+(y*4+0)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+2+(y*4+0)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+3+(y*4+0)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+0+(y*4+1)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+1+(y*4+1)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+2+(y*4+1)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+3+(y*4+1)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+0+(y*4+2)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+1+(y*4+2)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+2+(y*4+2)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+3+(y*4+2)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+0+(y*4+3)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+1+(y*4+3)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+2+(y*4+3)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c += (source.data[x*4+3+(y*4+3)*width*4] * 0x100000001uL) & 0x0000ff0000ff00ff;
                c >>= 4;
                c &= 0x0000ff0000ff00ff;
                c |= c >> 32;
                c &= 0xffffff;
                pixel(x,y,c);
            }
        }
    } else {
        assert(!"Cannot match sizes");
    }
}

