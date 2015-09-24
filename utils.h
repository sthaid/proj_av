#ifndef __UTILS_H__
#define __UTILS_H__

void microsec_sleep(long us);
long microsec_timer(void);

inline double sanitize_direction(double d) 
{
    while (true) {
        if (d >= 0 && d < 360) {
            return d;
        } else if (d < 0) {
            d += 360;
        } else {
            d -= 360;
        }
    }
}

#endif
