# Default target
all:;

# Set to yes for optimization.
OPTIMIZATION:=yes

# Compile flags
ifeq "$(OS)" "Windows_NT"
  LDLIBS=-lmingw32 -lSDLmain -lSDL
else
  CPPFLAGS=-D_GNU_SOURCE=1 -D_REENTRANT
  LDLIBS=-lSDL -lSDL_image -lrt -lpng
endif

ifeq "$(OPTIMIZATION)" "yes"
  CXXFLAGS=-Wall -Wextra -Ofast -g -Wno-unused-result -march=native -flto
  LDFLAGS=-fwhole-program -fuse-linker-plugin
else
  CXXFLAGS=-Wall -Wextra -O0 -g -Wno-unused-result
endif

# Deafult rule patterns
build/%.o: src/%.cpp
	@mkdir -p build
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<
build/%.d: src/%.cpp
	@mkdir -p build
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM $< -MT $(@) -MT $(@:.d=.o) > $@

# Clean target
clean:
	$(eval CLEAN_FILES:=$(wildcard $(addprefix build/,$(addsuffix .d,$(SOURCE)) $(addsuffix .o,$(SOURCE)) $(addsuffix .test,$(TESTS)) $(addsuffix .output,$(TESTS)))))
	$(if $(CLEAN_FILES),-$(RM) $(CLEAN_FILES))
	$(if $(wildcard build),-rmdir build)

# Other stuff
.PHONY: all clean info

# Test macro
TESTS :=
define test
-include $(addprefix build/,$(addsuffix .test,$(1)))
TESTS := $(sort $(TESTS) $(1))
ifneq "$(MAKECMDGOALS)" "clean"
build/$(1).test: tests/$(1).cpp
	@mkdir -p build
	@(($(LINK.cpp) $$^ $(2) -o /dev/null 2> build/$(1).output && echo "TEST_$(1):=yes")|| echo "TEST_$(1):=no") > $$@
endif
endef

# System tests
$(eval $(call test,capture,-lavcodec -lavformat -lavutil -lswscale))

# Target macro
SOURCE :=
define target
SOURCE := $(sort $(SOURCE) $(2))
all: $(1)
$(1): $(addprefix build/,$(addsuffix .o,$(2)))
	$(LINK.cc) $$^ $(LOADLIBES) $(LDLIBS) $(3) -o $$@
endef

# Target definitions
$(eval $(call target,voxel,main events art_sdl timing pointset quadtree octree_file octree_draw))
$(eval $(call target,benchmark,benchmark events art_sdl timing pointset quadtree octree_file octree_draw))
$(eval $(call target,convert,convert))
$(eval $(call target,convert2,convert2 pointset))
$(eval $(call target,ascii2bin,ascii2bin pointset))
$(eval $(call target,heightmap,heightmap pointset))
$(eval $(call target,build_db,build_db pointset timing octree_file))
# $(eval $(call target,cubemap,cubemap events art_gl timing,-lGL))
ifeq "$(TEST_capture)" "yes"
# $(eval $(call target,voxel_capture,main_capture events art timing pointset octree_file octree_draw quadtree capture,-lavcodec -lavformat -lavutil -lswscale))
endif

# Header dependencies
ifneq "$(MAKECMDGOALS)" "clean"
-include $(addprefix build/,$(addsuffix .d,$(SOURCE)))
endif

# Add info target telling config/test information.
info:
	@echo Has avcodec, avformat, avutil and swscale installed: $(TEST_capture)
