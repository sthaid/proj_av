#include <iostream>
#include <cstdint>
#include <cassert>
#include <string>

#include "display.h"
#include "event_sound.h"
#include "logging.h"

extern "C" {
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <png.h>
}

// -----------------  CONSTRUCTOR & DESTRUCTOR  ----------------------------------------

display::display(int w, int h)
{
    int ret;  

    // init all fields 
    window = NULL;
    renderer = NULL;
    win_width = 0;
    win_height = 0;
    win_minimized = false;
    max_pane = 0;
    bzero(pane,sizeof(pane));
    bzero(font,sizeof(font));
    bzero(eid_tbl,sizeof(eid_tbl));
    max_eid = 0;
    mouse_button_state = 0;
    mouse_button_motion_eid = 0;
    mouse_button_x = 0;
    mouse_button_y = 0;
    event_sound = NULL;

    // initialize Simple DirectMedia Layer  (SDL)
    ret = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
    if (ret != 0) {
        FATAL("SDL_Init failed" << endl);
    }

    // create SDL Window and Renderer
    ret = SDL_CreateWindowAndRenderer(w, h, SDL_WINDOW_RESIZABLE, &window, &renderer);
    if (ret != 0) {
        FATAL("SDL_CreateWindowAndRenderer failed" << endl);
    }
    SDL_GetWindowSize(window, &win_width, &win_height);
    INFO("win_width=" << win_width << " win_height=" << win_height << endl);

    // init event_sound
    ret = Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096);
    if (ret != 0) {
        FATAL("Mix_OpenAudio failed" << endl);
    }
    event_sound = Mix_QuickLoad_WAV(event_sound_wav);
    if (event_sound == NULL) {
        FATAL("Mix_QuickLoad_WAV failed" << endl);
    }
    Mix_VolumeChunk(event_sound,MIX_MAX_VOLUME/2);

    // initialize True Type Font
    ret = TTF_Init();
    if (ret != 0) {
        FATAL("TTF_Init failed" << endl);
    }

    const char * font0_path = "fonts/FreeMonoBold.ttf";         // normal 
    int      font0_ptsize = win_height / 18 - 1;
    const char * font1_path = "fonts/FreeMonoBold.ttf";         // extra large
    int      font1_ptsize = win_height / 13 - 1;

    font[0].font = TTF_OpenFont(font0_path, font0_ptsize);
    if (ret != 0) {
        FATAL("TTF_OpenFont failed" << endl);
    }
    TTF_SizeText(font[0].font, "X", &font[0].char_width, &font[0].char_height);
    INFO("font0 ptsize=" << font0_ptsize << 
         " width=" << font[0].char_width << 
         " height=" << font[0].char_height<< 
         endl);

    font[1].font = TTF_OpenFont(font1_path, font1_ptsize);
    if (ret != 0) {
        FATAL("TTF_OpenFont failed" << endl);
    }
    TTF_SizeText(font[1].font, "X", &font[1].char_width, &font[1].char_height);
    INFO("font1 ptsize=" << font1_ptsize << 
         " width=" << font[1].char_width << 
         " height=" << font[1].char_height << 
         endl);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

#if 0
// xxxxxxxxxxxxxxxxxxxxxx test
    // init pixels
    start();

    char * pixels = new char[1000000];
    memset(pixels,0,1000000);
    memset(pixels+500000, 4, 10000);

    // creae surface from pixels
    SDL_Surface * surface;
    surface = SDL_CreateRGBSurfaceFrom(pixels, 
                                       1000, 1000, 8, 1000, // width, height, depth, pitch
                                        0, 0, 0, 0);         // RGBA masks, not used
    cout << "surface " << surface << endl;

    // set surface palette
    SDL_Palette palette;
    SDL_Color colors[256] = { { 255,0,0,        255 },
                            { 0,255,0,        255 },
                            { 0,0,255,        255 },
                            { 255,255,255,    255 },
                            { 0,0,0,          255 },
                                    };
    palette.ncolors = 256;
    palette.colors = colors;
    ret = SDL_SetSurfacePalette(surface, &palette);
    cout << "set palette " << ret << " " << SDL_GetError() << endl;

    // create texture from surface
    SDL_Texture * texture;
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    cout << "texture " << texture << endl;

    //render copy
    ret = SDL_RenderCopy(renderer, texture, 
                         NULL,    // srcrect
                         NULL);   // dstrect
    cout << "render copy " << ret << endl;

    finish();
// xxxxxxxxxxxxxxxxxxxxxx end test
#endif
}

display::~display()
{
    INFO("destructor" << endl);

    Mix_FreeChunk(event_sound);
    Mix_CloseAudio();

    TTF_CloseFont(font[0].font);
    TTF_CloseFont(font[1].font);
    TTF_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// -----------------  DISPLAY START AND FINISH  ----------------------------------------

void display::start()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    pane[0].x = 0;
    pane[0].y = 0;
    pane[0].w = win_width;
    pane[0].h = win_height;

    max_pane = 1;
    max_eid = 0;
}

void display::start(int x1, int y1, int w1, int h1)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    pane[0].x = 0;
    pane[0].y = 0;
    pane[0].w = win_width;
    pane[0].h = win_height;

    pane[1].x = x1;
    pane[1].y = y1;
    pane[1].w = w1;
    pane[1].h = h1;

    max_pane = 2;
    max_eid = 0;
}

void display::start(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    pane[0].x = 0;
    pane[0].y = 0;
    pane[0].w = win_width;
    pane[0].h = win_height;

    pane[1].x = x1;
    pane[1].y = y1;
    pane[1].w = w1;
    pane[1].h = h1;

    pane[2].x = x2;
    pane[2].y = y2;
    pane[2].w = w2;
    pane[2].h = h2;

    max_pane = 3;
    max_eid = 0;
}

void display::finish()
{
    SDL_RenderPresent(renderer);
}

// -----------------  DRAWING  ---------------------------------------------------------

void display::set_color(enum color color)
{
    static const int red[] = {
       255,     // RED         
       255,     // ORANGE
       255,     // YELLOW
       0,       // GREEN
       0,       // BLUE
       127,     // PURPLE
       0,       // BLACK
       255,     // WHITE       
       224,     // GRAY        
       255,     // PINK 
       0,       // LIGHT_BLUE
            };

    static const int green[] = {
       0,       // RED         
       128,     // ORANGE
       255,     // YELLOW
       255,     // GREEN
       0,       // BLUE
       0,       // PURPLE
       0,       // BLACK
       255,     // WHITE       
       224,     // GRAY        
       105,     // PINK 
       255,     // LIGHT_BLUE
            };

    static const int blue[] = {
       0,       // RED         
       0,       // ORANGE
       0,       // YELLOW
       0,       // GREEN
       255,     // BLUE
       255,     // PURPLE
       0,       // BLACK
       255,     // WHITE       
       224,     // GRAY        
       180,     // PINK 
       255,     // LIGHT_BLUE
            };

    assert(color >= RED && color <= LIGHT_BLUE);
    SDL_SetRenderDrawColor(renderer, red[color], green[color], blue[color], SDL_ALPHA_OPAQUE);
}

void display::draw_point(int x, int y, int pid)
{
    assert(pid >= 0 && pid < max_pane);
    struct pane &p = pane[pid];

    SDL_RenderDrawPoint(renderer, x + p.x, y + p.y);
}

void display::draw_line(int x1, int y1, int x2, int y2, int pid)
{
    assert(pid >= 0 && pid < max_pane);
    struct pane &p = pane[pid];

    SDL_RenderDrawLine(renderer, x1 + p.x, y1 + p.y, x2 + p.x, y2 + p.y);
}

void display::draw_rect(int x, int y, int w, int h, int pid, int line_width)
{
    assert(pid >= 0 && pid < max_pane);
    struct pane &p = pane[pid];
    SDL_Rect rect;
    int32_t i;

    rect.x = x + p.x;
    rect.y = y + p.y;
    rect.w = w;
    rect.h = h;

    for (i = 0; i < line_width; i++) {
        SDL_RenderDrawRect(renderer, &rect);
        if (rect.w < 2 || rect.h < 2) {
            break;
        }
        rect.x += 1;
        rect.y += 1;
        rect.w -= 2;
        rect.h -= 2;
    }
}

void display::draw_filled_rect(int x, int y, int w, int h, int pid)
{
    assert(pid >= 0 && pid < max_pane);
    struct pane &p = pane[pid];
    SDL_Rect rect;

    rect.x = x + p.x;
    rect.y = y + p.y;
    rect.w = w;
    rect.h = h;
    SDL_RenderFillRect(renderer, &rect);
}

void display::draw_circle(int x, int y, int r, int pid)
{
    // XXX
}

void display::draw_filled_circle(int x, int y, int r, int pid)
{
    // XXX
}

int display::draw_text(std::string str, int row, int col, int pid, bool evreg,
                        int fid, bool center, int field_cols)
{
    SDL_Surface    * surface = NULL;
    SDL_Texture    * texture = NULL;
    SDL_Color        fg_color;
    SDL_Rect         pos;
    int              eid = EID_NONE;

    static const SDL_Color fg_color_normal = {255,255,255,255}; 
    static const SDL_Color fg_color_event  = {0,255,255,255}; 
    static const SDL_Color bg_color        = {0,0,0,255}; 

    // if zero length string then nothing to do
    if (str.length() == 0) {
        return EID_NONE;;
    }

    // if row or col are less than zero then adjust 
    if (row < 0) {
        row += get_pane_rows(pid,fid);
    }
    if (col < 0) {
        col += get_pane_cols(pid,fid);
    }

    // verify row, col, and field_cold
    if (row < 0 || row >= get_pane_rows(pid,fid) || 
        col < 0 || col >= get_pane_cols(pid,fid) ||
        field_cols <= 0) 
    {
        return EID_NONE;
    }

    // reduce field_cols if necessary to stay in pane
    if (field_cols > get_pane_cols(pid,fid) - col) {
         field_cols = get_pane_cols(pid,fid) - col;
    }

    // if centered then adjust col
    if (center && str.length() < static_cast<unsigned>(field_cols)) {
        col += (field_cols - str.length()) / 2;
    }

    // render the text to a surface, truncate str to at most field_cols
    fg_color = (evreg ? fg_color_event : fg_color_normal); 
    surface = TTF_RenderText_Shaded(font[fid].font, str.substr(0,field_cols).c_str(), fg_color, bg_color);
    if (surface == NULL) { 
        FATAL("TTF_RenderText_Shaded returned NULL" << endl);
    } 

    // determine the display location
    pos.x = pane[pid].x + col * font[fid].char_width;
    pos.y = pane[pid].y + row * font[fid].char_height;
    pos.w = surface->w;
    pos.h = surface->h;

    // create texture from the surface, and 
    // render the texture
    texture = SDL_CreateTextureFromSurface(renderer, surface); 
    SDL_RenderCopy(renderer, texture, NULL, &pos); 

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
    if (evreg) {
        // restore pos.x and pos.y to pane base,
        // and provide a 1/2 character larger area
        pos.x -= (pane[pid].x + font[fid].char_width/2);
        pos.y -= (pane[pid].y + font[fid].char_height/2);
        pos.w += font[fid].char_width;
        pos.h += font[fid].char_height;

        // register for the mouse click event
        eid = event_register(ET_MOUSE_CLICK, pid, pos.x, pos.y, pos.w, pos.h);
    }

    // return the registered event id or EID_NONE
    return eid;
}

struct display::image * display::create_image(unsigned char * pixels, int w, int h)
{
    // creae surface from pixels
    SDL_Surface * surface;
    surface = SDL_CreateRGBSurfaceFrom(pixels, 
                                       1000, 1000, 8, 1000, // width, height, depth, pitch
                                        0, 0, 0, 0);         // RGBA masks, not used
    cout << "surface " << surface << endl;

    // set surface palette
    SDL_Palette palette;  // XXX needs work
    SDL_Color colors[256] = { { 255,0,0,        255 },
                            { 0,255,0,        255 },
                            { 0,0,255,        255 },
                            { 255,255,255,    255 },
                            { 0,0,0,          255 },
                                    };
    palette.ncolors = 256;
    palette.colors = colors;
    int ret = SDL_SetSurfacePalette(surface, &palette);
    cout << "set palette " << ret << " " << SDL_GetError() << endl;

    // create texture from surface
    SDL_Texture * texture;
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    cout << "texture " << texture << endl;

    // return the texture
    return reinterpret_cast<struct image *>(texture);
}

void display::destroy_image(struct image * img)
{
    SDL_DestroyTexture(reinterpret_cast<SDL_Texture*>(img));
}

void display::draw_image(struct image * img, int pid)
{
    draw_image(img, 0, 0, pane[pid].w, pane[pid].h, pid);
}

void display::draw_image(struct image * img, int x, int y, int w, int h, int pid)
{
    int ret;
    SDL_Rect dstrect;

    //render copy
    dstrect.x = x + pane[pid].x;
    dstrect.y = y + pane[pid].y;
    dstrect.w = w;
    dstrect.h = h;
    ret = SDL_RenderCopy(renderer, reinterpret_cast<SDL_Texture*>(img), NULL, &dstrect);
    // XXX assert on the ret
    // XXX cout << "render copy " << ret << endl;
}

// -----------------  EVENT HANDLING  --------------------------------------------------

int display::event_register(enum event_type et, int pid, int x, int y, int w, int h)
{
    assert(pid >= 0 && pid < max_pane);
    assert(max_eid < MAX_EID);

#if 0
    assert(x >= 0);
    assert(y >= 0);
    assert(w > 0);
    assert(h > 0);
    assert(x + w <= pane[pid].w);  // XXX maybe reduce instead
    assert(y + h <= pane[pid].h);
#endif

    eid_tbl[max_eid].et =  et;
    eid_tbl[max_eid].x  = x + pane[pid].x;
    eid_tbl[max_eid].y  = y + pane[pid].y;
    eid_tbl[max_eid].w  = w;
    eid_tbl[max_eid].h  = h;

    max_eid++;

    return max_eid-1;
}

struct display::event display::poll_event()
{
    #define EID_TBL_POS_MATCH(_x,_y,_eid) (((_x) >= eid_tbl[_eid].x) && \
                                           ((_x) < eid_tbl[_eid].x + eid_tbl[_eid].w) && \
                                           ((_y) >= eid_tbl[_eid].y) && \
                                           ((_y) < eid_tbl[_eid].y + eid_tbl[_eid].h))

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
            mouse_button_motion_eid = EID_NONE; \
            mouse_button_x = 0; \
            mouse_button_y = 0; \
        } while (0)

    SDL_Event sdl_event;
    struct event event;

    bzero(&event, sizeof(event));
    event.eid = EID_NONE;

    while (true) {
        // get the next event, break out of loop if no event
        // note - this is return path for 'no event occurred'
        if (SDL_PollEvent(&sdl_event) == 0) {
            break;
        }

        // process the SDL event, this code
        // - sets event, and
        // - updates win_width, win_height, win_minimized
        switch (sdl_event.type) {
        case SDL_MOUSEBUTTONDOWN: {
            DEBUG("MOUSE DOWN which=" << sdl_event.button.which << 
                  " button=" << (sdl_event.button.button == SDL_BUTTON_LEFT   ? "LEFT" :
                                 sdl_event.button.button == SDL_BUTTON_MIDDLE ? "MIDDLE" :
                                 sdl_event.button.button == SDL_BUTTON_RIGHT  ? "RIGHT" :
                                                                         "???") <<
                  " state=" << (sdl_event.button.state == SDL_PRESSED  ? "PRESSED" :
                                sdl_event.button.state == SDL_RELEASED ? "RELEASED" :
                                                                  "???") <<
                  " x=" << sdl_event.button.x <<
                  " y=" << sdl_event.button.y << 
                  endl);

            // if not the left button then get out
            if (sdl_event.button.button != SDL_BUTTON_LEFT) {
                break;
            }

            // reset mouse_button_state
            MOUSE_BUTTON_STATE_RESET;

            // check for mouse-click event
            for (int eid = 0; eid < max_eid; eid++) {
                if (eid_tbl[eid].et == ET_MOUSE_CLICK &&
                    EID_TBL_POS_MATCH(sdl_event.button.x, sdl_event.button.y, eid)) 
                {
                    event.eid = eid;
                    event.val1 = 0; // not used for mouse click event
                    event.val2 = 0; 
                    break;
                }
            }

            // if the above found a match in the eid_tbl then 
            // play event sound, and we are done
            if (event.eid != EID_NONE) {
                play_event_sound();
                break;
            }

            // it is not a text event, so set MOUSE_BUTTON_STATE_DOWN
            mouse_button_state = MOUSE_BUTTON_STATE_DOWN;
            mouse_button_x = sdl_event.button.x;
            mouse_button_y = sdl_event.button.y;
            break; }

        case SDL_MOUSEBUTTONUP: {
            DEBUG("MOUSE UP which=" << sdl_event.button.which << 
                  " button=" << (sdl_event.button.button == SDL_BUTTON_LEFT   ? "LEFT" :
                                 sdl_event.button.button == SDL_BUTTON_MIDDLE ? "MIDDLE" :
                                 sdl_event.button.button == SDL_BUTTON_RIGHT  ? "RIGHT" :
                                                                         "???") <<
                  " state=" << (sdl_event.button.state == SDL_PRESSED  ? "PRESSED" :
                                sdl_event.button.state == SDL_RELEASED ? "RELEASED" :
                                                                  "???") <<
                  " x=" << sdl_event.button.x <<
                  " y=" << sdl_event.button.y << 
                  endl);

            // if not the left button then get out
            if (sdl_event.button.button != SDL_BUTTON_LEFT) {
                break;
            }

            // reset mouse_button_state,
            // this is where MOUSE_BUTTON_STATE_MOTION state is exitted
            MOUSE_BUTTON_STATE_RESET;
            break; }

        case SDL_MOUSEMOTION: {
            // if MOUSE_BUTTON_STATE_NONE then get out
            if (mouse_button_state == MOUSE_BUTTON_STATE_NONE) {
                break;
            }

            // if in MOUSE_BUTTON_STATE_DOWN then check for mouse motion event; and if so
            // then set state to MOUSE_BUTTON_STATE_MOTION
            if (mouse_button_state == MOUSE_BUTTON_STATE_DOWN) {
                for (int eid = 0; eid < max_eid; eid++) {
                    if (eid_tbl[eid].et == ET_MOUSE_MOTION &&
                        EID_TBL_POS_MATCH(sdl_event.motion.x, sdl_event.motion.y, eid)) 
                    {
                        mouse_button_state = MOUSE_BUTTON_STATE_MOTION;
                        mouse_button_motion_eid = eid;
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
            // XXX what if pos left the pane
            event.eid = mouse_button_motion_eid;
            event.val1 = 0;  // delta_x
            event.val2 = 0;  // delta_y
            do {
                event.val1 += sdl_event.motion.x - mouse_button_x;
                event.val2 += sdl_event.motion.y - mouse_button_y;
                mouse_button_x = sdl_event.motion.x;
                mouse_button_y = sdl_event.motion.y;
            } while (SDL_PeepEvents(&sdl_event, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION) == 1);
            break; }

        case SDL_MOUSEWHEEL: {
            int mouse_x, mouse_y, eid;

            // check if mouse wheel event is registered for the location of the mouse;
            // if did not find a registered mouse wheel event then get out
            SDL_GetMouseState(&mouse_x, &mouse_y);
            for (eid = 0; eid < max_eid; eid++) {
                if (eid_tbl[eid].et == ET_MOUSE_WHEEL && EID_TBL_POS_MATCH(mouse_x, mouse_y, eid)) {
                    break;
                }
            }
            if (eid == max_eid) {
                break;
            }

            // set return event
            event.eid = eid;
            event.val1 = sdl_event.wheel.x;  // dela_x
            event.val2 = sdl_event.wheel.y;  // delta_y
            break; }

        case SDL_KEYDOWN: {
            int  key   = sdl_event.key.keysym.sym;
            //bool shift = (sdl_event.key.keysym.mod & KMOD_SHIFT) != 0;
            bool ctrl  = (sdl_event.key.keysym.mod & KMOD_CTRL) != 0;
            int  eid;
            int  val1;

            // process printscreen (^p)
            if (ctrl && key == 'p') {
                print_screen();
                play_event_sound();
                break;
            }

            // if ET_KEYBOARD is not registered then we're done
            for (eid = 0; eid < max_eid; eid++) {
                if (eid_tbl[eid].et == ET_KEYBOARD) {
                    break;
                }
            }
            if (eid == max_eid) {
                break;
            }

            // determine value to return with the event
            // XXX handle shift 
            if (key < 128) {
                val1 = key;
            } else if (key == SDLK_HOME) {
                val1 = KEY_HOME;
            } else if (key == SDLK_END) {
                val1 = KEY_END;
            } else if (key == SDLK_PAGEUP) {
                val1= KEY_PGUP;
            } else if (key == SDLK_PAGEDOWN) {
                val1 = KEY_PGDN;
            }

            // fill in the event, and play event sound
            event.eid = eid;
            event.val1 = val1;
            event.val2 = 0;
            play_event_sound();
            break; }

       case SDL_WINDOWEVENT: {
            DEBUG("got event SDL_WINOWEVENT - " << SDL_WINDOWEVENT_STR(sdl_event.window.event) << endl);

            // get event_type
            enum event_type et = ET_NONE;
            switch (sdl_event.window.event)  {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                win_width  = sdl_event.window.data1;
                win_height = sdl_event.window.data2;
                et = ET_WIN_SIZE_CHANGE;
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
                win_minimized = true;
                et = ET_WIN_MINIMIZED;
                break;
            case SDL_WINDOWEVENT_RESTORED:
                win_minimized = false;
                et = ET_WIN_RESTORED;
                break;
            }
            if (et == ET_NONE) {
                break;
            }

            // find eid that has been registered for the event_type
            int eid;
            for (eid = 0; eid < max_eid; eid++) {
                if (eid_tbl[eid].et == et) {
                    break;
                }
            }
            if (eid == max_eid) {
                break;
            }

            // fill in the event, and play event sound
            event.eid = eid;
            event.val1 = 0;  // not used
            event.val2 = 0;  // not used
            if (et == ET_WIN_MINIMIZED || et == ET_WIN_RESTORED) {
                play_event_sound();
            }
            break; }

        case SDL_QUIT: {
            DEBUG("got event SDL_QUIT" << endl);

            // find eid that has been registered for ET_QUIT
            int eid;
            for (eid = 0; eid < max_eid; eid++) {
                if (eid_tbl[eid].et == ET_QUIT) {
                    break;
                }
            }
            if (eid == max_eid) {
                break;
            }

            // fill in the event, and play event sound
            event.eid = eid;
            event.val1 = 0;  // not used
            event.val2 = 0;  // not used
            play_event_sound();
            break; }

        default: {
            DEBUG("got unsupported event " << sdl_event.type << endl);
            break; }
        }

        // break if event is set,
        // note: this is return path for 'an event occurred'
        if (event.eid != EID_NONE) {
            break; 
        }
    }

    return event;
}

void display::play_event_sound(void)
{
    Mix_PlayChannel(-1, event_sound, 0); 
}

void display::print_screen(void)
{
    int   (*pixels)[4096] = NULL;
    FILE       *fp = NULL;
    SDL_Rect    rect;
    int    *row_pointers[4096];
    char        file_name[1000];
    png_structp png_ptr;
    png_infop   info_ptr;
    png_byte    color_type, bit_depth;
    int     y, ret;
    time_t      t;
    struct tm   tm;

    // allocate and read the pixels
    pixels = reinterpret_cast<int(*)[4096]>(calloc(1, win_height*sizeof(pixels[0])));
    if (pixels == NULL) {
        ERROR("allocate pixels failed" << endl);
        goto done;
    }

    rect.x = 0;
    rect.y = 0;
    rect.w = win_width;
    rect.h = win_height;   
    ret = SDL_RenderReadPixels(renderer, &rect, SDL_PIXELFORMAT_ABGR8888, pixels, sizeof(pixels[0]));
    if (ret < 0) {
        ERROR("SDL_RenderReadPixels" << SDL_GetError() << endl);
        goto done;
    }

    // creeate the file_name using localtime
    t = time(NULL);
    localtime_r(&t, &tm);
    sprintf(file_name, "display_%2.2d%2.2d%2.2d_%2.2d%2.2d%2.2d.png",
            tm.tm_year - 100, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);

    // write pixels to file_name ...

    // - init
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    bit_depth = 8;
    for (y = 0; y < win_height; y++) {
        row_pointers[y] = pixels[y];
    }

    // - create file 
    fp = fopen(file_name, "wb");
    if (!fp) {
        ERROR("fopen" << file_name << endl);
        goto done;  
    }

    // - initialize stuff 
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        ERROR("png_create_write_struct" << endl);
        goto done;  
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        ERROR("png_create_info_struct" << endl);
        goto done;  
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        ERROR("init_io failed" << endl);
        goto done;  
    }
    png_init_io(png_ptr, fp);

    // - write header 
    if (setjmp(png_jmpbuf(png_ptr))) {
        ERROR("writing header" << endl);
        goto done;  
    }
    png_set_IHDR(png_ptr, info_ptr, win_width, win_height,
                 bit_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);

    // - write bytes 
    if (setjmp(png_jmpbuf(png_ptr))) {
        ERROR("writing bytes" << endl);
        goto done;  
    }
    png_write_image(png_ptr, reinterpret_cast<png_bytepp>(row_pointers));

    // - end write 
    if (setjmp(png_jmpbuf(png_ptr))) {
        ERROR("end of write" << endl);
        goto done;  
    }
    png_write_end(png_ptr, NULL);

done:
    // clean up and return
    if (fp != NULL) {
        fclose(fp);
    }
    free(pixels);
}

