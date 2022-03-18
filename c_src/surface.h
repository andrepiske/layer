#ifndef __LAO_SURFACE_H__
#define __LAO_SURFACE_H__

#include <ruby.h>
#include <SDL.h>
#include <cairo.h>

struct LAO_Surface {
  SDL_Surface *sdl_surface;
  cairo_surface_t *cairo_surface;

  cairo_t *cairo_ctx;

  int borrowed;
};

VALUE lao_sfc_create_borrowed(SDL_Surface *sdl_surface, cairo_surface_t *cairo_surface, cairo_t *cairo_ctx);

#endif
