#ifndef __CAR_H__
#define __CAR_H__

class car {
public:
    car(double x, double y, double dir, double speed=0);
    ~car();

    double get_x() { return x; }
    double get_y() { return y; }
    double get_dir() { return dir; }
    double get_speed() { return speed; }
    double get_speed_ctl() { return speed_ctl; }
    double get_steer_ctl() { return steer_ctl; }

    void set_speed_ctl(double val);
    void set_steer_ctl(double val);

    void update_mechanics(double microsecs);

    virtual void update_controls(double microsecs) = 0;

private:
    double x;
    double y;
    double dir;
    double speed;
    double speed_ctl;
    double steer_ctl;
};

class fixed_control_car : public car {
public:
    fixed_control_car(double x, double y, double dir, double speed=0);
    ~fixed_control_car();

    virtual void update_controls(double microsecs);

private:
};

#endif
