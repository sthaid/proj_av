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

    void set_speed_ctl(double);
    void set_steer_ctl(double);

    void update(double microsecs);


private:
    double x;
    double y;
    double dir;
    double speed;

    double speed_ctl;
    double steer_ctl;
};

#endif
