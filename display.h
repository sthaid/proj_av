/*
Copyright (c) 2015 Steven Haid

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <string>

using std::string;

class display {
public:
    static const int EID_NONE  = -1;
    static const int KEY_HOME  = 128;
    static const int KEY_END   = 129;
    static const int KEY_PGUP  = 130;
    static const int KEY_PGDN  = 131;
    static const int KEY_UP    = 132;
    static const int KEY_DOWN  = 133;
    static const int KEY_LEFT  = 134;
    static const int KEY_RIGHT = 135;

    enum color { RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE, BLACK, WHITE, GRAY, PINK, LIGHT_BLUE,
                 TRANSPARENT };
    enum event_type { ET_NONE=-1, ET_QUIT, 
                      ET_WIN_SIZE_CHANGE, ET_WIN_MINIMIZED, ET_WIN_RESTORED, 
                      ET_MOUSE_LEFT_CLICK, ET_MOUSE_RIGHT_CLICK, ET_MOUSE_LEFT_MOTION, ET_MOUSE_RIGHT_MOTION, ET_MOUSE_WHEEL, 
                      ET_KEYBOARD };
    enum motion_state {MOTION_START, MOTION_CONT, MOTION_COMPLETE};

    struct event {
        int eid;
        union {
            struct {
                int x;
                int y;
            } click;
            struct {
                enum motion_state state;
                int x;
                int y;
                int delta_x;
                int delta_y;
            } motion;
            struct {
                int delta_x;
                int delta_y;
            } wheel;
            struct {
                int code;
            } key;
            struct {
                int width;
                int height;
            } win_size;
        };
    };

    struct texture;

    display(int w, int h, bool resizeable=false);
    ~display();

    int get_win_width() { return win_width; }
    int get_win_height() { return win_height; }
    bool get_win_minimized() { return win_minimized; }
    int get_pane_rows(int pid=0, int fid=0) { return pane[pid].h / font[fid].char_height; }
    int get_pane_cols(int pid=0, int fid=0) { return pane[pid].w / font[fid].char_width; }

    void start(int x0, int y0, int w0, int h0);
    void start(int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1);
    void start(int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1,
               int x2, int y2, int w2, int h2);
    void start(int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1,
               int x2, int y2, int w2, int h2, int x3, int y3, int w3, int h3);
    void start(int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1,
               int x2, int y2, int w2, int h2, int x3, int y3, int w3, int h3,
               int x4, int y4, int w4, int h4);
    void finish();

    void draw_set_color(enum color c);
    void draw_point(int x, int y, int pid=0);
    void draw_line(int x1, int y1, int x2, int y2, int pid=0); 
    void draw_rect(int x, int y, int w, int h, int pid=0, int line_width=1);
    void draw_filled_rect(int x, int y, int w, int h, int pid=0);
    void draw_pointer(int x, int y, int ptr_size, int pid=0);

    int text_draw(string str, double row, double col, int pid=0, bool evreg=false, int key_alias=0,
                  int fid=0, bool center=false, int field_cols=999);

    struct texture * texture_create(int w, int h);
    struct texture * texture_create(unsigned char * pixels, int w, int h);
    void texture_set_pixel(struct texture * t, int x, int y, unsigned char pixel);
    void texture_clr_pixel(struct texture * t, int x, int y);
    void texture_set_rect(struct texture * t, int x, int y, int w, int h, unsigned char * pixels, int pitch);
    void texture_destroy(struct texture * t);
    void texture_draw1(struct texture * t, int x, int y, int w, int h, int pid=0);
    void texture_draw2(struct texture * t, int pid=0, int x=0, int y=0, int w=0, int h=0);

    int event_register(enum event_type et, int pid=0);
    int event_register(enum event_type et, int pid, int x, int y, int w, int h);
    int event_register(enum event_type et, int pid, int x, int y, int w, int h, int key_alias);
    struct event event_poll();
    void event_play_sound(void);
private:
    // window state
    struct SDL_Window * window; 
    struct SDL_Renderer * renderer;
    int win_width;
    int win_height;
    bool  win_minimized;

    // pane locations
    int max_pane;
    struct pane {
        int x, y;
        int w, h;
    } pane[8];

    // fonts
    struct {
        struct _TTF_Font * font;
        int char_width;
        int char_height;
    } font[2];

    // events
    static const int MAX_EID = 100;
    struct {
        enum event_type et;
        int x, y, w, h;
        int key_alias;
    } eid_tbl[MAX_EID];
    int max_eid;
    int mouse_motion_eid;
    int mouse_motion_x;
    int mouse_motion_y;
    struct Mix_Chunk * event_sound;

    // print screen
    void print_screen(void);
};

#endif
