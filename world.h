/*
Copyright (c) 2015 Steven Haid

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __WORLD_H__
#define __WORLD_H__

#include <string>
#include "display.h"

using std::string;

class world {
public:
    static const int WORLD_WIDTH = 4096;
    static const int WORLD_HEIGHT = 4096;
    
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
    static const int MAX_GET_VIEW_XY = 500;
    static short get_view_dx_tbl[360][MAX_GET_VIEW_XY][MAX_GET_VIEW_XY];
    static short get_view_dy_tbl[360][MAX_GET_VIEW_XY][MAX_GET_VIEW_XY];

    // last draw
    int center_x;
    int center_y;
    double zoom;
};

#endif
