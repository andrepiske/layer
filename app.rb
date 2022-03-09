#!/usr/bin/env ruby
$:.unshift(File.absolute_path("lib", __dir__))
require 'layer'

class TheApp
  def initialize
    @wnd = Layer::Window.new("Hi there", 1024, 768)

    @sf = Layer::Surface.new(100, 100)
  end

  def run
    @wnd.show

    loop do
      draw_all

      @wnd.flip_buffers

      break unless Layer.sdl_poll
    end
  end

  def draw_all
  end
end

Layer.init

TheApp.new.run
