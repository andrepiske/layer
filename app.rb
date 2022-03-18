#!/usr/bin/env ruby
$:.unshift(File.absolute_path("lib", __dir__))
require 'layer'

class TheApp
  def initialize
    @wnd = Layer::Window.new("Hi there", 1024, 768)

    @sf = Layer::Surface.new(100, 100)

    @picture1 = Layer::Surface.new(0, 0)
    raise "could not load png" unless @picture1.load_png!("assets/element-23.png")

    @picture1 = Layer::Surface.new(128, 128)
    @picture1.src_color(0, 255, 0)
    @picture1.rectangle(0, 0, 128, 128)
    @picture1.fill
    @picture1.src_color(0, 0, 255)
    @picture1.rectangle(12, 12, 40, 20)
    @picture1.stroke

    @font = Layer::Font.new("assets/pacifico.ttf", 40)
  end

  def run
    @wnd.show

    @x = 0

    loop do
      draw_all

      break unless Layer.sdl_poll
    end
  end

  def draw_all
    wnd_sfc = @wnd.to_surface

    @x += 1

    x = Math.cos(@x / 10.0 * 1) * 200.0 + 600.0
    y = 200 # Math.sin(@x / 10.0 * 1) * 200.0 + 600.0

    @wnd.blit_surface(x, y, @picture1, 60.4, 60.3)
    @font.draw_text(wnd_sfc, 20, 120, "Hello World!")

    @wnd.flip_buffers
  end
end

Layer.init

TheApp.new.run
