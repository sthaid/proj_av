TARGETS = av
H_FILES = display.h event_sound.h world.h logging.h utils.h
CREATE_OBJS = av.o display.o world.o utils.o

CC = g++
CPPFLAGS = -std=gnu++11 -Wall -g -O0 $(shell sdl2-config --cflags) 

#
# build rules
#

all: $(TARGETS)

av: $(CREATE_OBJS) 
	$(CC) -lSDL2 -lSDL2_ttf -lSDL2_mixer -lpng -o $@ $(CREATE_OBJS)

#
# clean rule
#

clean:
	rm -f $(TARGETS) $(CREATE_OBJS)

#
# dependencies
# XXX automate this
#

av.o: av.cpp $(H_FILES)
display.o: display.cpp $(H_FILES)
world.o: world.cpp $(H_FILES)
utils.o: utils.cpp $(H_FILES)
