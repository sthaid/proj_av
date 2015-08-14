#ifndef __FIXED_CONTROL_CAR_H__
#define __FIXED_CONTROL_CAR_H__

#include "car.h"

class fixed_control_car : public car {
public:
    fixed_control_car(display &d, double x, double y, double dir, double speed=0);
    ~fixed_control_car();

    virtual void update_controls(double microsecs);

private:
};

#endif
