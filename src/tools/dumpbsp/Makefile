BIN		= dumpbsp
CC		= clang
CXX		= clang
LD		= clang

CFLAGS		= -g -Wall -pedantic
CXXFLAGS	= -g -Wall -pedantic
LDFLAGS		= -lm

COMMON		= ../../common
MATHLIB		= ../../common/mathlib
INCLUDES	+= -I$(COMMON) -I$(MATHLIB)

#disable some warnings when in dev mode
ifeq ($(DEV),1)
CFLAGS 		+= -Wno-unused-function -Wno-unneeded-internal-declaration
CXXFLAGS 	+= -Wno-unused-function -Wno-unneeded-internal-declaration
endif

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
