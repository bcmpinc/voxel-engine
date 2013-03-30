LDLIBS=-lmingw32 -lSDLmain -lSDL -mwindows
CXXFLAGS=-std=gnu++0x -Wall -O2 -Wno-unused-result -Dmain=SDL_main

vox: vox.cpp timing.o