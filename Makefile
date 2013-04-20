ifeq ($(OS),Windows_NT)
	CPPFLAGS=-DWINDOWS_NT
	LDLIBS=-lmingw32 -lSDLmain -lSDL
else
	CPPFLAGS=-I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
	LDLIBS=-lSDL -lSDL_image -lrt
endif
CXXFLAGS=-std=gnu++0x -Wall -O3 -Wno-unused-result -march=corei7
#CXXFLAGS=-std=gnu++0x -Wall -O0 -g -Wno-unused-result -march=corei7

all: vox convert heightmap

.PHONY: all

VOX_SOURCE = vox timing octree
SOURCE = $(VOX_SOURCE) convert heightmap

vox: $(addsuffix .o,$(VOX_SOURCE) )
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

convert: convert.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

heightmap: heightmap.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

	
%.d: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM $< -MT $(@) -MT $(@:.d=.o) > $@

clean:
	$(eval CLEAN_FILES:=$(vox $(addsuffix .d,$(SOURCE)) $(addsuffix .o,$(SOURCE)) ) )
	$(if $(CLEAN_FILES),-$(RM) $(CLEAN_FILES))

ifneq "$(MAKECMDGOALS)" "clean"
-include $(addsuffix .d,$(SOURCE) )
endif
