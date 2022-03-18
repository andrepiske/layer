# frozen_literal_string: true
require 'rubygems'

module DopeBuild
  class ToolConf
    attr_accessor :build_files

    def initialize
      pl = ::Gem::Platform::local
      @platform_name = "#{pl.cpu}-#{pl.os}#{pl.version}"
      @bins = {}
      @libs = {}
    end

    def generate_ninja
      lines = []
      lines << "ninja_required_version = 1.1\n"


      @libs.each do |lib_name, conf|
        lines << "\# LIB #{lib_name}"
        lines << "#{lib_name}_cflags = #{conf[:cflags]}"
        lines << "#{lib_name}_libs = #{conf[:libs]}"
        lines << ''
      end

      # ruby stuff
      lines << "warnflags = -Wall -Wextra -Wdeprecated-declarations -Wdivision-by-zero -Wimplicit-function-declaration -Wimplicit-int -Wmisleading-indentation -Wpointer-arith -Wshorten-64-to-32 -Wwrite-strings -Wmissing-noreturn -Wno-constant-logical-operand -Wno-long-long -Wno-missing-field-initializers -Wno-overlength-strings -Wno-parentheses-equality -Wno-self-assign -Wno-tautological-compare -Wno-unused-parameter -Wno-unused-value -Wunused-variable -Wextra-tokens"
      lines << "ruby_mkmf_cflags = -fno-common -pipe $warnflags\n"

      ruby_cflags = "-I#{@ruby_prefix}/include/ruby-#{@ruby_base_version} -I#{@ruby_prefix}/include/ruby-#{@ruby_base_version}/#{@platform_name}"
      lines << "ruby_cflags = #{ruby_cflags}"

      # FIXME: ruby 3 hardcoded
      ruby_ldflags = "-fstack-protector-strong -L#{@ruby_prefix}/lib -lruby.3.0 -Wl,-undefined,dynamic_lookup -Wl,-multiply_defined,suppress"
      lines << "ruby_ldflags = #{ruby_ldflags}"

      deps_cflags = @libs.map { |n, _| "$#{n}_cflags" }.join(' ')
      lines << "deps_cflags = #{deps_cflags}"
      linker_flags = @libs.map { |n, _| "$#{n}_libs" }.join(' ')
      lines << "linker_flags = #{linker_flags}"

      lines << "cflags = -O3 -g $deps_cflags $ruby_cflags $ruby_mkmf_cflags"

      lines << ''

      lines << 'rule cc'
      lines << "  command = #{bin :cc} -fdeclspec $cflags -c $in -o $out"

      lines << 'rule lcc'
      lines << "  command = #{bin :cc} -fdeclspec -dynamic -bundle $cflags $linker_flags $ruby_ldflags $in -o $out"

      lines << ''

      out_files = []
      build_files.each do |file|
        ext = File.extname(file)
        only_file = file[0...(-ext.length)]
        lines << "build out/#{only_file}.o: cc c_src/#{file}"
        out_files << "out/#{only_file}.o"
      end

      lines << ''

      lines << "build lib/ext/layer_ext.bundle: lcc #{out_files.join(' ')}"

      puts lines.join("\n")
    end

    def detect_bins
      set_bin(:pkg_config, detect_bin!('pkg-config', 'Install it via brew install pkg-config'))
      set_bin(:ninja, detect_bin!('ninja', 'Install it via brew install ninja'))
      set_bin(:cc, detect_bin_one_of!(%w(clang gcc cc)))
    end

    def detect_ruby
      rbenv_prefix = `rbenv prefix` rescue nil
      if rbenv_prefix == nil
        puts "Error: Missing rbenv installation"
        exit(1)
      end

      @ruby_prefix = rbenv_prefix.chomp

      unless File.directory?(@ruby_prefix)
        puts "Error: detected ruby at #{@ruby_prefix}, but that is not a valid directory"
        exit(1)
      end

      @ruby_version = `rbenv version`.match(/^\d+\.\d+\.\d+/)[0]
      @ruby_base_version = @ruby_version.split('.')[0] + '.0.0'
    end

    def require_lib!(name)
      cflags = `pkg-config --cflags #{name}`
      libs = `pkg-config --libs #{name}`

      @libs[name.to_sym] = {
        cflags: cflags.chomp,
        libs: libs.chomp
      }
    end

    private

    def set_bin(name, value)
      @bins[name.to_sym] = value
    end

    def bin(name)
      @bins.fetch(name.to_sym)
    end

    def detect_bin_one_of!(bin_names)
      bin_names.find do |bin_name|
        name = `which #{bin_name}`.chomp
        return name if name != '' && File.file?(name)
      end

      puts("Error: could not detect any of: '#{bin_name.join(', ')}'.")
      exit(1)
    end

    def detect_bin!(bin_name, install_instruction)
      name = `which #{bin_name}`.chomp
      return name if name != '' && File.file?(name)
      puts("Error: could not detect '#{bin_name}'. #{install_instruction}")
      exit(1)
    end
  end
end
