#include <iostream>
#include <fstream>
#include <string>
#include <memory.h>

#include "display.h"
#include "logging.h"
#include "utils.h"

using std::ifstream;
using std::ofstream;
using std::ios;
using std::string;

#define WORLD_WIDTH     4096
#define WORLD_HEIGHT    4096
#define DISPLAY_WIDTH   (512 + 38 + 250)
#define DISPLAY_HEIGHT  512

const double MAX_ZOOM = 31.99;
const double MIN_ZOOM = 0.51;
const double ZOOM_FACTOR = 1.1892071;

unsigned char pixels[WORLD_WIDTH][WORLD_HEIGHT];

// -----------------  MAIN  ------------------------------------------------------------------------

int main(int argc, char **argv)
{
    string        filename = "world.dat";
    double        zoom = 0.5;
    double        center_x = WORLD_WIDTH / 2;
    double        center_y = WORLD_HEIGHT / 2;

    // init pixels to green
    memset(pixels, display::GREEN, sizeof(pixels));
    for (int i = 500; i < 525; i++) {
        for (int j = 500; j < 525; j++) {
            pixels[i][j] = display::RED;
        }
    }

    // read pixels from filename
    ifstream ifs;
    ifs.open(filename, ios::in|ios::ate|ios::binary);
    if (ifs.is_open()) {
        if (ifs.tellg() != sizeof(pixels)) {
            FATAL(filename << " has incorrect size" << endl);
        }
        ifs.seekg(0,ios::beg);
        ifs.read(reinterpret_cast<char*>(pixels), sizeof(pixels));
        if (ifs.gcount() != sizeof(pixels)) {
            FATAL(filename << " read failed" << endl);
        }
        ifs.close();
        INFO(filename << " read" << endl);
    }
    ifs.close();

    // create the display
    // XXX option for resizing?
    display d(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // create texture from the pixels
    struct display::texture * texture = d.create_texture(reinterpret_cast<unsigned char *>(pixels), 
                                                         WORLD_WIDTH, WORLD_HEIGHT);

    // loop
    while (true) {
        // start
        d.start(0, 0, 512, 512,     // pane 0
                550, 0, 250, 512);  // pane 1

        // draw the world
        int w, h, x, y;
        w = WORLD_WIDTH / zoom;
        h = WORLD_HEIGHT / zoom;
        x = center_x - w/2;
        y = center_y - h/2;
        d.draw_texture(texture, x, y, w, h, 0);

        // register for events
        int eid_quit      = d.event_register(display::ET_QUIT);
        int eid_pan       = d.event_register(display::ET_MOUSE_MOTION, 0);
        int eid_write     = d.draw_text("WRITE",    1, 0, 1, true);      // r,c,pid,event
        int eid_draw_road = d.draw_text("DRAW_RD",  7, 0, 1, true);
        int eid_zoom      = d.event_register(display::ET_MOUSE_WHEEL, 0);

        // finish
        d.finish();

        // handle events
        struct display::event event = d.poll_event();
        if (event.eid == eid_quit) {
            break;
        } else if (event.eid == eid_pan) {
            center_x -= (double)event.val1 * 16 / zoom;
            center_y -= (double)event.val2 * 16 / zoom;
        } else if (event.eid == eid_write) {
            INFO("WRITE" << endl);
            ofstream ofs;
            ofs.open(filename, ios::out|ios::binary|ios::trunc);
            if (!ofs.is_open()) {
                ERROR(filename << " create failed" << endl);
            } else {
                ofs.write(reinterpret_cast<char*>(pixels), sizeof(pixels));
                if (ifs.gcount() != sizeof(pixels)) {
                    ERROR(filename << " write failed" << endl);
                }
                ofs.close();
                INFO(filename << " written" << endl);
            }
        } else if (event.eid == eid_zoom) {
            if (event.val2 > 0) {
                if (zoom > MIN_ZOOM) {
                    zoom /= ZOOM_FACTOR;
                }
            }
            if (event.val2 < 0) {
                if (zoom < MAX_ZOOM) {
                    zoom *= ZOOM_FACTOR;
                }
            }
            // xxx INFO("ZOOM IS " << zoom << endl);
        } else if (event.eid == eid_draw_road) {
            // d.update_teture
            // and update the pixels too
            INFO("DRAW " << endl);
        } else if (event.eid == display::EID_NONE) {
            ; 
        } else {
            FATAL("invalid eid " << event.eid << endl);
        }

        // delay 
        microsec_sleep(10000);
    }

    // exit
    return 0;
}
