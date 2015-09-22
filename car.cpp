#include <sstream>
#include <cassert>
#include <iomanip>
#include <cmath>
#include <cstring>

#include "car.h"
#include "logging.h"
#include "utils.h"

using std::ostringstream;
using std::setprecision;
using std::fixed;

// car pixels
const int CAR_PIXELS_HEIGHT = 17;
const int CAR_PIXELS_WIDTH  = 17;
unsigned char good_car_pixels[360][CAR_PIXELS_HEIGHT][CAR_PIXELS_WIDTH];
unsigned char failed_car_pixels[360][CAR_PIXELS_HEIGHT][CAR_PIXELS_WIDTH];

// -----------------  CAR CLASS STATIC INITIALIZATION  ------------------------------

void car::static_init(display &d)
{
    //
    // init car_pixels ...
    //

    // preset all pixels to transparent
    memset(good_car_pixels, display::TRANSPARENT, sizeof(good_car_pixels));
    memset(failed_car_pixels, display::TRANSPARENT, sizeof(failed_car_pixels));

    // create good_car_pixels at 0 degree rotation
    for (int h = 1; h <= 15; h++) {
        for (int w = 5; w <= 11; w++) {
            good_car_pixels[0][h][w] = display::BLUE;
        }
    }
    good_car_pixels[0][15][5]  = display::ORANGE;   // tail lights
    good_car_pixels[0][15][6]  = display::ORANGE;
    good_car_pixels[0][15][10] = display::ORANGE;
    good_car_pixels[0][15][11] = display::ORANGE;
    good_car_pixels[0][14][5]  = display::ORANGE;
    good_car_pixels[0][14][6]  = display::ORANGE;
    good_car_pixels[0][14][10] = display::ORANGE;
    good_car_pixels[0][14][11] = display::ORANGE;
    good_car_pixels[0][1][5]   = display::WHITE; // head lights
    good_car_pixels[0][1][6]   = display::WHITE;
    good_car_pixels[0][1][10]  = display::WHITE;
    good_car_pixels[0][1][11]  = display::WHITE;
    good_car_pixels[0][2][5]   = display::WHITE;
    good_car_pixels[0][2][6]   = display::WHITE;
    good_car_pixels[0][2][10]  = display::WHITE;
    good_car_pixels[0][2][11]  = display::WHITE;

    // create failed_car_pixels at 0 degree rotation
    for (int h = 1; h <= 15; h++) {
        for (int w = 5; w <= 11; w++) {
            failed_car_pixels[0][h][w] = display::PINK;
        }
    }
    failed_car_pixels[0][15][5]  = display::ORANGE;   // tail lights
    failed_car_pixels[0][15][6]  = display::ORANGE;
    failed_car_pixels[0][15][10] = display::ORANGE;
    failed_car_pixels[0][15][11] = display::ORANGE;
    failed_car_pixels[0][14][5]  = display::ORANGE;
    failed_car_pixels[0][14][6]  = display::ORANGE;
    failed_car_pixels[0][14][10] = display::ORANGE;
    failed_car_pixels[0][14][11] = display::ORANGE;
    failed_car_pixels[0][1][5]   = display::WHITE; // head lights
    failed_car_pixels[0][1][6]   = display::WHITE;
    failed_car_pixels[0][1][10]  = display::WHITE;
    failed_car_pixels[0][1][11]  = display::WHITE;
    failed_car_pixels[0][2][5]   = display::WHITE;
    failed_car_pixels[0][2][6]   = display::WHITE;
    failed_car_pixels[0][2][10]  = display::WHITE;
    failed_car_pixels[0][2][11]  = display::WHITE;

    // create good_car_pixels and failed_car_pixels at 1 to 359 degrees rotation, 
    // using the good/failed_car_pixels created above at 0 degrees as a template
    for (int dir = 1; dir <= 359; dir++) {
        double sin_dir = sin(dir *  M_PI/180.0);
        double cos_dir = cos(dir *  M_PI/180.0);
        int x,y,xprime,yprime;

        #define OVSF 3  // Over Sample Factor
        for (y = 0; y < CAR_PIXELS_HEIGHT*OVSF; y++) {
            for (x = 0; x < CAR_PIXELS_WIDTH*OVSF; x++) {
                xprime = (x-CAR_PIXELS_WIDTH*OVSF/2) * cos_dir - (y-CAR_PIXELS_HEIGHT*OVSF/2) * sin_dir + CAR_PIXELS_WIDTH*OVSF/2 + 0.001;
                yprime = (x-CAR_PIXELS_WIDTH*OVSF/2) * sin_dir + (y-CAR_PIXELS_HEIGHT*OVSF/2) * cos_dir + CAR_PIXELS_HEIGHT*OVSF/2 + 0.001;
                if (xprime < 0 || xprime >= CAR_PIXELS_WIDTH*OVSF || yprime < 0 || yprime >= CAR_PIXELS_HEIGHT*OVSF) {
                    continue;
                }
                good_car_pixels[dir][yprime/OVSF][xprime/OVSF] = good_car_pixels[0][y/OVSF][x/OVSF];
                failed_car_pixels[dir][yprime/OVSF][xprime/OVSF] = failed_car_pixels[0][y/OVSF][x/OVSF];
            }
        }
    }
}

// -----------------  CONSTRUCTOR / DESTRUCTOR  -------------------------------------

car::car(display &display, world &world, int id_arg, double x_arg, double y_arg, double dir_arg, double speed_arg, double max_speed_arg) 
    : d(display), w(world)
{
    id                 = id_arg;  
    x                  = x_arg;
    y                  = y_arg;
    dir                = dir_arg;
    speed              = speed_arg;
    max_speed          = max_speed_arg;
    speed_ctl          = 0;
    speed_ctl_smoothed = 0;
    steer_ctl          = 0;
    steer_ctl_smoothed = 0;
    failed             = false;

    // XXX check max_speed and others in constructor
}

car::~car()
{
}

// -----------------  UPDATE CAR CONTROLS  ------------------------------------------

void car::set_steer_ctl(double val) 
{
    if (get_failed()) {
        return;
    }

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
    if (get_failed()) {
        return;
    }

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
    const double WHEEL_BASE_LENGTH = 10;  // ft 

    double distance;
    double delta_dir;

    // XXX check for crash due to:
    // - over steering
    // - taking a turn at too high speed
    // - others?

    // if car has failed then do nothing
    if (get_failed()) {
        return;
    }

    // update car position based on current direction and speed
    distance = speed * microsecs * (5280./3600./1e6);
    x += distance * cos((dir+270.) * (M_PI/180.0));
    y += distance * sin((dir+270.) * (M_PI/180.0));

    // update car direction based upon steering control 
    delta_dir = atan(distance / WHEEL_BASE_LENGTH * sin(steer_ctl*(M_PI/180.))) * (180./M_PI);
    dir = sanitize_direction(dir + delta_dir);
    
    // update car speed based on speed control 
    speed += speed_ctl * (microsecs / 1000000.);
    if (speed < 0.2) {
        speed = 0;
    } else if (speed > max_speed) {
        speed = max_speed;
    }
}

// -----------------  PLACE CAR IN WORLD  -------------------------------------------

void car::place_car_in_world()
{
    int direction = sanitize_direction(dir + 0.5);
    assert(direction >= 0 && direction < 360);

    if (!get_failed()) {
        w.place_object(x, y, CAR_PIXELS_WIDTH, CAR_PIXELS_HEIGHT,
                    reinterpret_cast<unsigned char *>(good_car_pixels[direction]));
    } else {
        w.place_object(x, y, CAR_PIXELS_WIDTH, CAR_PIXELS_HEIGHT,
                    reinterpret_cast<unsigned char *>(failed_car_pixels[direction]));
    }
}

// -----------------  VIRTUAL DRAW_VIEW, DRAW_DASHBOARD, and UPDATE_CONTROLS --------

void car::draw_view(int pid)
{
    const int MAX_VIEW_WIDTH = 150;
    const int MAX_VIEW_HEIGHT = 390;
    static struct display::texture * t;
    unsigned char view[MAX_VIEW_WIDTH*MAX_VIEW_HEIGHT];

    if (t == NULL) {
        t = d.texture_create(MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
        assert(t);
    }

    w.get_view(x, y, dir, MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT, view);
    d.texture_set_rect(t, 0, 0, MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT, view, MAX_VIEW_WIDTH);
    d.texture_draw2(t, pid, 300-MAX_VIEW_WIDTH/2, 0, MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);

    d.text_draw("front", 0, 0, pid, false, 0, 1, true);
}

void car::draw_dashboard(int pid)
{
    // dashboard WxH = 600x200
    // 
    // this function uses the upper 600x100, reserving the lower 
    // half for derived class draw_dashboard virtual function

    // draw rect around base dashboard
    d.draw_set_color(display::WHITE);
    d.draw_rect(0,0,590,100,pid,2);

    // current speed
    std::ostringstream s;
    s << fixed << setprecision(0) << speed;
    d.text_draw(s.str(), 0.5, 1, pid);

    // id and failed_str / 'ok'
    s.str("");
    s << id << " ";
    if (get_failed()) {
        s << get_failed_str();
    } else {
        s << "OK";
    }
    d.text_draw(s.str(), 2.1, 1.5, pid, false, 0, 1);

    // steering control
    steer_ctl_smoothed = (steer_ctl + 9 * steer_ctl_smoothed) / 10;
    if (fabs(steer_ctl_smoothed) < .1) {
        steer_ctl_smoothed = 0;
    }

    d.draw_set_color(display::WHITE);
    d.draw_rect(150,10,300,50,pid,2);

    int x = 300;
    d.draw_line(x-1, 5, x-1, 9, pid);
    d.draw_line(x+0, 5, x+0, 9, pid);
    d.draw_line(x+1, 5, x+1, 9, pid);
    d.draw_line(x-1, 60, x-1, 64, pid);
    d.draw_line(x+0, 60, x+0, 64, pid);
    d.draw_line(x+1, 60, x+1, 64, pid);

    d.draw_set_color(display::ORANGE);
    x = 152 + (steer_ctl_smoothed - MIN_STEER_CTL/4) / (MAX_STEER_CTL/4 - MIN_STEER_CTL/4) * 296;
    if (x < 154) x = 154;
    if (x > 445) x = 445;
    d.draw_line(x-2, 12, x-2, 57, pid);
    d.draw_line(x-1, 12, x-1, 57, pid);
    d.draw_line(x+0, 12, x+0, 57, pid);
    d.draw_line(x+1, 12, x+1, 57, pid);
    d.draw_line(x+2, 12, x+2, 57, pid);

    // speed control - brake
    const int SPEED_CONTROL_HEIGHT = 88;
    const int SPEED_CONTROL_Y = 6;
    const int SPEED_CONTROL_BRAKE_X = 465;

    speed_ctl_smoothed = (speed_ctl + 9 * speed_ctl_smoothed) / 10;
    if (fabs(speed_ctl_smoothed) < .1) {
        speed_ctl_smoothed = 0;
    }

    d.draw_set_color(display::WHITE);
    d.draw_rect(SPEED_CONTROL_BRAKE_X,SPEED_CONTROL_Y,50,SPEED_CONTROL_HEIGHT,pid,2);
    if (speed_ctl_smoothed < 0) {
        d.draw_set_color(display::RED);
        int height = (speed_ctl_smoothed / MIN_SPEED_CTL) * (SPEED_CONTROL_HEIGHT-4) + 0.5;
        if (height < 1) height = 1;
        if (height > (SPEED_CONTROL_HEIGHT-4)) height = (SPEED_CONTROL_HEIGHT-4);
        d.draw_filled_rect(SPEED_CONTROL_BRAKE_X+2, SPEED_CONTROL_Y+(SPEED_CONTROL_HEIGHT-2)-height, 46, height, pid);
    }

    // speed control - gas  
    const int SPEED_CONTROL_GAS_X   = 530;
    d.draw_set_color(display::WHITE);
    d.draw_rect(SPEED_CONTROL_GAS_X,SPEED_CONTROL_Y,50,SPEED_CONTROL_HEIGHT,pid,2);
    if (speed_ctl_smoothed > 0) {
        d.draw_set_color(display::GREEN);
        int height = (speed_ctl_smoothed / MAX_SPEED_CTL) * (SPEED_CONTROL_HEIGHT-4) + 0.5;
        if (height < 1) height = 1;
        if (height > (SPEED_CONTROL_HEIGHT-4)) height = (SPEED_CONTROL_HEIGHT-4);
        d.draw_filled_rect(SPEED_CONTROL_GAS_X+2, SPEED_CONTROL_Y+(SPEED_CONTROL_HEIGHT-2)-height, 46, height, pid);
    }
}

void car::update_controls(double microsecs)
{
    // no control updates are provided in the base class
}
