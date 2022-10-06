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

  VALUE ruby_keyboard_handler;
  VALUE ruby_mouse_motion_handler;
  VALUE ruby_mouse_button_handler;
};

void lao_wnd_handle_event(struct LAO_Window *wnd, SDL_Event *ev);

#endif
