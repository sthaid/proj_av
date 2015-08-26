#ifndef __CAR_H__
#define __CAR_H__

#include "display.h"
#include "world.h"

class car {
public:
    car(display &display, world &w, double x, double y, double dir, double speed=0);
    ~car();

    static void static_init(display &d);

    const double MAX_STEER_CTL = 45;    // degrees   XXX should these const be private
    const double MIN_STEER_CTL = -45;   // degrees
    const double MAX_SPEED_CTL = 10;    // mph/sec
    const double MIN_SPEED_CTL = -30;   // mph/sec
    const double MAX_SPEED     = 50;    // mph
    const double CAR_LENGTH    = 10;    // ft   XXX wheel base

    world &get_world() { return w; }
    double get_x() { return x; }
    double get_y() { return y; }
    double get_dir() { return dir; }
    double get_speed() { return speed; }
    double get_speed_ctl() { return speed_ctl; }
    double get_steer_ctl() { return steer_ctl; }
    bool get_failed() { return failed; };

    void set_speed_ctl(double val);
    void set_steer_ctl(double val);
    void set_failed() { failed = true; }
    void update_mechanics(double microsecs);
    virtual void update_controls(double microsecs) = 0;

    void draw_front_view_and_dashboard(int front_view_pid, int dashboard_pid);
    void place_car_in_world();
private:
    // support front_view display
    display &d;
    world &w;
    static const int MAX_FRONT_VIEW_WIDTH = 101;
    static const int MAX_FRONT_VIEW_HEIGHT = 300;
    static display::texture *texture;

    // car state
    double x;
    double y;
    double dir;
    double speed;
    double speed_ctl;
    double steer_ctl;
    bool   failed;

    // car pixels
    static const int CAR_PIXELS_HEIGHT = 17;
    static const int CAR_PIXELS_WIDTH  = 17;
    static unsigned char car_pixels[360][CAR_PIXELS_HEIGHT][CAR_PIXELS_WIDTH];
};

#endif
