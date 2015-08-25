#include <fstream>
#include <cassert>
#include <memory.h>
#include <math.h>

#include "world.h"
#include "logging.h"
#include "utils.h"

using std::ifstream;
using std::ofstream;
using std::ios;

// -----------------  WORLD CLASS STATIC INITIALIZATION  ----------------------------

short world::get_view_dx_tbl[360][MAX_GET_VIEW_XY][MAX_GET_VIEW_XY];
short world::get_view_dy_tbl[360][MAX_GET_VIEW_XY][MAX_GET_VIEW_XY];  // xxx make these tables bigger

void world::static_init(void)
{
    // init get_view rotation tables
    int d1,h1,w1;
    INFO("sizeof of tables " << 
           (sizeof(get_view_dx_tbl) + sizeof(get_view_dy_tbl)) / 0x100000 << " MB" << endl);
    for (d1 = 0; d1 < 360; d1++) {
        double sindir = sin(d1 * (M_PI/180.0));
        double cosdir = cos(d1 * (M_PI/180.0));
        for (h1 = 0; h1 < MAX_GET_VIEW_XY; h1++) {
            for (w1 = -MAX_GET_VIEW_XY/2; w1 < -MAX_GET_VIEW_XY/2 + MAX_GET_VIEW_XY; w1++) {
                get_view_dx_tbl[d1][h1][w1+(MAX_GET_VIEW_XY/2)] = w1 * cosdir + h1 * sindir;
                get_view_dy_tbl[d1][h1][w1+(MAX_GET_VIEW_XY/2)] = w1 * sindir - h1 * cosdir;
            }
        }
    }
}

// -----------------  CONSTRUCTOR / DESTRUCTOR AND RELATED FUNCTIONS  ---------------

world::world(display &display) : d(display)
{
    static_pixels           = new unsigned char [WORLD_HEIGHT] [WORLD_WIDTH];
    memset(static_pixels, 0, WORLD_HEIGHT*WORLD_WIDTH);
    pixels                  = new unsigned char [WORLD_HEIGHT] [WORLD_WIDTH];
    memset(pixels, 0, WORLD_HEIGHT*WORLD_WIDTH);
    texture                 = NULL;
    memset(placed_object_list, 0, sizeof(placed_object_list));
    max_placed_object_list  = 0;

    clear();
}

world::~world()
{
    d.texture_destroy(texture);
    delete [] static_pixels;
    delete [] pixels;
}

void world::clear()
{
    memset(static_pixels, display::GREEN, WORLD_WIDTH*WORLD_HEIGHT); 
    memcpy(pixels, static_pixels, WORLD_WIDTH*WORLD_HEIGHT);
    d.texture_destroy(texture);
    texture = d.texture_create(reinterpret_cast<unsigned char *>(static_pixels), WORLD_WIDTH, WORLD_HEIGHT);
}

bool world::read(string filename)
{
    ifstream ifs;

    ifs.open(filename, ios::in|ios::ate|ios::binary);
    if (!ifs.is_open()) {
        ERROR(filename << " does not exist" << endl);
        return false;
    }
    if (ifs.tellg() != WORLD_WIDTH*WORLD_HEIGHT) {
        ERROR(filename << " has incorrect size" << endl);
        return false;
    }
    ifs.seekg(0,ios::beg);
    ifs.read(reinterpret_cast<char*>(static_pixels), WORLD_WIDTH*WORLD_HEIGHT); 
    if (!ifs.good()) {
        ERROR(filename << " read failed" << endl);
        return false;
    }

    memcpy(pixels, static_pixels, WORLD_WIDTH*WORLD_HEIGHT);
    d.texture_destroy(texture);
    texture = d.texture_create(reinterpret_cast<unsigned char *>(static_pixels), WORLD_WIDTH, WORLD_HEIGHT);

    return true;
}

bool world::write(string filename)
{
    ofstream ofs;

    ofs.open(filename, ios::out|ios::binary|ios::trunc);
    if (!ofs.is_open()) {
        ERROR(filename << " create failed" << endl);
        return false;
    }
    ofs.write(reinterpret_cast<char*>(static_pixels), WORLD_WIDTH*WORLD_HEIGHT);  
    if (!ofs.good()) {
        ERROR(filename << " write failed" << endl);
        return false;
    }

    return true;
}

// -----------------  DRAW WORLD AND WORLD OBJECTS  ---------------------------------

void world::place_object_init()
{
    for (int i = 0; i < max_placed_object_list; i++) {
        struct rect &rect = placed_object_list[i];

        // restores pixels from static_pixels
        for (int y = rect.y; y < rect.y+rect.h; y++) {
            memcpy(&pixels[y][rect.x], &static_pixels[y][rect.x], rect.w);
        }

        // restores the texture
        d.texture_set_rect(texture, 
                           rect.x, rect.y, rect.w, rect.h, 
                           &pixels[rect.y][rect.x], 
                           WORLD_WIDTH);
    }
    max_placed_object_list = 0;
}

void world::place_object(double x_arg, double y_arg, int w_arg, int h_arg, unsigned char * pixels_arg)
{
    // convert x,y to integer, and adjust to the top left corner of the object rect
    // xxx this could be more accurate, but need to retest car display in 4 totations
    int x  = (x_arg + 0.5);
    int y  = (y_arg + 0.5);
    x -= w_arg / 2;
    y -= h_arg / 2;

    // if object is off an edge of the world then skip
    if (x < 0 || x+w_arg >= WORLD_WIDTH ||
        y < 0 || y+h_arg >= WORLD_HEIGHT) 
    {
        return;
    }

    // save the location of the object being placed on placed_object_list
    struct rect &rect = placed_object_list[max_placed_object_list];
    rect.x = x;
    rect.y = y;
    rect.w = w_arg;
    rect.h = h_arg;
    max_placed_object_list++;

    // copy non transparent object pixels to pixels
    for (y = rect.y; y < rect.y+rect.h; y++) {
        for (x = rect.x; x < rect.x+rect.w; x++) {
            if (*pixels_arg != display::TRANSPARENT) {
                pixels[y][x] = *pixels_arg;
            }
            pixels_arg++;
        }
    }

    // update the texture
    d.texture_set_rect(texture, 
                       rect.x, rect.y, rect.w, rect.h, 
                       &pixels[rect.y][rect.x], 
                       WORLD_WIDTH);
}

void world::draw(int pid, double center_x, double center_y, double zoom)
{
    int w, h, x, y;

    w = WORLD_WIDTH / zoom;
    h = WORLD_HEIGHT / zoom;
    x = (int)center_x - w/2;
    y = (int)center_y - h/2;

    d.texture_draw(texture, x, y, w, h, pid);
}

// -----------------  GET VIEW OF THE WORLD  ----------------------------------------

void world::get_view(double x_arg, double y_arg, double dir_arg, int w_arg, int h_arg, unsigned char * p_arg)
{
    int x = x_arg + 0.5;
    int y = y_arg + 0.5;
    int d = dir_arg + 0.5;

    if (d == 360) d = 0;  // xxx

    assert(d >= 0 && d <= 359);

    for (int h = h_arg-1; h >= 0; h--) {
        for (int w = -w_arg/2; w < -w_arg/2+w_arg; w++) {
            int dx = get_view_dx_tbl[d][h][w+(MAX_GET_VIEW_XY/2)];
            int dy = get_view_dy_tbl[d][h][w+(MAX_GET_VIEW_XY/2)];
            if (y+dy >= 0 && y+dy < WORLD_HEIGHT && x+dx >= 0 && x+dx < WORLD_WIDTH) {
                *p_arg++ = pixels[y+dy][x+dx];
            } else {
                *p_arg++ = display::PURPLE;
            }
        }
    }
}

// -----------------  EDIT SUPPORT  -------------------------------------------------

void world::create_road_slice(double &x, double &y, double dir)
{
    double dpx, dpy, tmpx, tmpy;
    const double distance = 0.5;

    dir += 270;

    dpy = distance * sin((dir+90) * (M_PI/180.0));
    dpx = distance * cos((dir+90) * (M_PI/180.0));

    set_pixel(x,y,display::YELLOW);

    tmpx = x;
    tmpy = y;
    for (int i = 1; i <= 24; i++) {
        tmpx += dpx;
        tmpy += dpy;
        if (get_pixel(tmpx,tmpy) == display::GREEN) {
            set_pixel(tmpx,tmpy,display::BLACK);
        }
    }

    tmpx = x;
    tmpy = y;
    for (int i = 1; i <= 24; i++) {
        tmpx -= dpx;
        tmpy -= dpy;
        if (get_pixel(tmpx,tmpy) == display::GREEN) {
            set_pixel(tmpx,tmpy,display::BLACK);
        }
    }
}
        
void world::set_pixel(double x, double y, unsigned char p) 
{
    int ix = x; // + .5;
    int iy = y; // + .5;

    if (ix < 0 || ix >= WORLD_WIDTH || iy < 0 || iy >= WORLD_HEIGHT) {
        return;
    }

    static_pixels[iy][ix] = p;
    pixels[iy][ix] = p;
    d.texture_set_pixel(texture, ix, iy, p);
}

unsigned char world::get_pixel(double x, double y)
{
    int ix = x; //  + .5;
    int iy = y; //  + .5;

    if (ix < 0 || ix >= WORLD_WIDTH || iy < 0 || iy >= WORLD_HEIGHT) {
        ERROR("ix " << ix << " iy " << iy << endl);
        return display::GREEN;
    }

    return static_pixels[iy][ix];
}

