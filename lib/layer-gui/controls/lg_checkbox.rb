# frozen_string_literal: true

# Herança: classe LGCheckBox herda da classe LGFrame
#    LGFrame é a classe pai (em inglês: parent) da classe LGCheckBox
class LGCheckBox < LGFrame
  attr_accessor :text
  attr_accessor :state

  def on_mouse_state(new_state)
    @mouse_focused = (new_state != :exit)
  end

  def checked?
    @state == :checked
  end

  def on_mouse_button(x, y, button, btstate, clicks, which, ts)
    if button == 1 && btstate == 0
      # previous_state = checked?

      @state = (checked? ? :unchecked : :checked)
      puts "change state to #{@state}"

      if @state == :checked
        @animation_start = Time.now.to_f
      end
    end
  end

  # método render_self
  def render_self(dest_surface, ox, oy)
    sfc = Layer::Surface.new(@width, @height)

    sfc.preserving_state do |s|
      sfc.line_width = 3.0
      sfc.src_color('#ff0000')
      sfc.rectangle(0, 0, @width, @height)
      sfc.stroke

      if checked?
        time_delta = @animation_start ? (Time.now.to_f - @animation_start) : 0.0
        time_delta = ([time_delta, 0.3].min / 0.3)
        puts("#{Time.now.to_f} -- #{time_delta}")

        five = 5 * time_delta

        sfc.line_width = 2.0
        sfc.src_color('#00ff00')
        # sfc.src_color('#5555ff')
        sfc.move_to(five, five)
        sfc.line_to(@width - five, @height - five)
        sfc.stroke

        sfc.move_to(five, @height - five)
        sfc.line_to(@width - five, five)
        sfc.stroke
      end
    end

    # if @mouse_focused
    #   sfc.src_color('#1045db')
    # else
    #   sfc.src_color('#3969ef')
    # end
    # sfc.rectangle(0, 0, @width, @height)
    # sfc.fill
    #
    # font = context.font(12)
    # font.color = 0xffffffff
    # metrics = font.calc_metrics(@text)
    # baseline = metrics[:baseline]
    # max_height = metrics[:height]
    #
    # font.draw_text(sfc,
    #   (@width - metrics[:width]) / 2,
    #   baseline + (@height - max_height) / 2, @text)
    #
    # sfc.preserving_state do |s|
    #   s.src_color(0xff, 0xff, 0xff)
    #   s.move_to(0, 0)
    #   s.line_to(@width, 0)
    #   s.line_to(@width, @height)
    #   s.line_to(0, @height)
    #   s.line_to(0, 0)
    #   s.line_width = 1
    #   s.stroke
    # end

    dest_surface.blit_surface(sfc, ox, oy)
  end
end
