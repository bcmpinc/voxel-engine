/*
    Voxel - A CPU based sparse octree renderer.
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

#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <SDL.h>
#include <SDL_image.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "events.h"
#include "art.h"

using glm::min;
using glm::max;

namespace {
    // The screen surface
    SDL_Surface *screen = NULL;
    /** The projection matrix.
     * Note that we use a left handed axis system, hence we are initially looking down the positive Z-axis.
     * Up is positive Y and right is positive X.
     */
    const glm::dmat4 frustum_matrix = glm::scale(glm::frustum<double>(frustum::left, frustum::right, frustum::bottom, frustum::top, frustum::near, frustum::far),glm::dvec3(1,1,-1));
}

void init_screen(const char * caption) {
    // Initialize SDL 
    if (SDL_Init (SDL_INIT_VIDEO) < 0) {
        fprintf (stderr, "Couldn't initialize SDL: %s\n", SDL_GetError ());
        exit (2);
    }
    atexit (SDL_Quit);
#if defined _WIN32 || defined _WIN64
    freopen( "CON", "w", stdout );
    freopen( "CON", "w", stderr );
#endif

    // Set 32-bits OpenGL video mode
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 0 );
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
    // TODO: include SDL_RESIZABLE flag
    screen = SDL_SetVideoMode (SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_OPENGL);
    if (screen == NULL) {
        fprintf (stderr, "Couldn't set video mode: %s\n", SDL_GetError ());
        exit (3);
    }
    SDL_WM_SetCaption (caption, NULL);

    // Check OpenGL properties
    printf("OpenGL loaded\n");
    printf("Vendor:   %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version:  %s\n", glGetString(GL_VERSION));
    
    // Set flags
    glClearColor(0.0f, 0.5f, 1.0f, 0.0f);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Frustum
    glMatrixMode(GL_PROJECTION);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); // update context viewport size
    glLoadMatrixd(glm::value_ptr(frustum_matrix));
    glMatrixMode(GL_MODELVIEW);
    
    // Other
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    glClear(GL_COLOR_BUFFER_BIT);
    flip_screen();
}

void flip_screen() {
    SDL_GL_SwapBuffers();
    glClear(GL_COLOR_BUFFER_BIT);
}

void draw_box() {
    glm::dvec3 box_vert[48];
    int box_color[48];
    glm::dvec3 vertices[8]={
        glm::dvec3(-1,-1,-1),
        glm::dvec3( 1,-1,-1),
        glm::dvec3(-1, 1,-1),
        glm::dvec3( 1, 1,-1),
        glm::dvec3(-1,-1, 1),
        glm::dvec3( 1,-1, 1),
        glm::dvec3(-1, 1, 1),
        glm::dvec3( 1, 1, 1),
    };
    for (int a=0; a<8; a++) {
        vertices[a] = frustum::cubepos * orientation * vertices[a];
    }
    int i=0;
    for (int a=0; a<8; a++) {
        for (int b=0; b<a; b++) {
            int c = a^b;
            glm::dvec3 va = vertices[a];
            glm::dvec3 vb = vertices[b];
            
            // Note that OpenGL expects the color in BGRA order, which is 
            // written as 0xaarrggbb on a little-endian system, such as x86 and x86-64.
            switch(c) {
                case 1:
                case 2:
                case 4:
                    box_color[i]=0x000000; box_vert[i]=va; i++;
                    box_color[i]=0x000000; box_vert[i]=vb; i++;
                    break;
                case 3: // Z: blue
                    box_color[i]=0xfe0000>>!(a&b); box_vert[i]=va; i++;
                    box_color[i]=0xfe0000>>!(a&b); box_vert[i]=vb; i++;
                    break;
                case 5: // Y: green
                    box_color[i]=0x00fe00>>!(a&b); box_vert[i]=va; i++;
                    box_color[i]=0x00fe00>>!(a&b); box_vert[i]=vb; i++;
                    break;
                case 6: // X: red
                    box_color[i]=0x0000fe>>!(a&b); box_vert[i]=va; i++;
                    box_color[i]=0x0000fe>>!(a&b); box_vert[i]=vb; i++;
                    break;
            }
        }
    }
    fprintf(stderr,"box verts %d\n", i);
    glLoadIdentity();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_DOUBLE, sizeof(glm::dvec3), glm::value_ptr(box_vert[0]));
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(int), box_color);
    glDrawArrays(GL_LINES, 0, 48);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_TEXTURE_2D);
}

static const glm::dvec3 cubemap_face[] = {
    2.*glm::dvec3(frustum::left,  frustum::bottom, frustum::near),
    2.*glm::dvec3(frustum::right, frustum::bottom, frustum::near),
    2.*glm::dvec3(frustum::right, frustum::top,    frustum::near),
    2.*glm::dvec3(frustum::left,  frustum::top,    frustum::near),
};


void draw_cubemap(GLuint texture) {
    // Cubemap rendered fine, but upside down. I'm not sure why.
    // Using 'inverse-y coordinate' hack to patch this.
    glm::dmat3 inverse_orientation = glm::dmat3(1,0,0,0,-1,0,0,0,1)*glm::transpose(orientation);
    glm::dvec3 cubemap_texcoords[4];
    for (int i=0; i<4; i++) {
        cubemap_texcoords[i] = inverse_orientation*cubemap_face[i];
    }
    glEnable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glLoadIdentity();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_DOUBLE, sizeof(glm::dvec3), glm::value_ptr(cubemap_face[0]));
    glTexCoordPointer(3, GL_DOUBLE, sizeof(glm::dvec3), glm::value_ptr(cubemap_texcoords[0]));
    glDrawArrays(GL_QUADS, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_CUBE_MAP);
}

static void load_image_data(uint32_t target, const char* filename) {
    SDL_Surface * surf = IMG_Load(filename);
    if (!surf) {
        fprintf(stderr, "Failed to load '%s': %s\n", filename, SDL_GetError());
        exit(1);
    }
    if (
        (surf->w & (surf->w-1)) ||
        (surf->h & (surf->h-1)) 
    ) {
        fprintf(stderr, "Image '%s' dimensions (%dx%d) are not powers of 2.\n", filename, surf->w, surf->h);
        exit(1);
    }
    
    // get the number of channels in the SDL surface
    GLenum texture_format;
    switch(surf->format->BytesPerPixel) {
    case 4: // has alpha channel
        if (surf->format->Rmask == 0x000000ff)
            texture_format = GL_RGBA;
        else
            texture_format = GL_BGRA;
        break;
    case 3: // no alpha channel
        if (surf->format->Rmask == 0x000000ff)
            texture_format = GL_RGB;
        else
            texture_format = GL_BGR;
        break;
    default:
        printf("Image '%s' is not truecolor. Converting image.\n", filename);
        SDL_PixelFormat fmt={NULL,32,4, 0,0,0,0, 0,8,16,24, 0x000000ff,0x0000ff00,0x00ff0000,0xff000000, 0, 255};
        SDL_Surface * surf2 = surf;
        surf = SDL_ConvertSurface(surf2, &fmt, SDL_SWSURFACE);
        SDL_FreeSurface(surf2);
        if (surf==NULL) {
            fprintf(stderr,"Conversion failed for '%s'.",filename);
            exit(1);
        }
        texture_format = GL_RGBA;
        break;
    }
    
    // Flip image upside down.
    // This ensures that the bottom left pixel of the image is at 0,0.
    char * pixs = (char*)surf->pixels;
    int row = surf->format->BytesPerPixel * surf->w;
    char t[row];
    for (int y=0; y<surf->h/2; y++) {
        memcpy(t,&pixs[row*y],row);
        memcpy(&pixs[row*y],&pixs[row*(surf->h-y-1)],row);
        memcpy(&pixs[row*(surf->h-y-1)],t,row);
    }

 
    // Edit the texture object's image data using the information SDL_Surface gives us
    glTexImage2D( target, 0, surf->format->BytesPerPixel, surf->w, surf->h, 0,
                      texture_format, GL_UNSIGNED_BYTE, surf->pixels );
    
    // Free the surface
    SDL_FreeSurface(surf);
    
    printf("Successfully loaded image '%s'.\n",filename);
}

GLuint load_texture(const char* filename) {
    GLuint id;
    glGenTextures(1, &id);

    // Set the texture's stretching properties
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
    
    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, id);
    load_image_data( GL_TEXTURE_2D, filename );
    return id;
}


static GLuint cubetargets[6] = {
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
};

GLuint load_cubemap(const char * format) {
    GLuint id;
    glGenTextures(1,&id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    int len = strlen(format);
    char buf[len];
    for (int i=0; i<6; i++) {
        sprintf(buf, format, i);
        load_image_data(cubetargets[i], buf);
    }
    return id;
}