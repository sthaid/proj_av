TARGETS = av
H_FILES = display.h event_sound.h world.h car.h logging.h utils.h
OBJS = av.o display.o world.o car.o fixed_control_car.o utils.o

CC = g++
CPPFLAGS = -std=gnu++11 -Wall -g -O0 $(shell sdl2-config --cflags) 

#
# build rules
#

all: $(TARGETS)

av: $(OBJS) 
	$(CC) -lSDL2 -lSDL2_ttf -lSDL2_mixer -lpng -o $@ $(OBJS)

#
# clean rule
#

clean:
	rm -f $(TARGETS) $(OBJS)

#
# dependencies
# XXX automate this
#

av.o: av.cpp $(H_FILES)
display.o: display.cpp $(H_FILES)
world.o: world.cpp $(H_FILES)
car.o: car.cpp $(H_FILES)
fixed_control_car.o: fixed_control_car.cpp $(H_FILES)
utils.o: utils.cpp $(H_FILES)
