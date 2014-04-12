LIBRARY=libTiny2D

all: $(LIBRARY).a $(LIBRARY).so

MAJOR=0d1
MINOR=0

SOURCES = \
	Src/OpenGL/Tiny2D_OpenGL.cpp \
	Src/OpenGL/Tiny2D_OpenGLES.cpp \
	Src/OpenGL/Tiny2D_OpenGLMaterial.cpp \
	Src/SDL/Tiny2D_SDL.cpp \
	Src/Tiny2D_CPPWrappers.cpp \
	Src/Tiny2D_Common.cpp \
	Src/Tiny2D_Localization.cpp \
	Src/Tiny2D_Particles.cpp \
	Src/Tiny2D_Postprocessing.cpp \
	Src/Tiny2D_RapidXML.cpp \
	Src/Tiny2D_RectPacker.cpp \
	Src/Tiny2D_Shape.cpp \
	Src/Tiny2D_Sprite.cpp \
	Src/Tiny2D_Unicode.cpp \
	SDKs/OGGVorbis/stb_vorbis.cpp

INCLUDE_DIRS = -I"$(shell pwd)/Include" -I"$(shell pwd)/SDKs/"

SHARED_OBJS = $(SOURCES:.cpp=.shared.o)
STATIC_OBJS = $(SOURCES:.cpp=.static.o)

PKG_CONFIG=sdl2
PKG_CONFIG_CFLAGS=`pkg-config --cflags $(PKG_CONFIG)`
PKG_CONFIG_LIBS=`pkg-config --libs $(PKG_CONFIG)`

CFLAGS=-O2 -g -Wall 
EXTRA_CFLAGS= $(INCLUDE_DIRS) $(PKG_CONFIG_CFLAGS)
STATIC_CFLAGS= -O2 -g -Wall $(CFLAGS) $(EXTRA_CFLAGS)
SHARED_CFLAGS= $(STATIC_CFLAGS) -fPIC

LDFLAGS= -Wl,-z,defs -Wl,--as-needed -Wl,--no-undefined
EXTRA_LDFLAGS=
LIBS=$(PKG_CONFIG_LIBS) -lGL -lSDL2_ttf -lSDL2_mixer -lSDL2_image

$(LIBRARY).so.$(MAJOR).$(MINOR): $(SHARED_OBJS)
	g++ $(LDFLAGS) $(EXTRA_LDFLAGS) -shared \
		-Wl,-soname,$(LIBRARY).so.$(MAJOR) \
		-o $(LIBRARY).so.$(MAJOR).$(MINOR) \
		$+ -o $@ $(LIBS)

$(LIBRARY).so: $(LIBRARY).so.$(MAJOR).$(MINOR)
	rm -f $@.$(MAJOR)
	ln -s $@.$(MAJOR).$(MINOR) $@.$(MAJOR)
	rm -f $@
	ln -s $@.$(MAJOR) $@

$(LIBRARY).a: $(STATIC_OBJS)
	ar cru $@ $+

%.shared.o: %.cpp
	g++ -o $@ -c $+ $(SHARED_CFLAGS)

%.shared.o: %.c
	gcc -o $@ -c $+ $(SHARED_CFLAGS)

%.so : %.o
	g++ $(LDFLAGS) $(EXTRA_LDFLAGS) -shared $^ -o $@

%.static.o: %.cpp
	g++ -o $@ -c $+ $(STATIC_CFLAGS)

%.static.o: %.c
	gcc -o $@ -c $+ $(STATIC_CFLAGS)

clean:
	rm -f $(SHARED_OBJS)
	rm -f $(STATIC_OBJS)
	rm -f $(FIXED_OBJS)
	rm -f $(STATIC_FIXED_OBJS)
	rm -f *.so *.so* *.a *~
