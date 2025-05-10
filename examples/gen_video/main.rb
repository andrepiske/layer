#!/usr/bin/env ruby
$:.unshift(File.absolute_path("../../lib", __dir__))
require 'layer'
require_relative './particles'

class GenApp
  def initialize
    @sf = Layer::Surface.new(1024, 400)
    @smokes = (1..4).map do |idx|
      Layer::Surface.new(0, 0).tap do |surf|
        path = "../shared/assets/smoke/smoke_%02d.png" % [idx]
        raise "could not load #{path}" unless surf.load_png!(path)
      end
    end

    @particles = Particles.new(100, @smokes)

    # @sf = Layer::Surface.new(1920, 1080)
    # @sf = Layer::Surface.new(256, 256)
    # @font = Layer::Font.new("../shared/assets/pacifico.ttf", 40)
    @font = Layer::Font.new("../shared/assets/roboto-medium.ttf", 40)
    @pic3 = Layer::Surface.new(512, 512)
  end

  def run
    framerate = 60
    encoder = Layer::VideoEncoder.new(@sf.width, @sf.height, 'out-lay.mp4',
      # libx264_preset: 'slow',
      # pixel_format: 'yuv444p',
      framerate:)

    count = framerate * 10
    (0...count).each do |i|
      t = i / (count - 1).to_f

      @vid_t = t
      @base_x = (20 + (t * 470.0))
      @base_y = (10 + (t * 230.0))
      @particles.advance(0.1)
      draw_all(i)

      encoder.encode_frame(@sf)

      if i % (100 * framerate) == 0 || i == (count-1)
        puts "%.1f%%" % [i / (count-1).to_f * 100.0]
      end

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

    @sf.src_color('#1254f2')

    @sf.translate(370, 60)
    @sf.rotate(@vid_t * Math::PI * 2.0 * 3.0)
    # @sf.scale(1.0 + @vid_t * 10.0, 1.0)

    @sf.rectangle(-45, -45, 90, 90)
    # @sf.rectangle(370, 60, 90, 90)
    @sf.fill
    @sf.identity

    @particles.each do |par|
      @sf.identity
      @sf.translate(par[:x] + 32, par[:y] + 32)
      # @sf.rotate(par[:life] * Math::PI * 2.0)
      @sf.blit_surface(par[:sprite], -32, -32)
    end
    @sf.identity

    @font.color = '#dd7e85'
    @font.color = '#857edd'
    @font.color = '#cc1414'
    @font.draw_text(@sf,
      200 + Math.cos(5 * Math::PI * 2.0 * @vid_t)*20.0,
      160 + Math.sin(3 * Math::PI * 2.0 * @vid_t)*90.0,
      "Tis some text: #{time / 30} [#{time}]")
  end
end

Layer.init

GenApp.new.run
