// extern "C" {
#include "surface.h"
//  }

extern VALUE cLayer;
VALUE cLayerSurface;

#define DECLARESFC(o) \
  struct LAO_Surface *sfc; \
  TypedData_Get_Struct((o), struct LAO_Surface, &lao_surface_datatype, (sfc));

static void
t_sfc_gc_mark(void *_sfc) {
}

static void
t_sfc_free(void *_sfc) {
  struct LAO_Surface *sfc = (struct LAO_Surface*)_sfc;

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

static size_t
t_sfc_size(const void *_sfc) {
  const struct LAO_Surface *sfc = (const struct LAO_Surface*)_sfc;

  if (!sfc->borrowed) {
    size_t w = cairo_image_surface_get_width(sfc->cairo_surface);
    size_t h = cairo_image_surface_get_height(sfc->cairo_surface);
    return w * h * 4;
  }
  return 0;
}

static const rb_data_type_t lao_surface_datatype = {
  .wrap_struct_name = "Layer::Surface",
  .function = {
    .dmark = t_sfc_gc_mark,
    .dfree = t_sfc_free,
    .dsize = t_sfc_size,
  },
  .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

struct LAO_Surface *
lao_sfc_from_value(VALUE v) {
  DECLARESFC(v);
  return sfc;
}

static VALUE
t_sfc_allocator(VALUE klass) {
  struct LAO_Surface *sfc = (struct LAO_Surface*)xmalloc(sizeof(struct LAO_Surface));
  sfc->sdl_surface = 0;
  sfc->cairo_surface = 0;
  sfc->cairo_ctx = 0;
  sfc->borrowed = 0;

  // return Data_Wrap_Struct(klass, t_sfc_gc_mark, t_sfc_free, sfc);
  // return Data_Wrap_Struct(klass, 0, t_sfc_free, sfc);
  return TypedData_Wrap_Struct(klass, &lao_surface_datatype, sfc);
  // return TypedData_Make_Struct(klass, struct LAO_Surface, &lao_surface_datatype, 0);
}

#include "include/core/SkCanvas.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSurface.h"

#include "include/core/SkStream.h"              // for SkFILEWStream
#include "include/codec/SkEncodedImageFormat.h" // for SkEncodedImageFormat
// #include "include/core/SkImageEncoder.h"        // for SkEncodeImage

static VALUE
t_sfc_initialize(VALUE self, VALUE _width, VALUE _height) {
  DECLARESFC(self);

  int width = NUM2INT(_width);
  int height = NUM2INT(_height);

  const int initialize = (width > 0 && height > 0);

  SkGraphics::Init();
  // SkImageInfo imgi = SkImageInfo::Make(width, height, kBGRA_8888_SkColorType, kUnpremul_SkAlphaType);
  SkImageInfo imgi = SkImageInfo::MakeN32Premul(512, 256);
  // sk_sp<SkSurface> sk_sfc = SkSurfaces::Raster(imgi);
  sk_sp<SkSurface> sk_sfc = SkSurfaces::Null(512, 256);

  SkCanvas* sk_canvas = sk_sfc->getCanvas();
  SkPaint paint;
  paint.setColor(SK_ColorBLUE);
  paint.setAntiAlias(true);
  sk_canvas->drawCircle(128, 128, 100, paint);

  // sk_sp<SkImage> img = sk_sfc->makeImageSnapshot();
  // SkPixmap pixmap;
  // if (img->peekPixels(&pixmap)) {
  //     SkFILEWStream out("circle.png");
  //     SkEncodeImage(&out, pixmap, SkEncodedImageFormat::kPNG, 100);
  // }


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

static VALUE
t_sfc_save_png(VALUE self, VALUE file_name) {
  DECLARESFC(self);

  Check_Type(file_name, T_STRING);

  const char *_file_name = StringValueCStr(file_name);
  cairo_surface_write_to_png(sfc->cairo_surface, _file_name);

  // cairo_surface_t *png_sfc = cairo_image_surface_create_from_png(StringValueCStr(file_name));
  // if (!png_sfc) {
  //   return Qfalse;
  // }

  // TODO: free surface
  // free_surface();
  // sfc->cairo_surface = png_sfc;

  return Qtrue;
}

extern "C"
VALUE
lao_sfc_create_borrowed(SDL_Surface *sdl_surface, cairo_surface_t *cairo_surface, cairo_t *cairo_ctx) {
  VALUE sfc_obj = t_sfc_allocator(cLayerSurface);
  DECLARESFC(sfc_obj);
  sfc->sdl_surface = sdl_surface;
  // TODO: use reference counting for cairo objects
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
t_sfc_src__color(int argc, VALUE *argv, VALUE self) {
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

static VALUE
t_sfc_new_path(VALUE self) {
  DECLARESFC(self);
  cairo_new_path(sfc->cairo_ctx);
  return self;
}

static VALUE
t_sfc_set_pixel(VALUE self, VALUE x, VALUE y, VALUE r, VALUE g, VALUE b) {
  DECLARESFC(self);

  const unsigned int dest_x = NUM2UINT(x);
  const unsigned int dest_y = NUM2UINT(y);

  cairo_surface_t *img_sfc;
  const unsigned int dest_depth = 4;
  cairo_surface_t *const surface = sfc->cairo_surface;

  cairo_surface_flush(surface);
  img_sfc = cairo_surface_map_to_image(surface, 0);

  const unsigned int stride = (unsigned int)cairo_image_surface_get_stride(img_sfc);
  unsigned char *dest = cairo_image_surface_get_data(img_sfc);

  unsigned char *local_dest = dest + stride * dest_y + dest_x * dest_depth;

  unsigned int *cr = (unsigned int*)local_dest;
  *cr = (unsigned int)(

      ( ((unsigned char)NUM2UINT(b)) )
    | ( ((unsigned char)NUM2UINT(g)) << 8 )
    | ( ((unsigned char)NUM2UINT(r)) << 16 )
    | (255 << 24)
  );

  cairo_surface_unmap_image(surface, img_sfc);
  cairo_surface_mark_dirty(surface);

  return self;
}

static VALUE
t_sfc_to_rgb24(VALUE self) {
  DECLARESFC(self);

  cairo_surface_t *img_sfc;
  // const unsigned int dest_depth = 4;
  cairo_surface_t *const surface = sfc->cairo_surface;

  cairo_surface_flush(surface);
  img_sfc = cairo_surface_map_to_image(surface, 0);

  const unsigned int stride = (unsigned int)cairo_image_surface_get_stride(img_sfc);
  const unsigned char *src_data = cairo_image_surface_get_data(img_sfc);

  const unsigned int width = cairo_image_surface_get_width(sfc->cairo_surface);
  const unsigned int height = cairo_image_surface_get_height(sfc->cairo_surface);
  unsigned char *dest_data = (unsigned char*)malloc(width * height * 3);

  for (unsigned int y = 0; y < height; ++y) {
    for (unsigned int x = 0; x < width; ++x) {
      const unsigned char *psrc = src_data + (4 * x + y * stride);
      unsigned char *pdest = dest_data + (3 * (x + y * width));
      pdest[0] = psrc[2];
      pdest[1] = psrc[1];
      pdest[2] = psrc[0];
    }
  }

  cairo_surface_unmap_image(surface, img_sfc);
  cairo_surface_mark_dirty(surface);

  VALUE result = rb_str_new((const char*)dest_data, width * height * 3);
  free(dest_data);

  return result;
}

static VALUE
t_sfc_width(VALUE self) {
  DECLARESFC(self);
  return INT2NUM(cairo_image_surface_get_width(sfc->cairo_surface));
}

static VALUE
t_sfc_height(VALUE self) {
  DECLARESFC(self);
  return INT2NUM(cairo_image_surface_get_height(sfc->cairo_surface));
}

static VALUE
t_sfc_blit_surface(int argc, const VALUE *argv, VALUE self) {
  DECLARESFC(self);

  VALUE _x = 0;
  VALUE _y = 0;
  VALUE _src_sfc = 0;
  VALUE _width = 0;
  VALUE _height = 0;
  VALUE _source_x = 0;
  VALUE _source_y = 0;
  VALUE _scale_x = 0;
  VALUE _scale_y = 0;

  rb_scan_args(argc, argv, "36", &_src_sfc, &_x, &_y,
    &_width, &_height, &_source_x, &_source_y, &_scale_x, &_scale_y);

  // struct LAO_Surface *src_sfc = (struct LAO_Surface*)rb_data_object_get((_src_sfc));
  struct LAO_Surface *src_sfc = lao_sfc_from_value(_src_sfc);

  const double x = NUM2DBL(_x);
  const double y = NUM2DBL(_y);

  const double source_x = (_source_x == Qnil ? 0.0 : NUM2DBL(_source_x));
  const double source_y = (_source_y == Qnil ? 0.0 : NUM2DBL(_source_y));

  cairo_t *cr = sfc->cairo_ctx;

  if (!src_sfc->cairo_surface) {
    return Qfalse;
  }

  const int sfc_width = cairo_image_surface_get_width(src_sfc->cairo_surface);
  const int sfc_height = cairo_image_surface_get_height(src_sfc->cairo_surface);

  cairo_pattern_t *pat = cairo_pattern_create_for_surface(src_sfc->cairo_surface);
  cairo_matrix_t mat;
  cairo_matrix_init_translate(&mat, source_x - x, source_y - y);

  const double scale_x = (_scale_x == Qnil ? 1.0 : NUM2DBL(_scale_x));
  const double scale_y = (_scale_y == Qnil ? 1.0 : NUM2DBL(_scale_y));
  cairo_matrix_scale(&mat, 1.0 / scale_x, 1.0 / scale_y);

  cairo_pattern_set_matrix(pat, &mat);
  cairo_set_source(cr, pat);
  cairo_pattern_destroy(pat);

  cairo_rectangle(cr, x, y,
    _width == Qnil ? (double)sfc_width : NUM2DBL(_width),
    _height == Qnil ? (double)sfc_height : NUM2DBL(_height)
  );
  cairo_fill(cr);

  return Qtrue;
}

static VALUE
t_sfc_get_matrix(VALUE self) {
  DECLARESFC(self);
  cairo_matrix_t m;
  cairo_get_matrix(sfc->cairo_ctx, &m);

  return rb_ary_new_from_args(3,
    rb_ary_new_from_args(3, DBL2NUM(m.xx), DBL2NUM(m.xy), DBL2NUM(m.x0)),
    rb_ary_new_from_args(3, DBL2NUM(m.yx), DBL2NUM(m.yy), DBL2NUM(m.y0)),
    rb_ary_new_from_args(3, DBL2NUM(0), DBL2NUM(0), DBL2NUM(1))
  );
}

static VALUE
t_sfc_set_matrix(VALUE self, VALUE _new_matrix) {
  DECLARESFC(self);
  cairo_matrix_t m;
  const VALUE row0 = rb_ary_entry(_new_matrix, 0);
  const VALUE row1 = rb_ary_entry(_new_matrix, 1);

  m.xx = NUM2DBL(rb_ary_entry(row0, 0));
  m.xy = NUM2DBL(rb_ary_entry(row0, 1));
  m.x0 = NUM2DBL(rb_ary_entry(row0, 2));

  m.yx = NUM2DBL(rb_ary_entry(row1, 0));
  m.yy = NUM2DBL(rb_ary_entry(row1, 1));
  m.y0 = NUM2DBL(rb_ary_entry(row1, 2));

  cairo_set_matrix(sfc->cairo_ctx, &m);
  return self;
}

static VALUE
t_sfc_rotate(VALUE self, VALUE _angle) {
  DECLARESFC(self);
  cairo_rotate(sfc->cairo_ctx, NUM2DBL(_angle));
  return self;
}

static VALUE
t_sfc_scale(VALUE self, VALUE _x, VALUE _y) {
  DECLARESFC(self);
  cairo_scale(sfc->cairo_ctx, NUM2DBL(_x), NUM2DBL(_y));
  return self;
}

static VALUE
t_sfc_translate(VALUE self, VALUE _x, VALUE _y) {
  DECLARESFC(self);
  cairo_translate(sfc->cairo_ctx, NUM2DBL(_x), NUM2DBL(_y));
  return self;
}

static VALUE
t_sfc_identity(VALUE self) {
  DECLARESFC(self);
  cairo_identity_matrix(sfc->cairo_ctx);
  return self;
}


////////////////////////////////////////////////////////////////////////

extern "C"
void
LAO_Surface_Init() {
  cLayerSurface = rb_define_class_under(cLayer, "Surface", rb_cObject);
  rb_define_alloc_func(cLayerSurface, t_sfc_allocator);

  rb_define_method(cLayerSurface, "initialize", t_sfc_initialize, 2);
  rb_define_method(cLayerSurface, "load_png!", t_sfc_load_png_b, 1);
  rb_define_method(cLayerSurface, "save_png", t_sfc_save_png, 1);

  // Cairo Draw methods

  rb_define_method(cLayerSurface, "push_state", t_sfc_push_state, 0);
  rb_define_method(cLayerSurface, "pop_state", t_sfc_pop_state, 0);

  rb_define_method(cLayerSurface, "_src_color", t_sfc_src__color, -1);
  rb_define_method(cLayerSurface, "src_surface", t_sfc_src_surface, -1);
  rb_define_method(cLayerSurface, "line_width=", t_sfc_line_width_set, 1);
  rb_define_method(cLayerSurface, "line_join=", t_sfc_line_join_set, 1);
  rb_define_method(cLayerSurface, "fill", t_sfc_fill, -1);
  rb_define_method(cLayerSurface, "stroke", t_sfc_stroke, -1);

  rb_define_method(cLayerSurface, "move_to", t_sfc_move_to, 2);
  rb_define_method(cLayerSurface, "line_to", t_sfc_line_to, 2);
  rb_define_method(cLayerSurface, "rectangle", t_sfc_rectangle, 4);
  rb_define_method(cLayerSurface, "arc", t_sfc_arc, 5);
  rb_define_method(cLayerSurface, "new_path", t_sfc_new_path, 0);

  rb_define_method(cLayerSurface, "set_pixel", t_sfc_set_pixel, 5);
  rb_define_method(cLayerSurface, "to_rgb24", t_sfc_to_rgb24, 0);

  rb_define_method(cLayerSurface, "width", t_sfc_width, 0);
  rb_define_method(cLayerSurface, "height", t_sfc_height, 0);

  rb_define_method(cLayerSurface, "blit_surface", t_sfc_blit_surface, -1);

  // Matrix methods
  rb_define_method(cLayerSurface, "matrix", t_sfc_get_matrix, 0);
  rb_define_method(cLayerSurface, "matrix=", t_sfc_set_matrix, 1);
  rb_define_method(cLayerSurface, "rotate", t_sfc_rotate, 1);
  rb_define_method(cLayerSurface, "scale", t_sfc_scale, 2);
  rb_define_method(cLayerSurface, "translate", t_sfc_translate, 2);
  rb_define_method(cLayerSurface, "identity", t_sfc_identity, 0);
}
