#include <fstream>
#include <memory.h>

#include "world.h"
#include "logging.h"
#include "utils.h"

using std::ifstream;
using std::ofstream;
using std::ios;

world::world(display &display, std::string filename) : d(display)
{
    grid = new struct grid;
    t = NULL;
    read_ok_flag = false;
    write_ok_flag = false;

    memset(grid->v,display::GREEN,sizeof(grid->v));

    // XXX test
    for (int i = 500; i < 525; i++) {
        for (int j = 500; j < 525; j++) {
            grid->v[i][j] = display::BLACK;
        }
    }

    if (filename != "") {
        read(filename);
    }

    if (!read_ok_flag) {
        t = d.create_texture(reinterpret_cast<unsigned char *>(grid->v), WIDTH, HEIGHT);
    }

    if (t == NULL) {
        ERROR("create texture failed" << endl);
    }
}

world::~world()
{
    d.destroy_texture(t);
    delete grid;
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
    if (ifs.tellg() != sizeof(struct grid)) {
        ERROR(filename << " has incorrect size" << endl);
        return;
    }
    ifs.seekg(0,ios::beg);
    ifs.read(reinterpret_cast<char*>(grid), sizeof(struct grid));
    if (!ifs.good()) {
        ERROR(filename << " read failed" << endl);
        return;
    }
    read_ok_flag = true;

    d.destroy_texture(t);
    t = d.create_texture(reinterpret_cast<unsigned char *>(grid->v), WIDTH, HEIGHT);
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
    ofs.write(reinterpret_cast<char*>(grid), sizeof(struct grid));
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
    d.draw_texture(t, x, y, w, h, pid);
}
