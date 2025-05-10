#!/usr/bin/env ruby
$:.unshift(File.absolute_path("../../lib", __dir__))
require 'layer'

class GenApp
  def initialize
    @sf = Layer::Surface.new(2, 2)
  end

  def run
    draw_all

    @sf.save_png('/tmp/ff1.png')
  end

  def draw_all
    @sf.src_color(0xFF, 0xFF, 0xFF)
    @sf.rectangle(0, 0, @sf.width, @sf.height)
    @sf.fill

    @sf.set_pixel(0, 0, 255, 0, 0)
    @sf.set_pixel(1, 0, 0, 255, 0)
    @sf.set_pixel(0, 1, 0, 0, 255)
    @sf.set_pixel(1, 1, 255, 255, 255)
  end
end

Layer.init

GenApp.new.run
