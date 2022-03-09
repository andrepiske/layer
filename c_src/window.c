#include "window.h"
#include <string.h>

extern VALUE cLayer;
VALUE cLayerWindow;

#define DECLAREWND(o) \
  struct LAO_Window *wnd = (struct LAO_Window*)rb_data_object_get((o))

static void
t_wnd_gc_mark(struct LAO_Window *dv) {
}

static void
t_wnd_free(struct LAO_Window *dv) {
  xfree(dv);
}

static VALUE
t_wnd_allocator(VALUE klass) {
  struct LAO_Window *wnd = (struct LAO_Window*)xmalloc(sizeof(struct LAO_Window));
  wnd->sdl_wnd = 0;
  return Data_Wrap_Struct(klass, t_wnd_gc_mark, t_wnd_free, wnd);
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

  SDL_Surface *sfc = SDL_CreateRGBSurfaceWithFormat(0,
    512, 512, 32, SDL_PIXELFORMAT_BGRA32);

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

  return self;
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
t_wnd_flip_buffers(VALUE self)
{
  DECLAREWND(self);
  SDL_Surface *win_sfc = SDL_GetWindowSurface(wnd->sdl_wnd);

  SDL_FillRect(win_sfc, 0, SDL_MapRGB(win_sfc->format, 0, 0, 0));
  SDL_BlitSurface(wnd->sdl_surface, 0, win_sfc, 0);
  SDL_UpdateWindowSurface(wnd->sdl_wnd);
  return self;
}

////////////////////////////////////////////////////////////////////////

void
LAO_Window_Init() {
  cLayerWindow = rb_define_class_under(cLayer, "Window", rb_cObject);
  rb_define_alloc_func(cLayerWindow, t_wnd_allocator);

  rb_define_method(cLayerWindow, "initialize", t_wnd_initialize, 3);
  rb_define_method(cLayerWindow, "show", t_wnd_show, 0);
  rb_define_method(cLayerWindow, "flip_buffers", t_wnd_flip_buffers, 0);
}
