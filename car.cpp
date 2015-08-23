#include <sstream>
#include <cassert>
#include <math.h>
#include <memory.h>

#include "car.h"
#include "logging.h"
#include "utils.h"

using std::ostringstream;

unsigned char car::car_pixels[360][CAR_PIXELS_HEIGHT][CAR_PIXELS_WIDTH];
display::texture *car::texture;

// -----------------  CAR CLASS STATIC INITIALIZATION  ------------------------------

void car::static_init(display &d)
{
    //
    // xxx comment
    // xxx should be destroyed 
    //

    texture = d.texture_create(MAX_FRONT_VIEW_XY, MAX_FRONT_VIEW_XY);
    assert(texture);

    //
    // init car_pixels ...
    //

    // create car at 0 degree rotation
    unsigned char (&car)[CAR_PIXELS_HEIGHT][CAR_PIXELS_WIDTH] = car_pixels[0];
    memset(car, display::TRANSPARENT, sizeof(car));
    for (int h = 1; h <= 15; h++) {
        for (int w = 5; w <= 11; w++) {
            car[h][w] = display::BLUE;
        }
    }
    car[15][5]  = display::RED;   // tail lights
    car[15][6]  = display::RED;
    car[15][10] = display::RED;
    car[15][11] = display::RED;
    car[14][5]  = display::RED;
    car[14][6]  = display::RED;
    car[14][10] = display::RED;
    car[14][11] = display::RED;
    car[1][5]   = display::WHITE; // head lights
    car[1][6]   = display::WHITE;
    car[1][10]  = display::WHITE;
    car[1][11]  = display::WHITE;
    car[2][5]   = display::WHITE;
    car[2][6]   = display::WHITE;
    car[2][10]  = display::WHITE;
    car[2][11]  = display::WHITE;

    // create cars at 1 to 359 degrees rotation, 
    // using the car created above at 0 degrees as a template
    for (int dir = 1; dir <= 359; dir++) {
        double sin_dir = sin(dir *  M_PI/180.0);
        double cos_dir = cos(dir *  M_PI/180.0);
        unsigned char (&carprime)[CAR_PIXELS_HEIGHT][CAR_PIXELS_WIDTH] = car_pixels[dir];
        int x,y,xprime,yprime;

        #define OVSF 3  // Over Sample Factor
        memset(carprime, display::TRANSPARENT, sizeof(car));
        for (y = 0; y < CAR_PIXELS_HEIGHT*OVSF; y++) {
            for (x = 0; x < CAR_PIXELS_WIDTH*OVSF; x++) {
                xprime = (x-CAR_PIXELS_WIDTH*OVSF/2) * cos_dir - (y-CAR_PIXELS_HEIGHT*OVSF/2) * sin_dir + CAR_PIXELS_WIDTH*OVSF/2 + 0.001;
                yprime = (x-CAR_PIXELS_WIDTH*OVSF/2) * sin_dir + (y-CAR_PIXELS_HEIGHT*OVSF/2) * cos_dir + CAR_PIXELS_HEIGHT*OVSF/2 + 0.001;
                if (xprime < 0 || xprime >= CAR_PIXELS_WIDTH*OVSF || yprime < 0 || yprime >= CAR_PIXELS_HEIGHT*OVSF) {
                    continue;
                }
                carprime[yprime/OVSF][xprime/OVSF] = car[y/OVSF][x/OVSF];
            }
        }
    }
}

// -----------------  CONSTRUCTOR / DESTRUCTOR  -------------------------------------

car::car(display &display, world &world, double x_arg, double y_arg, double dir_arg, double speed_arg) 
    : d(display), w(world)
{
    x         = x_arg;
    y         = y_arg;
    dir       = dir_arg;
    speed     = speed_arg;
    speed_ctl = 0;
    steer_ctl = 0;
}

car::~car()
{
}

// -----------------  UPDATE CAR CONTROLS  ------------------------------------------

void car::set_steer_ctl(double val) 
{
    if (val > MAX_STEER_CTL) {
        steer_ctl = MAX_STEER_CTL;
    } else if (val < MIN_STEER_CTL) {
        steer_ctl = MIN_STEER_CTL;
    } else {
        steer_ctl = val;
    }
}

void car::set_speed_ctl(double val) 
{
    if (speed_ctl > MAX_SPEED_CTL) {
        speed_ctl = MAX_SPEED_CTL;
    } else if (speed_ctl < MIN_SPEED_CTL) {
        speed_ctl = MIN_SPEED_CTL;
    } else {
        speed_ctl = val;
    }
}

// -----------------  UPDATE CAR POSITION, DIR, SPEED--------------------------------

// units:
// - distance:  feet
// - speed:     mph
// - dir:       degrees
// - steer_ctl: degrees
// - speed_ctl: mph/sec

void car::update_mechanics(double microsecs)
{
    double distance;
    double delta_dir;

    // XXX check for crash due to 
    // - over steering
    // - taking a turn at too high speed

    // update car position based on current direction and speed
    distance = speed * microsecs * (5280./3600./1e6);
    x += distance * cos((dir+270.) * (M_PI/180.0));
    y += distance * sin((dir+270.) * (M_PI/180.0));

    // update car direction based upon steering control 
    delta_dir = atan(distance / CAR_LENGTH * sin(steer_ctl*(M_PI/180.))) * (180./M_PI);
    dir += delta_dir;
    if (dir < 0) {  // xxx look at everywhere 360 is used, make an inline util for this
        dir += 360;
    } else if (dir > 360) {
        dir -= 360;
    }
    
    // update car speed based on speed control 
    speed += speed_ctl * (microsecs / 1000000.);
    if (speed < 0) {
        speed = 0;
    } else if (speed > MAX_SPEED) {
        speed = MAX_SPEED;
    }
}

// -----------------  DRAW CAR FRONT VIEW AND DASHBOARD  ----------------------------

void car::draw_front_view_and_dashboard(int front_view_pid, int dashboard_pid)
{
    // front view
    unsigned char front_view[MAX_FRONT_VIEW_XY*MAX_FRONT_VIEW_XY];
    w.get_view(x, y, dir, MAX_FRONT_VIEW_XY, MAX_FRONT_VIEW_XY, front_view);
    d.texture_set_rect(texture, 0, 0, MAX_FRONT_VIEW_XY, MAX_FRONT_VIEW_XY, front_view, MAX_FRONT_VIEW_XY);
    d.texture_draw(texture, 0, 0, MAX_FRONT_VIEW_XY, MAX_FRONT_VIEW_XY, front_view_pid);

    // dashboard  (600 x 100)
    // xxx should be able to get the pane w,h if desired from display.h

    // current speed
    std::ostringstream s;
    s << speed;
    d.text_draw(s.str(), 0.7, 1, dashboard_pid);

    // steering control
    int x;
    d.draw_set_color(display::WHITE);
    d.draw_rect(150,25,300,50,dashboard_pid,2);

    x = 300;
    d.draw_line(x-1, 20, x-1, 24, dashboard_pid);
    d.draw_line(x+0, 20, x+0, 24, dashboard_pid);
    d.draw_line(x+1, 20, x+1, 24, dashboard_pid);

    d.draw_line(x-1, 75, x-1, 79, dashboard_pid);
    d.draw_line(x+0, 75, x+0, 79, dashboard_pid);
    d.draw_line(x+1, 75, x+1, 79, dashboard_pid);

    d.draw_set_color(display::ORANGE);
    x = 152 + (steer_ctl - MIN_STEER_CTL) / (MAX_STEER_CTL - MIN_STEER_CTL) * 296;
    if (x < 152) x = 152;
    if (x > 447) x = 447;
    d.draw_line(x-1, 27, x-1, 72, dashboard_pid);
    d.draw_line(x+0, 27, x+0, 72, dashboard_pid);
    d.draw_line(x+1, 27, x+1, 72, dashboard_pid);

    // speed control - brake
    d.draw_set_color(display::WHITE);
    d.draw_rect(470,0,50,100,dashboard_pid,2);
    if (speed_ctl < 0) {
        d.draw_set_color(display::RED);
        int height = (speed_ctl / MIN_SPEED_CTL) * 96 + 0.5;
        if (height < 1) height = 1;
        if (height > 96) height = 96;
        d.draw_filled_rect(472, 98-height, 46, height, dashboard_pid);
    }

    // speed control - gas  
    d.draw_set_color(display::WHITE);
    d.draw_rect(540,0,50,100,dashboard_pid,2);
    if (speed_ctl > 0) {
        d.draw_set_color(display::GREEN);
        int height = (speed_ctl / MAX_SPEED_CTL) * 96 + 0.5;
        if (height < 1) height = 1;
        if (height > 96) height = 96;
        d.draw_filled_rect(542, 98-height, 46, height, dashboard_pid);
    }
}

// -----------------  PLACE CAR IN WORLD  -------------------------------------------

void car::place_car_in_world()
{
    int direction = dir + 0.5;
    if (direction >= 360) direction -= 360;

    w.place_object(x, y, CAR_PIXELS_WIDTH, CAR_PIXELS_HEIGHT,
                   reinterpret_cast<unsigned char *>(car_pixels[direction]));
}
