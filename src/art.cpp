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
    GLuint shader_texture;
    const glm::dmat4 frustum_matrix = glm::frustum<double>(frustum::left, frustum::right, frustum::bottom, frustum::top, frustum::near, frustum::far);
}

static void compile_shader(GLuint shader, const char * filename) {
    printf("Compiling shader : %s\n", filename);
    assert(shader);
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {perror("Could not open file"); exit(1);}
    int size = lseek(fd, 0, SEEK_END);
    GLchar * data = (GLchar *)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {perror("Could not map file to memory"); exit(1);}

    // Compile the shader    
    glShaderSource(shader, 1, &data, &size);
    glCompileShader(shader);

    // Check compile result
    int result;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (!result) {
        int info_log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
        if ( info_log_length > 0 ) {
            char info_log[info_log_length];
            glGetShaderInfoLog(shader, info_log_length, NULL, info_log);
            printf("Compile error: %s\n", info_log);
        }
    }
    
    munmap(data, size);
    close(fd);
}

static GLuint link_program(const char * file_vert, const char * file_frag) {
    GLuint shader_vert = glCreateShader(GL_VERTEX_SHADER);
    GLuint shader_frag = glCreateShader(GL_FRAGMENT_SHADER);
    compile_shader(shader_vert, file_vert);
    compile_shader(shader_frag, file_frag);

    // Link the program
    printf("Linking shader program\n");
    GLuint program = glCreateProgram();
    glAttachShader(program, shader_vert);
    glAttachShader(program, shader_frag);
    glLinkProgram(program);

    // Check the program result
    int result;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (!result) {
        int info_log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
        if ( info_log_length > 0 ) {
            char info_log[info_log_length];
            glGetProgramInfoLog(program, info_log_length, NULL, info_log);
            printf("Link error: %s\n", info_log);
            exit(1);
        }
    }
    
    glDeleteShader(shader_vert);
    glDeleteShader(shader_frag);

    return program;
}

static void load_shaders() {
    shader_texture = link_program(
        "shaders/texture.vert",
        "shaders/texture.frag"
    );
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
    
    // Load shaders
    load_shaders();
    
    // Set flags
    glClearColor(0.0f, 0.5f, 1.0f, 0.0f);
    glDisable(GL_DEPTH_TEST);

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
        vertices[a] = orientation * vertices[a];
    }
    for (int a=0; a<8; a++) {
        for (int b=0; b<8; b++) {
            int c = a^b;
            glm::dvec3 va = vertices[a];
            glm::dvec3 vb = vertices[b];
            switch(c) {
                case 1:
                case 2:
                case 4:
//                    line(va,vb, 0x000000);
                    break;
                case 3:
//                    line(va,vb, 0x0000fe>>!(a&b));
                    break;
                case 5:
//                    line(va,vb, 0x00fe00>>!(a&b));
                    break;
                case 6:
//                    line(va,vb, 0xfe0000>>!(a&b));
                    break;
            }
        }
    }
}

static const GLshort squarevert[] = {
    -frustum::cubepos, -frustum::cubepos,
     frustum::cubepos, -frustum::cubepos,
     frustum::cubepos,  frustum::cubepos,
    -frustum::cubepos,  frustum::cubepos
};

static const GLshort squareuv[] = {
    0, 0,
    1, 0,
    1, 1,
    0, 1
};

void draw_cubemap(uint32_t texture, int face) {
    assert(face>=0 && face<6);
    glUseProgram(shader_texture);
    GLuint projection = glGetAttribLocation(shader_texture, "projection");
    GLuint vert = glGetAttribLocation(shader_texture, "vertexPos");
    GLuint uv = glGetAttribLocation(shader_texture, "vertexUV");
    glm::mat4 projection_matrix(glm::dmat4(orientation)*frustum_matrix);
    glUniformMatrix4fv(projection, 1, false, glm::value_ptr(projection_matrix));
    glEnableVertexAttribArray(vert);
    glVertexAttribPointer(vert, 4, GL_SHORT, false, sizeof(GLshort), squarevert);
    glEnableVertexAttribArray(uv);
    glVertexAttribPointer(uv, 4, GL_SHORT, false, sizeof(GLshort), squareuv);
    
    glDrawArrays(GL_QUADS, 0, 4);
    
    glDisableVertexAttribArray(vert);
    glDisableVertexAttribArray(uv);
}

void load_texture(uint32_t id, const char* filename) {
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

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, id);
 
    // Set the texture's stretching properties
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
 
    // Edit the texture object's image data using the information SDL_Surface gives us
    glTexImage2D( GL_TEXTURE_2D, 4, surf->format->BytesPerPixel, surf->w, surf->h, 0,
                      texture_format, GL_UNSIGNED_BYTE, surf->pixels );
    
    // Free the surface
    SDL_FreeSurface(surf);
    
    printf("Successfully loaded image '%s'.\n",filename);
}

