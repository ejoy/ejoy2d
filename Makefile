.PHONY : mingw ej2d linux undefined

CFLAGS := -g -Wall -Ilib
LDFLAGS := 

SRC := \
lib/shader.c \
lib/lshader.c \
lib/ejoy2dgame.c \
lib/fault.c \
lib/screen.c \
lib/texture.c \
lib/ppm.c \
lib/spritepack.c \
lib/sprite.c \
lib/lsprite.c \
lib/matrix.c \
lib/lmatrix.c \
lib/dfont.c \
lib/label.c \
lib/particle.c

UNAME=$(shell uname)
SYS=$(if $(filter Linux%,$(UNAME)),linux,\
	    $(if $(filter MINGW%,$(UNAME)),mingw,\
	    $(if $(filter Darwin%,$(UNAME)),macos,\
	        undefined\
)))

all: $(SYS)

undefined:
	@echo "I can't guess your platform, please do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "      linux mingw macos"


mingw : TARGET := ej2d.exe
mingw : CFLAGS += -I/usr/include -I/usr/local/include
mingw : LDFLAGS += -L/usr/bin -lgdi32 -lglew32 -lopengl32 -L/usr/local/bin -llua52
mingw : SRC += mingw/window.c mingw/winfw.c mingw/winfont.c

mingw : $(SRC) ej2d

linux : TARGET := ej2d
linux : CFLAGS += -I/usr/include -I/usr/local/include $(shell freetype-config --cflags)
linux : LDFLAGS +=  -lGLEW -lGL -lX11 -lfreetype -llua -lm
linux : SRC += linux/window.c linux/winfw.c linux/winfont.c

linux : $(SRC) ej2d

macos : TARGET := ej2d
macos : CFLAGS += -I/usr/X11R6/include -I/usr/include -I/usr/local/include $(shell freetype-config --cflags)
macos : LDFLAGS += -L/usr/X11R6/lib  -lGLEW -lGL -lX11 -lfreetype -llua -lm
macos : SRC += mac/window.c mac/winfw.c mac/winfont.c

macos : $(SRC) ej2d

ej2d : 
	gcc $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean : 
	-rm -f ej2d.exe
	-rm -f ej2d
