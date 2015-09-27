#ifndef __CAR_H__
#define __CAR_H__

#include "display.h"
#include "world.h"

class car {
public:
    car(display &display, world &w, int id, double x, double y, double dir, double speed, double max_speed);
    virtual ~car();

    static void static_init(display &d);

    const double MAX_STEER_CTL = 45;    // degrees
    const double MIN_STEER_CTL = -45;   // degrees
    const double MAX_SPEED_CTL = 10;    // mph/sec
    const double MIN_SPEED_CTL = -30;   // mph/sec

    world &get_world() { return w; }
    display &get_display() { return d; }
    int get_id() { return id; }
    double get_x() { return x; }
    double get_y() { return y; }
    double get_dir() { return dir; }
    double get_speed() { return speed; }
    double get_max_speed() { return max_speed; }
    double get_speed_ctl() { return speed_ctl; }
    double get_steer_ctl() { return steer_ctl; }
    bool get_failed() { return failed; };
    string get_failed_str() { return failed_str; };

    void set_speed_ctl(double val);
    void set_steer_ctl(double val);
    void set_failed(const string &str) { failed_str = str; failed = true; }
    void update_mechanics(double microsecs);
    void place_car_in_world();

    virtual void draw_view(int pid);
    virtual void draw_dashboard(int pid);
    virtual void update_controls(double microsecs);
private:
    // support front_view display
    display &d;
    world &w;
    static display::texture *texture;

    // car state
    int    id;
    double x;
    double y;
    double dir;
    double speed;
    double max_speed;
    double speed_ctl;
    double speed_ctl_smoothed;
    double steer_ctl;
    double steer_ctl_smoothed;
    bool   failed;
    string failed_str;
    long   run_time_us;
};

#endif
