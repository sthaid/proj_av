#ifndef __WORLD_H__
#define __WORLD_H__

#include <string>
#include "display.h"

using std::string;

class world {
public:
    static const int WORLD_WIDTH = 4096;
    static const int WORLD_HEIGHT = 4096;
    static const int MAX_GET_VIEW_XY = 1000;  // xxx private ?
    
    static void static_init();

    world(display &display);
    ~world();

    void place_object_init();
    void place_object(int x, int y, int w, int h, unsigned char * pixels);
    void draw(int pid, int center_x, int center_y, double zoom);

    void get_view(int x, int y, double dir, int w, int h, unsigned char * pixels);

    void clear();
    bool read(string filename);
    bool write(string filename);
    void set_static_pixel(int x, int y, unsigned char c);
    unsigned char get_static_pixel(int x, int y);
    unsigned char get_world_pixel(int x, int y);
    void cvt_coord_pixel_to_world(double pixel_x, double pixel_y, int &world_x, int &world_y);
    void cvt_coord_world_to_pixel(int world_x, int world_y, double &pixel_x, double &pixel_y);
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

    // last draw
    int center_x;
    int center_y;
    double zoom;
};

#endif
