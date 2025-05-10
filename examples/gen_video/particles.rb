# frozen_string_literal: true

class Particles
  attr_reader :amount, :sprites

  def initialize(amount, sprites)
    @amount = amount
    @sprites = sprites
    @particles = nil

    reset_particles!
  end

  def each(&blk)
    @particles.each do |pa|
      blk[pa]
    end
  end

  def reset_particles!
    @particles = (0...@amount).map do
      create_particle
    end
  end

  def create_particle
    sprite_id = rand(0...@sprites.length)

    {
      x: -20.0 + rand(-10.0..10.0), y: 200 + rand(-20.0..20.0),
      sprite_id:,
      sprite: @sprites[sprite_id],
      break: [rand(0.5..3.0), rand(0.5..2.5)],
      speed: [rand(20.0..80.0), rand(-15.0..15.0)],
      life: rand(0.5..3.0)
    }
  end

  def advance(t)
    t = t.to_f

    (0...@particles.length).each do |idx|
      pa = @particles[idx]
      advance_particle(pa, t)

      if pa[:life] < 0
        @particles[idx] = create_particle
      end
    end
  end

  private

  def advance_particle(a, factor)
    a[:life] -= factor * rand(0.2..0.4)
    return if a[:life] < 0.0

    pos = [a[:x], a[:y]]
    (0..1).each do |i|
      pos[i] += a[:speed][i] * rand(0.8..1.2) * factor
      new_speed = a[:speed][i] - (a[:break][i] * rand(0.8..1.2) * factor)
      if i == 0
        a[:speed][i] = [new_speed, 0.0].max
      else
        a[:speed][i] = new_speed
      end
    end

    a[:x] = pos[0]
    a[:y] = pos[1]
  end
end
