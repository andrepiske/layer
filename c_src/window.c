#include "window.h"
#include "surface.h"
#include <string.h>

extern VALUE cLayer;
VALUE cLayerWindow;

#define DECLAREWND(o) \
  struct LAO_Window *wnd = (struct LAO_Window*)rb_data_object_get((o))

static void
t_wnd_gc_mark(struct LAO_Window *wnd) {
  if (wnd->ruby_keyboard_handler) {
    rb_gc_mark(wnd->ruby_keyboard_handler);
  }

  if (wnd->ruby_mouse_motion_handler) {
    rb_gc_mark(wnd->ruby_mouse_motion_handler);
  }

  if (wnd->ruby_mouse_button_handler) {
    rb_gc_mark(wnd->ruby_mouse_button_handler);
  }
}

static void
t_wnd_free(struct LAO_Window *wnd) {
  if (wnd->sdl_surface) {
    SDL_FreeSurface(wnd->sdl_surface);
    wnd->sdl_surface = 0;
  }
  if (wnd->cairo_surface) {
    cairo_surface_destroy(wnd->cairo_surface);
    wnd->cairo_surface = 0;
  }
  if (wnd->cairo_ctx) {
    cairo_destroy(wnd->cairo_ctx);
    wnd->cairo_ctx = 0;
  }
  if (wnd->sdl_wnd) {
    SDL_DestroyWindow(wnd->sdl_wnd);
    wnd->sdl_wnd = 0;
  }
  xfree(wnd);
}

static VALUE
t_wnd_allocator(VALUE klass) {
  struct LAO_Window *wnd = (struct LAO_Window*)xmalloc(sizeof(struct LAO_Window));
  wnd->sdl_wnd = 0;
  wnd->sdl_surface = 0;
  wnd->cairo_ctx = 0;
  wnd->cairo_surface = 0;
  return Data_Wrap_Struct(klass, t_wnd_gc_mark, t_wnd_free, wnd);
}

void
reallocate_wnd_buffers(struct LAO_Window *wnd) {
  if (wnd->cairo_surface) {
    cairo_surface_destroy(wnd->cairo_surface);
    wnd->cairo_surface = 0;
  }
  if (wnd->cairo_ctx) {
    cairo_destroy(wnd->cairo_ctx);
    wnd->cairo_ctx = 0;
  }
  if (wnd->sdl_surface) {
    SDL_FreeSurface(wnd->sdl_surface);
    wnd->sdl_surface = 0;
  }

  int width = 0, height = 0;
  SDL_GetWindowSize(wnd->sdl_wnd, &width, &height);

  SDL_Surface *sfc = SDL_CreateRGBSurfaceWithFormat(0,
    width, height, 32, SDL_PIXELFORMAT_BGRA32);

  SDL_SetSurfaceBlendMode(sfc, SDL_BLENDMODE_NONE);
  wnd->sdl_surface = sfc;

  // TODO: check SDL_MUSTLOCK on the image
  // SDL_LockSurface(sfc);
  wnd->cairo_surface = cairo_image_surface_create_for_data(
    sfc->pixels,
    CAIRO_FORMAT_ARGB32,
    sfc->w, sfc->h,
    sfc->pitch);

  wnd->cairo_ctx = cairo_create(wnd->cairo_surface);
}

static VALUE
t_wnd_initialize(VALUE self, VALUE _title, VALUE _width, VALUE _height) {
  DECLAREWND(self);

  Check_Type(_title, T_STRING);

  int width = NUM2INT(_width);
  int height = NUM2INT(_height);
  const char *title = StringValueCStr(_title);

  wnd->sdl_wnd = SDL_CreateWindow(title,
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    width, height,
    SDL_WINDOW_RESIZABLE
  );

  SDL_SetWindowData(wnd->sdl_wnd, "L", wnd);

  reallocate_wnd_buffers(wnd);

  return self;
}

////////////////////////////////////////////////////////////////////////

void
lao_wnd_handle_event(struct LAO_Window *wnd, SDL_Event *ev) {
  if (ev->type == SDL_WINDOWEVENT) {
    SDL_WindowEvent *win_ev = &ev->window;
    if (win_ev->event == SDL_WINDOWEVENT_RESIZED) {
      reallocate_wnd_buffers(wnd);
    }
  }

  else if (ev->type == SDL_MOUSEMOTION) {
    if (wnd->ruby_mouse_motion_handler) {

      rb_funcall(wnd->ruby_mouse_motion_handler, rb_intern("call"), 7,
        INT2NUM(ev->motion.x),
        INT2NUM(ev->motion.y),
        UINT2NUM(ev->motion.state),
        UINT2NUM(ev->motion.which),
        INT2NUM(ev->motion.xrel),
        INT2NUM(ev->motion.yrel),
        UINT2NUM(ev->motion.timestamp)
      );
    }
  }

  else if (ev->type == SDL_MOUSEBUTTONDOWN || ev->type == SDL_MOUSEBUTTONUP) {
    if (wnd->ruby_mouse_button_handler) {
      rb_funcall(wnd->ruby_mouse_button_handler, rb_intern("call"), 1, Qnil);
    }
  }
}

////////////////////////////////////////////////////////////////////////

static VALUE
t_wnd_show(VALUE self) {
  DECLAREWND(self);

  SDL_ShowWindow(wnd->sdl_wnd);
  SDL_RaiseWindow(wnd->sdl_wnd);

  return self;
}

static VALUE
t_wnd_flip_buffers(VALUE self) {
  DECLAREWND(self);
  SDL_Surface *win_sfc = SDL_GetWindowSurface(wnd->sdl_wnd);

  SDL_FillRect(win_sfc, 0, SDL_MapRGB(win_sfc->format, 0, 0, 0));
  SDL_BlitSurface(wnd->sdl_surface, 0, win_sfc, 0);
  SDL_UpdateWindowSurface(wnd->sdl_wnd);

  SDL_FillRect(wnd->sdl_surface, 0, 0xFF000000);
  cairo_identity_matrix(wnd->cairo_ctx);

  return self;
}

static VALUE
t_wnd_blit_surface(int argc, const VALUE *argv, VALUE self) {
  DECLAREWND(self);

  VALUE _x;
  VALUE _y;
  VALUE _sfc;
  VALUE _width;
  VALUE _height;

  rb_scan_args(argc, argv, "32", &_sfc, &_x, &_y, &_width, &_height);

  struct LAO_Surface *sfc = (struct LAO_Surface*)rb_data_object_get((_sfc));

  double x = NUM2DBL(_x);
  double y = NUM2DBL(_y);

  cairo_t *cr = wnd->cairo_ctx;

  // cairo_set_source_rgba(cr, 0x88 / 255.0, 0xd9 / 255.0, 0xde / 255.0, 1.0); // #88d9de #ded988
  // cairo_rectangle(cr, (double)x, (double)y, 128.0, 128.0);
  // cairo_fill(cr);

  if (!sfc->cairo_surface) {
    return Qfalse;
  }

  int sfc_width = cairo_image_surface_get_width(sfc->cairo_surface);
  int sfc_height = cairo_image_surface_get_height(sfc->cairo_surface);

  cairo_matrix_t matrix;
  if (_width != Qnil && _height != Qnil) {
    cairo_scale(cr, NUM2DBL(_width) / (double)sfc_width, NUM2DBL(_height) / (double)sfc_height);
    cairo_get_matrix(cr, &matrix);
  }

  cairo_set_source_surface(cr,
    sfc->cairo_surface, x, y
  );
  cairo_rectangle(cr, x, y, sfc_width, sfc_height);
  cairo_fill(cr);

  if (_width != Qnil && _height != Qnil) {
    cairo_set_matrix(cr, &matrix);
  }

  return Qtrue;
}

static VALUE
t_wnd_to_surface(VALUE self) {
  DECLAREWND(self);
  return lao_sfc_create_borrowed(wnd->sdl_surface, wnd->cairo_surface, wnd->cairo_ctx);
}

static VALUE g_sym_keyboard;
static VALUE g_sym_mouse_motion;
static VALUE g_sym_mouse_button;

static VALUE
t_wnd_on(VALUE self, VALUE on_what) {
  DECLAREWND(self);
  rb_need_block();

  VALUE blk = rb_block_proc();

  if (on_what == g_sym_keyboard) {
    wnd->ruby_keyboard_handler = blk;
  }
  else if (on_what == g_sym_mouse_motion) {
    wnd->ruby_mouse_motion_handler = blk;
  }
  else if (on_what == g_sym_mouse_button) {
    wnd->ruby_mouse_button_handler = blk;
  }

  return self;
}

////////////////////////////////////////////////////////////////////////

void
LAO_Window_Init() {
  g_sym_keyboard = ID2SYM(rb_intern("keyboard"));
  g_sym_mouse_motion = ID2SYM(rb_intern("mouse_motion"));
  g_sym_mouse_button = ID2SYM(rb_intern("mouse_button"));

  cLayerWindow = rb_define_class_under(cLayer, "Window", rb_cObject);
  rb_define_alloc_func(cLayerWindow, t_wnd_allocator);

  rb_define_method(cLayerWindow, "initialize", t_wnd_initialize, 3);
  rb_define_method(cLayerWindow, "show", t_wnd_show, 0);
  rb_define_method(cLayerWindow, "blit_surface", t_wnd_blit_surface, -1);
  rb_define_method(cLayerWindow, "flip_buffers", t_wnd_flip_buffers, 0);
  rb_define_method(cLayerWindow, "to_surface", t_wnd_to_surface, 0);
  rb_define_method(cLayerWindow, "on", t_wnd_on, 1);
}
