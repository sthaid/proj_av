TARGETS  = av edw
H_FILES  = display.h event_sound.h world.h car.h autonomous_car.h logging.h utils.h
AV_OBJS  = av.o  display.o world.o utils.o car.o autonomous_car.o
EDW_OBJS = edw.o display.o world.o utils.o car.o 

CC = g++
CPPFLAGS = -std=gnu++11 -Wall -g -O2 $(shell sdl2-config --cflags) 

#
# build rules
#

all: $(TARGETS)

av: $(AV_OBJS) 
	$(CC) -lSDL2 -lSDL2_ttf -lSDL2_mixer -lpng -o $@ $(AV_OBJS)

edw: $(EDW_OBJS) 
	$(CC) -lSDL2 -lSDL2_ttf -lSDL2_mixer -lpng -o $@ $(EDW_OBJS)

#
# clean rule
#

clean:
	rm -f $(TARGETS) $(AV_OBJS) $(EDW_OBJS)

#
# dependencies
#

av.o: av.cpp $(H_FILES)
edw.o: edw.cpp $(H_FILES)
display.o: display.cpp $(H_FILES)
world.o: world.cpp $(H_FILES)
car.o: car.cpp $(H_FILES)
autonomous_car.o: autonomous_car.cpp $(H_FILES)
utils.o: utils.cpp $(H_FILES)
