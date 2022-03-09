#include "surface.h"

extern VALUE cLayer;
VALUE cLayerSurface;

#define DECLARESFC(o) \
  struct LAO_Surface *sfc = (struct LAO_Surface*)rb_data_object_get((o))

static void
t_sfc_gc_mark(struct LAO_Surface*) {
}

static void
t_sfc_free(struct LAO_Surface *sfc) {
  xfree(sfc);
}

static VALUE
t_sfc_allocator(VALUE klass) {
  struct LAO_Surface *sfc = (struct LAO_Surface*)xmalloc(sizeof(struct LAO_Surface));
  sfc->sdl_surface = 0;
  sfc->cairo_surface = 0;
  sfc->cairo_ctx = 0;
  return Data_Wrap_Struct(klass, t_sfc_gc_mark, t_sfc_free, sfc);
}

static VALUE
t_sfc_initialize(VALUE self, VALUE _width, VALUE _height) {
  DECLARESFC(self);

  int width = NUM2INT(_width);
  int height = NUM2INT(_height);

  // SDL_Surface *sfc = SDL_CreateRGBSurfaceWithFormat(0,
  //   512, 512, 32, SDL_PIXELFORMAT_BGRA32);

  // SDL_SetSurfaceBlendMode(sfc, SDL_BLENDMODE_NONE);
  // wnd->sdl_surface = sfc;

  sfc->cairo_surface = cairo_image_surface_create(
    CAIRO_FORMAT_ARGB32,
    width, height);

  sfc->cairo_ctx = cairo_create(sfc->cairo_surface);
  return self;
}

////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////

void
LAO_Surface_Init() {
  cLayerSurface = rb_define_class_under(cLayer, "Surface", rb_cObject);
  rb_define_alloc_func(cLayerSurface, t_sfc_allocator);

  rb_define_method(cLayerSurface, "initialize", t_sfc_initialize, 2);
}
