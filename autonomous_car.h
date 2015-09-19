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
    enum state { STATE_DRIVING, 
                 STATE_STOPPED_AT_STOP_LINE, STATE_STOPPED_AT_VEHICLE, STATE_STOPPED_AT_END_OF_ROAD, STATE_STOPPED,
                 STATE_CONTINUING_FROM_STOP };
    enum obstruction { OBSTRUCTION_NONE, OBSTRUCTION_STOP_LINE, OBSTRUCTION_REAR_VEHICLE, OBSTRUCTION_FRONT_VEHICLE,
                       OBSTRUCTION_END_OF_ROAD };
    typedef unsigned char (view_t)[MAX_VIEW_HEIGHT][MAX_VIEW_WIDTH];
    typedef struct {
        bool valid;
        double y_start_view;
        double x_start_view;
        double y_start_fixed;
        double x_start_fixed;
        double y_end_view;
        double x_end_view;
        double y_end_fixed;
        double x_end_fixed;
    } fullgap_t;

    enum state state;
    long time_in_this_state_us;
    enum obstruction obstruction;
    int distance_road_is_clear;
    double x_line[MAX_VIEW_HEIGHT];
    fullgap_t fullgap;

    void scan_road(view_t & view);
    double scan_across_for_center_line(view_t &view, int y, double x);
    enum obstruction scan_across_for_obstruction(view_t &view, int y, double x);
    bool scan_ahead_for_end_of_road(view_t &view, int y, double x, double slope);
    void scan_ahead_for_minigap(view_t &view, int y, double x, double slope,
            int &minigap_y_last, double &minigap_slope, string &minigap_type_str);
    void scan_ahead_for_continuing_center_lines(view_t &view, int y, int x, double slope,
            int &y_straight, int &x_straight, int &y_left, int &x_left, int &y_right, int &x_right);

    void set_car_controls();

    void state_change(enum state new_state);
    const string state_string(enum state s);
    const string obstruction_string(enum obstruction o);
    void coord_convert_view_to_fixed(double x_view, double y_view, double &x_fixed, double &y_fixed);
    void coord_convert_fixed_to_view(double x_fixed, double y_fixed, double &x_view, double &y_view);
};

#endif
