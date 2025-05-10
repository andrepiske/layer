# frozen_string_literal: true

class Layer::VideoEncoder
  attr_accessor :pixel_format, :framerate, :width, :height, :output
  attr_accessor :max_rate_k, :buffer_size_k, :gop, :libx264_preset

  def initialize(width, height, output,
    framerate: 30,
    for_streaming: false,
    pixel_format: 'yuv420p',
    libx264_preset: 'veryfast',
    max_rate_k: 3000,
    buffer_size_k: 6000,
    gop: 50
  )
    @width = width
    @height = height
    @framerate = framerate
    @for_streaming = for_streaming
    @pixel_format = pixel_format
    @output = output

    @max_rate_k = max_rate_k
    @buffer_size_k = buffer_size_k
    @gop = gop
    @libx264_preset = libx264_preset

    @initialized = false
    @pipe = nil
  end

  def start
    ensure_initialized!
  end

  def encode_frame(surface)
    ensure_initialized! unless @initialized
    @pipe.write(surface.to_rgb24)
  end

  private

  def ensure_initialized!
    return if @initialized
    @initialized = true

    @cmd = %w[ffmpeg -y -f rawvideo -vcodec rawvideo -s]
    @cmd << "#{@width}x#{@height}"
    @cmd += ["-pix_fmt", "rgb24", "-r", "#{@framerate}.00"]
    @cmd += %w[-i -]
    # @cmd += %w[-filter scale=in_color_matrix=auto:in_range=auto:out_color_matrix=bt709:out_range=tv]
    @cmd += %W[-vcodec libx264 -pix_fmt #{@pixel_format} -preset #{@libx264_preset}]
    @cmd += %w[-v warning]
    # @cmd += %w[-color_primaries bt709 -color_trc bt709 -colorspace bt709 -color_range tv]

    if @for_streaming
      @cmd += %W[-maxrate #{@max_rate_k}k -bufsize #{@buffer_size_k}k -g #{@gop}]
      @cmd += %W[-f rtsp #{@output}]
    end

    @cmd << output
    cmd_str = @cmd.join(' ')
    puts "Will run ffmpeg: #{cmd_str}"
    @pipe = IO.popen(cmd_str, "r+")
  end
end
