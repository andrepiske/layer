#ifndef __LAO_WINDOW_H__
#define __LAO_WINDOW_H__

#include <ruby.h>
#include <cairo.h>
#include <SDL.h>

struct LAO_Window {
  SDL_Window *sdl_wnd;

  SDL_Surface *sdl_surface;
  cairo_t *cairo_ctx;
  cairo_surface_t *cairo_surface;
};

#endif
