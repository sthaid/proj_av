#include <cassert>

#include "display.h"
#include "event_sound.h"
#include "logging.h"

extern "C" {
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <png.h>
}

static const SDL_Color colors[256] = {
    { 255,   0,   0, 255 },    // RED         
    { 255, 128,   0, 255 },    // ORANGE
    { 255, 255,   0, 255 },    // YELLOW
    {   0, 255,   0, 255 },    // GREEN
    {   0,   0, 255, 255 },    // BLUE
    { 127,   0, 255, 255 },    // PURPLE
    {   0,   0,   0, 255 },    // BLACK
    { 255, 255, 255, 255 },    // WHITE       
    { 224, 224, 224, 255 },    // GRAY        
    { 255, 105, 180, 255 },    // PINK 
    {   0, 255, 255, 255 },    // LIGHT_BLUE
    {   0,   0,   0,   0 },    // TRANSPARENT
        };

// SDL_PIXELFORMAT_ARGB8888
static const unsigned int raw_pixel[256] = {
    0xffff0000,                // RED         
    0xffff8000,                // ORANGE
    0xffffff00,                // YELLOW
    0xff00ff00,                // GREEN
    0xff0000ff,                // BLUE
    0xff7f00ff,                // PURPLE
    0xff000000,                // BLACK
    0xffffffff,                // WHITE       
    0xffe0e0e0,                // GRAY        
    0xffff69b4,                // PINK 
    0xff00ffff,                // LIGHT_BLUE
    0x00000000,                // TRANSPARENT
        };

// -----------------  CONSTRUCTOR & DESTRUCTOR  ----------------------------------------

display::display(int w, int h, bool resizeable)
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
    mouse_motion_eid = EID_NONE;
    mouse_motion_x = 0;
    mouse_motion_y = 0;
    event_sound = NULL;

    // initialize Simple DirectMedia Layer  (SDL)
    ret = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
    if (ret != 0) {
        FATAL("SDL_Init failed" << endl);
    }

    // create SDL Window and Renderer
    ret = SDL_CreateWindowAndRenderer(w, h, 
                                      resizeable ? SDL_WINDOW_RESIZABLE : 0, 
                                      &window, &renderer);
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

    // xxx configure font size
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

    // clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
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

void display::start(int x0, int y0, int w0, int h0)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    pane[0].x = x0;
    pane[0].y = y0;
    pane[0].w = w0;
    pane[0].h = h0;

    max_pane = 1;
    max_eid = 0;
}

void display::start(int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    pane[0].x = x0;
    pane[0].y = y0;
    pane[0].w = w0;
    pane[0].h = h0;

    pane[1].x = x1;
    pane[1].y = y1;
    pane[1].w = w1;
    pane[1].h = h1;

    max_pane = 2;
    max_eid = 0;
}

void display::start(int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1,
                    int x2, int y2, int w2, int h2)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    pane[0].x = x0;
    pane[0].y = y0;
    pane[0].w = w0;
    pane[0].h = h0;

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

void display::start(int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1,
                    int x2, int y2, int w2, int h2, int x3, int y3, int w3, int h3)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    pane[0].x = x0;
    pane[0].y = y0;
    pane[0].w = w0;
    pane[0].h = h0;

    pane[1].x = x1;
    pane[1].y = y1;
    pane[1].w = w1;
    pane[1].h = h1;

    pane[2].x = x2;
    pane[2].y = y2;
    pane[2].w = w2;
    pane[2].h = h2;

    pane[3].x = x3;
    pane[3].y = y3;
    pane[3].w = w3;
    pane[3].h = h3;

    max_pane = 4;
    max_eid = 0;
}

void display::start(int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1,
                    int x2, int y2, int w2, int h2, int x3, int y3, int w3, int h3,
                    int x4, int y4, int w4, int h4)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    pane[0].x = x0;
    pane[0].y = y0;
    pane[0].w = w0;
    pane[0].h = h0;

    pane[1].x = x1;
    pane[1].y = y1;
    pane[1].w = w1;
    pane[1].h = h1;

    pane[2].x = x2;
    pane[2].y = y2;
    pane[2].w = w2;
    pane[2].h = h2;

    pane[3].x = x3;
    pane[3].y = y3;
    pane[3].w = w3;
    pane[3].h = h3;

    pane[4].x = x4;
    pane[4].y = y4;
    pane[4].w = w4;
    pane[4].h = h4;

    max_pane = 5;
    max_eid = 0;
}

void display::finish()
{
    SDL_RenderPresent(renderer);
}

// -----------------  DRAWING  ---------------------------------------------------------

void display::draw_set_color(enum color color)
{
    assert(color >= RED && color <= LIGHT_BLUE);
    SDL_SetRenderDrawColor(renderer, colors[color].r, colors[color].g, colors[color].b, colors[color].a);
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
    int i;

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

// -----------------  TEXT  ------------------------------------------------------------

int display::text_draw(string str, int row, int col, int pid, bool evreg, int key_alias,
                        int fid, bool center, int field_cols)
{
    SDL_Surface    * surface = NULL;
    SDL_Texture    * texture = NULL;
    SDL_Rect         pos;
    int              eid = EID_NONE;

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
    surface = TTF_RenderText_Shaded(font[fid].font, str.substr(0,field_cols).c_str(), 
                                     evreg ? colors[LIGHT_BLUE] : colors[WHITE],
                                     colors[BLACK]);
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
        pos.x -= pane[pid].x;
        pos.y -= pane[pid].y;

        // register for the mouse click event
        eid = event_register(ET_MOUSE_LEFT_CLICK, pid, pos.x, pos.y, pos.w, pos.h, key_alias);
    }

    // return the registered event id or EID_NONE
    return eid;
}

// -----------------  TEXTURES  --------------------------------------------------------

struct display::texture * display::texture_create(int w, int h)
{
    unsigned char * pixels;
    texture * t;

    pixels = new unsigned char [w*h];
    memset(pixels,TRANSPARENT,w*h); 
    t = texture_create(pixels,w,h);
    delete [] pixels;    
    return t;
}

struct display::texture * display::texture_create(unsigned char * pixels, int w, int h)
{
    // creae surface from pixels
    SDL_Surface * surface;
    surface = SDL_CreateRGBSurfaceFrom(pixels, 
                                       w, h, 8, w,   // width, height, depth, pitch
                                       0, 0, 0, 0);  // RGBA masks, not used
    INFO("surface " << surface << endl);

    // set surface palette
    SDL_Palette * palette = SDL_AllocPalette(256);
    SDL_SetPaletteColors(palette, colors, 0, 256);
    int ret = SDL_SetSurfacePalette(surface, palette);
    INFO("set palette " << ret << " " << SDL_GetError() << endl);

    // create texture from surface
    SDL_Texture * texture;
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    INFO("texture " << texture << " " << SDL_GetError() << " renderer " << renderer << endl);

    // free the surface
    SDL_FreeSurface(surface); 

    ret = SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    INFO("xxx SET BLEND " << ret << " " << SDL_GetError() << endl);

    // assert that the texture has the expected pixel format
    unsigned int fmt;
    SDL_QueryTexture(texture, &fmt, NULL, NULL, NULL);                               
    assert(fmt == SDL_PIXELFORMAT_ARGB8888);

    // return the texture
    return reinterpret_cast<struct texture *>(texture);
}

void display::texture_set_pixel(struct texture * t, int x, int y, unsigned char pixel)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = 1;
    rect.h = 1;

    SDL_UpdateTexture(reinterpret_cast<SDL_Texture*>(t), &rect, &raw_pixel[pixel], 1);
}

void display::texture_clr_pixel(struct texture * t, int x, int y)
{
    texture_set_pixel(t, x, y, TRANSPARENT);
}

void display::texture_set_rect(struct texture * t, int x, int y, int w, int h, unsigned char * pixels, int pitch)
{
    unsigned int raw_pixels[20000];  // xxx
    unsigned int * rp = raw_pixels;
    SDL_Rect rect;

    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;

    for (y = 0; y < rect.h; y++) {
        for (x = 0; x < rect.w; x++) {
            *rp++ = raw_pixel[*pixels++];
        }
        pixels += (pitch - rect.w);
    }

    SDL_UpdateTexture(reinterpret_cast<SDL_Texture*>(t), &rect, raw_pixels, 4*w);
}

void display::texture_clr_rect(struct texture * t, int x, int y, int w, int h)
{
    static unsigned int transparent_pixels[20000];  // xxx

    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;

    SDL_UpdateTexture(reinterpret_cast<SDL_Texture*>(t), &rect, transparent_pixels, 4*w);
}

void display::texture_destroy(struct texture * t)
{
    if (t == NULL) {
        return;
    }
    SDL_DestroyTexture(reinterpret_cast<SDL_Texture*>(t));
}

void display::texture_draw(struct texture * t, int x, int y, int w, int h, int pid)
{
    SDL_Rect dstrect, srcrect;
    int tw, th;
    double f1, f2;

    SDL_QueryTexture(reinterpret_cast<SDL_Texture*>(t), NULL, NULL, &tw, &th);

    if (x >= 0 && x+w <= tw) {
        // okay
        f1 = 0;
        f2 = 1;
    } else if (x >= 0 && x < tw) {
        // off to the right
        f1 = 0;
        f2 = (double)(tw - x) / w;
    } else if (x < 0 && x + w <= tw) {
        // off to the left
        f1 = (double)(-x) / w;
        f2 = 1;
    } else if (x < 0 && x + w >= tw) {
        // off the left and right
        f1 = (double)(-x) / w;
        f2 = (double)(tw - x) / w;
    } else { 
        // won't be displayed
        f1 = 0;
        f2 = 1;
    }
    dstrect.x = f1 * pane[pid].w;
    dstrect.w = f2 * pane[pid].w - dstrect.x;
    dstrect.x += pane[pid].x;

    if (y >= 0 && y+h <= th) {
        // okay
        f1 = 0;
        f2 = 1;
    } else if (y >= 0 && y < th) {
        // off the botton
        f1 = 0;
        f2 = (double)(th - y) / h;
    } else if (y < 0 && y + h <= th) {
        // off the top
        f1 = (double)(-y) / h;
        f2 = 1;
    } else if (y < 0 && y + h >= th) {
        // off the top and bottom
        f1 = (double)(-y) / h;
        f2 = (double)(th - y) / h;
    } else { 
        // won't be displayed
        f1 = 0;
        f2 = 1;
    }
    dstrect.y = f1 * pane[pid].h;
    dstrect.h = f2 * pane[pid].h - dstrect.y;
    dstrect.y += pane[pid].y;

    srcrect.x = x;
    srcrect.y = y;
    srcrect.w = w;
    srcrect.h = h;

    SDL_RenderCopy(renderer, reinterpret_cast<SDL_Texture*>(t), &srcrect, &dstrect);
}

// -----------------  EVENTS  ----------------------------------------------------------

int display::event_register(enum event_type et, int pid)
{
    return event_register(et, pid, pane[pid].x, pane[pid].y, pane[pid].w, pane[pid].h);
}

int display::event_register(enum event_type et, int pid, int x, int y, int w, int h)
{
    return event_register(et, pid, x, y, w, h, 0);
}

int display::event_register(enum event_type et, int pid, int x, int y, int w, int h, int key_alias)
{
    assert(pid >= 0 && pid < max_pane);
    assert(max_eid < MAX_EID);

    if (x < 0) {
        x = 0;
    }
    if (x + w > pane[pid].w) {
        w = pane[pid].w - x;
    }
    if (y < 0) {
        y = 0;
    }
    if (y + h > pane[pid].h) {
        h = pane[pid].h - y;
    }

    eid_tbl[max_eid].et =  et;
    eid_tbl[max_eid].x  = x + pane[pid].x;
    eid_tbl[max_eid].y  = y + pane[pid].y;
    eid_tbl[max_eid].w  = w;
    eid_tbl[max_eid].h  = h;
    eid_tbl[max_eid].key_alias = key_alias;

    max_eid++;

    return max_eid-1;
}

struct display::event display::event_poll()
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
            INFO ("MOUSE DOWN which=" << sdl_event.button.which << 
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

            // if left button check then ...
            if (sdl_event.button.button == SDL_BUTTON_LEFT) {
                // check for registered left mouse click event
                for (int eid = 0; eid < max_eid; eid++) {
                    if (eid_tbl[eid].et == ET_MOUSE_LEFT_CLICK &&
                        EID_TBL_POS_MATCH(sdl_event.button.x, sdl_event.button.y, eid)) 
                    {
                        event.eid = eid;
                        event.val1 = sdl_event.button.x - eid_tbl[eid].x;
                        event.val2 = sdl_event.button.y - eid_tbl[eid].y;
                        break;
                    }
                }
                if (event.eid != EID_NONE) {
                    play_event_sound();
                    break;
                }

                // check for registered mouse motion event
                for (int eid = 0; eid < max_eid; eid++) {
                    if (eid_tbl[eid].et == ET_MOUSE_MOTION &&
                        EID_TBL_POS_MATCH(sdl_event.button.x, sdl_event.button.y, eid)) 
                    {
                        INFO("GOT MOUSE MOTION for eid " << eid << endl);
                        mouse_motion_eid = eid;
                        mouse_motion_x = sdl_event.button.x;
                        mouse_motion_y = sdl_event.button.y;
                        break;
                    }
                }

                // done with mouse click processing
                break;
            }

            // if right button then ...
            if (sdl_event.button.button == SDL_BUTTON_RIGHT) {
                // check for registered right mouse click event
                for (int eid = 0; eid < max_eid; eid++) {
                    if (eid_tbl[eid].et == ET_MOUSE_RIGHT_CLICK &&
                        EID_TBL_POS_MATCH(sdl_event.button.x, sdl_event.button.y, eid)) 
                    {
                        event.eid = eid;
                        event.val1 = sdl_event.button.x - eid_tbl[eid].x;
                        event.val2 = sdl_event.button.y - eid_tbl[eid].y;
                        break;
                    }
                }
                if (event.eid != EID_NONE) {
                    play_event_sound();
                    break;
                }

                // done with mouse click processing
                break;
            }
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

            // if left button is up then clear mouse motion
            if (sdl_event.button.button == SDL_BUTTON_LEFT) {
                mouse_motion_eid = EID_NONE;
                break;
            }

            // no processing for other mouse buttons up
            break; }

        case SDL_MOUSEMOTION: {
            // if mouse motion is not active then break
            if (mouse_motion_eid == EID_NONE) {
                break;
            }

            // get all additional pending mouse motion events, and sum the motion
            // xxx what if pos left the pane
            event.eid = mouse_motion_eid;
            event.val1 = 0;  // delta_x
            event.val2 = 0;  // delta_y
            do {
                event.val1 += sdl_event.motion.x - mouse_motion_x;
                event.val2 += sdl_event.motion.y - mouse_motion_y;
                mouse_motion_x = sdl_event.motion.x;
                mouse_motion_y = sdl_event.motion.y;
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
            event.val1 = sdl_event.wheel.x;  // delta_x
            event.val2 = sdl_event.wheel.y;  // delta_y
            break; }

        case SDL_KEYDOWN: {
            int  key   = sdl_event.key.keysym.sym;
            bool shift = (sdl_event.key.keysym.mod & KMOD_SHIFT) != 0;
            bool ctrl  = (sdl_event.key.keysym.mod & KMOD_CTRL) != 0;
            int  val1  = 0;
            int  eid;

            // process control chars
            if (ctrl) {
                // printscreen
                if (key == 'p') {
                    print_screen();
                    play_event_sound();
                }
                break;
            }

            // determine value to return with the event
            // note: the following keyboard chars are supported:
            //   a-z, A-Z, 0-9, space, home, end, pageup, pagedown
            if (key >= 0 && key <= 255) {
                if (islower(key)) {
                    val1 = (!shift ? key : toupper(key));
                } else if (isdigit(key) && !shift) {
                    val1 = key;
                } else if (isblank(key)) {
                    val1 = key;
                } else {
                    break; // not an event
                }
            } else if (key == SDLK_HOME) {
                val1 = KEY_HOME;
            } else if (key == SDLK_END) {
                val1 = KEY_END;
            } else if (key == SDLK_PAGEUP) {
                val1= KEY_PGUP;
            } else if (key == SDLK_PAGEDOWN) {
                val1 = KEY_PGDN;
            } else if (key == SDLK_UP) {
                val1 = KEY_UP;
            } else if (key == SDLK_DOWN) {
                val1 = KEY_DOWN;
            } else if (key == SDLK_RIGHT) {
                val1 = KEY_RIGHT;
            } else if (key == SDLK_LEFT) {
                val1 = KEY_LEFT;
            } else {
                break; // not an event
            }

            // search for eid with matching key_alias
            for (eid = 0; eid < max_eid; eid++) {
                if (eid_tbl[eid].key_alias == val1) {
                    break;
                }
            }

            // if eid with matching key_alias is not found then 
            // search for registered ET_KEYBOARD 
            if (eid == max_eid) {
                for (eid = 0; eid < max_eid; eid++) {
                    if (eid_tbl[eid].et == ET_KEYBOARD) {
                        break;
                    }
                }
            }

            // break out if no event found
            if (eid == max_eid) {
                break;
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

