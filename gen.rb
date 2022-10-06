#!/usr/bin/env ruby
$:.unshift(File.absolute_path("lib", __dir__))
require 'layer'

class GenApp
  def initialize
    @sf = Layer::Surface.new(1024, 300)
    @font = Layer::Font.new("assets/pacifico.ttf", 40)
    # @font = Layer::Font.new("assets/roboto-medium.ttf", 40)
    @pic3 = Layer::Surface.new(512, 512)
  end

  def run
    draw_all

    @sf.save_png('/tmp/ff1.png')
  end

  def draw_all
    @sf.src_color(0xFF, 0xFF, 0xFF)
    @sf.rectangle(0, 0, @sf.width, @sf.height)
    @sf.fill

    @sf.src_color('#ce9751')
    @sf.rectangle(20, 30, 90, 90)
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
    @font.draw_text(@sf, 20, 120, "Hello World!")
  end
end

Layer.init

GenApp.new.run
