#!/usr/bin/env ruby
# $:.unshift(File.expand_path("./dopebuild", __dir__))
# require 'pry'
# require 'debug'

require_relative './dopebuild.rb'

conf = DopeBuild::ToolConf.new
conf.detect_ruby
conf.detect_bins

conf.require_lib!('sdl2')
conf.require_lib!('freetype2')
conf.require_lib!('cairo')

conf.build_files = [
  'font.c',
  'layer_ext.c',
  'surface.c',
  'window.c',
]

conf.generate_ninja
