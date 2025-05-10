#ifndef __LAO_VIDEO_ENCODER_H__
#define __LAO_VIDEO_ENCODER_H__

#include <ruby.h>

struct LAO_VideoEncoder {
  int qp_min;
  int qp_max;
  int kbps;

  unsigned int width, height;
  unsigned char *encoding_buffer;

  void *encoder_obj;
  void *scratch_mem;
  void *create_param;
};

#endif

