inline int rgb(double r, double g, double b) {
    return rgb((int)(r*255+.5),(int)(g*255+.5),(int)(b*255+.5));
}

/** Draws a line. */
void line(double x1, double y1, double x2, double y2, int c) {
    int d = (int)(1+max(abs(x1-x2),abs(y1-y2)));
    for (int i=0; i<=d; i++) {
        double x=(x1+(x2-x1)*i/d);
        double y=(y1+(y2-y1)*i/d);
        pix(x,y,c);
    }
}

/** Fills the screen with a nice rainbow spiral. */
void spirals() {
    int N = 6;
    int D = 10;
    int A[] = { 0x000001, 0x000101, 0x000100, 0x010100, 0x010000, 0x010001, };
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            int cx=x-SCREEN_WIDTH/2;
            int cy=y-SCREEN_HEIGHT/2;
            double d = hypot(cx,cy);
            double z = d + N*D*(atan2(cx,cy)/M_PI/2+.5);
            int n = (int)(fmod(z,N*D)/D);
            z = 2.5-fabs(fmod(z,D)-D/2)/2;
            d = d/320;
            if (d>1) d=1;
            z*=d;
            double q=z-1;
            if (z>1) z=1;
            if (z<0) z=0;
            if (q>1) q=1;
            if (q<0) q=0;
            int w = (int)(z*255+.5);
            int v = (int)(q*255+.5);
            pix(x,y,w*A[n]+v*A[(n+3)%N]);
        }
    }
}

void clear_screen(int c){
    for (int i = 0; i<(SCREEN_HEIGHT)*(SCREEN_WIDTH); i++) {
        pixs[i] = c;
    }  
}

void holefill() {
    abort();
    int * zbuf = NULL;
    
    static const int W1=SCREEN_WIDTH;
    static const int W2=SCREEN_WIDTH*2;
    static const int W3=SCREEN_WIDTH*3;
    // #..#
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH-3; x++) {
            int i = x+y*SCREEN_WIDTH;
            int64_t depth = min(zbuf[i], zbuf[i+3]);
            depth += depth>>8;
            if (
                zbuf[i  ]<depth &&
                zbuf[i+1]>depth && 
                zbuf[i+2]>depth &&
                zbuf[i+3]<depth
            ) {
                zbuf[i+1] = zbuf[i];
                pixs[i+1] = pixs[i];
                zbuf[i+2] = zbuf[i+3];
                pixs[i+2] = pixs[i+3];
            }
        }
    }
    
    
    // #
    // .
    // #
    for (int y=0; y<SCREEN_HEIGHT-2; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            int i = x+y*SCREEN_WIDTH;
            int64_t depth = min(zbuf[i], zbuf[i+W2]);
            depth += depth>>8;
            if (
                zbuf[i  ]<depth &&
                zbuf[i+W1]>depth && 
                zbuf[i+W2]<depth
            ) {
                zbuf[i+W1] = (zbuf[i]+zbuf[i+W2])/2;
                pixs[i+W1] = ((pixs[i]&0xfefefe)+(pixs[i+W2]&0xfefefe))/2;
            }
        }
    }
    
    
    // #
    // .
    // .
    // #
    for (int y=0; y<SCREEN_HEIGHT-3; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            int i = x+y*SCREEN_WIDTH;
            int64_t depth = min(zbuf[i], zbuf[i+W3]);
            depth += depth>>8;
            if (
                zbuf[i  ]<depth &&
                zbuf[i+W1]>depth && 
                zbuf[i+W2]>depth &&
                zbuf[i+W3]<depth
            ) {
                zbuf[i+W1] = zbuf[i];
                pixs[i+W1] = pixs[i];
                zbuf[i+W2] = zbuf[i+W3];
                pixs[i+W2] = pixs[i+W3];
            }
        }
    }
    
    
    // #.#
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH-2; x++) {
            int i = x+y*SCREEN_WIDTH;
            int64_t depth = min(zbuf[i], zbuf[i+2]);
            depth += depth>>8;
            if (
                zbuf[i  ]<depth &&
                zbuf[i+1]>depth && 
                zbuf[i+2]<depth
            ) {
                zbuf[i+1] = (zbuf[i]+zbuf[i+2])/2;
                pixs[i+1] = ((pixs[i]&0xfefefe)+(pixs[i+2]&0xfefefe))/2;
            }
        }
    }
}

void draw_axis() {
    line(orientation*(glm::dvec3(3<<17,4<<17,4<<17)-position),orientation*(glm::dvec3(5<<17,4<<17,4<<17)-position),0xff00ff);
    line(orientation*(glm::dvec3(4<<17,3<<17,4<<17)-position),orientation*(glm::dvec3(4<<17,5<<17,4<<17)-position),0xff00ff);
    line(orientation*(glm::dvec3(4<<17,4<<17,3<<17)-position),orientation*(glm::dvec3(4<<17,4<<17,5<<17)-position),0xff00ff);
}

void draw_cube() {
    glm::dmat3 io = glm::transpose(orientation);
    double d = 1.0/SCREEN_HEIGHT;
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            glm::dvec3 p( (x-SCREEN_WIDTH/2)*d, (SCREEN_HEIGHT/2-y)*d, 1 );
            p = io * p;
            double ax=fabs(p.x);
            double ay=fabs(p.y);
            double az=fabs(p.z);
        
            if ((x&1)==0 && (y&1)==0) continue;
            if ((x&1)==1 && (y&1)==0 && p.x<0) continue;
            if ((x&1)==0 && (y&1)==1 && p.y<0) continue;
            if ((x&1)==1 && (y&1)==1 && p.z<0) continue;
        
            if (ax>=ay && ax>=az) {
                pix(x,y,p.x>0?0x7f0000:0x3f0000);
            } else if (ay>=ax && ay>=az) {
                pix(x,y,p.y>0?0x007f00:0x003f00);
            } else if (az>=ax && az>=ay) {
                pix(x,y,p.z>0?0x00007f:0x00003f);
            } else {
                pix(x,y,0x808080);
            }
        }
    }
}

void draw_cubemap(struct SDL_Surface ** cubemap) {
    glm::dmat3 io = glm::transpose(orientation);
    double d = 1.0/SCREEN_HEIGHT;
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        for (int x=0; x<SCREEN_WIDTH; x++) {
            glm::dvec3 p( (x-SCREEN_WIDTH/2)*d, (SCREEN_HEIGHT/2-y)*d, 1 );
            p = io * p;
            double ax=fabs(p.x);
            double ay=fabs(p.y);
            double az=fabs(p.z);
        
            if (ax>=ay && ax>=az) {
                if (p.x>0) {
                    SDL_Surface * s = cubemap[2];
                    if (s) {
                        int fx = s->w*(-p.z/ax/2+0.5);
                        int fy = s->h*(-p.y/ax/2+0.5);
                        fx = CLAMP(fx, 0, s->w);
                        fy = CLAMP(fy, 0, s->h);
                        pix(x,y, ((unsigned int*)s->pixels)[fx+fy*s->w]);
                    }
                } else {
                    SDL_Surface * s = cubemap[4];
                    if (s) {
                        int fx = s->w*(p.z/ax/2+0.5);
                        int fy = s->h*(-p.y/ax/2+0.5);
                        fx = CLAMP(fx, 0, s->w);
                        fy = CLAMP(fy, 0, s->h);
                        pix(x,y, ((unsigned int*)s->pixels)[fx+fy*s->w]);
                    }
                }
            } else if (ay>=ax && ay>=az) {
                if (p.y>0) {
                    SDL_Surface * s = cubemap[0];
                    if (s) {
                        int fx = s->w*(p.x/ay/2+0.5);
                        int fy = s->h*(p.z/ay/2+0.5);
                        fx = CLAMP(fx, 0, s->w);
                        fy = CLAMP(fy, 0, s->h);
                        pix(x,y, ((unsigned int*)s->pixels)[fx+fy*s->w]);
                    }
                } else {
                    SDL_Surface * s = cubemap[5];
                    if (s) {
                        int fx = s->w*(p.x/ay/2+0.5);
                        int fy = s->h*(-p.z/ay/2+0.5);
                        fx = CLAMP(fx, 0, s->w);
                        fy = CLAMP(fy, 0, s->h);
                        pix(x,y, ((unsigned int*)s->pixels)[fx+fy*s->w]);
                    }                    
                }
            } else if (az>=ax && az>=ay) {
                if (p.z>0) {
                    SDL_Surface * s = cubemap[1];
                    if (s) {
                        int fx = s->w*(p.x/az/2+0.5);
                        int fy = s->h*(-p.y/az/2+0.5);
                        fx = CLAMP(fx, 0, s->w);
                        fy = CLAMP(fy, 0, s->h);
                        pix(x,y, ((unsigned int*)s->pixels)[fx+fy*s->w]);
                    }
                } else {
                    SDL_Surface * s = cubemap[3];
                    if (s) {
                        int fx = s->w*(-p.x/az/2+0.5);
                        int fy = s->h*(-p.y/az/2+0.5);
                        fx = CLAMP(fx, 0, s->w);
                        fy = CLAMP(fy, 0, s->h);
                        pix(x,y, ((unsigned int*)s->pixels)[fx+fy*s->w]);
                    }
                }
            }
        }
    }
}



SDL_PixelFormat fmt = {
  NULL,
  32,
  4,
  0,0,0,0,
  16,8,0,24,
  0xff0000, 0xff00, 0xff, 0xff000000,
  0,
  0
};

void load_cubemap(SDL_Surface ** cubemap) {
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    cubemap[0] = SDL_ConvertSurface(IMG_Load("img2/cubemap0.jpg"), &fmt, SDL_SWSURFACE);
    cubemap[1] = SDL_ConvertSurface(IMG_Load("img2/cubemap1.jpg"), &fmt, SDL_SWSURFACE);
    cubemap[2] = SDL_ConvertSurface(IMG_Load("img2/cubemap2.jpg"), &fmt, SDL_SWSURFACE);
    cubemap[3] = SDL_ConvertSurface(IMG_Load("img2/cubemap3.jpg"), &fmt, SDL_SWSURFACE);
    cubemap[4] = SDL_ConvertSurface(IMG_Load("img2/cubemap4.jpg"), &fmt, SDL_SWSURFACE);
    cubemap[5] = SDL_ConvertSurface(IMG_Load("img2/cubemap5.jpg"), &fmt, SDL_SWSURFACE);
}

SDL_Surface* create_surface(Uint32 flags,int width,int height) {
  return SDL_CreateRGBSurface(flags,width,height,
                  fmt.BitsPerPixel,
                  fmt.Rmask,fmt.Gmask,fmt.Bmask,fmt.Amask );
}

void create_cubemap(SDL_Surface ** cubemap, int size) {
    for (int i=0; i<6; i++) 
        cubemap[i] = create_surface(SDL_SWSURFACE, size, size);
}

void copy_cubemap(SDL_Surface ** src, SDL_Surface ** dest) {
    for (int i=0; i<6; i++) {
        assert(src[i]->w == dest[i]->w);
        assert(src[i]->h == dest[i]->h);
        memcpy(dest[i]->pixels, src[i]->pixels, 4*src[i]->w*src[i]->h);
    }
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
