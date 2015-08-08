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

world::world(display &display, string fn) : d(display)
{
    location = new struct location;
    t = NULL;  // xxx name
    t_ovl = NULL;
    read_ok_flag = false;
    write_ok_flag = false;
    filename = fn;

    read();
    if (!read_ok_flag) {
        clear();
    }
    assert(t);

    t_ovl = d.texture_create(WIDTH, HEIGHT);
    assert(t_ovl);
    INFO("t_ovl " << t_ovl);
}

world::~world()
{
    INFO("desctructor" << endl);
    d.texture_destroy(t);
    delete location;
}

void world::clear()
{
    memset(location->c, display::GREEN, sizeof(location->c));
    d.texture_destroy(t);
    t = d.texture_create(reinterpret_cast<unsigned char *>(location->c), WIDTH, HEIGHT);
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
    if (ifs.tellg() != sizeof(struct location)) {
        ERROR(filename << " has incorrect size" << endl);
        return;
    }
    ifs.seekg(0,ios::beg);
    ifs.read(reinterpret_cast<char*>(location), sizeof(struct location));
    if (!ifs.good()) {
        ERROR(filename << " read failed" << endl);
        return;
    }
    read_ok_flag = true;

    d.texture_destroy(t);
    t = d.texture_create(reinterpret_cast<unsigned char *>(location->c), WIDTH, HEIGHT);
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
    ofs.write(reinterpret_cast<char*>(location), sizeof(struct location));
    if (!ofs.good()) {
        ERROR(filename << " write failed" << endl);
        return;
    }
    write_ok_flag = true;
}

void world::draw(int pid, double center_x, double center_y, double zoom)
{
    int w, h, x, y;

    // XXX temp
#if 0
    static int count;
    unsigned int pixels[6][15];
    memset(pixels, 0, sizeof(pixels));
    for (int i = 0; i < 200; i++) {
        d.texture_set_rect(t_ovl, (20*i)%4000 + count, (20*i)%4000, 15, 6, reinterpret_cast<unsigned char *>(pixels));
    }

    count++;
    if (count > 3000) count = 0;

    memset(pixels, 255, sizeof(pixels));
    pixels[3][7] = 0;
    pixels[3][8] = 0;
    pixels[3][9] = 0;
    pixels[3][10] = 0;
    for (int i = 0; i < 200; i++) {
        d.texture_set_rect(t_ovl, (20*i)%4000 + count, (20*i)%4000, 15, 6, reinterpret_cast<unsigned char *>(pixels));
    }
#else
    static int count;
    unsigned char pixels[6][15];

    for (int i = 0; i < 200; i++) {
        d.texture_clr_rect(t_ovl, (20*i)%4000 + count, (20*i)%4000, 15, 6);
    }

    count++;
    if (count > 3000) count = 0;

    memset(pixels, display::ORANGE, sizeof(pixels));
    pixels[3][7] = display::TRANSPARENT;
    pixels[3][8] = display::TRANSPARENT;
    pixels[3][9] = display::TRANSPARENT;
    pixels[3][10] = display::TRANSPARENT;
    for (int i = 0; i < 200; i++) {
        d.texture_set_rect(t_ovl, (20*i)%4000 + count, (20*i)%4000, 15, 6, reinterpret_cast<unsigned char *>(pixels));
    }
#endif



    w = WIDTH / zoom;
    h = HEIGHT / zoom;
    x = center_x - w/2;
    y = center_y - h/2;
    d.texture_draw(t, x, y, w, h, pid);
    d.texture_draw(t_ovl, x, y, w, h, pid);
}

void world::create_road_slice(double &x, double &y, double dir)
{
    double dx, dy, dpx, dpy, tmpx, tmpy;

    dy  = .5 * sin(dir * (M_PI/180.0));
    dx  = .5 * cos(dir * (M_PI/180.0));
    dpy = .5 * sin((dir+90) * (M_PI/180.0));
    dpx = .5 * cos((dir+90) * (M_PI/180.0));

    set_location(x,y,display::YELLOW);

    tmpx = x;
    tmpy = y;
    for (int i = 1; i <= 24; i++) {
        tmpx += dpx;
        tmpy += dpy;
        if (get_location(tmpx,tmpy) == display::GREEN) {
            set_location(tmpx,tmpy,display::BLACK);
        }
    }

    tmpx = x;
    tmpy = y;
    for (int i = 1; i <= 24; i++) {
        tmpx -= dpx;
        tmpy -= dpy;
        if (get_location(tmpx,tmpy) == display::GREEN) {
            set_location(tmpx,tmpy,display::BLACK);
        }
    }

    x += dx;
    y += dy;
}
        
// XXX add integer versions
void world::set_location(double x, double y, unsigned char c) 
{
    int ix = x + .5;
    int iy = y + .5;

    if (ix < 0 || ix >= WIDTH || iy < 0 || iy >= HEIGHT) {
        return;
    }

    location->c[iy][ix] = c;
    d.texture_set_pixel(t, ix, iy, static_cast<enum display::color>(c));
}

unsigned char world::get_location(double x, double y)
{
    int ix = x + .5;
    int iy = y + .5;

    if (ix < 0 || ix >= WIDTH || iy < 0 || iy >= HEIGHT) {
        ERROR("ix " << ix << " iy " << iy << endl);
        return display::GREEN;
    }

    return location->c[iy][ix];
}
