#ifdef IN_IDE_PARSER
# define FOUND_PNG
#endif

#include <cassert>
#include "surface.h"

void surface::pixel(uint32_t x, uint32_t y, uint32_t c) {
    assert(x<width && y<height);
    int64_t i = x+y*(width);
    data[i] = c;
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
