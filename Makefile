TARGETS = create
H_FILES = display.h event_sound.h logging.h util.h
CREATE_OBJS = create.o display.o utils.o

CC = g++
CPPFLAGS = -std=gnu++11 -Wall -g -O0 $(shell sdl2-config --cflags) 

#
# build rules
#

all: $(TARGETS)

create: $(CREATE_OBJS) 
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

create.o: create.cpp $(H_FILES)
display.o: display.cpp $(H_FILES)
util.o: util.cpp $(H_FILES)
