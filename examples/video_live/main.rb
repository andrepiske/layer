#!/usr/bin/env ruby
$:.unshift(File.absolute_path("../../lib", __dir__))
require 'layer'
require 'layer/video_encoder'
require 'time'

class ControlledLoop
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

class GenApp
  def initialize
    @sf = Layer::Surface.new(1024, 400)
    # @sf = Layer::Surface.new(1920, 1080)
    # @sf = Layer::Surface.new(256, 256)
    @font = Layer::Font.new("../shared/assets/pacifico.ttf", 40)
    # @font = Layer::Font.new("assets/roboto-medium.ttf", 40)
    @pic3 = Layer::Surface.new(512, 512)
  end

  def run
    framerate = 30
    # encoder = Layer::VideoEncoder.new(1920, 1080, gop: 360, qp: 5, framerate:)
    encoder = Layer::VideoEncoder.new(@sf.width, @sf.height, framerate:, for_streaming: true)
    puts("encoder = #{encoder}")

    ControlledLoop.new(framerate).run do |i|
      t = i / 1000.to_f
    # (0...count).each do |i|
      # t = i / (count - 1).to_f

      @vid_t = t
      @base_x = (20 + (t * 470.0))
      @base_y = (10 + (t * 230.0))
      draw_all(i)

      buffer = encoder.encode_frame(@sf)

      # if i % 150 == 0 || i == (count-1)
      #   puts "%.1f%%" % [i / (count-1).to_f * 100.0]
      # end

      # @sf.save_png('./vid/frame-%04d.png' % i)
      # puts "Gen frame #{i}/#{count}"
    end

    puts "done!"
  end

  def draw_all(time)
    @sf.src_color(0xFF, 0xFF, 0xFF)
    @sf.rectangle(0, 0, @sf.width, @sf.height)
    @sf.fill

    @sf.src_color('#ce9751')
    @sf.rectangle(@base_x, @base_y, 90, 90)
    @sf.fill

    @sf.src_color('#42bb66')
    @sf.rectangle(160, 70, 90, 90)
    @sf.fill

    @sf.src_color('#4776e8')
    @sf.rectangle(270, 60, 90, 90)
    @sf.fill

    @font.color = '#dd7e85'
    @font.color = '#857edd'
    @font.color = '#be2525'
    @font.draw_text(@sf,
      200 + Math.cos(25 * Math::PI * 2.0 * @vid_t)*20.0,
      160 + Math.sin(5 * Math::PI * 2.0 * @vid_t)*70.0,
      "Hey: #{time / 30} [#{time}]")
  end
end

Layer.init

GenApp.new.run
