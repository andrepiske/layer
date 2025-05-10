#!/usr/bin/env ruby
$:.unshift(File.absolute_path("../../lib", __dir__))
require 'layer'
require 'securerandom'

class CaptchaGenerator
  def initialize
    @sf = Layer::Surface.new(600, 250)
    @font = Layer::Font.new("../shared/assets/pacifico.ttf", 40)
  end

  def run
    @captcha_string = SecureRandom.alphanumeric(8)

    draw_all

    @sf.save_png('/tmp/ff1.png')
  end

  def draw_all
    @sf.src_color(0xFF, 0xFF, 0xFF)
    @sf.rectangle(0, 0, @sf.width, @sf.height)
    @sf.fill

    @sf.src_color('#ce9751')
    @sf.rectangle(0, 0, 90, 90)
    @sf.fill

    @sf.src_color('#42bb66')
    @sf.rectangle(160, 70, 90, 90)
    @sf.fill

    @sf.src_color('#4776e8')
    @sf.rectangle(270, 60, 90, 90)
    @sf.fill

    # @sf.transform_matrix = [ [1, 0, 0], [0, 1, 0], [0, 0, 1], ]

    @font.color = '#dd7e85'
    @font.color = '#857edd'
    @font.color = '#be2525'

    text = Layer::Surface.new(600, 250)
    @font.draw_text(text, 30, 100, "Hello World")

    @sf.rotate(-20.0 * Math::PI / 180.0)
    @sf.blit_surface(text, 0, 0)

    # @font.draw_text(@sf, 30, 100, @captcha_string)
  end
end

Layer.init

CaptchaGenerator.new.run
