#include "window.h"
#include "surface.h"
#include <string.h>

extern VALUE cLayer;
VALUE cLayerWindow;

static VALUE g_sym_keyboard;
static VALUE g_sym_mouse_motion;
static VALUE g_sym_mouse_button;

static VALUE g_sym_shift;
static VALUE g_sym_ctrl;
static VALUE g_sym_alt;
static VALUE g_sym_cmd;

#define DECLAREWND(o) \
  struct LAO_Window *wnd; \
  TypedData_Get_Struct((o), struct LAO_Window, &lao_window_datatype, (wnd));

static void
t_wnd_gc_mark(void *_wnd) {
  struct LAO_Window *wnd = (struct LAO_Window*)_wnd;

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
t_wnd_free(void *_wnd) {
  struct LAO_Window *wnd = (struct LAO_Window*)_wnd;

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

static size_t
t_wnd_data_size(const void *_wnd) {
  return 0;
}

static const rb_data_type_t lao_window_datatype = {
  .wrap_struct_name = "Layer::Window",
  .function = {
    .dmark = t_wnd_gc_mark,
    .dfree = t_wnd_free,
    .dsize = t_wnd_data_size,
  },
  .flags = RUBY_TYPED_FREE_IMMEDIATELY
};


static VALUE
t_wnd_allocator(VALUE klass) {
  struct LAO_Window *wnd = (struct LAO_Window*)xmalloc(sizeof(struct LAO_Window));
  memset(wnd, 0, sizeof(struct LAO_Window));

  return TypedData_Wrap_Struct(klass, &lao_window_datatype, wnd);
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
      rb_funcall(wnd->ruby_mouse_button_handler, rb_intern("call"), 7,
        INT2NUM(ev->button.x),
        INT2NUM(ev->button.y),
        INT2NUM(ev->button.button),
        UINT2NUM(ev->button.state),
        INT2NUM(ev->button.clicks),
        INT2NUM(ev->button.which),
        UINT2NUM(ev->button.timestamp)
      );
    }
  }

  else if (ev->type == SDL_KEYDOWN || ev->type == SDL_KEYUP) {
    if (wnd->ruby_keyboard_handler) {
      rb_funcall(wnd->ruby_keyboard_handler, rb_intern("call"), 6,
        INT2NUM(ev->key.state),
        INT2NUM(ev->key.repeat),
        INT2NUM(ev->key.keysym.scancode),
        INT2NUM(ev->key.keysym.sym),
        INT2NUM(ev->key.keysym.mod),
        UINT2NUM(ev->key.timestamp)
      );
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
t_wnd_to_surface(VALUE self) {
  DECLAREWND(self);
  return lao_sfc_create_borrowed(wnd->sdl_surface, wnd->cairo_surface, wnd->cairo_ctx);
}

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
  else {
    rb_raise(rb_eArgError, "Invalid argument passed to 'on'");
  }

  return self;
}

static VALUE
t_wnd_size(VALUE self) {
  DECLAREWND(self);
  int w = -1, h = -1;
  SDL_GetWindowSizeInPixels(wnd->sdl_wnd, &w, &h);
  return rb_ary_new_from_args(2, INT2NUM(w), INT2NUM(h));
}

static VALUE
t_modf_keys(VALUE _) {
  int numkeys = 0;
  const Uint8 *keys = SDL_GetKeyboardState(&numkeys);
  if (numkeys < 228 || !numkeys || !keys) {
    // TODO: raise error?
    return Qnil;
  }

  VALUE r = rb_hash_new_capa(4);
  rb_hash_aset(r, g_sym_shift, keys[225] ? Qtrue : Qfalse);
  rb_hash_aset(r, g_sym_ctrl, keys[224] ? Qtrue : Qfalse);
  rb_hash_aset(r, g_sym_alt, keys[226] ? Qtrue : Qfalse);
  rb_hash_aset(r, g_sym_cmd, keys[227] ? Qtrue : Qfalse);
  return r;
}

////////////////////////////////////////////////////////////////////////

void
LAO_Window_Init() {
  g_sym_keyboard = ID2SYM(rb_intern("keyboard"));
  g_sym_mouse_motion = ID2SYM(rb_intern("mouse_motion"));
  g_sym_mouse_button = ID2SYM(rb_intern("mouse_button"));

  g_sym_shift = ID2SYM(rb_intern("shift"));
  g_sym_ctrl = ID2SYM(rb_intern("ctrl"));
  g_sym_alt = ID2SYM(rb_intern("alt"));
  g_sym_cmd = ID2SYM(rb_intern("cmd"));

  cLayerWindow = rb_define_class_under(cLayer, "Window", rb_cObject);
  rb_define_alloc_func(cLayerWindow, t_wnd_allocator);

  rb_define_method(cLayerWindow, "initialize", t_wnd_initialize, 3);
  rb_define_method(cLayerWindow, "show", t_wnd_show, 0);
  rb_define_method(cLayerWindow, "flip_buffers", t_wnd_flip_buffers, 0);
  rb_define_method(cLayerWindow, "to_surface", t_wnd_to_surface, 0);
  rb_define_method(cLayerWindow, "on", t_wnd_on, 1);
  rb_define_method(cLayerWindow, "size", t_wnd_size, 0);
  rb_define_method(cLayerWindow, "modf_keys", t_modf_keys, 0);
}
