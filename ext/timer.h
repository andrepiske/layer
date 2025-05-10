#ifndef __LAO_TIMER_H__
#define __LAO_TIMER_H__

#include <ruby.h>
#include <SDL.h>

struct LAO_Timer {
  VALUE self;
  SDL_TimerID sdl_timer_id;
  VALUE cb_func;
  int interval;
  char state;
  char fire_once;
};

#endif
