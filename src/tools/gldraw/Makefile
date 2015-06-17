CC		= clang
CXX		= clang
CFLAGS		= -g -ggdb -DGL_GLEXT_PROTOTYPES
CXXFLAGS	= -g -ggdb -DGL_GLEXT_PROTOTYPES
OBJECTS 	= read.o draw.o buffer.o main.o
LDLIBS		= -lGL -lglut -lm
BIN		= gldraw

ifeq ($(APPLE),1)
CFLAGS += -I/opt/X11/include
CXXFLAGS += -I/opt/X11/include
LDFLAGS = -L/opt/X11/lib
endif

$(BIN): $(OBJECTS)
	clang $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(BIN)

clean:
	rm -rf $(BIN) $(OBJECTS)
