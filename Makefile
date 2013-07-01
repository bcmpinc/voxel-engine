# Default target
all:;

# Compile flags
ifeq ($(OS),Windows_NT)
  LDLIBS=-lmingw32 -lSDLmain -lSDL
else
  CPPFLAGS=-I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
  LDLIBS=-lSDL -lSDL_image -lrt
endif
CXXFLAGS=-std=gnu++0x -Wall -Ofast -Wno-unused-result -march=native -flto -g
#CXXFLAGS=-std=gnu++0x -Wall -O2 -g -Wno-unused-result
#CXXFLAGS=-std=gnu++0x -Wall -O0 -g -Wno-unused-result
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
$(eval $(call target,voxel,main events art timing pointset octree renderer))
$(eval $(call target,convert,convert))
$(eval $(call target,convert2,convert2 pointset))
$(eval $(call target,ascii2bin,ascii2bin pointset))
$(eval $(call target,heightmap,heightmap pointset))
$(eval $(call target,hilbertsort,hilbertsort pointset timing octree))

# Dependencies
ifneq "$(MAKECMDGOALS)" "clean"
-include $(addprefix build/,$(addsuffix .d,$(SOURCE)))
endif

