# frozen_string_literal: true

class Layer::Matrix33
  attr_accessor :p00, :p01, :p02
  attr_accessor :p10, :p11, :p12
  attr_accessor :p20, :p21, :p22

  def initialize(src_array=nil)
    if src_array
      from_array!(src_array)
    else
      identity!
    end
  end

  def from_array!(v)
    @p00, @p01, @p02 = v[0][0], v[0][1], v[0][2]
    @p10, @p11, @p12 = v[1][0], v[1][1], v[1][2]
    @p20, @p21, @p22 = v[2][0], v[2][1], v[2][2]
  end

  def mul_vector(vec)
    [
      @p00 * vec[0] + @p01 * vec[1] + @p02 * vec[2],
      @p10 * vec[0] + @p11 * vec[1] + @p12 * vec[2],
      @p20 * vec[0] + @p21 * vec[1] + @p22 * vec[2]
    ]
  end

  def identity!
    @p00, @p01, @p02 = 1.0, 0.0, 0.0
    @p10, @p11, @p12 = 0.0, 1.0, 0.0
    @p20, @p21, @p22 = 0.0, 0.0, 1.0
  end

  def to_a
    [
      [@p00, @p01, @p02],
      [@p10, @p11, @p12],
      [@p20, @p21, @p22]
    ]
  end
  #
  # def to_s(decimals=2)
  #
  # end
end
