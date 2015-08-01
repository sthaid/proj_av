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

#define WORLD_WIDTH     1024
#define WORLD_HEIGHT    1024
#define DISPLAY_WIDTH   (512 + 38 + 250)
#define DISPLAY_HEIGHT  512

int main(int argc, char **argv)
{
    unsigned char pixels[WORLD_WIDTH][WORLD_HEIGHT];
    string        filename = "world.dat";
    int           zoom = 1;
    int           center_x = WORLD_WIDTH / 2;
    int           center_y = WORLD_HEIGHT / 2;

    // init pixels to green
    memset(pixels, display::GREEN, sizeof(pixels));
    for (int i = 500; i < 510; i++) {
        for (int j = 500; j < 510; j++) {
            pixels[i][j] = display::RED;
        }
    }

    // read pixels from filename
    ifstream ifs;
    ifs.open(filename, ios::in|ios::ate|ios::binary);
    if (ifs.is_open()) {
        // check size, if incorrect then exit
        if (ifs.tellg() != sizeof(pixels)) {
            FATAL("file " << filename << " has incorrect size" << endl);
        }

        // read pixels
        ifs.seekg(0,ios::beg);
        ifs.read(reinterpret_cast<char*>(pixels), sizeof(pixels));
        if (ifs.gcount() != sizeof(pixels)) {
            FATAL("file " << filename << " read faild" << endl);
        }

        // close file
        ifs.close();
    }
    ifs.close();

    // create the display
    // XXX option for resizing?
    display d(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // create texture from the pixels
    struct display::texture * texture = d.create_texture(reinterpret_cast<unsigned char *>(pixels), WORLD_WIDTH, WORLD_HEIGHT);

    // loop
    while (true) {
        // start
        d.start(0, 0, 512, 512,     // pane 0
                550, 0, 250, 512);  // pane 1

        // draw the world
        int w, h, x, y;
        w = 2 * WORLD_WIDTH / zoom;
        h = 2 * WORLD_HEIGHT / zoom;
        x = center_x - w/2;
        y = center_y - h/2;
        d.draw_texture(texture, x, y, w, h, 0);

        // register for events
        int eid_quit      = d.event_register(display::ET_QUIT);
        int eid_pan       = d.event_register(display::ET_MOUSE_MOTION, 0);
        int eid_write     = d.draw_text("WRITE",    1, 0, 1, true);      // r,c,pid,event
        int eid_zoom_in   = d.draw_text("ZOOM_IN",  3, 0, 1, true);
        int eid_zoom_out  = d.draw_text("ZOOM_OUT", 5, 0, 1, true);
        int eid_draw_road = d.draw_text("DRAW_RD",  7, 0, 1, true);

        // finish
        d.finish();

        // handle events
        struct display::event event = d.poll_event();
        if (event.eid == eid_quit) {
            INFO("QUIT" << endl);
            break;
        } else if (event.eid == eid_pan) {
            center_x -= event.val1;
            center_y -= event.val2;
            INFO("PAN " << center_x << " " << center_y << endl);
        } else if (event.eid == eid_write) {
            INFO("WRITE" << endl);
            //ofstream ofs;
            //ofs.open(filename, ios::out|ios::binary);
            //if (!ofs.is_open()) {
                // display_mesage
            //}
        } else if (event.eid == eid_zoom_in) {
            if (zoom < 256) {
                zoom *= 2;
            }
            INFO("ZOOM IN " << zoom << endl);
        } else if (event.eid == eid_zoom_out) {
            if (zoom > 1) {
                zoom /= 2;
            }
            INFO("ZOOM OUT " << zoom << endl);
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
