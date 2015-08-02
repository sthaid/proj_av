#ifndef __WORLD_H__
#define __WORLD_H__

#include <string>
#include "display.h"

class world {
public:
    static const int WIDTH = 4096;
    static const int HEIGHT = 4096;

    struct grid { // xxx private ?
        unsigned char v[WIDTH][HEIGHT];
    };

    world();
    world(std::string filename);
    ~world();

    //get_pixels
    //set_pixels

    int read(std::string filename);
    int write(std::string filename);

    void draw(display &d, int pid, double center_x, double center_y, double zoom);
private:
    struct grid * grid;
    display::texture * texture;
};

#endif
