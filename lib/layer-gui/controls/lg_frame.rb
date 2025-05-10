# frozen_string_literal: true

class LGFrame
  # attr_accessor :pos_x, :pos_y, :width, :height
  attr_accessor :pos_y, :width, :height
  attr_accessor :width, :height
  attr_accessor :context
  attr_accessor :children
  attr_accessor :name

  # attr_accessor :pos_x
  # equivalente a:
  def pos_x # Getter
    @pos_x
  end

  def pos_x=(value) # Setter
    @pos_x = value
  end

  # attr_accessor :double_buffered

  def initialize(ctx)
    @context = ctx
    @pos_x = 0
    @pos_y = 0
    @width = 200
    @height = 90
  end

  def add_child(c)
    @children ||= []
    @children << c
  end

  def point_inside?(x, y)
    x >= @pos_x && y >= @pos_y && (
      x < (@pos_x + @width) &&
      y < (@pos_y + @height)
    )
  end

  def render(dest_surface, dx, dy)
    cx, cy = dx + @pos_x, dy + @pos_y

    render_self(dest_surface, cx, cy)
    return unless children

    children.each do |ch|
      ch.render(dest_surface, cx, cy)
    end
  end

  # @overridable
  def render_self(dest_surface, ox, oy)
  end

  # @overridable
  def on_click(event)
  end

  # @overridable -- overridden
  def on_mouse_button(x, y, button, state, clicks, which, ts)
  end

  # @overridable
  def on_mouse_move(x, y, state, which, xy, yr, ts)
  end

  # @overridable
  def on_mouse_state(new_state)
    # new_state in [:drag_begin, :drag_end, :enter, :exit]
  end

  def find_element_at_pos(x, y)
    return nil unless point_inside?(x, y)
    if @children
      ch = @children.find do |c|
        c.find_element_at_pos(x - @pos_x, y - @pos_y)
      end

      return ch if ch
    end

    self
  end

  def handle_mouse_button(x, y, button, state, clicks, which, ts)
    return false unless point_inside?(x, y)
    children_handled = false
    if children
      children_handled = children.any? do |c|
        c.handle_mouse_button(x - pos_x, y - pos_y, button, state, clicks, which, ts)
      end
    end

    unless children_handled
      # if self != @context._mouse_last_over
      #   if last = @context._mouse_last_over
      #     last.on_mouse_state(:exit)
      #   end
      #
      #   @context._mouse_last_over = self
      #   self.on_mouse_state(:enter)
      # end

      on_mouse_button(x - @pos_x, y - @pos_y, button, state, clicks, which, ts)
    end

    true
  end

  def handle_mouse_motion(x, y, state, which, xr, yr, ts)
    return false unless point_inside?(x, y)
    children_handled = false
    if children
      children_handled = children.any? do |c|
        c.handle_mouse_motion(x - @pos_x, y - @pos_y, state, which, xr, yr, ts)
      end
    end

    unless children_handled
      if self != @context._mouse_last_over
        if last = @context._mouse_last_over
          last.on_mouse_state(:exit)
        end

        @context._mouse_last_over = self
        self.on_mouse_state(:enter)
      end

      on_mouse_move(x - @pos_x, y - @pos_y, state, which, xr, yr, ts)
    end

    true
  end
end
