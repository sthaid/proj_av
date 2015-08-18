#ifndef __CAR_H__
#define __CAR_H__

#include "display.h"
#include "world.h"

class car {
public:
    car(display &display, world &w, double x, double y, double dir, double speed=0);
    ~car();

    static void static_init(display &d);

    double get_x() { return x; }
    double get_y() { return y; }
    double get_dir() { return dir; }
    double get_speed() { return speed; }
    double get_speed_ctl() { return speed_ctl; }
    double get_steer_ctl() { return steer_ctl; }
    world &get_world() { return w; }

    void draw(int front_view_pid, int dashboard_pid);

    void set_speed_ctl(double val);
    void set_steer_ctl(double val);

    void update_mechanics(double microsecs);
    virtual void update_controls(double microsecs) = 0;
private:
    // support front_view display
    display &d;
    world &w;
    static const int MAX_FRONT_VIEW_XY = 300;
    static display::texture *texture;

    // car state
    double x;
    double y;
    double dir;
    double speed;
    double speed_ctl;
    double steer_ctl;
};

#endif
