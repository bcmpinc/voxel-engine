# Default target
all:;

# Compile flags
ifeq ($(OS),Windows_NT)
  LDLIBS=-lmingw32 -lSDLmain -lSDL
else
  CPPFLAGS=-D_GNU_SOURCE=1 -D_REENTRANT
  LDLIBS=-lSDL -lSDL_image -lrt -lGL
endif

#CXXFLAGS=-Wall -Ofast -Wno-unused-result -march=native -flto -g
#CXXFLAGS=-Wall -O2 -g -Wno-unused-result
CXXFLAGS=-Wall -O0 -g -Wno-unused-result
LDFLAGS=-fwhole-program -fuse-linker-plugin

# Deafult rule patterns
build/%.o: src/%.cpp
	@mkdir -p build
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<
build/%.d: src/%.cpp
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM $< -MT $(@) -MT $(@:.d=.o) > $@

# Clean target
clean:
	$(eval CLEAN_FILES:=$(wildcard $(addprefix build/,$(addsuffix .d,$(SOURCE)) $(addsuffix .o,$(SOURCE)))))
	$(if $(CLEAN_FILES),-$(RM) $(CLEAN_FILES))
	$(if $(wildcard build),-rmdir build)

# Other stuff
.PHONY: all clean
SOURCE :=

# Target macro
define target
SOURCE := $(sort $(SOURCE) $(2))
all: $(1)
$(1): $(addprefix build/,$(addsuffix .o,$(2)))
	$(LINK.cc) $$^ $(LOADLIBES) $(LDLIBS) -o $$@
endef

# Target definitions
$(eval $(call target,voxel,main events art timing pointset octree_file octree_draw quadtree))
$(eval $(call target,convert,convert))
$(eval $(call target,convert2,convert2 pointset))
$(eval $(call target,ascii2bin,ascii2bin pointset))
$(eval $(call target,heightmap,heightmap pointset))
$(eval $(call target,build_db,build_db pointset timing octree_file))
$(eval $(call target,cubemap,cubemap events art timing))

# Dependencies
ifneq "$(MAKECMDGOALS)" "clean"
-include $(addprefix build/,$(addsuffix .d,$(SOURCE)))
endif

