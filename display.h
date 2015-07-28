#ifndef __DISPLAY__
#define __DISPLAY__

class display {
public:
    typedef struct {
        int32_t event;
        union {
            struct {
                int32_t x;
                int32_t y;
            } mouse_click;
            struct {
                int32_t delta_x;
                int32_t delta_y;;
            } mouse_motion;
            struct {
                int32_t delta_x;
                int32_t delta_y;;
            } mouse_wheel;
        };
    } event_t;

    display(int w, int h);
    ~display();

    event_t poll_event();

    // events  XXX review the keybd events, maybe not needed hre
    // - no event
    static const int SDL_EVENT_NONE            = 0;
    // - ascii events 
    static const int SDL_EVENT_KEY_BS          = 8;
    static const int SDL_EVENT_KEY_TAB         = 9;
    static const int SDL_EVENT_KEY_ENTER       = 13;
    static const int SDL_EVENT_KEY_ESC         = 27;
    // - special keys
    static const int SDL_EVENT_KEY_HOME        = 130;
    static const int SDL_EVENT_KEY_END         = 131;
    static const int SDL_EVENT_KEY_PGUP        = 132;
    static const int SDL_EVENT_KEY_PGDN        = 133;
    // - window
    static const int SDL_EVENT_WIN_SIZE_CHANGE = 140;
    static const int SDL_EVENT_WIN_MINIMIZED   = 141;
    static const int SDL_EVENT_WIN_RESTORED    = 142;
    // - quit
    static const int SDL_EVENT_QUIT            = 150;
    // - available to be defined by users
    static const int SDL_EVENT_USER_START      = 160;
    static const int SDL_EVENT_USER_END        = 999;

    // event types
    static const int SDL_EVENT_TYPE_NONE         = 0;
    static const int SDL_EVENT_TYPE_TEXT         = 1;  // XXX maybe don't need this
    static const int SDL_EVENT_TYPE_MOUSE_CLICK  = 2;
    static const int SDL_EVENT_TYPE_MOUSE_MOTION = 3;
    static const int SDL_EVENT_TYPE_MOUSE_WHEEL  = 4;

#if 0
    clear
    present
    text
    rect
    line

    create_texture
    copy_texture
#endif

private:
    static const int MAX_SDL_FONT  = 2;
    static const int MAX_SDL_EVENT = 1000;

    // window state
    struct SDL_Window * sdl_window;  // XXX rm sdl_
    struct SDL_Renderer * sdl_renderer;
    int32_t sdl_win_width;
    int32_t sdl_win_height;
    bool    sdl_win_minimized;

    // program quit requested
    bool sdl_quit;

    // XXX
    struct Mix_Chunk * sdl_button_sound;

    struct {
        struct _TTF_Font * font;
        int32_t    char_width;
        int32_t    char_height;
    } sdl_font[MAX_SDL_FONT];

    int32_t mouse_button_state;
    int32_t mouse_button_motion_event;
    int32_t mouse_button_x;
    int32_t mouse_button_y;

    struct {
        // XXX SDL_Rect pos;
        int32_t x;
        int32_t y;
        int32_t w;
        int32_t h;
        int32_t type;
    } sdl_event_reg_tbl[MAX_SDL_EVENT];
    int32_t sdl_event_max;

    void sdl_play_event_sound(void);
};

#endif
