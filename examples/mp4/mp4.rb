# frozen_string_literal: true

require 'debug'

class BufferReader
  attr_reader :cursor

  def initialize(buffer, offset=0)
    @cursor = offset
    @buffer = buffer
  end

  def read_byte
    c = @cursor
    check_bounds!
    @cursor += 1
    @buffer[c].ord
  end

  def read_u24
    c = @cursor
    @cursor += 3
    check_bounds!
    ("\0" + @buffer[c...c+3]).unpack('L>').first
  end

  def read_u32
    c = @cursor
    @cursor += 4
    check_bounds!
    @buffer[c...c+4].unpack('L>').first
  end

  def read_u64
    c = @cursor
    @cursor += 8
    check_bounds!
    @buffer[c...c+8].unpack('Q>').first
  end

  def read_chars(n)
    c = @cursor
    @cursor += n
    check_bounds!
    @buffer[c...c+n]
  end

  def read_bytes(n)
    read_chars(n).split('').map(&:ord)
  end

  def read_fixed32
    c = @cursor
    @cursor += 4
    check_bounds!
    @buffer[c...c+4].unpack('s>S>').map(&:to_s).join('.').to_f
  end

  def read_fixed16
    c = @cursor
    @cursor += 2
    check_bounds!
    @buffer[c...c+2].unpack('cC').map(&:to_s).join('.').to_f
  end

  private

  def check_bounds!
    if @cursor >= @buffer.length
      raise "Cursor out of bounds: #{@cursor} >= #{@buffer.length}"
    end
  end
end

class Mp4Reader
  def initialize(file_name)
    @fp = File.open(file_name, 'r')
  end

  def read_all
    read_box while !@fp.eof?
  end

  private

  def read_box
    box_size = @fp.read(4).unpack('l>')[0]
    type = @fp.read(4)
    data = @fp.read(box_size - 8)
    reader = BufferReader.new(data)

    puts("#{type} - #{data.length}")

    if type == 'ftyp'
      u = data.unpack('aaaaL>')
      supported = (0...((data.length - 8) / 4)).map do |i|
        data[(8 + i*4)..].unpack('aaaa').join
      end
      puts "  main type=#{u[0..3].join}, v=#{u[4]}. Others: #{supported.join(', ')}"
    end

    if type == 'mdat'
      stuff = data.unpack('L>L>')
      otherstuff = data.unpack('Q>')
      puts "  stuff=#{stuff[0]}, #{stuff[1]}, other=#{otherstuff.first}"
    end

    if type == 'moov'
      offset = reader.read_u32
      atom = reader.read_chars(4)
      puts("  offset = #{offset} [#{reader.cursor}]")

      if atom == 'mvhd'
        version = reader.read_byte
        flags = reader.read_u24

        created_at, mod_at = if version == 0
          [reader.read_u32, reader.read_u32]
        else
          [reader.read_u64, reader.read_u64]
        end

        timescale = reader.read_u32
        duration = version == 0 ? reader.read_u32 : reader.read_u64
        speed = reader.read_fixed32
        volume = reader.read_fixed16

        puts "  mvhd v#{version}, f=#{flags}, ts=#{timescale}, d=#{duration}, spd=#{speed}, vol=#{volume}"

        reader.read_chars(10) # reserved data

        9.times do
          v = reader.read_fixed32
          # puts "mv = #{v}"
        end

        prev_start, prev_len = reader.read_u32, reader.read_u32
        puts "  prev=#{prev_start}..#{prev_len}"
        reader.read_u32
        sel_start, sel_len = reader.read_u32, reader.read_u32
        puts "  sel=#{sel_start}..#{sel_len}"
        curr_time = reader.read_u32
        next_track_id = reader.read_u32
        puts "  curr=#{curr_time}, ntid=#{next_track_id}"
      end

      puts("  END cursor = [#{reader.cursor}]")
      reader.read_u32
      atom = reader.read_chars(4)
      puts "next atom = #{atom}"
      reader.read_u32
      atom = reader.read_chars(4)
      puts "next atom = #{atom}"
    end
  end
end

Mp4Reader.new('vid1.mp4').read_all
