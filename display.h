#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <string>

class display {
public:
    static const int EID_NONE = -1;
    static const int KEY_HOME = 128;
    static const int KEY_END  = 129;
    static const int KEY_PGUP = 130;
    static const int KEY_PGDN = 131;

    enum color { RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE, BLACK, WHITE, GRAY, PINK, LIGHT_BLUE };
    enum event_type { ET_NONE=-1, ET_QUIT, 
                      ET_WIN_SIZE_CHANGE, ET_WIN_MINIMIZED, ET_WIN_RESTORED, 
                      ET_MOUSE_CLICK, ET_MOUSE_MOTION, ET_MOUSE_WHEEL, 
                      ET_KEYBOARD };

    struct event {
        int eid;
        int val1;  // delta_x, or key
        int val2;  // deltay_y
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
    void finish();

    void set_color(enum color);
    void draw_point(int x, int y, int pid=0);   // xxx multi
    void draw_line(int x1, int y1, int x2, int y2, int pid=0);   // xxx multi
    void draw_rect(int x, int y, int w, int h, int pid=0, int line_width=1);
    void draw_filled_rect(int x, int y, int w, int h, int pid=0);
    void draw_circle(int x, int y, int r, int pid=0);
    void draw_filled_circle(int x, int y, int r, int pid=0);
    int draw_text(std::string str, int row, int col, int pid=0, bool evreg=false, 
                  int fid=0, bool center=false, int field_cols=999);

    struct texture * create_texture(unsigned char * pixels, int w, int h);
    void destroy_texture(struct texture * t);
    void draw_texture(struct texture * t, int x, int y, int w, int h, int pid=0);

    int event_register(enum event_type et, int pid=0);
    int event_register(enum event_type et, int pid, int x, int y, int w, int h);
    struct event poll_event();
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
    } pane[4];

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
    } eid_tbl[MAX_EID];
    int max_eid;
    int mouse_button_state;
    int mouse_button_motion_eid;
    int mouse_button_x;
    int mouse_button_y;
    struct Mix_Chunk * event_sound;
    void play_event_sound(void);

    // print screen
    void print_screen(void);
};

#endif
