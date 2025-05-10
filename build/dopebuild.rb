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

      ruby_cflags = "-I#{@ruby_include_path} -I#{@ruby_include_path}/#{@platform_name}"
      lines << "ruby_cflags = #{ruby_cflags}"

      ruby_link = if RUBY_PLATFORM =~ /darwin/
        "ruby.#{@ruby_base_version[0..1].join('.')}"
      else
        'ruby'
      end
      # FIXME: ruby 3 hardcoded
      ruby_ldflags = "-fstack-protector-strong -L#{@ruby_prefix}/lib -l#{ruby_link} -Wl,-undefined,dynamic_lookup"

      # -multiply_defined is obsoloete. Let's omit it.
      # if RUBY_PLATFORM =~ /darwin/
      #   ruby_ldflags = "#{ruby_ldflags} -Wl,-multiply_defined,suppress"
      #   # ruby_ldflags = "#{ruby_ldflags} -Wl,suppress"
      # end

      lines << "ruby_ldflags = #{ruby_ldflags}"

      deps_cflags = @libs.map { |n, _| "$#{n}_cflags" }.join(' ')
      lines << "deps_cflags = #{deps_cflags}"
      linker_flags = @libs.map { |n, _| "$#{n}_libs" }.join(' ')
      lines << "linker_flags = #{linker_flags}"

      lines << "cflags = -O3 -g $deps_cflags $ruby_cflags $ruby_mkmf_cflags"

      lines << ''

      cc_cpp_args = \
        if RUBY_PLATFORM =~ /darwin/
          "-fdeclspec $cflags -c $in -o $out"
        else
          "-fdeclspec -fPIC $cflags -c $in -o $out"
        end

      lines << 'rule cc'
      lines << "  command = #{bin :cc} #{cc_cpp_args}"
      lines << 'rule cpp'
      lines << "  command = #{bin :cpp} -std=c++20 #{cc_cpp_args}"

      lines << 'rule lcc'
      if RUBY_PLATFORM =~ /darwin/
        lines << "  command = #{bin :cc} -fdeclspec -dynamic -bundle $cflags $linker_flags $ruby_ldflags $in -o $out"
      else
        lines << "  command = #{bin :cc} -fdeclspec -dynamic -shared $cflags $linker_flags $ruby_ldflags $in -o $out"
      end

      lines << ''

      out_files = []
      build_files.each do |file|
        ext = File.extname(file)
        only_file = file[0...(-ext.length)]

        compiler = {
          '.c' => 'cc',
          '.cpp' => 'cpp',
        }.fetch(ext)

        lines << "build out/#{only_file}.o: #{compiler} ext/#{file}"
        out_files << "out/#{only_file}.o"
      end

      lines << ''

      if RUBY_PLATFORM =~ /darwin/
        lines << "build lib/ext/layer_ext.bundle: lcc #{out_files.join(' ')}"
      else
        lines << "build lib/ext/layer_ext.so: lcc #{out_files.join(' ')}"
      end

      puts lines.join("\n")
    end

    def detect_bins
      set_bin(:pkg_config, detect_bin!('pkg-config', 'Install it via brew install pkg-config'))
      set_bin(:ninja, detect_bin!('ninja', 'Install it via brew install ninja'))
      set_bin(:cc, detect_bin_one_of!(%w(clang-15 clang gcc cc)))
      set_bin(:cpp, detect_bin_one_of!(%w(clang++ g++)))
    end

    def detect_ruby
      unless detect_ruby_rbenv! || detect_ruby_native!
        puts("Could not detect a ruby installation")
        exit(1)
      end
    end

    def detect_ruby_native!
      @ruby_prefix = "/usr/local"
      @ruby_version = RUBY_VERSION
      @ruby_base_version = (@ruby_version.split('.')[0..1] + ['0'])
      @ruby_include_path = File.join(@ruby_prefix, 'include', "ruby-#{@ruby_base_version.join('.')}")
      unless File.directory?(@ruby_include_path)
        puts "Error: could not find includes path at #{@ruby_include_path}"
        exit(1)
      end

      true
    end

    def detect_ruby_rbenv!
      rbenv_prefix = `rbenv prefix` rescue nil
      if rbenv_prefix == nil
        STDERR.puts "rbenv not detected"
        return false
      end

      @ruby_prefix = rbenv_prefix.chomp

      unless File.directory?(@ruby_prefix)
        puts "Error: detected ruby at #{@ruby_prefix}, but that is not a valid directory"
        exit(1)
      end

      @ruby_version = `rbenv version`.match(/^\d+\.\d+\.\d+/)[0]
      @ruby_base_version = (@ruby_version.split('.')[0..1] + ['0'])

      base_include_path = File.join(@ruby_prefix, 'include')
      subdir_name = File.basename(Dir["#{base_include_path}/*"].first)
      # @ruby_include_path = File.join(@ruby_prefix, 'include', "ruby-#{@ruby_base_version.join('.')}")
      @ruby_include_path = File.join(@ruby_prefix, 'include', subdir_name)
      unless File.directory?(@ruby_include_path)
        puts "Error: could not find includes path at #{@ruby_include_path}"
        exit(1)
      end

      true
    end

    def require_lib!(name)
      cflags = `pkg-config --cflags #{name}`
      libs = `pkg-config --libs #{name}`

      @libs[name.to_sym] = {
        cflags: cflags.chomp,
        libs: libs.chomp
      }
    end

    def require_lib_2!(name, base_path, includes, lib_rel_paths, link_libs)
      cflags = includes.map do |ri|
        "-I#{File.join(base_path, ri).chomp('/')}"
      end.join(' ')

      libs = lib_rel_paths.map do |lp|
        "-L" + File.join(base_path, lp).chomp('/')
      end
      libs << link_libs.map{ |x| "-l#{x}" }

      libs = libs.join(' ')

      @libs[name.to_sym] = { cflags:, libs: }
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
