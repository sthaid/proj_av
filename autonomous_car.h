#ifndef __AUTONOMOUS_CAR_H__ 
#define __AUTONOMOUS_CAR_H__ 

#include "car.h"

class autonomous_car : public car {
public:
    autonomous_car(display &d, world &world, int id, double x, double y, double dir, double speed);
    ~autonomous_car();

    virtual void draw_view(int pid);
    virtual void draw_dashboard(int pid);
    virtual void update_controls(double microsecs);

private:
};

#endif
