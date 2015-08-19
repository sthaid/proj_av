#ifndef __AUTONOMOUS_CAR_H__ 
#define __AUTONOMOUS_CAR_H__ 

#include "car.h"

class autonomous_car : public car {
public:
    autonomous_car(display &d, world &world, double x, double y, double dir, double speed=0);
    ~autonomous_car();

    virtual void update_controls(double microsecs);

private:
};

#endif
