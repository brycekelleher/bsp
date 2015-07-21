BIN		= bsp
CC		= clang
CXX		= clang++
LD		= clang++

CFLAGS		= -g -ggdb -Wall -pedantic
CXXFLAGS	= -g -ggdb -Wall -pedantic
LDFLAGS		= -lm -ggdb

COMMON		= ../../common
MATHLIB		= ../../common/mathlib
TOOLCOMMON	= ../common
INCLUDES	+= -I$(COMMON) -I$(MATHLIB)

#disable some warnings when in dev mode
ifeq ($(DEV),1)
CFLAGS 		+= -Wno-unused-function -Wno-unneeded-internal-declaration
CXXFLAGS 	+= -Wno-unused-function -Wno-unneeded-internal-declaration
endif

OBJECTS		+= $(MATHLIB)/vec3.o $(MATHLIB)/box3.o $(MATHLIB)/plane.o $(MATHLIB)/polygon.o
OBJECTS		+= $(COMMON)/toollib.o
OBJECTS		+= token.o debug.o test.o
OBJECTS		+= tree.o map.o output.o
OBJECTS		+= main.o

CFLAGS		+= $(INCLUDES)
CXXFLAGS		+= $(INCLUDES)

$(BIN): $(OBJECTS)
	$(LD) $(OBJECTS) $(LDFLAGS) -o $(BIN)

clean:
	rm -rf $(OBJECTS)
	rm -rf test
	rm -rf bsp

test: bsp
	./bsp -o outtest.bsp modelsrc/bsp/bsp_test0.txt
