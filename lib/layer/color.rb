# frozen_string_literal: true

# Color is always encoded in ARGB
class Layer::Color
  attr_accessor :value

  # Color.new('#6decc2') - web color. Can also have alpha
  # Color.new([r, g, b])
  # Color.new([r, g, b, a])
  # Color.new(r, g, b)
  # Color.new(r, g, b, a)
  # Color.new(value)
  # Color.new - white
  def initialize(*args)
    if args.length > 1
      @value = self.class.rgba2num(*args)
    elsif args.length == 1
      x = args[0]
      case x
      when Array
        @value = self.class.rgba2num(*x)
      when String
        from_s!(x)
      else
        @value = x.to_i & 0xFFFF_FFFF
      end
    else
      @value = 0xFFFF_FFFF
    end
  end

  def from_s!(s)
    if s.length == 4 # e.g. #FFF
      @value = self.class.rgba2num(
        (s[1] * 2).to_i(16),
        (s[2] * 2).to_i(16),
        (s[3] * 2).to_i(16)
      )
    elsif s.length == 7 || s.length == 9 # e.g. #FFFFFF
      @value = self.class.rgba2num(
        s[1..2].to_i(16),
        s[3..4].to_i(16),
        s[5..6].to_i(16),
        s.length > 7 ? s[7..8].to_i(16) : 0xFF
      )
    else
      raise "Invalid color string: '#{s}'. It must be either 4, 7 or 9 chars long"
    end
  end

  def to_s(alpha: true)
    alpha ? ('#%02x%02x%02x%02x' % [r, g, b, a]) : ('#%02x%02x%02x' % [r, g, b])
  end

  def to_i
    @value
  end

  def to_a(alpha: true)
    alpha ? [r, g, b, a] : [r, g, b]
  end

  def a
    (@value >> 24) & 0xFF
  end

  def r
    (@value >> 16) & 0xFF
  end

  def g
    (@value >> 8) & 0xFF
  end

  def b
    @value & 0xFF
  end

  def a=(v)
    @value = (@value & 0x00FF_FFFF) | ((v & 0xFF) << 24)
  end

  def r=(v)
    @value = (@value & 0xFF00_FFFF) | ((v & 0xFF) << 16)
  end

  def g=(v)
    @value = (@value & 0xFFFF_00FF) | ((v & 0xFF) << 8)
  end

  def b=(v)
    @value = (@value & 0xFFFF_FF00) | ((v & 0xFF))
  end

  def self.rgba2num(r, g, b, a=0xFF)
    ((a.to_i & 0xFF) << 24) |
    ((r.to_i & 0xFF) << 16) |
    ((g.to_i & 0xFF) << 8) |
    ((b.to_i) & 0xFF)
  end

  def self.num2rgba(n)
    [
      ((n >> 16) & 0xFF),
      ((n >> 8) & 0xFF),
      ((n) & 0xFF),
      ((n >> 24) & 0xFF)
    ]
  end
end
