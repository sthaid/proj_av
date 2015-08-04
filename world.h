#ifndef __WORLD_H__
#define __WORLD_H__

#include <string>
#include "display.h"

class world {
public:
    static const int WIDTH = 4096;
    static const int HEIGHT = 4096;

    struct location { // xxx private ?
        unsigned char c[HEIGHT][WIDTH];
    };

    world(display &display, std::string filename="");
    ~world();

    void read(std::string filename);
    void write(std::string filename);
    bool read_ok() { return read_ok_flag; }
    bool write_ok() { return write_ok_flag; }

    void draw(int pid, double center_x, double center_y, double zoom);

    void create_road_slice(double &x, double &y, double dir);

    void set_location(double x, double y, unsigned char c);
    unsigned char get_location(double x, double y);
private:
    display &d;
    struct location *location;
    display::texture *t;
    bool read_ok_flag;
    bool write_ok_flag;
};

#endif
