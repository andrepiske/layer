# frozen_string_literal: true

class LGContext
  attr_accessor :font_path

  attr_accessor :_mouse_last_over
  attr_accessor :_dragging_frame

  def initialize
    @_mouse_last_over = nil
    @_dragging_frame = nil
  end

  def font(size)
    @font ||= Layer::Font.new(@font_path, size)
  end
end
