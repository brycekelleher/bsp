BIN             = bspview
CC              = clang                                                                                                        
CXX             = clang++
LD              = clang++

CFLAGS          = -g -ggdb -Wall -pedantic
CXXFLAGS        = -g -ggdb -Wall -pedantic
LDFLAGS         = -lm -g -ggdb

COMMON          = ../../common
MATHLIB         = ../../common/mathlib
TOOLCOMMON      = ../common
INCLUDES        += -I$(COMMON) -I$(MATHLIB)

#disable some warnings when in dev mode
ifeq ($(DEV),1)
CFLAGS          += -Wno-unused-function -Wno-unneeded-internal-declaration
CXXFLAGS        += -Wno-unused-function -Wno-unneeded-internal-declaration
endif

ifeq ($(APPLE),1)
CFLAGS += -I/opt/X11/include
CXXFLAGS += -I/opt/X11/include
LDFLAGS = -L/opt/X11/lib
endif

OBJECTS         += $(MATHLIB)/vec3.o $(MATHLIB)/box3.o $(MATHLIB)/plane.o
OBJECTS         += $(COMMON)/toollib.o
OBJECTS         += main.o

CFLAGS          += $(INCLUDES)
CXXFLAGS        += $(INCLUDES)

$(BIN): $(OBJECTS)
	clang++ $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(BIN)

clean:
	rm -rf $(BIN) $(OBJECTS)

