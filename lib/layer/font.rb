# frozen_string_literal: true

class Layer::Font
  def color=(c)
    unless c.is_a?(Layer::Color)
      c = Layer::Color.new(c)
    end

    self._color = c.to_i
  end

  def color
    Layer::Color.new(_color)
  end
end
