# frozen_string_literal: true

# Herança: classe LGButton herda da classe LGFrame
class LGButton < LGFrame
  attr_accessor :text

  def on_mouse_state(new_state)
    @mouse_focused = (new_state != :exit)
  end

  def render_self(dest_surface, ox, oy)
    sfc = Layer::Surface.new(@width, @height)

    if @mouse_focused
      sfc.src_color('#f7ac1a')
      # sfc.src_color('#1045db')
    else
      sfc.src_color('#3969ef')
    end
    sfc.rectangle(0, 0, @width, @height)
    sfc.fill

    font = context.font(12)
    font.color = 0xffffffff
    metrics = font.calc_metrics(@text)
    baseline = metrics[:baseline]
    max_height = metrics[:height]

    font.draw_text(sfc,
      (@width - metrics[:width]) / 2,
      baseline + (@height - max_height) / 2, @text)

    sfc.preserving_state do |s|
      s.src_color(0xff, 0xff, 0xff)
      s.move_to(0, 0)
      s.line_to(@width, 0)
      s.line_to(@width, @height)
      s.line_to(0, @height)
      s.line_to(0, 0)
      s.line_width = 1
      s.stroke
    end

    dest_surface.blit_surface(sfc, ox, oy)
  end
end
