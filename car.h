#ifndef __CAR_H__
#define __CAR_H__

#include "display.h"

class car {
public:
    car(display &display, double x, double y, double dir, double speed=0);
    ~car();

    double get_x() { return x; }
    double get_y() { return y; }
    double get_dir() { return dir; }
    double get_speed() { return speed; }
    double get_speed_ctl() { return speed_ctl; }
    double get_steer_ctl() { return steer_ctl; }

    void draw(int front_view_pid, int dashboard_pid);

    void set_speed_ctl(double val);
    void set_steer_ctl(double val);

    void update_mechanics(double microsecs);

    virtual void update_controls(double microsecs) = 0;

private:
    // display
    display &d;
    display::texture *texture;

    // car state
    double x;
    double y;
    double dir;
    double speed;
    double speed_ctl;
    double steer_ctl;
    unsigned char front_view[100][100]; // XXX
};

#endif
