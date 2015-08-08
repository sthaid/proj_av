#ifndef __WORLD_H__
#define __WORLD_H__

#include <string>
#include "display.h"

using std::string;

class world {
public:
    static const int WIDTH = 4096;
    static const int HEIGHT = 4096;

    world(display &display, string filename);
    ~world();

    void clear();

    void read();
    void write();
    bool read_ok() { return read_ok_flag; }
    bool write_ok() { return write_ok_flag; }

    void draw(int pid, double center_x, double center_y, double zoom);

//XXX draw cars

    void create_road_slice(double &x, double &y, double dir);

private:
    // XXX  work this
    struct location { // xxx private ?
        unsigned char c[HEIGHT][WIDTH];
    };

    display &d;
    struct location *location;
    display::texture *t;
    display::texture *t_ovl;
    bool read_ok_flag;
    bool write_ok_flag;
    string filename;

    void set_location(double x, double y, unsigned char c);
    unsigned char get_location(double x, double y);
};

#endif
