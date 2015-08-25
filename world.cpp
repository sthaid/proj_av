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
    center_x                = 0;
    center_y                = 0;
    zoom                    = 0;

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

void world::place_object(int x, int y, int w, int h, unsigned char * p)
{
    // adjuct x,y to top left corner of the object's rect
    x -= w / 2;
    y -= h / 2;

    // if object is off an edge of the world then skip
    if (x < 0 || x+w >= WORLD_WIDTH ||
        y < 0 || y+h >= WORLD_HEIGHT) 
    {
        return;
    }

    // save the location of the object being placed on placed_object_list
    struct rect &rect = placed_object_list[max_placed_object_list];
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    max_placed_object_list++;

    // copy non transparent object pixels to pixels
    for (y = rect.y; y < rect.y+rect.h; y++) {
        for (x = rect.x; x < rect.x+rect.w; x++) {
            if (*p != display::TRANSPARENT) {
                pixels[y][x] = *p;
            }
            p++;
        }
    }

    // update the texture
    d.texture_set_rect(texture, 
                       rect.x, rect.y, rect.w, rect.h, 
                       &pixels[rect.y][rect.x], 
                       WORLD_WIDTH);
}

void world::draw(int pid, int center_x_arg, int center_y_arg, double zoom_arg)
{
    int w, h, x, y;

    w = WORLD_WIDTH / zoom_arg;
    h = WORLD_HEIGHT / zoom_arg;
    x = center_x_arg - w/2;
    y = center_y_arg - h/2;

    d.texture_draw(texture, x, y, w, h, pid);

    center_x = center_x_arg;
    center_y = center_y_arg;
    zoom     = zoom_arg;
}

// -----------------  GET VIEW OF THE WORLD  ----------------------------------------

void world::get_view(int x, int y, double dir, int W, int H, unsigned char * p)
{
    int d = sanitize_direction(dir + 0.5);
    assert(d >= 0 && d <= 359);

    for (int h = H-1; h >= 0; h--) {
        for (int w = -W/2; w < -W/2+W; w++) {
            int dx = get_view_dx_tbl[d][h][w+(MAX_GET_VIEW_XY/2)];
            int dy = get_view_dy_tbl[d][h][w+(MAX_GET_VIEW_XY/2)];
            if (y+dy >= 0 && y+dy < WORLD_HEIGHT && x+dx >= 0 && x+dx < WORLD_WIDTH) {
                *p++ = pixels[y+dy][x+dx];
            } else {
                *p++ = display::PURPLE;
            }
        }
    }
}

// -----------------  EDIT SUPPORT  -------------------------------------------------

void world::set_pixel(int x, int y, unsigned char p) 
{
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) {
        return;
    }

    static_pixels[y][x] = p;
    pixels[y][x] = p;
    d.texture_set_pixel(texture, x, y, p);
}

unsigned char world::get_pixel(int x, int y)
{
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) {
        return display::GREEN;
    }

    return static_pixels[y][x];
}

void world::cvt_coord_pixel_to_world(double pixel_x, double pixel_y, int &world_x, int &world_y)
{
    assert(zoom != 0);
    int  world_display_width  = WORLD_WIDTH / zoom;
    int  world_display_height = WORLD_HEIGHT / zoom;

    world_x = (center_x - world_display_width / 2) + (world_display_width * pixel_x);
    world_y = (center_y - world_display_height / 2) + (world_display_height * pixel_y);
}
