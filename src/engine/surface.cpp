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
#include <random>
#include <glm/glm.hpp>
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
        delete data;
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

static bool ssao_initalized = false;
static const int SSAO_PROBES = 1024;
static glm::dvec4 ssao_probe[SSAO_PROBES]; 
static void ssao_init() {
    if (ssao_initalized) return;
    std::mt19937_64 rand;
    std::uniform_real_distribution<> disc(-1, 1);
    for (int i=0; i<SSAO_PROBES; i++) {
        glm::dvec4 v;
        do {
            v = glm::dvec4(disc(rand),disc(rand),disc(rand), 0);
        } while (glm::dot(v,v) > 1);
        v.a = -sqrt(1 - v.x*v.x - v.y*v.y);
        ssao_probe[i] = v;
    }
    ssao_initalized = true;
}

static int clamp(int v, int v_min, int v_max) {
    if (v > v_max) return v_max;
    if (v < v_min) return v_min;
    return v;
}

static uint32_t modulate(uint32_t color, double light) {
    if (light < 0 || light > 1) {
        printf("Bad light: %lf\n", light);
        abort();
    }
    int r = (color>> 0) & 0xff;
    int g = (color>> 8) & 0xff;
    int b = (color>>16) & 0xff;
    r *= light;
    g *= light;
    b *= light;
    return r | (g<<8) | (b<<16);
}

void surface::apply_ssao(double radius, double projection) {
    ssao_init();
    for (int y=0; y<(int)height; y++) {
        for (int x=0; x<(int)width; x++) {
            int i = x+y*width;
            int offset = ((x&3)<<4) | ((y&3)<<7);
            int count = 0;
            int64_t md = depth[i];
            for (int j=0; j<16; j++) {
                glm::dvec4 probe(ssao_probe[offset+j]);
                int dx = probe.x*radius + 0.5;
                int dy = probe.y*radius + 0.5;
                int64_t pd1 = depth[clamp(x + dx, 0, width-1) + clamp(y + dy, 0, height-1)*width];
                int64_t pd2 = depth[clamp(x - dx, 0, width-1) + clamp(y - dy, 0, height-1)*width];
                double reld1 = (pd1-md)/(md*projection);
                double reld2 = (pd2-md)/(md*projection);
                // Range check
                if (reld1 < probe.a || reld2 < probe.a) {
                    count++;
                } else {
                    // Occlusion check
                    if (reld1 > probe.z) count++;
                    if (reld2 > -probe.z) count++;
                }
            }
            count = clamp(count, 0, 16);
            data[i] = modulate(data[i], count/16.0);
        }
    }
}

