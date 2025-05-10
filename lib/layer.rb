lib_name = File.absolute_path("./ext/layer_ext.so", __dir__)
require(lib_name)

require 'layer/matrix33'
require 'layer/color'
require 'layer/surface'
require 'layer/font'
require 'layer/timer'
require 'layer/video_encoder'
require 'layer/video_stream_controller'
