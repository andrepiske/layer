# frozen_string_literal: true

class Layer::Surface
  def src_color(*args)
    c = args[0]
    unless args.length == 1 && c.is_a?(Layer::Color)
      c = Layer::Color.new(*args)
    end

    _src_color(c.r, c.g, c.b, c.a)
  end

  def preserving_state
    push_state

    yield(self)
  ensure
    pop_state
  end
end
