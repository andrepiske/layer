#include <ruby.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <stdlib.h>
#include <cairo.h>
#include <SDL.h>
#include "window.h"

VALUE cLayer;
FT_Library s_freetype_lib;

struct S_Layer {
  int version;
};

static void
t_layer_gc_mark(struct S_Layer *layer) {
}

static void
t_layer_free(struct S_Layer *layer) {
  xfree(layer);
}

static VALUE
t_layer_init(VALUE self) {
  printf("Initializing Layer...");

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  FT_Init_FreeType(&s_freetype_lib);

  printf("done!\n");
  return self;
}

static VALUE
t_layer_allocator(VALUE klass) {
  struct S_Layer *layer = (struct S_Layer*)xmalloc(sizeof(struct S_Layer));
  layer->version = 1;

  return Data_Wrap_Struct(klass, t_layer_gc_mark, t_layer_free, layer);
}

static VALUE
t_layer_sdl_poll(VALUE self) {
  SDL_Event ev;

  if (SDL_PollEvent(&ev)) {
    if (ev.type == SDL_QUIT) {
      return Qfalse;
    }
    else if (ev.type == SDL_WINDOWEVENT) {
      SDL_Window *wnd = SDL_GetWindowFromID(ev.window.windowID);
      void *usrdata = SDL_GetWindowData(wnd, "L");
      lao_wnd_handle_event((struct LAO_Window*)usrdata, &ev.window);
    }
  }

  return Qtrue;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void LAO_Window_Init();
void LAO_Surface_Init();
void LAO_Font_Init();

extern void Init_layer_ext() {
  cLayer = rb_define_module("Layer");

  rb_define_module_function(cLayer, "init", t_layer_init, 0);
  rb_define_module_function(cLayer, "sdl_poll", t_layer_sdl_poll, 0);

  LAO_Window_Init();
  LAO_Surface_Init();
  LAO_Font_Init();
}
