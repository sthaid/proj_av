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

unsigned char world::car_pixels[360][CAR_HEIGHT][CAR_WIDTH];
short world::get_view_dx_tbl[360][MAX_GET_VIEW_XY][MAX_GET_VIEW_XY];
short world::get_view_dy_tbl[360][MAX_GET_VIEW_XY][MAX_GET_VIEW_XY];  // xxx make these tables bigger

void world::static_init(void)
{
    //
    // init car_pixels ...
    //

    // create car at 0 degree rotation
    unsigned char (&car)[CAR_HEIGHT][CAR_WIDTH] = car_pixels[0];
    memset(car, display::TRANSPARENT, sizeof(car));
    for (int h = 5; h <= 11; h++) {
        for (int w = 1; w <= 15; w++) {
            car[h][w] = display::BLUE;
        }
    }
    car[5][15]  = display::WHITE;   // head lights
    car[6][15]  = display::WHITE;
    car[10][15] = display::WHITE;
    car[11][15] = display::WHITE;
    car[5][14]  = display::WHITE;
    car[6][14]  = display::WHITE;
    car[10][14] = display::WHITE;
    car[11][14] = display::WHITE;
    car[5][1]   = display::RED;     // tail lights
    car[6][1]   = display::RED;
    car[10][1]  = display::RED;
    car[11][1]  = display::RED;
    car[5][2]   = display::RED;
    car[6][2]   = display::RED;
    car[10][2]  = display::RED;
    car[11][2]  = display::RED;

    // create cars at 1 to 359 degrees rotation, 
    // using the car created above at 0 degrees as a template
    for (int dir = 1; dir <= 359; dir++) {
        double sin_dir = sin(dir *  M_PI/180.0);
        double cos_dir = cos(dir *  M_PI/180.0);
        unsigned char (&carprime)[CAR_HEIGHT][CAR_WIDTH] = car_pixels[dir];
        int x,y,xprime,yprime;

        #define OVSF 3  // Over Sample Factor
        memset(carprime, display::TRANSPARENT, sizeof(car));
        for (y = 0; y < CAR_HEIGHT*OVSF; y++) {
            for (x = 0; x < CAR_WIDTH*OVSF; x++) {
                xprime = (x-CAR_WIDTH*OVSF/2) * cos_dir - (y-CAR_HEIGHT*OVSF/2) * sin_dir + CAR_WIDTH*OVSF/2 + 0.001;
                yprime = (x-CAR_WIDTH*OVSF/2) * sin_dir + (y-CAR_HEIGHT*OVSF/2) * cos_dir + CAR_HEIGHT*OVSF/2 + 0.001;
                if (xprime < 0 || xprime >= CAR_WIDTH*OVSF || yprime < 0 || yprime >= CAR_HEIGHT*OVSF) {
                    continue;
                }
                carprime[yprime/OVSF][xprime/OVSF] = car[y/OVSF][x/OVSF];
            }
        }
    }

    //
    // init get_view rotation tables
    // 

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

// -----------------  CONSTRUCTOR / DESTRUCTOR  -------------------------------------

world::world(display &display, string fn) : d(display)
{
    static_pixels           = new unsigned char [WORLD_HEIGHT] [WORLD_WIDTH];
    memset(static_pixels, 0, WORLD_HEIGHT*WORLD_WIDTH);
    pixels                  = new unsigned char [WORLD_HEIGHT] [WORLD_WIDTH];
    memset(pixels, 0, WORLD_HEIGHT*WORLD_WIDTH);
    texture                 = NULL;
    memset(placed_car_list, 0, sizeof(placed_car_list));
    max_placed_car_list     = 0;
    filename                = "";
    read_ok_flag            = false;
    write_ok_flag           = false;

    filename = fn;
    read();
    if (!read_ok_flag) {
        clear();
    }
    assert(texture);
}

world::~world()
{
    d.texture_destroy(texture);
    delete [] static_pixels;
    delete [] pixels;
}

// -----------------  CAR SUPPORT  --------------------------------------------------

void world::place_car_init()
{
    for (int i = 0; i < max_placed_car_list; i++) {
        struct rect &rect = placed_car_list[i];

        // restores pixels from static_pixels
        for (int y = rect.y; y < rect.y+rect.h; y++) {
            memcpy(&pixels[y][rect.x], &static_pixels[y][rect.x], CAR_WIDTH);
        }

        // restores the texture
        d.texture_set_rect(texture, 
                           rect.x, rect.y, rect.w, rect.h, 
                           &pixels[rect.y][rect.x], 
                           WORLD_WIDTH);
    }
    max_placed_car_list = 0;
}

void world::place_car(double x_arg, double y_arg, double dir_arg)
{
    // convert dir to integer, and adjust so 0 degrees is up
    int dir = (dir_arg + 0.5);
    dir = ((dir + 270) % 360);
    if (dir < 0) dir += 360;

    // convert x,y to integer, and adjust to the top left corner of the car rect
    int x  = (x_arg + 0.5);
    int y  = (y_arg + 0.5);
    x -= CAR_WIDTH / 2;
    y -= CAR_HEIGHT / 2;

    // if car is off an edge of the world then skip
    if (x < 0 || x+CAR_WIDTH >= WORLD_WIDTH ||
        y < 0 || y+CAR_HEIGHT >= WORLD_HEIGHT) 
    {
        return;
    }

    // save the location of the car being placed on placed_car_list
    struct rect &rect = placed_car_list[max_placed_car_list];
    rect.x = x;
    rect.y = y;
    rect.w = CAR_WIDTH;
    rect.h = CAR_HEIGHT;
    max_placed_car_list++;

    // copy non transparent car pixels to pixels
    unsigned char * cp = reinterpret_cast<unsigned char *>(car_pixels[dir]);  //xxx cast
    for (y = rect.y; y < rect.y+rect.h; y++) {
        for (x = rect.x; x < rect.x+rect.w; x++) {
            if (*cp != display::TRANSPARENT) {
                pixels[y][x] = *cp;
            }
            cp++;
        }
    }

    // update the texture
    d.texture_set_rect(texture, 
                       rect.x, rect.y, rect.w, rect.h, 
                       &pixels[rect.y][rect.x], 
                       WORLD_WIDTH);
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

// -----------------  DRAW THE WORLD  -----------------------------------------------

void world::draw(int pid, double center_x, double center_y, double zoom)
{
    int w, h, x, y;

    w = WORLD_WIDTH / zoom;
    h = WORLD_HEIGHT / zoom;
    x = center_x - w/2;
    y = center_y - h/2;

    d.texture_draw(texture, x, y, w, h, pid);
}

// -----------------  EDIT STATIC PIXELS SUPPORT  -----------------------------------

void world::create_road_slice(double &x, double &y, double dir)
{
    double dpx, dpy, tmpx, tmpy;
    const double distance = 0.5;

    dir += 270;

    dpy = distance * sin((dir+90) * (M_PI/180.0));
    dpx = distance * cos((dir+90) * (M_PI/180.0));

    set_static_pixel(x,y,display::YELLOW);

    tmpx = x;
    tmpy = y;
    for (int i = 1; i <= 24; i++) {
        tmpx += dpx;
        tmpy += dpy;
        if (get_static_pixel(tmpx,tmpy) == display::GREEN) {
            set_static_pixel(tmpx,tmpy,display::BLACK);
        }
    }

    tmpx = x;
    tmpy = y;
    for (int i = 1; i <= 24; i++) {
        tmpx -= dpx;
        tmpy -= dpy;
        if (get_static_pixel(tmpx,tmpy) == display::GREEN) {
            set_static_pixel(tmpx,tmpy,display::BLACK);
        }
    }
}
        
void world::set_static_pixel(double x, double y, unsigned char p) 
{
    int ix = x + .5;
    int iy = y + .5;

    if (ix < 0 || ix >= WORLD_WIDTH || iy < 0 || iy >= WORLD_HEIGHT) {
        return;
    }

    static_pixels[iy][ix] = p;
    pixels[iy][ix] = p;
    d.texture_set_pixel(texture, ix, iy, p);
}

unsigned char world::get_static_pixel(double x, double y)
{
    int ix = x + .5;
    int iy = y + .5;

    if (ix < 0 || ix >= WORLD_WIDTH || iy < 0 || iy >= WORLD_HEIGHT) {
        ERROR("ix " << ix << " iy " << iy << endl);
        return display::GREEN;
    }

    return static_pixels[iy][ix];
}

void world::clear()
{
    memset(static_pixels, display::GREEN, WORLD_WIDTH*WORLD_HEIGHT); 
    memcpy(pixels, static_pixels, WORLD_WIDTH*WORLD_HEIGHT);
    d.texture_destroy(texture);
    texture = d.texture_create(reinterpret_cast<unsigned char *>(static_pixels), 
                                             WORLD_WIDTH, WORLD_HEIGHT);
}

void world::read()
{
    ifstream ifs;

    read_ok_flag = false;
    ifs.open(filename, ios::in|ios::ate|ios::binary);
    if (!ifs.is_open()) {
        ERROR(filename << " does not exist" << endl);
        return;
    }
    if (ifs.tellg() != WORLD_WIDTH*WORLD_HEIGHT) {
        ERROR(filename << " has incorrect size" << endl);
        return;
    }
    ifs.seekg(0,ios::beg);
    ifs.read(reinterpret_cast<char*>(static_pixels), WORLD_WIDTH*WORLD_HEIGHT); 
    if (!ifs.good()) {
        ERROR(filename << " read failed" << endl);
        return;
    }
    read_ok_flag = true;

    memcpy(pixels, static_pixels, WORLD_WIDTH*WORLD_HEIGHT);

    d.texture_destroy(texture);
    texture = d.texture_create(reinterpret_cast<unsigned char *>(static_pixels), 
                               WORLD_WIDTH, WORLD_HEIGHT);
}

void world::write()
{
    ofstream ofs;

    write_ok_flag = false;
    ofs.open(filename, ios::out|ios::binary|ios::trunc);
    if (!ofs.is_open()) {
        ERROR(filename << " create failed" << endl);
        return;
    }
    ofs.write(reinterpret_cast<char*>(static_pixels), WORLD_WIDTH*WORLD_HEIGHT);  
    if (!ofs.good()) {
        ERROR(filename << " write failed" << endl);
        return;
    }
    write_ok_flag = true;
}
