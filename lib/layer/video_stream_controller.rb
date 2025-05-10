# frozen_string_literal: true

class Layer::VideoStreamController
  attr_reader :started_at, :fps, :index

  def initialize(fps)
    @fps = fps
    @index = 0 # frame number
    @buffer_us = 1_000_000 * 10
    @render_times = []
  end

  def start_reporter_thread
    @reporter = Thread.new do
      loop do
        buf = @last_buffered ? ("%.2f" % [@last_buffered / 1_000_000.to_f]) : 'n/a'
        rt = @render_times[0...100]
        rt_avg = rt.empty? ? 0.0 : (rt.sum / rt.length.to_f)
        rt_avg_str = "%.3fms" % [rt_avg / 1000.0]

        puts "i=#{@index}, buf=#{buf}, rt=#{rt_avg_str}"
        sleep 0.7
      end
    end
  end

  def run
    start_reporter_thread

    @started_at = get_time_us
    loop do
      a = get_time_us
      yield(@index)
      @index += 1
      b = get_time_us

      @render_times << (b - a)
      if @render_times.length > 2_000
        @render_times = @render_times[0...100]
      end

      loop do
        elapsed_time = get_time_us - @started_at
        elapsed_time_frames = (@index * 1_000_000) / @fps
        buffered = elapsed_time_frames - elapsed_time
        @last_buffered = buffered

        break if (buffered < @buffer_us)

        if buffered > 1_200_000
          sleep(1)
        elsif buffered > 500_000
          sleep(0.1)
        else
          sleep(0.001)
        end
      end
    end
  end

  private

  def get_time_us
    Process.clock_gettime(:CLOCK_MONOTONIC, :microsecond)
  end
end
