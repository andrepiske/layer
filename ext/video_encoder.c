#include "video_encoder.h"
#include "minih264e.h"
#include "surface.h"

static ID i_key, i_gop, i_qp_max, i_qp_min, i_qp, i_kbps, i_speed;

extern VALUE cLayer;
VALUE cLayerVideoEncoder;

#define DECLAREVE(o) \
  struct LAO_VideoEncoder *ve = (struct LAO_VideoEncoder*)rb_data_object_get((o))

static void
t_ve_gc_mark(struct LAO_VideoEncoder *ve) {
}

static void
t_ve_free(struct LAO_VideoEncoder *ve) {
  if (ve->encoding_buffer) {
    xfree(ve->encoding_buffer);
    ve->encoding_buffer = 0;
  }
  if (ve->encoder_obj) {
    xfree(ve->encoder_obj);
    ve->encoder_obj = 0;
  }
  if (ve->scratch_mem) {
    xfree(ve->scratch_mem);
    ve->scratch_mem = 0;
  }
  if (ve->create_param) {
    xfree(ve->create_param);
    ve->create_param = 0;
  }
  xfree(ve);
}

static VALUE
t_ve_allocator(VALUE klass) {
  struct LAO_VideoEncoder *ve = (struct LAO_VideoEncoder*)xmalloc(sizeof(struct LAO_VideoEncoder));
  memset(ve, 0, sizeof(struct LAO_VideoEncoder));
  return Data_Wrap_Struct(klass, t_ve_gc_mark, t_ve_free, ve);
}

static VALUE
t_ve_initialize(int argc, const VALUE *argv, VALUE self) {
  DECLAREVE(self);

  VALUE _width;
  VALUE _height;
  VALUE opts;

  rb_scan_args(argc, argv, "20:", &_width, &_height, &opts);
  if (NIL_P(opts))
    opts = rb_hash_new();

  int persist_size = 0;
  int scratch_size = 0;

  H264E_create_param_t *param = (H264E_create_param_t*)xmalloc(sizeof(H264E_create_param_t));
  ve->create_param = (void*)param;
  memset(param, 0, sizeof(H264E_create_param_t));

  ve->width = param->width = NUM2UINT(_width);
  ve->height = param->height = NUM2UINT(_height);
  param->gop = 180;
  param->const_input_flag = 1;
  param->num_layers = 1;
  param->enableNEON = 1;

  if (RTEST(rb_funcall(opts, i_key, 1, ID2SYM(i_gop)))) {
    VALUE val_gop = rb_hash_aref(opts, ID2SYM(i_gop));
    param->gop = NUM2UINT(val_gop);
  }

  int has_qp_min = RTEST(rb_funcall(opts, i_key, 1, ID2SYM(i_qp_min)));
  int has_qp_max = RTEST(rb_funcall(opts, i_key, 1, ID2SYM(i_qp_max)));
  int has_qp = RTEST(rb_funcall(opts, i_key, 1, ID2SYM(i_qp)));
  int has_kbps = RTEST(rb_funcall(opts, i_key, 1, ID2SYM(i_kbps)));

  // TODO: test for conficting values

  if (has_qp_min) {
    VALUE val = rb_hash_aref(opts, ID2SYM(i_qp_min));
    ve->qp_min = NUM2UINT(val);
  } else {
    ve->qp_min = 36;
  }

  if (has_qp_max) {
    VALUE val = rb_hash_aref(opts, ID2SYM(i_qp_max));
    ve->qp_max = NUM2UINT(val);
  } else {
    ve->qp_max = 36;
  }

  if (has_qp) {
    VALUE val = rb_hash_aref(opts, ID2SYM(i_qp));
    ve->qp_max = ve->qp_min = NUM2UINT(val);
  }

  if (has_kbps) {
    VALUE val = rb_hash_aref(opts, ID2SYM(i_kbps));
    ve->kbps = NUM2UINT(val);
  }

  if (H264E_sizeof(param, &persist_size, &scratch_size) != 0) {
    // TODO: raise error
    printf("size of failed\n");
    exit(1);
  }

  H264E_persist_t *enc = (H264E_persist_t*)xmalloc(persist_size);
  if (H264E_init(enc, param) != 0) {
    // TODO: raise error
    printf("it did not work\n");
    exit(1);
  }

  ve->encoder_obj = (void*)enc;
  ve->scratch_mem = xmalloc(scratch_size);
  return self;
}

static void
surface_to_frame(struct LAO_VideoEncoder *ve, H264E_io_yuv_t *frame, struct LAO_Surface *laosfc, unsigned char *buffer_in) {
  cairo_surface_t *sfc = laosfc->cairo_surface;
  cairo_surface_flush(sfc);

  const unsigned int w = ve->width;
  const unsigned int h = ve->height;

  const unsigned int src_stride = (unsigned int)cairo_image_surface_get_stride(sfc);
  const unsigned char *rgb_src = cairo_image_surface_get_data(sfc);

  unsigned char *buf_u = buffer_in + (w * h);
  unsigned char *buf_v = buf_u + (w * h / 4);

  for (unsigned int y = 0; y < h; y++) {
    for (unsigned int x = 0; x < w; x++) {
      const double b_val = (double)rgb_src[src_stride * y + 4*x] / 255.0;
      const double g_val = (double)rgb_src[src_stride * y + 4*x + 1] / 255.0;
      const double r_val = (double)rgb_src[src_stride * y + 4*x + 2] / 255.0;

      double y_val = 0.299 * r_val + 0.587 * g_val + 0.114 * b_val;
      // double u_val = (b_val - y_val) * 0.565;
      // double v_val = (r_val - y_val) * 0.713;
      double u_val = (b_val - y_val) * 0.492;
      double v_val = (r_val - y_val) * 0.877;

      buffer_in[w * y + x] = (unsigned char)(y_val * 255.0);
      // buffer_in[w * y + x] = (unsigned char)(y_val * 219.0 + 16.0);
      if ((x & 1) == 0 && (y & 1) == 0) {
        unsigned int vx = x >> 1;
        unsigned int vy = y >> 1;

        // buf_u[(vy * w / 2) + vx] = (unsigned char)( (u_val * 1.14678899 + 0.5) * 255.0 );
        // buf_v[(vy * w / 2) + vx] = (unsigned char)( (v_val * 0.81300813 + 0.5) * 255.0 );
        buf_u[(vy * w / 2) + vx] = (unsigned char)( (u_val * 1.14678899 + 0.5) * 255.0 );
        buf_v[(vy * w / 2) + vx] = (unsigned char)( (v_val * 0.81300813 + 0.5) * 255.0 );
      }
    }
  }
}

static VALUE
t_ve_encode_frame(int argc, const VALUE *argv, VALUE self) {
  DECLAREVE(self);
  VALUE opts = Qnil;
  VALUE _surface = Qnil;

  rb_scan_args(argc, argv, "10:", &_surface, &opts);
  if (NIL_P(opts))
    opts = rb_hash_new();

  struct LAO_Surface *sfc = (struct LAO_Surface*)rb_data_object_get(_surface);

  // H264E_create_param_t *param = (H264E_create_param_t*)ve->create_param;
  H264E_persist_t *enc = (H264E_persist_t*)ve->encoder_obj;
  void *scratch = ve->scratch_mem;

  H264E_run_param_t run_param;
  memset(&run_param, 0, sizeof(H264E_run_param_t));

  if (!ve->encoding_buffer) {
    ve->encoding_buffer = xmalloc(ve->width * ve->height * 3 / 2);
  }

  H264E_io_yuv_t frame;
  surface_to_frame(ve, &frame, sfc, ve->encoding_buffer);

  frame.yuv[0] = ve->encoding_buffer;
  frame.stride[0] = ve->width;
  frame.yuv[1] = ve->encoding_buffer + ve->width*ve->height;
  frame.stride[1] = ve->width / 2;
  frame.yuv[2] = ve->encoding_buffer + ve->width*ve->height*5 / 4;
  frame.stride[2] = ve->width / 2;

  run_param.encode_speed = 0;
  run_param.frame_type = 0;
  run_param.qp_min = ve->qp_min;
  run_param.qp_max = ve->qp_max;

  if (RTEST(rb_funcall(opts, i_key, 1, ID2SYM(i_speed)))) {
    VALUE val = rb_hash_aref(opts, ID2SYM(i_speed));
    run_param.encode_speed = NUM2INT(val);
  }

  int coded_data_size = 0;
  unsigned char *coded_data = 0;

  int error = H264E_encode(enc, scratch, &run_param, &frame,
    &coded_data,
    &coded_data_size);

  if (error) {
    printf("something went wrong during frame encoding\n");
    exit(1);
  }

  return rb_str_new((const char*)coded_data, coded_data_size);
}

////////////////////////////////////////////////////////////////////////

void
LAO_VideoEncoder_Init() {
  i_key = rb_intern("[]");
  i_gop = rb_intern("gop");
  i_qp_max = rb_intern("qp_max");
  i_qp_min = rb_intern("qp_min");
  i_qp = rb_intern("qp");
  i_kbps = rb_intern("kbps");
  i_speed = rb_intern("speed");

  cLayerVideoEncoder = rb_define_class_under(cLayer, "VideoEncoder", rb_cObject);
  rb_define_alloc_func(cLayerVideoEncoder, t_ve_allocator);

  rb_define_method(cLayerVideoEncoder, "initialize", t_ve_initialize, -1);
  rb_define_method(cLayerVideoEncoder, "encode_frame", t_ve_encode_frame, -1);
  // rb_define_method(cLayerVideoEncoder, "blit_surface", t_ve_blit_surface, -1);
  // rb_define_method(cLayerVideoEncoder, "flip_buffers", t_ve_flip_buffers, 0);
  // rb_define_method(cLayerVideoEncoder, "to_surface", t_ve_to_surface, 0);
  // rb_define_method(cLayerVideoEncoder, "on", t_ve_on, 1);
}
