#include <fstream>
#include <memory.h>
#include <math.h>

#include "world.h"
#include "logging.h"
#include "utils.h"

using std::ifstream;
using std::ofstream;
using std::ios;

world::world(display &display, std::string filename) : d(display)
{
    location = new struct location;
    t = NULL;
    read_ok_flag = false;
    write_ok_flag = false;

    memset(location->c, display::GREEN, sizeof(location->c));

#if 0 // xxx test
    for (int i = 500; i < 525; i++) {
        for (int j = 500; j < 525; j++) {
            location->c[i][j] = display::BLACK;
        }
    }
#endif

    if (filename != "") {
        read(filename);
    }

    if (!read_ok_flag) {
        t = d.texture_create(reinterpret_cast<unsigned char *>(location->c), WIDTH, HEIGHT);
    }

    if (t == NULL) {
        ERROR("create texture failed" << endl);
    }
}

world::~world()
{
    d.texture_destroy(t);
    delete location;
}

void world::read(std::string filename)
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

void world::write(std::string filename)
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

    w = WIDTH / zoom;
    h = HEIGHT / zoom;
    x = center_x - w/2;
    y = center_y - h/2;
    d.texture_draw(t, x, y, w, h, pid);
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
    d.texture_modify_pixel(t, ix, iy, static_cast<enum display::color>(c));
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
