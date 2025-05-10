#include "timer.h"
#include <ruby/thread.h>

Uint32 layer_timer_user_event_type = 0;
extern VALUE cLayer;
VALUE cLayerTimer;

static void _set_timer_state(struct LAO_Timer*, char);

#define DECLARETIMER(o) \
  struct LAO_Timer *timer; \
  TypedData_Get_Struct((o), struct LAO_Timer, &lao_timer_datatype, (timer));

static void
t_timer_gc_mark(void *_timer) {
  struct LAO_Timer *timer = (struct LAO_Timer*)_timer;
  if (timer->cb_func) {
    rb_gc_mark(timer->cb_func);
  }
}

static void
t_timer_free(void *_timer) {
  struct LAO_Timer *timer = (struct LAO_Timer*)_timer;
  _set_timer_state(timer, 0);
  xfree(timer);
}

static size_t
t_timer_size(const void *_timer) {
  return 16;
}

static const rb_data_type_t lao_timer_datatype = {
  .wrap_struct_name = "Layer::Timer",
  .function = {
    .dmark = t_timer_gc_mark,
    .dfree = t_timer_free,
    .dsize = t_timer_size,
  },
  .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE
t_timer_allocator(VALUE klass) {
  struct LAO_Timer *timer = (struct LAO_Timer*)xmalloc(sizeof(struct LAO_Timer));
  timer->cb_func = 0;
  timer->sdl_timer_id = 0;
  timer->state = 0;
  timer->fire_once = 0;
  timer->interval = 0;
  timer->self = TypedData_Wrap_Struct(klass, &lao_timer_datatype, timer);
  return timer->self;
}

static VALUE
t_timer_initialize(int argc, const VALUE *argv, VALUE self) {
  DECLARETIMER(self);
  rb_need_block();

  VALUE _interval = 0;
  VALUE _second = 0;
  VALUE cb_func = rb_block_proc();
  rb_scan_args(argc, argv, "11", &_interval, &_second);

  int starting_state = 1;
  if (_second == Qfalse) {
    // Start it paused
    starting_state = 0;
  }
  else if (_second == ID2SYM(rb_intern("once"))) {
    // Fire once and stop
    timer->fire_once = 1;
  }

  timer->cb_func = cb_func;
  timer->interval = RB_NUM2INT(_interval);
  _set_timer_state(timer, starting_state);

  return self;
}

static Uint32 _timer_callback(Uint32, void*);

static void
_re_register_time(struct LAO_Timer *timer) {
  timer->sdl_timer_id = SDL_AddTimer(
    (Uint32)timer->interval,
    _timer_callback,
    (void*)timer
  );
}

static Uint32
_timer_callback(Uint32 interval, void* datum) {
  struct LAO_Timer *timer = (struct LAO_Timer*)datum;
  if (!timer->fire_once) {
    _re_register_time(timer);
  } else {
    timer->state = 0;
  }

  SDL_Event event;
  SDL_zero(event);
  event.type = layer_timer_user_event_type;
  event.user.code = 1;
  event.user.data1 = (void*)datum;
  event.user.data2 = 0;
  SDL_PushEvent(&event);
  return 0;
}

static void
_set_timer_state(struct LAO_Timer *timer, char new_state) {
  if (new_state == timer->state) {
    return;
  }

  if (new_state) {
    _re_register_time(timer);
  } else {
    SDL_RemoveTimer(timer->sdl_timer_id);
    timer->sdl_timer_id = 0;
  }

  timer->state = new_state;
}

static VALUE
t_timer_interval(VALUE self) {
  DECLARETIMER(self);
  return RB_INT2NUM(timer->interval);
}

static VALUE
t_timer_interval_set(VALUE self, VALUE _new_interval) {
  DECLARETIMER(self);
  int new_interval = RB_NUM2INT(_new_interval);
  if (new_interval != timer->interval) {
    timer->interval = new_interval;
    if (timer->state) {
      _set_timer_state(timer, 0);
      _set_timer_state(timer, 1);
    }
  }
  return self;
}

static VALUE
t_timer_start(VALUE self) {
  DECLARETIMER(self);
  _set_timer_state(timer, 1);
  return self;
}

static VALUE
t_timer_stop(VALUE self) {
  DECLARETIMER(self);
  _set_timer_state(timer, 0);
  return self;
}

static VALUE
t_timer_fire_once_p(VALUE self) {
  DECLARETIMER(self);
  return timer->fire_once ? Qtrue : Qfalse;
}

////////////////////////////////////////////////////////////////////////

void
LAO_Timer_Init() {
  cLayerTimer = rb_define_class_under(cLayer, "Timer", rb_cObject);
  rb_define_alloc_func(cLayerTimer, t_timer_allocator);

  rb_define_method(cLayerTimer, "initialize", t_timer_initialize, -1);

  rb_define_method(cLayerTimer, "interval", t_timer_interval, 0);
  rb_define_method(cLayerTimer, "interval=", t_timer_interval_set, 1);

  rb_define_method(cLayerTimer, "start", t_timer_start, 0);
  rb_define_method(cLayerTimer, "stop", t_timer_stop, 0);

  rb_define_method(cLayerTimer, "fire_once?", t_timer_fire_once_p, 0);

  layer_timer_user_event_type = SDL_RegisterEvents(1);
}
