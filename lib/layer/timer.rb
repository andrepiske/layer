# frozen_string_literal: true

module Layer
  class TimerHolder
    def initialize
      @timers = {}
    end

    def add(timer)
      @timers[timer] = true

      timer
    end

    def drop(timer)
      @timers.delete(timer)
    end
  end

  class Timer
    def self.hold_timer(interval, kind=nil, &blk)
      _timer_holder.add(new(interval, kind, &blk))
    end

    def self.drop_hold(timer)
      _timer_holder.drop(timer)
    end

    def self._timer_holder
      @timer_holder ||= TimerHolder.new
    end
  end
end
