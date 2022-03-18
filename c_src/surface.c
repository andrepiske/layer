#include "surface.h"

extern VALUE cLayer;
VALUE cLayerSurface;

#define DECLARESFC(o) \
  struct LAO_Surface *sfc = (struct LAO_Surface*)rb_data_object_get((o))

static void
t_sfc_gc_mark(struct LAO_Surface *sfc) {
}

static void
t_sfc_free(struct LAO_Surface *sfc) {
  if (!sfc->borrowed) {
    if (sfc->sdl_surface) {
      SDL_FreeSurface(sfc->sdl_surface);
      sfc->sdl_surface = 0;
    }
    if (sfc->cairo_surface) {
      cairo_surface_destroy(sfc->cairo_surface);
      sfc->cairo_surface = 0;
    }
    if (sfc->cairo_ctx) {
      cairo_destroy(sfc->cairo_ctx);
      sfc->cairo_ctx = 0;
    }
  }
  xfree(sfc);
}

static VALUE
t_sfc_allocator(VALUE klass) {
  struct LAO_Surface *sfc = (struct LAO_Surface*)xmalloc(sizeof(struct LAO_Surface));
  sfc->sdl_surface = 0;
  sfc->cairo_surface = 0;
  sfc->cairo_ctx = 0;
  sfc->borrowed = 0;
  return Data_Wrap_Struct(klass, t_sfc_gc_mark, t_sfc_free, sfc);
}

static VALUE
t_sfc_initialize(VALUE self, VALUE _width, VALUE _height) {
  DECLARESFC(self);

  int width = NUM2INT(_width);
  int height = NUM2INT(_height);

  const int initialize = (width > 0 && height > 0);

  // SDL_Surface *sfc = SDL_CreateRGBSurfaceWithFormat(0,
  //   512, 512, 32, SDL_PIXELFORMAT_BGRA32);

  // SDL_SetSurfaceBlendMode(sfc, SDL_BLENDMODE_NONE);
  // wnd->sdl_surface = sfc;

  if (initialize) {
    sfc->cairo_surface = cairo_image_surface_create(
      CAIRO_FORMAT_ARGB32,
      width, height);

    sfc->cairo_ctx = cairo_create(sfc->cairo_surface);
  }

  return self;
}

static VALUE
t_sfc_load_png_b(VALUE self, VALUE file_name) {
  DECLARESFC(self);

  Check_Type(file_name, T_STRING);

  cairo_surface_t *png_sfc = cairo_image_surface_create_from_png(StringValueCStr(file_name));
  if (!png_sfc) {
    return Qfalse;
  }

  // TODO: free surface
  // free_surface();


  sfc->cairo_surface = png_sfc;

  return Qtrue;
}

VALUE
lao_sfc_create_borrowed(SDL_Surface *sdl_surface, cairo_surface_t *cairo_surface, cairo_t *cairo_ctx) {
  VALUE sfc_obj = t_sfc_allocator(cLayerSurface);
  DECLARESFC(sfc_obj);
  sfc->sdl_surface = sdl_surface;
  sfc->cairo_surface = cairo_surface;
  sfc->cairo_ctx = cairo_ctx;
  sfc->borrowed = 1;
  return sfc_obj;
}

////////////////////////////////////////////////////////////////////////

#define DECLARECTX(o) \
  DECLARESFC((o)); \
  cairo_t *ctx = sfc->cairo_ctx

static VALUE
t_sfc_push_state(VALUE self) {
  DECLARECTX(self);
  cairo_save(ctx);
  return self;
}

static VALUE
t_sfc_pop_state(VALUE self) {
  DECLARECTX(self);
  cairo_restore(ctx);
  return self;
}

static VALUE
t_sfc_src_color(int argc, VALUE *argv, VALUE self) {
  DECLARECTX(self);

  VALUE r;
  VALUE g;
  VALUE b;
  VALUE a;

  rb_scan_args(argc, argv, "31", &r, &g, &b, &a);

  if (a == Qnil) {
    cairo_set_source_rgb(ctx,
      NUM2DBL(r),
      NUM2DBL(g),
      NUM2DBL(b)
    );
  } else {
    cairo_set_source_rgba(ctx,
      NUM2DBL(r),
      NUM2DBL(g),
      NUM2DBL(b),
      NUM2DBL(a)
    );
  }

  return self;
}

static VALUE
t_sfc_src_color_b(int argc, VALUE *argv, VALUE self) {
  DECLARECTX(self);

  VALUE r;
  VALUE g;
  VALUE b;
  VALUE a;

  rb_scan_args(argc, argv, "31", &r, &g, &b, &a);

  if (a == Qnil) {
    cairo_set_source_rgb(ctx,
      NUM2DBL(r) / 255.0,
      NUM2DBL(g) / 255.0,
      NUM2DBL(b) / 255.0
    );
  } else {
    cairo_set_source_rgba(ctx,
      NUM2DBL(r) / 255.0,
      NUM2DBL(g) / 255.0,
      NUM2DBL(b) / 255.0,
      NUM2DBL(a) / 255.0
    );
  }

  return self;
}

static VALUE
t_sfc_src_surface(int argc, VALUE *argv, VALUE self) {
  DECLARECTX(self);

  VALUE _src_sfc;
  VALUE _x;
  VALUE _y;

  rb_scan_args(argc, argv, "12", &_src_sfc, &_x, &_y);

  const double x = (_x == Qnil) ? 0.0 : NUM2DBL(_x);
  const double y = (_y == Qnil) ? 0.0 : NUM2DBL(_y);

  struct LAO_Surface *src_sfc = (struct LAO_Surface*)rb_data_object_get((_src_sfc));

  cairo_set_source_surface(ctx, src_sfc->cairo_surface, x, y);

  return self;
}

static VALUE
t_sfc_fill(int argc, VALUE *argv, VALUE self) {
  DECLARECTX(self);

  VALUE preserve;
  rb_scan_args(argc, argv, "01", &preserve);

  if (preserve == Qtrue) {
    cairo_fill_preserve(ctx);
  } else {
    cairo_fill(ctx);
  }

  return self;
}

static VALUE
t_sfc_stroke(int argc, VALUE *argv, VALUE self) {
  DECLARECTX(self);

  VALUE preserve;
  rb_scan_args(argc, argv, "01", &preserve);

  if (preserve == Qtrue) {
    cairo_stroke_preserve(ctx);
  } else {
    cairo_stroke(ctx);
  }

  return self;
}


static VALUE
t_sfc_move_to(VALUE self, VALUE x, VALUE y) {
  DECLARECTX(self);
  cairo_move_to(ctx, NUM2DBL(x), NUM2DBL(y));
  return self;
}

static VALUE
t_sfc_line_to(VALUE self, VALUE x, VALUE y) {
  DECLARECTX(self);
  cairo_line_to(ctx, NUM2DBL(x), NUM2DBL(y));
  return self;
}

static VALUE
t_sfc_rectangle(VALUE self, VALUE x, VALUE y, VALUE width, VALUE height) {
  DECLARECTX(self);
  cairo_rectangle(ctx,
    NUM2DBL(x),
    NUM2DBL(y),
    NUM2DBL(width),
    NUM2DBL(height)
  );
  return self;
}

static VALUE
t_sfc_arc(VALUE self, VALUE xc, VALUE yc, VALUE radius, VALUE angle1, VALUE angle2) {
  DECLARECTX(self);
  cairo_arc(ctx,
    NUM2DBL(xc),
    NUM2DBL(yc),
    NUM2DBL(radius),
    NUM2DBL(angle1),
    NUM2DBL(angle2)
  );
  return self;
}

static VALUE
t_sfc_line_width_set(VALUE self, VALUE width) {
  DECLARECTX(self);
  cairo_set_line_width(ctx, NUM2DBL(width));
  return self;
}

static VALUE
t_sfc_line_join_set(VALUE self, VALUE _join) {
  DECLARECTX(self);
  Check_Type(_join, T_SYMBOL);

  cairo_line_join_t join;

  ID join_id = SYM2ID(_join);
  if (join_id == rb_intern("miter")) {
    join = CAIRO_LINE_JOIN_MITER;
  } else if (join_id == rb_intern("round")) {
    join = CAIRO_LINE_JOIN_ROUND;
  } else if (join_id == rb_intern("bevel")) {
    join = CAIRO_LINE_JOIN_BEVEL;
  } else {
    join = CAIRO_LINE_JOIN_BEVEL;
  }

  cairo_set_line_join(ctx, join);
  return self;
}

////////////////////////////////////////////////////////////////////////

void
LAO_Surface_Init() {
  cLayerSurface = rb_define_class_under(cLayer, "Surface", rb_cObject);
  rb_define_alloc_func(cLayerSurface, t_sfc_allocator);

  rb_define_method(cLayerSurface, "initialize", t_sfc_initialize, 2);
  rb_define_method(cLayerSurface, "load_png!", t_sfc_load_png_b, 1);

  // Cairo Draw methods

  rb_define_method(cLayerSurface, "push_state", t_sfc_push_state, 0);
  rb_define_method(cLayerSurface, "pop_state", t_sfc_pop_state, 0);

  rb_define_method(cLayerSurface, "src_color", t_sfc_src_color, -1);
  rb_define_method(cLayerSurface, "src_color_b", t_sfc_src_color_b, -1);
  rb_define_method(cLayerSurface, "src_surface", t_sfc_src_surface, -1);
  rb_define_method(cLayerSurface, "line_width=", t_sfc_line_width_set, 1);
  rb_define_method(cLayerSurface, "line_join=", t_sfc_line_join_set, 1);
  rb_define_method(cLayerSurface, "fill", t_sfc_fill, -1);
  rb_define_method(cLayerSurface, "stroke", t_sfc_stroke, -1);

  rb_define_method(cLayerSurface, "move_to", t_sfc_move_to, 2);
  rb_define_method(cLayerSurface, "line_to", t_sfc_line_to, 2);
  rb_define_method(cLayerSurface, "rectangle", t_sfc_rectangle, 4);
  rb_define_method(cLayerSurface, "arc", t_sfc_arc, 5);

}
