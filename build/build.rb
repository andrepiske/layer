#!/usr/bin/env ruby
# $:.unshift(File.expand_path("./dopebuild", __dir__))
# require 'pry'
require 'debug'

require_relative './dopebuild.rb'

conf = DopeBuild::ToolConf.new
conf.detect_ruby
conf.detect_bins

conf.require_lib!('sdl2')
conf.require_lib!('freetype2')
conf.require_lib!('cairo')

# To use this:
# Download prebuilt Skia binaries here: https://github.com/JetBrains/skia-pack/releases
# e.g. Skia-m132-a00c390e98-1-macos-Debug-arm64.zip
# then unzip it into under ~/Downloads/_skia
#
conf.require_lib_2!('skia', "#{ENV['HOME']}/Downloads/_skia", [
  ''
],
['out/Debug-macos-arm64'],
[
  'skia'
])

# include_video = (ENV['WITH_VIDEO'] == 'true')

conf.build_files = [
  'font.cpp',
  'layer_ext.c',
  'surface.cpp',
  'window.c',
  'timer.c',
]
# if include_video
#   conf.build_files += [
#     'video_encoder.c',
#     'minih264e.c',
#   ]
# end

conf.generate_ninja
