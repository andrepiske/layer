#ifndef __LAO_SURFACE_H__
#define __LAO_SURFACE_H__

#include <ruby.h>
#include <SDL.h>
#include <cairo.h>

struct LAO_Surface {
  SDL_Surface *sdl_surface;
  cairo_surface_t *cairo_surface;

  cairo_t *cairo_ctx;
};

#endif
