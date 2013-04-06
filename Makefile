#LDLIBS=-lmingw32 -lSDLmain -lSDL
LDLIBS=-lSDL
CXXFLAGS=-std=gnu++0x -Wall -O3 -Wno-unused-result -march=corei7

SOURCE = vox timing octree

vox: $(addsuffix .o,$(SOURCE) )
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.d: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM $< -MT $(@) -MT $(@:.d=.o) > $@

clean:
	$(eval CLEAN_FILES:=$(vox $(addsuffix .d,$(SOURCE)) $(addsuffix .o,$(SOURCE)) ) )
	$(if $(CLEAN_FILES),-$(RM) $(CLEAN_FILES))

ifneq "$(MAKECMDGOALS)" "clean"
-include $(addsuffix .d,$(SOURCE) )
endif