#ifndef __WORLD_H__
#define __WORLD_H__

#include <string>
#include "display.h"

using std::string;

class world {
public:
    static const int WORLD_WIDTH = 4096;
    static const int WORLD_HEIGHT = 4096;
    static const int MAX_GET_VIEW_XY = 1000;
    
    static void static_init();

    world(display &display);
    ~world();
    void clear();
    bool read(string filename);
    bool write(string filename);

    void place_object_init();
    void place_object(double x, double y, int w, int h, unsigned char * pixels);
    void draw(int pid, double center_x, double center_y, double zoom);

    void get_view(double x, double y, double dir, int w, int h, unsigned char * pixels);

    void create_road_slice(double &x, double &y, double dir);
    void set_pixel(double x, double y, unsigned char c);
    unsigned char get_pixel(double x, double y);
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
    struct rect placed_object_list[1000];
    int max_placed_object_list;

    // get view 
    static short get_view_dx_tbl[360][MAX_GET_VIEW_XY][MAX_GET_VIEW_XY];
    static short get_view_dy_tbl[360][MAX_GET_VIEW_XY][MAX_GET_VIEW_XY];
};

#endif
