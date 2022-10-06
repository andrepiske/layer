#!/usr/bin/env ruby
$:.unshift(File.absolute_path("lib", __dir__))
require 'layer'

class TheApp
  def initialize
    @wnd = Layer::Window.new("Hi there", 1024, 768)

    @wnd.on(:keyboard) do |ev|
      puts('Keyboard!')
    end

    @wnd.on(:mouse_motion) do |x, y, state, which, xr, yr, ts|
      puts("M -> (#{x}, #{y}) [#{state}] [#{which}] (#{xr}, #{yr}) [#{ts}]")
        # INT2NUM(ev->motion.x),
        # INT2NUM(ev->motion.y),
        # UINT2NUM(ev->motion.state),
        # UINT2NUM(ev->motion.which),
        # INT2NUM(ev->motion.xrel),
        # INT2NUM(ev->motion.yrel),
        # UINT2NUM(ev->motion.timestamp),
    end

    @wnd.on(:mouse_button) do |ev|
      puts('mouse Button!')
    end

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

    puts("pic1 = #{@picture1.width}x#{@picture1.height}")

    @pic2 = Layer::Surface.new(512, 512)

    @font = Layer::Font.new("assets/pacifico.ttf", 40)
    @roboto = Layer::Font.new("assets/roboto-medium.ttf", 16)
    @pic3 = Layer::Surface.new(512, 512)
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

    x = Math.cos(@x / 10.0) * 800.0 + 950.0
    y = 290 # Math.sin(@x / 10.0 * 1) * 200.0 + 600.0

    @wnd.blit_surface(@picture1, x, y, 60.4, 60.3)

    @font.color = 0xdd7e85ff
    @font.draw_text(wnd_sfc, 20, 120, "Hello World!")

    @pic2.src_color(0xfb, 0xeb, 0x67) # #fbeb67
    @pic2.rectangle(0, 0, 512, 512)
    @pic2.fill

    10.times do |i|
      u = (Math.sin(@x / 10.0) + 1.0) / 2.0 + 0.5

      @pic2.src_color('#4f57ce')
      @pic2.arc(256, 256, i * 15 * u, 0, Math::PI * 2)
      @pic2.stroke
    end

    @wnd.blit_surface(@pic2, 10, 330, 512, 512)

    @pic3.src_color(0xfb, 0xeb, 0x67) # #fbeb67
    # @pic3.src_color(0xff, 0xff, 0xff) # #fbeb67
    @pic3.rectangle(0, 0, 512, 512)
    @pic3.fill

    @pic3.src_color(0, 0, 0) # #fbeb67
    @roboto.draw_text(@pic3, 20, 120, "Welcome, I'm the roboto font!")

    @wnd.blit_surface(@pic3, 600, 330, 512, 512)

    @wnd.flip_buffers
  end
end

Layer.init

TheApp.new.run
