#include <fstream>
#include <memory.h>

#include "display.h"
#include "logging.h"
#include "utils.h"

using std::ifstream;
using std::ofstream;
using std::ios;

void world::world()
{
    grid = new struct grid;
    memset(grid->v,0,sizeof(grid->v));


    memset(pixels, display::GREEN, sizeof(pixels));
    for (int i = 500; i < 525; i++) {
        for (int j = 500; j < 525; j++) {
            pixels[i][j] = display::RED;
        }
    }

}

void world::~world()
{
    delete grid;
    destroy texture;
}

bool world::read(std::string filename)
{
    ifstream ifs;

    ifs.open(filename, ios::in|ios::ate|ios::binary);
    if (!ifs.is_open() {
        ERROR(filename << " does not exist" << endl);
        return false;
    }
        
    if (ifs.tellg() != sizeof(pixels)) {
        ERROR(filename << " has incorrect size" << endl);
        return false;
    }

    ifs.seekg(0,ios::beg);
    ifs.read(reinterpret_cast<char*>(pixels), sizeof(pixels));
    if (ifs.gcount() != sizeof(pixels)) {
        ERROR(filename << " read failed" << endl);
        return false;
    }

    ifs.close();

    texture = d.create_texture(reinterpret_cast<unsigned char *>(grid.v),
                               WORLD_WIDTH, WORLD_HEIGHT);

    INFO(filename << " read successful" << endl);
    return true;
}

bool world::write(std::string filename)
{
    ofstream ofs;

    ofs.open(filename, ios::out|ios::binary|ios::trunc);
    if (!ofs.is_open()) {
        ERROR(filename << " create failed" << endl);
        return false;
    }

    ofs.write(reinterpret_cast<char*>(pixels), sizeof(pixels));
    if (ifs.gcount() != sizeof(pixels)) {
        ERROR(filename << " write failed" << endl);
        return false;
    }

    ofs.close();

    INFO(filename << " write successful" << endl);
    return true;
}

void world::draw(display &d, double center_x, double center_y, double zoom)
{
    int w, h, x, y;

    w = WORLD_WIDTH / zoom;
    h = WORLD_HEIGHT / zoom;
    x = center_x - w/2;
    y = center_y - h/2;
    d.draw_texture(texture, x, y, w, h, 0);
}
