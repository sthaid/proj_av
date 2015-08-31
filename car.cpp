#include <sstream>
#include <cassert>
#include <iomanip>
#include <math.h>
#include <memory.h>

#include "car.h"
#include "logging.h"
#include "utils.h"

using std::ostringstream;
using std::setprecision;
using std::fixed;

unsigned char car::car_pixels[360][CAR_PIXELS_HEIGHT][CAR_PIXELS_WIDTH];

// -----------------  CAR CLASS STATIC INITIALIZATION  ------------------------------

void car::static_init(display &d)
{
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

car::car(display &display, world &world, int id_arg, double x_arg, double y_arg, double dir_arg, double speed_arg, double max_speed_arg) 
    : d(display), w(world)
{
    id        = id_arg;  
    x         = x_arg;
    y         = y_arg;
    dir       = dir_arg;
    speed     = speed_arg;
    max_speed = max_speed_arg;
    speed_ctl = 0;
    steer_ctl = 0;
    failed    = false;

    // XXX check max_speed and others in constructor
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

    w.place_object(x, y, CAR_PIXELS_WIDTH, CAR_PIXELS_HEIGHT,
                   reinterpret_cast<unsigned char *>(car_pixels[direction]));
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

    // car id
    std::ostringstream s;
    s << id;
    d.text_draw(s.str(), 0.2, 1.0, pid);

    // current speed
    s.str("");
    s << fixed << setprecision(0) << speed;
    d.text_draw(s.str(), 1.2, 1.0, pid);

    // steering control
    int x;
    d.draw_set_color(display::WHITE);
    d.draw_rect(150,25,300,50,pid,2);

    x = 300;
    d.draw_line(x-1, 20, x-1, 24, pid);
    d.draw_line(x+0, 20, x+0, 24, pid);
    d.draw_line(x+1, 20, x+1, 24, pid);

    d.draw_line(x-1, 75, x-1, 79, pid);
    d.draw_line(x+0, 75, x+0, 79, pid);
    d.draw_line(x+1, 75, x+1, 79, pid);

    d.draw_set_color(display::ORANGE);
    x = 152 + (steer_ctl - MIN_STEER_CTL) / (MAX_STEER_CTL - MIN_STEER_CTL) * 296;
    if (x < 152) x = 152;
    if (x > 447) x = 447;
    d.draw_line(x-1, 27, x-1, 72, pid);
    d.draw_line(x+0, 27, x+0, 72, pid);
    d.draw_line(x+1, 27, x+1, 72, pid);

    // speed control - brake
    const int SPEED_CONTROL_HEIGHT = 88;
    const int SPEED_CONTROL_Y = 6;
    const int SPEED_CONTROL_BRAKE_X = 465;
    d.draw_set_color(display::WHITE);
    d.draw_rect(SPEED_CONTROL_BRAKE_X,SPEED_CONTROL_Y,50,SPEED_CONTROL_HEIGHT,pid,2);
    if (speed_ctl < 0) {
        d.draw_set_color(display::RED);
        int height = (speed_ctl / MIN_SPEED_CTL) * (SPEED_CONTROL_HEIGHT-4) + 0.5;
        if (height < 1) height = 1;
        if (height > (SPEED_CONTROL_HEIGHT-4)) height = (SPEED_CONTROL_HEIGHT-4);
        d.draw_filled_rect(SPEED_CONTROL_BRAKE_X+2, SPEED_CONTROL_Y+(SPEED_CONTROL_HEIGHT-2)-height, 46, height, pid);
    }

    // speed control - gas  
    const int SPEED_CONTROL_GAS_X   = 530;
    d.draw_set_color(display::WHITE);
    d.draw_rect(SPEED_CONTROL_GAS_X,SPEED_CONTROL_Y,50,SPEED_CONTROL_HEIGHT,pid,2);
    if (speed_ctl > 0) {
        d.draw_set_color(display::GREEN);
        int height = (speed_ctl / MAX_SPEED_CTL) * (SPEED_CONTROL_HEIGHT-4) + 0.5;
        if (height < 1) height = 1;
        if (height > (SPEED_CONTROL_HEIGHT-4)) height = (SPEED_CONTROL_HEIGHT-4);
        d.draw_filled_rect(SPEED_CONTROL_GAS_X+2, SPEED_CONTROL_Y+(SPEED_CONTROL_HEIGHT-2)-height, 46, height, pid);
    }
}

void car::update_controls(double microsecs)
{
    // no control updates are provided in the base class
}
