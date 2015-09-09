#ifndef __AUTONOMOUS_CAR_H__ 
#define __AUTONOMOUS_CAR_H__ 

#include "car.h"

class autonomous_car : public car {
public:
    autonomous_car(display &d, world &world, int id, double x, double y, double dir, double speed, double max_speed);
    ~autonomous_car();

    virtual void draw_view(int pid);
    virtual void draw_dashboard(int pid);
    virtual void update_controls(double microsecs);

private:
    static const int MAX_VIEW_WIDTH = 151;
    static const int MAX_VIEW_HEIGHT = 390;
    static const int xo = MAX_VIEW_WIDTH/2;
    static const int yo = MAX_VIEW_HEIGHT-1;
    typedef unsigned char (view_t)[MAX_VIEW_HEIGHT][MAX_VIEW_WIDTH];
    enum state { STATE_NONE, STATE_DRIVING, STATE_STOPPED_AT_STOP_LINE };
    enum obstruction { OBSTRUCTION_NONE, OBSTRUCTION_STOP_LINE, OBSTRUCTION_VEHICLE, OBSTRUCTION_END_OF_ROAD,
                       OBSTRUCTION_OTHER };

    enum state state;
    enum obstruction obstruction;
    int distance_road_is_clear;
    double x_line[MAX_VIEW_HEIGHT];

    void scan_road(view_t & view);
    double scan_for_center_line(view_t &view, int y, double x_double);
    void set_car_controls();
    const string state_string();
    const string obstruction_string();

};

#endif
