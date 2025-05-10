#!/usr/bin/env ruby
$:.unshift(File.absolute_path("../../lib", __dir__))
require 'layer'
require 'layer-gui'
require 'get_process_mem'
require 'objspace'
require 'debug'

class TheApp
  def initialize
    @wnd = Layer::Window.new("Olha esta janela", 1024, 768)

    @font = Layer::Font.new("../shared/assets/pacifico.ttf", 40)
    @roboto = Layer::Font.new("../shared/assets/roboto-medium.ttf", 16)

    @gui_ctx = LGContext.new
    @gui_ctx.font_path = "../shared/assets/roboto-medium.ttf"

    @gui = LGFrame.new(@gui_ctx)
    @gui.width = 1024
    @gui.height = 768

    but_1 = LGButton.new(@gui_ctx)
    # but_1.pos_x, but_1.pos_y = 50, 50
    but_1.pos_x = 150
    but_1.pos_y = 50
    but_1.text = "First button"

    puts("posição x do botão = #{but_1.pos_x}")

    @gui.add_child(but_1)

    but_2 = LGButton.new(@gui_ctx)
    but_2.pos_x, but_2.pos_y = 50, 150
    but_2.text = "Second one"
    but_2.height = 40
    @gui.add_child(but_2)

    cb1 = LGCheckBox.new(@gui_ctx)
    cb1.pos_x, cb1.pos_y = 400, 150
    cb1.text = "A checkbox"
    cb1.height = 30
    cb1.width = 30
    @gui.add_child(cb1)

    @dx = 600
    @dy = 330
    @dx = 0
    @dy = 0

    @dsx = 0
    @dsy = 0

    # @sprite_id = 14*24 + 11
    @sprite_id = 0
    @dragging = nil

    @winter = Layer::Surface.new(0, 0)
    raise "could not load png" unless @winter.load_png!("#{ENV['HOME']}/Downloads/winter-creatures.png")
    @monpic = Layer::Surface.new(0, 0)
    raise "could not load png" unless @monpic.load_png!("#{ENV['HOME']}/Downloads/mon.png")

    @wnd.on(:mouse_motion) do |x, y, state, which, xr, yr, ts|
      # puts("M -> (#{x}, #{y}) [#{state}] [#{which}] (#{xr}, #{yr}) [#{ts}]")
      next if @gui.handle_mouse_motion(x, y, state, which, xr, yr, ts)

      if @dragging != nil
        if @dragging[:shift]
          @dragging[:dsx] = x - @dragging[:x]
          @dragging[:dsy] = y - @dragging[:y]

          @dsx = @dragging[:osx] + @dragging[:dsx]
          @dsy = @dragging[:osy] + @dragging[:dsy]
        else
          @dragging[:dx] = x - @dragging[:x]
          @dragging[:dy] = y - @dragging[:y]

          @dx = @dragging[:ox] + @dragging[:dx]
          @dy = @dragging[:oy] + @dragging[:dy]
        end

        # puts "dx=#{@dx}, dy=#{@dy}"
        # puts "W=#{@wnd.size[0]}, H=#{@wnd.size[1]}"
      end
    end

    @wnd.on(:mouse_button) do |x, y, button, state, clicks, which, ts|
      next if @gui.handle_mouse_button(x, y, button, state, clicks, which, ts)

      # state: 1==mousedown, 0==mouseup
      # button: 1=LMB, 3=RMB, 2=MMB
      # clicks: click count. Usually 1, can be zero (e.g. 3-finger drag)
      # which: seems to be always zero?
      # ts: relative timestamp in ms

      shift = @wnd.modf_keys[:shift]
      puts("B -> (#{x}, #{y}, b=#{button}, c=#{clicks}, w=#{which}, s=#{state}, #{ts}) (#{shift ? 'shift' : '-'})")
      if state == 1 && button == 1
        @dragging = { x:, y:, dx: 0, dy: 0, ox: @dx, oy: @dy, osx: @dsx, osy: @dsy, shift: }
      elsif state == 0 && button == 1
        @dragging = nil
      end

      # elem = @gui.find_element_at_pos(x, y)
      # puts "elem = #{elem.class}"
    end

    @wnd.on(:keyboard) do |state, repeat, scancode, sym, mod, ts|
      # puts("K -> (#{scancode}, #{sym}, #{mod}, #{state}, #{repeat})")
      if state == 1 # press down
        if scancode == 82 # up
          @sprite_id += 1
        elsif scancode == 81 # down
          @sprite_id -= 1
        end
        puts("sprite = #{@sprite_id}")
      end
    end

    @sf = Layer::Surface.new(100, 100)

    @picture1 = Layer::Surface.new(0, 0)
    raise "could not load png" unless @picture1.load_png!("../shared/assets/element-23.png")

    # @picture1 = Layer::Surface.new(128, 128)
    # @picture1.src_color(0, 255, 0)
    # @picture1.rectangle(0, 0, 128, 128)
    # @picture1.fill
    # @picture1.src_color(0, 0, 255)
    # @picture1.rectangle(12, 12, 40, 20)
    # @picture1.stroke

    puts("pic1 = #{@picture1.width}x#{@picture1.height}")

    @pic2 = Layer::Surface.new(512, 512)

    @pic3 = Layer::Surface.new(512, 512)
  end

  def run
    @wnd.show

    # @x = 0
    # avg_values = []
    # start_time = Process.clock_gettime(Process::CLOCK_REALTIME, :nanosecond)
    # no_calls = 0
    #
    # loop do
    #   curr_time = Process.clock_gettime(Process::CLOCK_REALTIME, :nanosecond)
    #
    #   draw_all
    #
    #   no_calls += 1
    #
    #   diff = curr_time - start_time
    #   if diff >= 1_000_000_000
    #     avg = avg_values.empty? ? 0.0 : (avg_values.sum.to_f / avg_values.length)
    #     memsize = GetProcessMem.new.mb
    #     ma = ObjectSpace.memsize_of_all / (1024 * 1024).to_f
    #
    #     puts "leap! - #{no_calls} [+#{diff - 1_000_000_000}] AVG #{'%.3f' % avg} M=#{memsize}, #{'%.3f' % ma}"
    #
    #     avg_values = ([no_calls] + avg_values)[0...10]
    #     no_calls = 0
    #     start_time = Process.clock_gettime(Process::CLOCK_REALTIME, :nanosecond)
    #   end
    #
    #   break unless Layer.sdl_poll
    # end

    loop do
      draw_all

      break unless Layer.sdl_poll
    end
  end

  def draw_all
    wnd_sfc = @wnd.to_surface

    @gui.render(wnd_sfc, 0, 0)

    # @roboto.color = 0xffffffff
    # @roboto.draw_text(wnd_sfc, 20, 120, "just testing")

    @wnd.flip_buffers
  end

  def draw_all_2
    wnd_sfc = @wnd.to_surface
    @x += 0.3

    x = Math.cos(@x / 10.0) * 80 + 100
    y = 100 # Math.sin(@x / 10.0 * 1) * 200.0 + 600.0

    # @wnd.blit_surface(@picture1, x, y)
    # @wnd.blit_surface(@picture1, x, y, 282-60, 282-60, 30, 30)

    @font.color = 0xdd7e85ff
    @font.draw_text(wnd_sfc, 20, 120, "sp = #{@sprite_id}, x=#{'%.2f' % @x}")

    @pic2.src_color(0xfb, 0xeb, 0x67) # #fbeb67
    @pic2.rectangle(0, 0, 512, 512)
    @pic2.fill

    10.times do |i|
      u = (Math.sin(@x / 10.0) + 1.0) / 2.0 + 0.5

      @pic2.src_color('#4f57ce')
      @pic2.arc(256, 256, i * 15 * u, 0, Math::PI * 2)
      @pic2.stroke
    end

    @wnd.to_surface.blit_surface(@pic2, 10, 330, 512, 512)

    @pic3.src_color(0xfb, 0xeb, 0x67) # #fbeb67
    # @pic3.src_color(0xff, 0xff, 0xff) # #fbeb67
    @pic3.rectangle(0, 0, 512, 512)
    @pic3.fill

    @pic3.src_color(0, 0, 0) # #fbeb67
    @roboto.draw_text(@pic3, 20, 120, "Welcome, I'm the roboto font!")

    # @wnd.blit_surface(@pic3, @dx, @dy, 128, 256, 20, 20)

    xx = Layer::Surface.new(64, 64)
    # xx.src_color(0xff, 0xff, 0xff)
    # xx.rectangle(0, 0, 64, 64)
    # xx.fill

    @sprite_id = 366 + ((@x / 50.0).floor % 6)

    sprite_x = (@sprite_id % 24.0).floor * 32
    sprite_y = (@sprite_id / 24.0).floor * 32

    xx.blit_surface(@monpic, 0, 0, 32, 32, sprite_x, sprite_y)
    # xx.blit_surface(@monpic, 0, 0, 64, 64, @dx, @dy)
    # xx.blit_surface(@monpic, 0, 0, 64, 64, @dx, @dy, 1.5, 1.5)
    # @wnd.blit_surface(xx, 100+@dsx, 100+@dsy)

    @wnd.to_surface.blit_surface(xx, 100+@dsx, 100+@dsy)

    @wnd.flip_buffers
  end
end

puts("Starting with ruby #{RUBY_VERSION}/#{RUBY_ENGINE}/#{RUBY_PLATFORM}")

Layer.init
TheApp.new.run
