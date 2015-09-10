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
    enum state { STATE_DRIVING, 
                 STATE_STOPPED_AT_STOP_LINE, STATE_STOPPED_AT_VEHICLE, STATE_STOPPED_AT_END_OF_ROAD, STATE_STOPPED,
                 STATE_CONTINUING_FROM_STOP };
    enum obstruction { OBSTRUCTION_NONE, OBSTRUCTION_STOP_LINE, OBSTRUCTION_VEHICLE, OBSTRUCTION_END_OF_ROAD };

    enum state state;
    long time_in_this_state_us;
    enum obstruction obstruction;
    int distance_road_is_clear;
    double x_line[MAX_VIEW_HEIGHT];

    void scan_road(view_t & view);
    double scan_for_center_line(view_t &view, int y, double x_double, bool &end_of_road);
    void set_car_controls();

    void state_change(enum state new_state);
    const string state_string();
    const string obstruction_string();

};

#endif
