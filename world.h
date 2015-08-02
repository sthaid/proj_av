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

    world(display &display, std::string filename="");
    ~world();

    void read(std::string filename);
    void write(std::string filename);
    bool read_ok() { return read_ok_flag; }
    bool write_ok() { return write_ok_flag; }

    void draw(int pid, double center_x, double center_y, double zoom);

    struct grid * get_grid() { return grid; }
    // XXX set_grid

private:
    display &d;
    struct grid *grid;
    display::texture *t;
    bool read_ok_flag;
    bool write_ok_flag;
};

#endif
