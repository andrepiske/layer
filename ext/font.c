#include "font.h"
#include "surface.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>

extern VALUE cLayer;
extern FT_Library s_freetype_lib;
VALUE cLayerFont;

#define DECLAREFONT(o) \
  struct LAO_Font *font; \
  TypedData_Get_Struct((o), struct LAO_Font, &lao_font_datatype, (font));


static void
t_font_gc_mark(void *font) {
}

static void
t_font_free(void *font) {
  xfree(font);
}

static size_t
t_font_data_size(const void *_font) {
  // const struct Lao_Font *font = (const struct Lao_Font*)_font;
  return 0;
}

static const rb_data_type_t lao_font_datatype = {
  .wrap_struct_name = "Layer::Font",
  .function = {
    .dmark = t_font_gc_mark,
    .dfree = t_font_free,
    .dsize = t_font_data_size,
  },
  .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE
t_font_allocator(VALUE klass) {
  struct LAO_Font *font = (struct LAO_Font*)xmalloc(sizeof(struct LAO_Font));
  font->face = 0;
  font->size = 0;
  font->color = 0;

  return TypedData_Wrap_Struct(klass, &lao_font_datatype, font);

  // return Data_Wrap_Struct(klass, t_font_gc_mark, t_font_free, font);
}


static VALUE
t_font_initialize(VALUE self, VALUE _file_name, VALUE _size) {
  DECLAREFONT(self);

  int err;
  float size = (float)NUM2DBL(_size);
  FT_Face face = 0;

  const char *file_name = StringValueCStr(_file_name);

  if ((err = FT_New_Face(s_freetype_lib, file_name, 0, &face))) {
    printf("Failed to load font: %d\n", err);

    if (err == FT_Err_Unknown_File_Format) {
      printf("\t(unknown font format)\n");
    }

    exit(1);
  }

  err = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
  if (err) {
    printf("Error to set charmap: 0x%02x\n", err);
    exit(1);
  }

  err = FT_Set_Char_Size(face, 0, (int)(size * 64.0), 128, 128);
  if (err) {
    printf("Error to set char size: 0x%02x\n", err);
    exit(1);
  }

  font->face = (void*)face;
  font->size = size;
  font->color = 0xFF000000;

  return self;
}

static unsigned int
blend_colors(unsigned char sR, unsigned char sG, unsigned char sB, unsigned char sA,
  unsigned char dR, unsigned char dG, unsigned char dB, unsigned char dA)
{
  unsigned int final_color;
  unsigned char *color = (unsigned char*)&final_color;

  color[0] = (unsigned char)(  ((float)(sB / 255.f) * (float)(sA / 255.f) + (float)(dB / 255.f) * (float)(1.f - sA / 255.f)) * 255.f );
  color[1] = (unsigned char)(  ((float)(sG / 255.f) * (float)(sA / 255.f) + (float)(dG / 255.f) * (float)(1.f - sA / 255.f)) * 255.f );
  color[2] = (unsigned char)(  ((float)(sR / 255.f) * (float)(sA / 255.f) + (float)(dR / 255.f) * (float)(1.f - sA / 255.f)) * 255.f );
  color[3] = (unsigned char)(  ((float)(sA / 255.f)                       + (float)(dA / 255.f) * (float)(1.f - sA / 255.f)) * 255.f );

  return final_color;
}

static VALUE
t_font_calc_metrics(int argc, const VALUE *argv, VALUE self)
{
  DECLAREFONT(self);

  VALUE _text;
  rb_scan_args(argc, argv, "1", &_text);
  Check_Type(_text, T_STRING);

  const char *text = StringValueCStr(_text);
  const int txt_length = (int)strlen(text);

  FT_Face face = (FT_Face)font->face;
  FT_GlyphSlot slot = face->glyph;

  VALUE result = rb_hash_new_capa(3);

  int text_height = 0;
  int text_width = 0;
  int baseline = 0;

  for (int i = 0; i < txt_length; ++i) {
    FT_UInt glyph_index = FT_Get_Char_Index(face, text[i]);
    FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);

    if (slot->metrics.height > text_height) {
      text_height = (int)slot->metrics.height;
    }
    if (slot->metrics.horiBearingY > baseline) {
      baseline = (int)slot->metrics.horiBearingY;
    }

    text_width += slot->metrics.horiAdvance;

    // rb_hash_aset(item, ID2SYM(rb_intern("width")), INT2NUM(slot->metrics.width / 64));
    // rb_hash_aset(item, ID2SYM(rb_intern("height")), INT2NUM(slot->metrics.height / 64));
    // rb_hash_aset(item, ID2SYM(rb_intern("h_bearing_x")), INT2NUM(slot->metrics.horiBearingX / 64));
    // rb_hash_aset(item, ID2SYM(rb_intern("h_bearing_y")), INT2NUM(slot->metrics.horiBearingY / 64));
    // rb_hash_aset(item, ID2SYM(rb_intern("h_advance")), INT2NUM(slot->metrics.horiAdvance / 64));
    // rb_ary_push(result, item);
  }

  rb_hash_aset(result, ID2SYM(rb_intern("height")), INT2NUM(text_height >> 6));
  rb_hash_aset(result, ID2SYM(rb_intern("width")), INT2NUM(text_width >> 6));
  rb_hash_aset(result, ID2SYM(rb_intern("baseline")), INT2NUM(baseline >> 6));

  return result;
}

static VALUE
t_font_draw_text(int argc, const VALUE *argv, VALUE self)
{
  DECLAREFONT(self);

  VALUE _dest;
  VALUE _x;
  VALUE _y;
  VALUE _text;

  rb_scan_args(argc, argv, "4", &_dest, &_x, &_y, &_text);

  // struct LAO_Surface *dest_obj = ((struct LAO_Surface*)rb_data_object_get(_dest));
  struct LAO_Surface *dest_obj = lao_sfc_from_value(_dest);
  // TypedData_Get_Struct((_dest), struct LAO_Surface, &lao_surface_datatype, (sfc));

  Check_Type(_text, T_STRING);
  const char *text = StringValueCStr(_text);
  const int txt_length = (int)strlen(text);

  cairo_surface_t *img_sfc;
  cairo_surface_t *surface = dest_obj->cairo_surface;

  cairo_surface_flush(surface);
  img_sfc = cairo_surface_map_to_image(surface, 0);

  const unsigned int stride = (unsigned int)cairo_image_surface_get_stride(img_sfc);
  unsigned char *dest = cairo_image_surface_get_data(img_sfc);

  FT_Face face = (FT_Face)font->face;
  FT_GlyphSlot slot = face->glyph;
  int err;
  int pos_y = NUM2INT(_y);
  int pos_x = NUM2INT(_x);
  unsigned int dest_depth = 4;
  unsigned int src_depth = 1;

  unsigned int current_color = font->color;

  for (int i = 0; i < txt_length; ++i) {
    FT_UInt glyph_index = FT_Get_Char_Index(face, text[i]);
    if ((err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT))) {
      printf("failed to load glyph for '%c' with err 0x%02x\n", text[i], err);
      exit(1);
    }
    // if ((err = FT_Render_Glyph(slot, FT_RENDER_MODE_LCD))) {
    if ((err = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL))) {
      printf("failed to render glyph for '%c' with err 0x%02x\n", text[i], err);
      exit(1);
    }

    int glyph_width = slot->bitmap.width;
    if (slot->bitmap.pixel_mode == 5) {
      glyph_width = slot->bitmap.width / 3;
    }

    for (int y = 0; y < (int)slot->bitmap.rows; y++) {
      for (int x = 0; x < (int)glyph_width; x++) {

        int dest_y = (int)(y + pos_y) - (int)slot->bitmap_top;
        int dest_x = (int)(x + pos_x);

        if (dest_y < 0 || dest_x < 0) {
          continue;
        }

        unsigned char dst_b = (current_color) & 0xFF;
        unsigned char dst_g = (current_color >> 8) & 0xFF;
        unsigned char dst_r = (current_color >> 16) & 0xFF;

        if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_LCD) {
          src_depth = 3;
        } else if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
        } else {
          printf("Unknown pixel mode %d\n", slot->bitmap.pixel_mode);
          exit(1);
        }

        unsigned char *local_dest = dest + stride * dest_y + dest_x * dest_depth;
        unsigned char *local_src = slot->bitmap.buffer + slot->bitmap.pitch * y + x * src_depth;

        unsigned char src_a = 0;

        if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_LCD) {
          printf("LCD not implemented\n");
          exit(1);
        } else if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
          src_a = local_src[0];
        }

        if (src_a > 0.f) {
          unsigned int color = blend_colors(
            dst_r, dst_g, dst_b, src_a,

            local_dest[2],
            local_dest[1],
            local_dest[0],
            local_dest[3]);

          *((unsigned int*)local_dest) = color;
        }

        // if (x == 0 || y == 0 || x == (int)slot->bitmap.width - 1 || y == (int)slot->bitmap.rows - 1) {
        //   *((unsigned int*)local_dest) = 0xFF00FFFF;
        // }
      }
    }

    pos_x += slot->advance.x >> 6;
  }

  cairo_surface_unmap_image(surface, img_sfc);
  cairo_surface_mark_dirty(surface);

  return Qtrue;
}

static VALUE
t_font_size_set(VALUE self, VALUE _size) {
  DECLAREFONT(self);
  font->size = (float)NUM2DBL(_size);
  FT_Set_Char_Size(font->face, 0, (int)(font->size * 64.0), 128, 128);
  return Qnil;
}

static VALUE
t_font_size(VALUE self) {
  DECLAREFONT(self);
  return DBL2NUM((double)font->size);
}

static VALUE t_font__color(VALUE self) {
  DECLAREFONT(self);
  return UINT2NUM(font->color);
}

static VALUE t_font__color_set(VALUE self, VALUE _value) {
  DECLAREFONT(self);
  font->color = NUM2UINT(_value);
  return _value;
}

////////////////////////////////////////////////////////////////////////

void
LAO_Font_Init() {
  cLayerFont = rb_define_class_under(cLayer, "Font", rb_cObject);
  rb_define_alloc_func(cLayerFont, t_font_allocator);

  rb_define_method(cLayerFont, "initialize", t_font_initialize, 2);
  rb_define_method(cLayerFont, "draw_text", t_font_draw_text, -1);
  rb_define_method(cLayerFont, "calc_metrics", t_font_calc_metrics, -1);

  rb_define_method(cLayerFont, "size", t_font_size, 0);
  rb_define_method(cLayerFont, "size=", t_font_size_set, 1);
  rb_define_method(cLayerFont, "_color", t_font__color, 0);
  rb_define_method(cLayerFont, "_color=", t_font__color_set, 1);
}
