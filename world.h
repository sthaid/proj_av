#ifndef __WORLD_H__
#define __WORLD_H__

#include <string>
#include "display.h"

using std::string;

class world {
public:
    static const int WORLD_WIDTH = 4096;
    static const int WORLD_HEIGHT = 4096;

    world(display &display, string filename);
    ~world();

    void place_car_init();
    void place_car(double x, double y, double dir);

    void draw(int pid, double center_x, double center_y, double zoom);

    void create_road_slice(double &x, double &y, double dir);
    void clear();
    void read();
    void write();
    bool read_ok() { return read_ok_flag; }
    bool write_ok() { return write_ok_flag; }
private:
    // display
    display &d;

    // world data
    struct rect {
        int x,y,w,h;
    };
    unsigned char (*static_pixels)[WORLD_WIDTH];
    unsigned char (*pixels)[WORLD_WIDTH];
    display::texture *texture;
    struct rect placed_car_list[1000];
    int max_placed_car_list;

    // car pixels
    static const int CAR_WIDTH = 17;
    static const int CAR_HEIGHT = 17;
    unsigned char car_pixels[360][CAR_HEIGHT][CAR_WIDTH];
    void init_car_pixels();

    // edit static pixels support
    void set_static_pixel(double x, double y, unsigned char c);
    unsigned char get_static_pixel(double x, double y);

    // read / write static_pixels
    string filename;
    bool read_ok_flag;
    bool write_ok_flag;
};

#endif
