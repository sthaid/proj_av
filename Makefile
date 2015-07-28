TARGETS = av

CC = g++
CPPFLAGS = -std=gnu++11 -Wall -g -O0 $(shell sdl2-config --cflags) 

OBJS = main.o display.o

#
# build rules
#

all: $(TARGETS)

av: $(OBJS) 
	$(CC) -pthread -lSDL2 -lSDL2_ttf -lSDL2_mixer -lrt -lm -lpng -o $@ $(OBJS)

#
# clean rule
#

clean:
	rm -f $(TARGETS) $(OBJS)

#
# compile rules
#

main.o: main.cpp display.h
display.o: display.cpp display.h
