#include <cassert>
#include <math.h>

#include "autonomous_car.h"
#include "logging.h"
#include "utils.h"

// -----------------  CONSTRUCTOR / DESTRUCTOR  -------------------------------------

autonomous_car::autonomous_car(display &display, world &world, int id, double x, double y, double dir, double speed)
    : car(display,world,id,x,y,dir,speed)
{
}

autonomous_car::~autonomous_car()
{
}

// -----------------  DRAW VIEW VIRTUAL FUNCTION  ---------------------------------

void autonomous_car::draw_view(int pid)
{
    const int MAX_VIEW_WIDTH = 150;
    const int MAX_VIEW_HEIGHT = 390;
    static struct display::texture * t;
    unsigned char view[MAX_VIEW_WIDTH*MAX_VIEW_HEIGHT];
    class display &d = get_display();
    class world &w = get_world();

    if (t == NULL) {
        t = d.texture_create(MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
        assert(t);
    }

    w.get_view(get_x(), get_y(), get_dir(), MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT, view);
    d.texture_set_rect(t, 0, 0, MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT, view, MAX_VIEW_WIDTH);
    d.texture_draw2(t, pid, 300-MAX_VIEW_WIDTH/2, 0, MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);

    d.text_draw("FRONT", 0, 0, pid, false, 0, 1, true);
}

// -----------------  DRAW DASHBOARD VIRTUAL FUNCTION  -----------------------------

void autonomous_car::draw_dashboard(int pid)
{
    class display &d = get_display();

    // call base class draw_dashboard 
    car::draw_dashboard(pid);

    // draw around the autonomous dashboard
    d.draw_rect(0,98,590,102,pid,2);

    // XXX autonomous dash content is tbd
    d.text_draw("AUTONOMOUS DASH", 3.0, 0, pid, false, 0, 0, true);
}

// -----------------  UPDATE CONTROLS VIRTUAL FUNCTION  -----------------------------

void autonomous_car::update_controls(double microsecs)
{
    // if car has failed then return
    if (get_failed()) {
        return;
    }

    // get front_view, this is done to just get an idea of the processing tie
    unsigned char fv[300][101];
    world &w = get_world();
    w.get_view(get_x(), get_y(), get_dir(), 101, 300, reinterpret_cast<unsigned char *>(fv));

#if 0
    INFO("front_view " << 
             (int)fv[299][50] << " " <<
             (int)fv[298][50] << " " <<
             (int)fv[297][50] << " " <<
             (int)fv[296][50] << " " <<
             (int)fv[295][50] << " " <<
             (int)fv[294][50] << " " <<
             (int)fv[293][50] << " " <<
             (int)fv[291][50] << " " <<   // <==
             (int)fv[291][50] << " " <<
             (int)fv[290][50] << " " << endl);

    INFO("front_view " << 
             (int)fv[291][50] << " " <<
             (int)fv[291][49] << " " <<
             (int)fv[291][48] << " " <<
             (int)fv[291][47] << " " <<
             (int)fv[291][46] << " " <<
             (int)fv[291][45] << " " <<
             (int)fv[291][44] << " " <<
             (int)fv[291][43] << " " <<
             (int)fv[291][42] << " " << endl);
#endif
    INFO ("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX \n");

#if 0
    int Y,X;
    for (Y = 299; Y > 260; Y--) {
        for (X = 50; X > 30; X--) {
            cout << (int)fv[Y][X];
        } 
        cout << endl;
    }
    cout << endl;
#endif

    // NOTES
    // - lane width   = 12 ft
    // - car width    =  7 ft
    // - car length   = 15 ft

    // front view (fv) coords of the center of the car
    int x0 = 50; 
    int y0 = 299;
    INFO("x0,y0 " << x0 << " " << y0 << endl);

#if 0
    // coords of the front center of the car
    int xf = x0;     
    int yf = y0 - 7;
    INFO("xf,yf " << xf << " " << yf << endl);
#endif
    int yf = y0;
    int xf = x0;

    // locate the yellow line by scanning to the left at the front of the car
    #define LIMIT 15
    int i,j;
    j = 0;
    for (i = 0; i < LIMIT; i++) {
        if (fv[yf-j][xf-i] == display::YELLOW) {
            break;
        }
    }
    if (i == LIMIT) {
        j = 1;
        INFO("trying again to find yellow line "<< endl);
        for (i = 0; i < LIMIT; i++) {
            if (fv[yf-j][xf-i] == display::YELLOW) {
                break;
            }
        }
        if (i == LIMIT) {
            j = 2;
            INFO("trying yet again to find yellow line "<< endl);
            for (i = 0; i < LIMIT; i++) {
                if (fv[yf-j][xf-i] == display::YELLOW) {
                    break;
                }
            }
            if (i == LIMIT) {
                j = 3;
                INFO("trying yet yet again to find yellow line "<< endl);
                for (i = 0; i < LIMIT; i++) {
                    if (fv[yf-j][xf-i] == display::YELLOW) {
                        break;
                    }
                }
                if (i == LIMIT) {
                    INFO("FAILED - could not find yellow line" << endl);
                    set_failed();  
                    return;
                }
            }
        }
    }

    // coords of the the yellow line near the front of the car
    int xy = xf-i;
    int yy = yf;
    INFO("xy,yy " << xy << " " << yy << endl);

    // verify distance to yellow line is in range
    int distance_to_yellow_line = xf - xy;
    INFO("distance_to_yellow_line " << distance_to_yellow_line << endl);
    if (distance_to_yellow_line < 4) {
        INFO("FAILED - too close to yellow line " << distance_to_yellow_line << " ft" << endl);
        set_failed();
        return;
    }
    if (distance_to_yellow_line > 15) {  // XXX limit shoudl be 9
        INFO("FAILED - too far from yellow line " << distance_to_yellow_line << " ft" << endl);
        set_failed();
        return;
    }

    // locate the location of the yellow line DIST feet up the road
    #define DIST 30
    int xyscan = xy;
    int yyscan = yy;
    for (i = 0; i < DIST; i++) {
        if (fv[yyscan-1][xyscan] == display::YELLOW) {
            xyscan = xyscan;
            yyscan = yyscan - 1;
            continue;
        }
        if (fv[yyscan-1][xyscan-1] == display::YELLOW) {
            xyscan = xyscan - 1;
            yyscan = yyscan - 1;
            continue;
        }
        if (fv[yyscan-1][xyscan+1] == display::YELLOW) {
            xyscan = xyscan + 1;
            yyscan = yyscan - 1;
            continue;
        }
        if (fv[yyscan-1][xyscan-2] == display::YELLOW) {
            xyscan = xyscan - 2;
            yyscan = yyscan - 1;
            continue;
        }
        if (fv[yyscan-1][xyscan+2] == display::YELLOW) {
            xyscan = xyscan + 2;
            yyscan = yyscan - 1;
            continue;
        }

        if (fv[yyscan-2][xyscan] == display::YELLOW) {
            xyscan = xyscan;
            yyscan = yyscan - 2;
            continue;
        }
        if (fv[yyscan-2][xyscan-1] == display::YELLOW) {
            xyscan = xyscan - 1;
            yyscan = yyscan - 2;
            continue;
        }
        if (fv[yyscan-2][xyscan+1] == display::YELLOW) {
            xyscan = xyscan + 1;
            yyscan = yyscan - 2;
            continue;
        }
        if (fv[yyscan-2][xyscan-2] == display::YELLOW) {
            xyscan = xyscan - 2;
            yyscan = yyscan - 2;
            continue;
        }
        if (fv[yyscan-2][xyscan+2] == display::YELLOW) {
            xyscan = xyscan + 2;
            yyscan = yyscan - 2;
            continue;
        }
        if (fv[yyscan-2][xyscan-3] == display::YELLOW) {
            xyscan = xyscan - 3;
            yyscan = yyscan - 2;
            continue;
        }
        if (fv[yyscan-2][xyscan+3] == display::YELLOW) {
            xyscan = xyscan + 3;
            yyscan = yyscan - 2;
            continue;
        }

        if (fv[yyscan-3][xyscan] == display::YELLOW) {
            xyscan = xyscan;
            yyscan = yyscan - 3;
            continue;
        }
        if (fv[yyscan-3][xyscan-1] == display::YELLOW) {
            xyscan = xyscan - 1;
            yyscan = yyscan - 3;
            continue;
        }
        if (fv[yyscan-3][xyscan+1] == display::YELLOW) {
            xyscan = xyscan + 1;
            yyscan = yyscan - 3;
            continue;
        }
        if (fv[yyscan-3][xyscan-2] == display::YELLOW) {
            xyscan = xyscan - 2;
            yyscan = yyscan - 3;
            continue;
        }
        if (fv[yyscan-3][xyscan+2] == display::YELLOW) {
            xyscan = xyscan + 2;
            yyscan = yyscan - 3;
            continue;
        }
        if (fv[yyscan-3][xyscan-3] == display::YELLOW) {
            xyscan = xyscan - 3;
            yyscan = yyscan - 3;
            continue;
        }
        if (fv[yyscan-3][xyscan+3] == display::YELLOW) {
            xyscan = xyscan + 3;
            yyscan = yyscan - 3;
            continue;
        }

        if (fv[yyscan-4][xyscan] == display::YELLOW) {
            xyscan = xyscan;
            yyscan = yyscan - 4;
            continue;
        }
        if (fv[yyscan-4][xyscan-1] == display::YELLOW) {
            xyscan = xyscan - 1;
            yyscan = yyscan - 4;
            continue;
        }
        if (fv[yyscan-4][xyscan+1] == display::YELLOW) {
            xyscan = xyscan + 1;
            yyscan = yyscan - 4;
            continue;
        }
        if (fv[yyscan-4][xyscan-2] == display::YELLOW) {
            xyscan = xyscan - 2;
            yyscan = yyscan - 4;
            continue;
        }
        if (fv[yyscan-4][xyscan+2] == display::YELLOW) {
            xyscan = xyscan + 2;
            yyscan = yyscan - 4;
            continue;
        }
        if (fv[yyscan-4][xyscan-3] == display::YELLOW) {
            xyscan = xyscan - 3;
            yyscan = yyscan - 4;
            continue;
        }
        if (fv[yyscan-4][xyscan+3] == display::YELLOW) {
            xyscan = xyscan + 3;
            yyscan = yyscan - 4;
            continue;
        }


        INFO("FAILED - lost yellow line, last seen at " << xyscan << " " << yyscan << endl);
        set_failed();
        return;
    }
    INFO("xyscan,yyscan " << xyscan << " " << yyscan << endl);

    int x2 = xyscan + 7;
    int y2 = yyscan;

    // determine the angle to the location of the yellow line DIST feet up the road
    double angle = atan((double)(x2-xf)/-(y2-yf)) * (180./M_PI);
    INFO("angle " << angle << endl);

    // set steering control
    set_steer_ctl(angle);




    //microsec_sleep(1000000);

#if 0
locate yellow line in front left of car, up to N feet away, scanning in 
the current direction of the car

if yellow line is not found then fail the car

set steering and speed based on the characteristics of the yellow line

check the road ahead for obstructions, including
- other vehicle
- stop line
- dead end road
and adjust speed accordingly

if car has come to a stop at a stop line (only T intersecions supported)
  if turn is not chosen then 
    choose left or right turn at random
    locate cross road yellow line in both directions
  endif

  if right turn and no approaching cars from the left
    initiate right turn
  endif

  if left turn and no approaching cars from the left or right
    initiate left turn
  endif
endif

todo
- display failed cars in red
- status counters for the number of failed and good cars

#endif
    //set_steer_ctl(get_dir() < 90 ? 10 : 0);
    //set_speed_ctl(get_speed() < 20 ? 10 : 0);
}

