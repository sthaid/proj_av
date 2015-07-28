#include <iostream>
#include <cstdint>
#include <cassert>
#include <string>

#include "display.h"
#include "button_sound.h"

extern "C" {
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
}

// XXX put in logging.h

using std::cout;
using std::endl;

#define INFO(x) \
    do { \
        cout << "INFO " << __func__ << ": " << x; \
    } while (0)
#define ERROR(x) \
    do { \
        cout << "ERROR " << __func__ << ": " << x; \
    } while (0)
#define FATAL(x) \
    do { \
        cout << "FATAL " << __func__ << ": " << x; \
        abort(); \
    } while (0)
#define DEBUG(x) \
    do { \
        cout << "DEBUG " << __func__ << ": " << x; \
    } while (0)

// window init  XXX ANDROID ?
#ifndef ANDROID 
#define SDL_FLAGS SDL_WINDOW_RESIZABLE
#else
#define SDL_FLAGS SDL_WINDOW_FULLSCREEN
#endif
#define MAX_SDL_FONT 2
#define SDL_EVENT_MAX 1000

// -----------------  CONSTRUCTOR & DESTRUCTOR  ----------------------------------------

display::display(int w, int h)
{
    int ret;  // XXX use int32_t

    // initialize Simple DirectMedia Layer  (SDL)
    ret = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
    assert(ret == 0);  // XXX check the rets

    // create SDL Window and Renderer
    ret = SDL_CreateWindowAndRenderer(w, h, SDL_FLAGS, &sdl_window, &sdl_renderer);
    assert(ret == 0);
    SDL_GetWindowSize(sdl_window, &sdl_win_width, &sdl_win_height);
    INFO("sdl_win_width=" << sdl_win_width << " sdl_win_height=" << sdl_win_height << endl);

    // init button_sound
    ret = Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096);
    assert(ret == 0);
    sdl_button_sound = Mix_QuickLoad_WAV(button_sound_wav);  // XXX 
    assert(sdl_button_sound);
    Mix_VolumeChunk(sdl_button_sound,MIX_MAX_VOLUME/2);

    // initialize True Type Font
    ret = TTF_Init();
    assert(ret == 0);

    // XXX rethink fonts, maybe this can be an array, maybe don't need keyboard
    const char * font0_path = "fonts/FreeMonoBold.ttf";         // normal 
    int32_t      font0_ptsize = sdl_win_height / 18 - 1;
    const char * font1_path = "fonts/FreeMonoBold.ttf";         // extra large, for keyboard
    int32_t      font1_ptsize = sdl_win_height / 13 - 1;

    sdl_font[0].font = TTF_OpenFont(font0_path, font0_ptsize);
    assert(sdl_font[0].font);
    TTF_SizeText(sdl_font[0].font, "X", &sdl_font[0].char_width, &sdl_font[0].char_height);
    INFO("font0 ptsize=" << font0_ptsize << 
         " width=" << sdl_font[0].char_width << 
         " height=" << sdl_font[0].char_height<< 
         endl);

    sdl_font[1].font = TTF_OpenFont(font1_path, font1_ptsize);
    assert(sdl_font[1].font);
    TTF_SizeText(sdl_font[1].font, "X", &sdl_font[1].char_width, &sdl_font[1].char_height);
    INFO("font1 ptsize=" << font1_ptsize << 
         " width=" << sdl_font[1].char_width << 
         " height=" << sdl_font[1].char_height << 
         endl);
}

display::~display()
{
    INFO("destructor" << endl);

    Mix_FreeChunk(sdl_button_sound);
    Mix_CloseAudio();

    for (int i = 0; i < MAX_SDL_FONT; i++) {
        TTF_CloseFont(sdl_font[i].font);
    }
    TTF_Quit();

    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}

// -----------------  EVENT HANDLING  --------------------------------------------------

display::event_t display::poll_event(void)
{
    #define AT_POS(X,Y,pos) (((X) >= (pos).x) && \
                             ((X) < (pos).x + (pos).w) && \
                             ((Y) >= (pos).y) && \
                             ((Y) < (pos).y + (pos).h))

    #define SDL_WINDOWEVENT_STR(x) \
       ((x) == SDL_WINDOWEVENT_SHOWN        ? "SDL_WINDOWEVENT_SHOWN"        : \
        (x) == SDL_WINDOWEVENT_HIDDEN       ? "SDL_WINDOWEVENT_HIDDEN"       : \
        (x) == SDL_WINDOWEVENT_EXPOSED      ? "SDL_WINDOWEVENT_EXPOSED"      : \
        (x) == SDL_WINDOWEVENT_MOVED        ? "SDL_WINDOWEVENT_MOVED"        : \
        (x) == SDL_WINDOWEVENT_RESIZED      ? "SDL_WINDOWEVENT_RESIZED"      : \
        (x) == SDL_WINDOWEVENT_SIZE_CHANGED ? "SDL_WINDOWEVENT_SIZE_CHANGED" : \
        (x) == SDL_WINDOWEVENT_MINIMIZED    ? "SDL_WINDOWEVENT_MINIMIZED"    : \
        (x) == SDL_WINDOWEVENT_MAXIMIZED    ? "SDL_WINDOWEVENT_MAXIMIZED"    : \
        (x) == SDL_WINDOWEVENT_RESTORED     ? "SDL_WINDOWEVENT_RESTORED"     : \
        (x) == SDL_WINDOWEVENT_ENTER        ? "SDL_WINDOWEVENT_ENTER"        : \
        (x) == SDL_WINDOWEVENT_LEAVE        ? "SDL_WINDOWEVENT_LEAVE"        : \
        (x) == SDL_WINDOWEVENT_FOCUS_GAINED ? "SDL_WINDOWEVENT_FOCUS_GAINED" : \
        (x) == SDL_WINDOWEVENT_FOCUS_LOST   ? "SDL_WINDOWEVENT_FOCUS_LOST"   : \
        (x) == SDL_WINDOWEVENT_CLOSE        ? "SDL_WINDOWEVENT_CLOSE"        : \
                                              "????")

    #define MOUSE_BUTTON_STATE_NONE     0
    #define MOUSE_BUTTON_STATE_DOWN     1
    #define MOUSE_BUTTON_STATE_MOTION   2

    #define MOUSE_BUTTON_STATE_RESET \
        do { \
            mouse_button_state = MOUSE_BUTTON_STATE_NONE; \
            mouse_button_motion_event = SDL_EVENT_NONE; \
            mouse_button_x = 0; \
            mouse_button_y = 0; \
        } while (0)

    SDL_Event ev;
    event_t event;
    int32_t i;

    bzero(&event, sizeof(event));
    event.event = SDL_EVENT_NONE;

    while (true) {
        // get the next event, break out of loop if no event
        if (SDL_PollEvent(&ev) == 0) {
            break;
        }

        // process the SDL event, this code
        // - sets event and sdl_quit
        // - updates sdl_win_width, sdl_win_height, sdl_win_minimized
        switch (ev.type) {
        case SDL_MOUSEBUTTONDOWN: {
            DEBUG("MOUSE DOWN which=" << ev.button.which << 
                  " button=" << (ev.button.button == SDL_BUTTON_LEFT   ? "LEFT" :
                                 ev.button.button == SDL_BUTTON_MIDDLE ? "MIDDLE" :
                                 ev.button.button == SDL_BUTTON_RIGHT  ? "RIGHT" :
                                                                         "???") <<
                  " state=" << (ev.button.state == SDL_PRESSED  ? "PRESSED" :
                                ev.button.state == SDL_RELEASED ? "RELEASED" :
                                                                  "???") <<
                  " x=" << ev.button.x <<
                  " y=" << ev.button.y << 
                  endl);

            // if not the left button then get out
            if (ev.button.button != SDL_BUTTON_LEFT) {
                break;
            }

            // reset mouse_button_state
            MOUSE_BUTTON_STATE_RESET;

            // check for text or mouse-click event
            for (i = 0; i < sdl_event_max; i++) {
                if ((sdl_event_reg_tbl[i].type == SDL_EVENT_TYPE_TEXT ||
                     sdl_event_reg_tbl[i].type == SDL_EVENT_TYPE_MOUSE_CLICK) &&
                    AT_POS(ev.button.x, ev.button.y, sdl_event_reg_tbl[i]))   // XXX new name
                {
                    // XXX set event x,y for mouse_click
                    event.event = i;
                    break;
                }
            }
            if (event.event != SDL_EVENT_NONE) {
                sdl_play_event_sound();
                break;
            }

            // it is not a text event, so set MOUSE_BUTTON_STATE_DOWN
            mouse_button_state = MOUSE_BUTTON_STATE_DOWN;
            mouse_button_x = ev.button.x;
            mouse_button_y = ev.button.y;
            break; }

        case SDL_MOUSEBUTTONUP: {
            DEBUG("MOUSE UP which=" << ev.button.which << 
                  " button=" << (ev.button.button == SDL_BUTTON_LEFT   ? "LEFT" :
                                 ev.button.button == SDL_BUTTON_MIDDLE ? "MIDDLE" :
                                 ev.button.button == SDL_BUTTON_RIGHT  ? "RIGHT" :
                                                                         "???") <<
                  " state=" << (ev.button.state == SDL_PRESSED  ? "PRESSED" :
                                ev.button.state == SDL_RELEASED ? "RELEASED" :
                                                                  "???") <<
                  " x=" << ev.button.x <<
                  " y=" << ev.button.y << 
                  endl);

            // if not the left button then get out
            if (ev.button.button != SDL_BUTTON_LEFT) {
                break;
            }

            // reset mouse_button_state,
            // this is where MOUSE_BUTTON_STATE_MOTION state is exitted
            MOUSE_BUTTON_STATE_RESET;
            break; }

#if 0
        case SDL_MOUSEMOTION: {
            // if MOUSE_BUTTON_STATE_NONE then get out
            if (mouse_button_state == MOUSE_BUTTON_STATE_NONE) {
                break;
            }

            // if in MOUSE_BUTTON_STATE_DOWN then check for mouse motion event; and if so
            // then set state to MOUSE_BUTTON_STATE_MOTION
            if (mouse_button_state == MOUSE_BUTTON_STATE_DOWN) {
                for (i = 0; i < sdl_event_max; i++) {
                    if (sdl_event_reg_tbl[i].type == SDL_EVENT_TYPE_MOUSE_MOTION &&
                        AT_POS(ev.motion.x, ev.motion.y, sdl_event_reg_tbl[i].pos)) 
                    {
                        mouse_button_state = MOUSE_BUTTON_STATE_MOTION;
                        mouse_button_motion_event = i;
                        break;
                    }
                }
            }

            // if did not find mouse_motion_event and not already in MOUSE_BUTTON_STATE_MOTION then 
            // reset mouse_button_state, and get out
            if (mouse_button_state != MOUSE_BUTTON_STATE_MOTION) {
                MOUSE_BUTTON_STATE_RESET;
                break;
            }

            // get all additional pending mouse motion events, and sum the motion
            event.event = mouse_button_motion_event;
            event.mouse_motion.delta_x = 0;
            event.mouse_motion.delta_y = 0;
            do {
                event.mouse_motion.delta_x += ev.motion.x - mouse_button_x;
                event.mouse_motion.delta_y += ev.motion.y - mouse_button_y;
                mouse_button_x = ev.motion.x;
                mouse_button_y = ev.motion.y;
            } while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION) == 1);
            break; }

        case SDL_MOUSEWHEEL: {
            int32_t mouse_x, mouse_y;

            // check if mouse wheel event is registered for the location of the mouse
            SDL_GetMouseState(&mouse_x, &mouse_y);
            for (i = 0; i < sdl_event_max; i++) {
                if (sdl_event_reg_tbl[i].type == SDL_EVENT_TYPE_MOUSE_WHEEL &&
                    AT_POS(mouse_x, mouse_y, sdl_event_reg_tbl[i].pos)) 
                {
                    break;
                }
            }

            // if did not find a registered mouse wheel event then get out
            if (i == sdl_event_max) {
                break;
            }

            // set return event
            event.event = i;
            event.mouse_wheel.delta_x = ev.wheel.x;
            event.mouse_wheel.delta_y = ev.wheel.y;
            break; }
#endif

        case SDL_KEYDOWN: {
            int32_t  key = ev.key.keysym.sym;
            bool     shift = (ev.key.keysym.mod & KMOD_SHIFT) != 0;
            bool     ctrl = (ev.key.keysym.mod & KMOD_CTRL) != 0;
            int32_t  possible_event = -1;

            if (ctrl) {
                if (key == 'p') {
                    // XXX sdl_print_screen();
                    sdl_play_event_sound();
                }
                break;
            }

            if (key < 128) {
                possible_event = key;
            } else if (key == SDLK_HOME) {
                possible_event = SDL_EVENT_KEY_HOME;
            } else if (key == SDLK_END) {
                possible_event = SDL_EVENT_KEY_END;
            } else if (key == SDLK_PAGEUP) {
                possible_event= SDL_EVENT_KEY_PGUP;
            } else if (key == SDLK_PAGEDOWN) {
                possible_event = SDL_EVENT_KEY_PGDN;
            }

            if (possible_event != -1) {
                if ((possible_event >= 'a' && possible_event <= 'z') &&
                    (sdl_event_reg_tbl[possible_event].w ||
                     sdl_event_reg_tbl[toupper(possible_event)].w))
                {
                    event.event = !shift ? possible_event : toupper(possible_event);
                    sdl_play_event_sound();
                } else if (sdl_event_reg_tbl[possible_event].w) {
                    event.event = possible_event;
                    sdl_play_event_sound();
                }
            }
            break; }

       case SDL_WINDOWEVENT: {
            DEBUG("got event SDL_WINOWEVENT - " << SDL_WINDOWEVENT_STR(ev.window.event) << endl);
            switch (ev.window.event)  {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                sdl_win_width = ev.window.data1;
                sdl_win_height = ev.window.data2;
                event.event = SDL_EVENT_WIN_SIZE_CHANGE;
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
                sdl_win_minimized = true;
                event.event = SDL_EVENT_WIN_MINIMIZED;
                break;
            case SDL_WINDOWEVENT_RESTORED:
                sdl_win_minimized = false;
                event.event = SDL_EVENT_WIN_RESTORED;
                break;
            }
            break; }

        case SDL_QUIT: {
            DEBUG("got event SDL_QUIT" << endl);
            sdl_quit = true;
            event.event = SDL_EVENT_QUIT;
            sdl_play_event_sound();
            break; }

        default: {
            DEBUG("got unsupported event " << ev.type << endl);
            break; }
        }

        // break if event is set
        if (event.event != SDL_EVENT_NONE) {
            break; 
        }
    }

    return event;
}

void display::sdl_play_event_sound(void)
{
    Mix_PlayChannel(-1, sdl_button_sound, 0);  // XXX rename button_sound
}

#if 0
// -----------------  SDL SUPPORT  ---------------------------------------

static void sdl_play_event_sound(void);
static void sdl_print_screen(void);

// - - - - - - - - -  EVENT HANDLING  - - - - - - - - - - - - - - - - - - 

typedef struct {
    SDL_Rect pos;
    int32_t type;
} sdl_event_reg_t;

static sdl_event_reg_t sdl_event_reg_tbl[SDL_EVENT_MAX];

void sdl_event_init(void)
{
    bzero(sdl_event_reg_tbl, sizeof(sdl_event_reg_tbl));
    sdl_event_max = 0;
}

void sdl_event_register(int32_t event_id, int32_t event_type, SDL_Rect * pos)
{
    sdl_event_reg_tbl[event_id].pos  = *pos;
    sdl_event_reg_tbl[event_id].type = event_type;

    if (event_id >= sdl_event_max) {
        sdl_event_max = event_id + 1;
    }
}



void sdl_print_screen(void)
{
#ifndef ANDROID
    int32_t   (*pixels)[4096] = NULL;
    FILE       *fp = NULL;
    SDL_Rect    rect;
    int32_t    *row_pointers[4096];
    char        file_name[1000];
    png_structp png_ptr;
    png_infop   info_ptr;
    png_byte    color_type, bit_depth;
    int32_t     y, ret;
    time_t      t;
    struct tm   tm;

    //
    // allocate and read the pixels
    //

    pixels = calloc(1, sdl_win_height*sizeof(pixels[0]));
    if (pixels == NULL) {
        ERROR("allocate pixels failed\n");
        goto done;
    }

    rect.x = 0;
    rect.y = 0;
    rect.w = sdl_win_width;
    rect.h = sdl_win_height;   
    ret = SDL_RenderReadPixels(sdl_renderer, &rect, SDL_PIXELFORMAT_ABGR8888, pixels, sizeof(pixels[0]));
    if (ret < 0) {
        ERROR("SDL_RenderReadPixels, %s\n", SDL_GetError());
        goto done;
    }

    //
    // creeate the file_name using localtime
    //

    t = time(NULL);
    localtime_r(&t, &tm);
    sprintf(file_name, "entropy_%2.2d%2.2d%2.2d_%2.2d%2.2d%2.2d.png",
            tm.tm_year - 100, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);

    //
    // write pixels to file_name ...
    //

    // init
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    bit_depth = 8;
    for (y = 0; y < sdl_win_height; y++) {
        row_pointers[y] = pixels[y];
    }

    // create file 
    fp = fopen(file_name, "wb");
    if (!fp) {
        ERROR("fopen %s\n", file_name);
        goto done;  
    }

    // initialize stuff 
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        ERROR("png_create_write_struct\n");
        goto done;  
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        ERROR("png_create_info_struct\n");
        goto done;  
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        ERROR("init_io failed\n");
        goto done;  
    }
    png_init_io(png_ptr, fp);

    // write header 
    if (setjmp(png_jmpbuf(png_ptr))) {
        ERROR("writing header\n");
        goto done;  
    }
    png_set_IHDR(png_ptr, info_ptr, sdl_win_width, sdl_win_height,
                 bit_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);

    // write bytes 
    if (setjmp(png_jmpbuf(png_ptr))) {
        ERROR("writing bytes\n");
        goto done;  
    }
    png_write_image(png_ptr, (void*)row_pointers);

    // end write 
    if (setjmp(png_jmpbuf(png_ptr))) {
        ERROR("end of write\n");
        goto done;  
    }
    png_write_end(png_ptr, NULL);

done:
    //
    // clean up and return
    //
    if (fp != NULL) {
        fclose(fp);
    }
    free(pixels);
#else
    WARN("not supported on Andorid version\n");
#endif
}

// - - - - - - - - -  RENDER TEXT WITH EVENT HANDLING - - - - - - - - - - 

void sdl_render_text_font0(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event)
{
    sdl_render_text_ex(pane, row, col, str, event, SDL_PANE_COLS(pane,0)-col, false, 0);
}

void sdl_render_text_font1(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event)
{
    sdl_render_text_ex(pane, row, col, str, event, SDL_PANE_COLS(pane,1)-col, false, 1);
}

void sdl_render_text_ex(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event, 
        int32_t field_cols, bool center, int32_t font_id)
{
    SDL_Surface    * surface = NULL;
    SDL_Texture    * texture = NULL;
    SDL_Color        fg_color;
    SDL_Rect         pos;
    char             s[500];
    int32_t          slen;

    static SDL_Color fg_color_normal = {255,255,255,255}; 
    static SDL_Color fg_color_event  = {0,255,255,255}; 
    static SDL_Color bg_color        = {0,0,0,255}; 

    // if zero length string then nothing to do
    if (str[0] == '\0') {
        return;
    }

    // if row or col are less than zero then adjust 
    if (row < 0) {
        row += SDL_PANE_ROWS(pane,font_id);
    }
    if (col < 0) {
        col += SDL_PANE_COLS(pane,font_id);
    }

    // verify row, col, and field_cold
    if (row < 0 || row >= SDL_PANE_ROWS(pane,font_id) || 
        col < 0 || col >= SDL_PANE_COLS(pane,font_id) ||
        field_cols <= 0) 
    {
        return;
    }

    // reduce field_cols if necessary to stay in pane
    if (field_cols > SDL_PANE_COLS(pane,font_id) - col) {
         field_cols = SDL_PANE_COLS(pane,font_id) - col;
    }

    // make a copy of the str arg, and shorten if necessary
    strcpy(s, str);
    slen = strlen(s);
    if (slen > field_cols) {
        s[field_cols] = '\0';
        slen = field_cols;
    }

    // if centered then adjust col
    if (center) {
        col += (field_cols - slen) / 2;
    }

    // render the text to a surface
    fg_color = (event != SDL_EVENT_NONE ? fg_color_event : fg_color_normal); 
    surface = TTF_RenderText_Shaded(sdl_font[font_id].font, s, fg_color, bg_color);
    if (surface == NULL) { 
        FATAL("TTF_RenderText_Shaded returned NULL\n");
    } 

    // determine the display location
    pos.x = pane->x + col * sdl_font[font_id].char_width;
    pos.y = pane->y + row * sdl_font[font_id].char_height;
    pos.w = surface->w;
    pos.h = surface->h;

    // create texture from the surface, and 
    // render the texture
    texture = SDL_CreateTextureFromSurface(sdl_renderer, surface); 
    SDL_RenderCopy(sdl_renderer, texture, NULL, &pos); 

    // clean up
    if (surface) {
        SDL_FreeSurface(surface); 
        surface = NULL;
    }
    if (texture) {
        SDL_DestroyTexture(texture); 
        texture = NULL;
    }

    // if there is a event then save the location for the event handler
    if (event != SDL_EVENT_NONE) {
        pos.x -= sdl_font[font_id].char_width/2;
        pos.y -= sdl_font[font_id].char_height/2;
        pos.w += sdl_font[font_id].char_width;
        pos.h += sdl_font[font_id].char_height;

        sdl_event_register(event, SDL_EVENT_TYPE_TEXT, &pos);
    }
}

// - - - - - - - - -  RENDER RECTANGLES & CIRCLES - - - - - - - - - - - - 

// color to rgba
uint32_t sdl_pixel_rgba[] = {
    //    red           green          blue    alpha
       (127 << 24) |               (255 << 8) | 255,     // PURPLE
                                   (255 << 8) | 255,     // BLUE
                     (255 << 16) | (255 << 8) | 255,     // LIGHT_BLUE
                     (255 << 16)              | 255,     // GREEN
       (255 << 24) | (255 << 16)              | 255,     // YELLOW
       (255 << 24) | (128 << 16)              | 255,     // ORANGE
       (255 << 24)                            | 255,     // RED         
       (224 << 24) | (224 << 16) | (224 << 8) | 255,     // GRAY        
       (255 << 24) | (105 << 16) | (180 << 8) | 255,     // PINK 
       (255 << 24) | (255 << 16) | (255 << 8) | 255,     // WHITE       
                                        };

uint32_t sdl_pixel_r[] = {
       127,     // PURPLE
       0,       // BLUE
       0,       // LIGHT_BLUE
       0,       // GREEN
       255,     // YELLOW
       255,     // ORANGE
       255,     // RED         
       224,     // GRAY        
       255,     // PINK 
       255,     // WHITE       
            };

uint32_t sdl_pixel_g[] = {
       0,       // PURPLE
       0,       // BLUE
       255,     // LIGHT_BLUE
       255,     // GREEN
       255,     // YELLOW
       128,     // ORANGE
       0,       // RED         
       224,     // GRAY        
       105,     // PINK 
       255,     // WHITE       
            };

uint32_t sdl_pixel_b[] = {
       255,     // PURPLE
       255,     // BLUE
       255,     // LIGHT_BLUE
       0,       // GREEN
       0,       // YELLOW
       0,       // ORANGE
       0,       // RED         
       224,     // GRAY        
       180,     // PINK 
       255,     // WHITE       
            };

void sdl_render_rect(SDL_Rect * rect_arg, int32_t line_width, uint32_t rgba)
{
    SDL_Rect rect = *rect_arg;
    int32_t i;
    uint8_t r, g, b, a;

    // xxx endian
    r = (rgba >> 24) & 0xff;
    g = (rgba >> 16) & 0xff;
    b = (rgba >>  8) & 0xff;
    a = (rgba      ) & 0xff;

    SDL_SetRenderDrawColor(sdl_renderer, r, g, b, a);

    for (i = 0; i < line_width; i++) {
        SDL_RenderDrawRect(sdl_renderer, &rect);
        if (rect.w < 2 || rect.h < 2) {
            break;
        }
        rect.x += 1;
        rect.y += 1;
        rect.w -= 2;
        rect.h -= 2;
    }
}

SDL_Texture * sdl_create_filled_circle_texture(int32_t radius, uint32_t rgba)
{
    int32_t width = 2 * radius + 1;
    int32_t x = radius;
    int32_t y = 0;
    int32_t radiusError = 1-x;
    int32_t pixels[width][width];
    SDL_Texture * texture;

    #define DRAWLINE(Y, XS, XE, V) \
        do { \
            int32_t i; \
            for (i = XS; i <= XE; i++) { \
                pixels[Y][i] = (V); \
            } \
        } while (0)

    // initialize pixels
    bzero(pixels,sizeof(pixels));
    while(x >= y) {
        DRAWLINE(y+radius, -x+radius, x+radius, rgba);
        DRAWLINE(x+radius, -y+radius, y+radius, rgba);
        DRAWLINE(-y+radius, -x+radius, x+radius, rgba);
        DRAWLINE(-x+radius, -y+radius, y+radius, rgba);
        y++;
        if (radiusError<0) {
            radiusError += 2 * y + 1;
        } else {
            x--;
            radiusError += 2 * (y - x) + 1;
        }
    }

    // create the texture and copy the pixels to the texture
    texture = SDL_CreateTexture(sdl_renderer,
                                SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_STATIC,
                                width, width);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(texture, NULL, pixels, width*4);

    // return texture
    return texture;
}

void sdl_render_circle(int32_t x, int32_t y, SDL_Texture * circle_texture)
{
    SDL_Rect rect;
    int32_t w, h;

    SDL_QueryTexture(circle_texture, NULL, NULL, &w, &h);

    rect.x = x - w/2;
    rect.y = y - h/2;
    rect.w = w;
    rect.h = h;

    SDL_RenderCopy(sdl_renderer, circle_texture, NULL, &rect);
}

// - - - - - - - - -  PREDEFINED DISPLAYS  - - - - - - - - - - - - - - - 

// user events used in this file
#define SDL_EVENT_KEY_SHIFT       (SDL_EVENT_USER_START+0)
#define SDL_EVENT_MOUSE_MOTION    (SDL_EVENT_USER_START+1)
#define SDL_EVENT_MOUSE_WHEEL     (SDL_EVENT_USER_START+2)
#define SDL_EVENT_FIELD_SELECT    (SDL_EVENT_USER_START+3)    // through +9
#define SDL_EVENT_LIST_CHOICE     (SDL_EVENT_USER_START+10)   // through +49
#define SDL_EVENT_BACK            (SDL_EVENT_USER_START+50)

void sdl_display_get_string(int32_t count, ...)
{
    char        * prompt_str[10]; 
    char        * curr_str[10];
    char        * ret_str[10];

    va_list       ap;
    SDL_Rect      keybdpane; 
    char          str[200];
    int32_t       i;
    sdl_event_t * event;

    int32_t       field_select;
    bool          shift;

    // this supports up to 4 fields
    if (count > 4) {
        ERROR("count %d too big, max 4\n", count);
    }

    // transfer variable arg list to array
    va_start(ap, count);
    for (i = 0; i < count; i++) {
        prompt_str[i] = va_arg(ap, char*);
        curr_str[i] = va_arg(ap, char*);
        ret_str[i] = va_arg(ap, char*);
    }
    va_end(ap);

    // init ret_str to curr_str
    for (i = 0; i < count; i++) {
        strcpy(ret_str[i], curr_str[i]);
    }

    // other init
    field_select = 0;
    shift = false;
    
    // loop until ENTER or ESC event received
    while (true) {
        // short sleep
        usleep(5000);

        // init
        SDL_INIT_PANE(keybdpane, 0, 0, sdl_win_width, sdl_win_height);
        sdl_event_init();

        // clear window
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        // draw keyboard
        static char * row_chars_unshift[4] = { "1234567890_",
                                               "qwertyuiop",
                                               "asdfghjkl",
                                               "zxcvbnm,." };
        static char * row_chars_shift[4]   = { "1234567890_",
                                               "QWERTYUIOP",
                                               "ASDFGHJKL",
                                               "ZXCVBNM,." };
        char ** row_chars;
        int32_t r, c, i, j;

        row_chars = (!shift ? row_chars_unshift : row_chars_shift);
        r = SDL_PANE_ROWS(&keybdpane,1) / 2 - 10;
        if (r < 0) {
            r = 0;
        }
        c = (SDL_PANE_COLS(&keybdpane,1) - 33) / 2;

        for (i = 0; i < 4; i++) {
            for (j = 0; row_chars[i][j] != '\0'; j++) {
                char s[2];
                s[0] = row_chars[i][j];
                s[1] = '\0';
                sdl_render_text_ex(&keybdpane, r+2*i, c+3*j, s, s[0], 1, false, 1);
            }
        }
        sdl_render_text_ex(&keybdpane, r+6, c+27,  "SPACE",  ' ',   5, false, 1);

        sdl_render_text_ex(&keybdpane, r+8, c+0,  "SHIFT",   SDL_EVENT_KEY_SHIFT,   5, false, 1);
        sdl_render_text_ex(&keybdpane, r+8, c+8,  "BS",      SDL_EVENT_KEY_BS,      2, false, 1);
        sdl_render_text_ex(&keybdpane, r+8, c+13, "TAB",     SDL_EVENT_KEY_TAB,     3, false, 1);
        sdl_render_text_ex(&keybdpane, r+8, c+19, "ESC",     SDL_EVENT_KEY_ESC,     3, false, 1);
        sdl_render_text_ex(&keybdpane, r+8, c+25, "ENTER",   SDL_EVENT_KEY_ENTER,   5, false, 1);

        // draw prompts
        for (i = 0; i < count; i++) {
            sprintf(str, "%s: %s", prompt_str[i], ret_str[i]);
            if ((i == field_select) && ((microsec_timer() % 1000000) > 500000) ) {
                strcat(str, "_");
            }
            sdl_render_text_ex(&keybdpane, 
                               r + 10 + 2 * (i % 2), 
                               i / 2 * SDL_PANE_COLS(&keybdpane,1) / 2, 
                               str, 
                               SDL_EVENT_FIELD_SELECT+i,
                               SDL_FIELD_COLS_UNLIMITTED, false, 1);
        }

        // present the display
        SDL_RenderPresent(sdl_renderer);

        // handle events 
        event = sdl_poll_event();
        if (event->event == SDL_EVENT_QUIT) {
            break;
        } else if (event->event == SDL_EVENT_KEY_SHIFT) {
            shift = !shift;
        } else if (event->event == SDL_EVENT_KEY_BS) {
            int32_t len = strlen(ret_str[field_select]);
            if (len > 0) {
                ret_str[field_select][len-1] = '\0';
            }
        } else if (event->event == SDL_EVENT_KEY_TAB) {
            field_select = (field_select + 1) % count;
        } else if (event->event == SDL_EVENT_KEY_ENTER) {
            break;
        } else if (event->event == SDL_EVENT_KEY_ESC) {
            if (strcmp(ret_str[field_select], curr_str[field_select])) {
                strcpy(ret_str[field_select], curr_str[field_select]);
            } else {
                for (i = 0; i < count; i++) {
                    strcpy(ret_str[i], curr_str[i]);
                }
                break;
            }
        } else if (event->event >= SDL_EVENT_FIELD_SELECT && event->event < SDL_EVENT_FIELD_SELECT+count) {
            field_select = event->event - SDL_EVENT_FIELD_SELECT;
        } else if (event->event >= 0x20 && event->event <= 0x7e) {
            char s[2];
            s[0] = event->event;
            s[1] = '\0';
            strcat(ret_str[field_select], s);
        }
    }
}

void sdl_display_text(char * text)
{
    SDL_Rect       dstrect;
    SDL_Texture ** texture = NULL;
    SDL_Rect       disp_pane;
    sdl_event_t  * event;
    char         * p;
    int32_t        i;
    int32_t        lines_per_display;
    bool           done = false;
    int32_t        max_texture = 0;
    int32_t        max_texture_alloced = 0;
    int32_t        text_y = 0;
    int32_t        pixels_per_row = sdl_font[0].char_height;

    // create a texture for each line of text
    //
    // note - this could be improved by doing just the first portion initially,
    //        and then finish up in the loop below
    // note - my Android program crashes if number of lines is too large (> 3800);
    //        this is probably because there is a limit on the number of textures
    for (p = text; *p; ) {
        SDL_Surface * surface;
        SDL_Color     fg_color= {255,255,255,255}; 
        SDL_Color     bg_color = {0,0,0,255}; 
        char        * newline;
        char          line[200];

        newline = strchr(p, '\n');
        if (newline) {
            memcpy(line, p, newline-p);
            line[newline-p] = '\0';
            p = newline + 1;
        } else {
            strcpy(line, p);
            p += strlen(line);
        }

        if (max_texture+1 > max_texture_alloced) {
            max_texture_alloced += 1000;
            texture = realloc(texture, max_texture_alloced*sizeof(void*));
        }

        surface = TTF_RenderText_Shaded(sdl_font[0].font, line, fg_color, bg_color);
        texture[max_texture++] = SDL_CreateTextureFromSurface(sdl_renderer, surface);
        SDL_FreeSurface(surface);
    }

    // loop until done
    while (!done) {
        // short delay
        usleep(5000);

        // init disp_pane and event 
        SDL_INIT_PANE(disp_pane, 0, 0, sdl_win_width, sdl_win_height);
        sdl_event_init();
        lines_per_display = sdl_win_height / pixels_per_row;

        // clear display
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        // sanitize text_y, this is the location of the text that is displayed
        // at the top of the display
        if (text_y > pixels_per_row * (max_texture - lines_per_display + 2)) {
            text_y = pixels_per_row * (max_texture - lines_per_display + 2);
        }
        if (text_y < 0) {
            text_y = 0;
        } 

        // display the text
        for (i = 0; ; i++) {
            int32_t w,h;

            if (i == max_texture) {
                break;
            }

            SDL_QueryTexture(texture[i], NULL, NULL, &w, &h);
            dstrect.x = 0;
            dstrect.y = i*pixels_per_row - text_y;
            dstrect.w = w;
            dstrect.h = h;

            if (dstrect.y + dstrect.h < 0) {
                continue;
            }
            if (dstrect.y >= sdl_win_height) {
                break;
            }

            SDL_RenderCopy(sdl_renderer, texture[i], NULL, &dstrect);
        }

        // display controls 
        if (max_texture > lines_per_display) {
            sdl_render_text_font0(&disp_pane, 0, -5, "HOME", SDL_EVENT_KEY_HOME);
            sdl_render_text_font0(&disp_pane, 2, -5, "END",  SDL_EVENT_KEY_END);
            sdl_render_text_font0(&disp_pane, 4, -5, "PGUP", SDL_EVENT_KEY_PGUP);
            sdl_render_text_font0(&disp_pane, 6, -5, "PGDN", SDL_EVENT_KEY_PGDN);
        }
        sdl_render_text_font0(&disp_pane, -1, -5, "BACK", SDL_EVENT_BACK);
        sdl_event_register(SDL_EVENT_MOUSE_MOTION, SDL_EVENT_TYPE_MOUSE_MOTION, &disp_pane);
        sdl_event_register(SDL_EVENT_MOUSE_WHEEL, SDL_EVENT_TYPE_MOUSE_WHEEL, &disp_pane);

        // present the display
        SDL_RenderPresent(sdl_renderer);

        // check for event
        event = sdl_poll_event();
        switch (event->event) {
        case SDL_EVENT_MOUSE_MOTION:
            text_y -= event->mouse_motion.delta_y;
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            text_y -= event->mouse_wheel.delta_y * 2 * pixels_per_row;
            break;
        case SDL_EVENT_KEY_HOME:
            text_y = 0;
            break;
        case SDL_EVENT_KEY_END:
            text_y = INT_MAX;
            break;
        case SDL_EVENT_KEY_PGUP:
            text_y -= (lines_per_display - 2) * pixels_per_row;
            break;
        case SDL_EVENT_KEY_PGDN:
            text_y += (lines_per_display - 2) * pixels_per_row;
            break;
        case SDL_EVENT_BACK:
        case SDL_EVENT_QUIT: 
            done = true;
        }
    }

    // free allocations
    for (i = 0; i < max_texture; i++) {
        SDL_DestroyTexture(texture[i]);
    }
}

void  sdl_display_choose_from_list(char * title_str, char ** choice, int32_t max_choice, int32_t * selection)
{
    SDL_Texture  * title_texture = NULL;
    SDL_Texture ** texture = NULL;
    SDL_Color      fg_color_title = {255,255,255,255}; 
    SDL_Color      fg_color_choice = {0,255,255,255};
    SDL_Color      bg_color = {0,0,0,255}; 
    SDL_Rect       disp_pane;
    SDL_Surface  * surface;
    sdl_event_t  * event;
    int32_t        i;
    int32_t        lines_per_display;
    int32_t        text_y = 0;
    bool           done = false;
    int32_t        pixels_per_row = sdl_font[0].char_height + 2;

    // preset return
    *selection = -1;

    // create texture for title
    surface = TTF_RenderText_Shaded(sdl_font[0].font, title_str, fg_color_title, bg_color);
    title_texture = SDL_CreateTextureFromSurface(sdl_renderer, surface);
    SDL_FreeSurface(surface);

    // create textures for each choice string
    texture = calloc(max_choice, sizeof(void*));
    if (texture == NULL) {
        ERROR("calloc texture, max_choice %d\n", max_choice);
        goto done;
    }
    for (i = 0; i < max_choice; i++) {
        surface = TTF_RenderText_Shaded(sdl_font[0].font, choice[i], fg_color_choice, bg_color);
        texture[i] = SDL_CreateTextureFromSurface(sdl_renderer, surface);
        SDL_FreeSurface(surface);
    }

    // loop until selection made, or aborted
    while (!done) {
        // short delay
        usleep(5000);

        // init disp_pane and event 
        SDL_INIT_PANE(disp_pane, 0, 0, sdl_win_width, sdl_win_height);
        sdl_event_init();
        lines_per_display = (sdl_win_height - 2*pixels_per_row) / pixels_per_row;

        // clear window
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        // sanitize text_y, this is the location of the text that is displayed
        // at the top of the display
        if (text_y > pixels_per_row * (2*(max_choice+1)-1 - lines_per_display+2)) {
            text_y = pixels_per_row * (2*(max_choice+1)-1 - lines_per_display+2);
        }
        if (text_y < 0) {
            text_y = 0;
        }

        // display the title and choices, and register for choices events
        for (i = 0; ; i++) {
            int32_t w,h;
            SDL_Rect dstrect;
            SDL_Texture * t;

            if (i == max_choice+1) {
                break;
            }

            t = (i == 0 ? title_texture : texture[i-1]);

            SDL_QueryTexture(t, NULL, NULL, &w, &h);
            dstrect.x = 0;
            dstrect.y = i * 2 * pixels_per_row - text_y;
            dstrect.w = w;
            dstrect.h = h;

            if (dstrect.y + dstrect.h < 0) {
                continue;
            }
            if (dstrect.y >= sdl_win_height) {
                break;
            }

            SDL_RenderCopy(sdl_renderer, t, NULL, &dstrect);

            if (i >= 1) {
                dstrect.x -= sdl_font[0].char_width/2;
                dstrect.y -= sdl_font[0].char_height/2;
                dstrect.w += sdl_font[0].char_width;
                dstrect.h += sdl_font[0].char_height;

                sdl_event_register(SDL_EVENT_LIST_CHOICE+i-1, SDL_EVENT_TYPE_TEXT, &dstrect);
            }
        }

        // display controls 
        if (2*(max_choice+1)-1 > lines_per_display) {
            sdl_render_text_font0(&disp_pane, 0, -5, "HOME", SDL_EVENT_KEY_HOME);
            sdl_render_text_font0(&disp_pane, 2, -5, "END",  SDL_EVENT_KEY_END);
            sdl_render_text_font0(&disp_pane, 4, -5, "PGUP", SDL_EVENT_KEY_PGUP);
            sdl_render_text_font0(&disp_pane, 6, -5, "PGDN", SDL_EVENT_KEY_PGDN);
        }
        sdl_render_text_font0(&disp_pane, -1, -5, "BACK", SDL_EVENT_BACK);
        sdl_event_register(SDL_EVENT_MOUSE_MOTION, SDL_EVENT_TYPE_MOUSE_MOTION, &disp_pane);
        sdl_event_register(SDL_EVENT_MOUSE_WHEEL, SDL_EVENT_TYPE_MOUSE_WHEEL, &disp_pane);

        // present the display
        SDL_RenderPresent(sdl_renderer);

        // check for event
        event = sdl_poll_event();
        switch (event->event) {
        case SDL_EVENT_LIST_CHOICE ... SDL_EVENT_LIST_CHOICE+39:
            *selection = event->event - SDL_EVENT_LIST_CHOICE;
            done = true;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            text_y -= event->mouse_motion.delta_y;
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            text_y -= event->mouse_wheel.delta_y * 2 * pixels_per_row;
            break;
        case SDL_EVENT_KEY_HOME:
            text_y = 0;
            break;
        case SDL_EVENT_KEY_END:
            text_y = INT_MAX;
            break;
        case SDL_EVENT_KEY_PGUP:
            text_y -= (lines_per_display - 2) * pixels_per_row;
            break;
        case SDL_EVENT_KEY_PGDN:
            text_y += (lines_per_display - 2) * pixels_per_row;
            break;
        case SDL_EVENT_BACK:
        case SDL_EVENT_QUIT: 
            done = true;
        }
    }

done:
    // free allocations
    if (title_texture) {
        SDL_DestroyTexture(title_texture);
    }
    for (i = 0; i < max_choice; i++) {
        if (texture[i]) {
            SDL_DestroyTexture(texture[i]);
        }
    }
    free(texture);
}

void sdl_display_error(char * err_str0, char * err_str1, char * err_str2)
{
    SDL_Rect      disp_pane;
    sdl_event_t * event;
    int32_t       row, col;
    bool          done = false;

    // loop until done
    while (!done) {
        // short delay
        usleep(5000);

        // init disp_pane and event 
        SDL_INIT_PANE(disp_pane, 0, 0, sdl_win_width, sdl_win_height);
        sdl_event_init();

        // clear display
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        // display the error strings
        row = SDL_PANE_ROWS(&disp_pane,0) / 2 - 2;
        if (row < 0) {
            row = 0;
        }
        col = 0;
        sdl_render_text_ex(&disp_pane, row,   col, err_str0, SDL_EVENT_NONE, SDL_FIELD_COLS_UNLIMITTED, true, 0);
        sdl_render_text_ex(&disp_pane, row+1, col, err_str1, SDL_EVENT_NONE, SDL_FIELD_COLS_UNLIMITTED, true, 0);
        sdl_render_text_ex(&disp_pane, row+2, col, err_str2, SDL_EVENT_NONE, SDL_FIELD_COLS_UNLIMITTED, true, 0);

        // display controls 
        sdl_render_text_font0(&disp_pane, -1, -5, "BACK", SDL_EVENT_BACK);

        // present the display
        SDL_RenderPresent(sdl_renderer);

        // check for event
        event = sdl_poll_event();
        switch (event->event) {
        case SDL_EVENT_BACK:
        case SDL_EVENT_QUIT: 
            done = true;
        }
    }
}

#endif
