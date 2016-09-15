#!/usr/bin/env ruby

#
# Script for packaging nightly builds
# https://doom64ex.dotfloat.com/
#

# Install the following gems:
# git

require 'git'
require 'fileutils'
include FileUtils

REPO = 'https://github.com/svkaiser/doom64ex'
ROOT_DIR = '/tmp/doom64ex'

targets =
  [{ compiler: 'mingw',
     os: 'win32',
     toolchain: 'toolchain-mingw32.cmake',
     binary: 'doom64ex.exe',
     extern: 'https://dl.dropboxusercontent.com/u/7122698/extern-win32.zip' },
   { compiler: 'gcc',
     os: 'linux32',
     binary: 'doom64ex',
     extern: 'https://dl.dropboxusercontent.com/u/7122698/linux-x86.tar.gz',
     extern_copy_libs: 'lib/*.so',
     env: { CFLAGS: '-m32', CXXFLAGS: '-m32' },
     defines: { CMAKE_SKIP_RPATH: 'ON' } },
   { compiler: 'gcc',
     os: 'linux64',
     binary: 'doom64ex',
     extern: 'https://dl.dropboxusercontent.com/u/7122698/linux-x86_64.tar.gz',
     extern_copy_libs: 'lib/*.so',
     defines: { CMAKE_SKIP_RPATH: 'ON' } }]

install =
  [{ archive: 'doom64ex-win32.zip',
     build: { win32: '/' },
     kexwad: '/' },
   { archive: 'doom64ex-linux.tar.gz',
     build: { linux32: '/linux32', linux64: '/linux64' },
     kexwad: '/data',
     bootstrap: '/start.sh' }]

class Hash
  attr_accessor :logfile

  def system(args)
    logfile.puts(%x[ #{args} ])
    raise RuntimeError if $? != 0
  end
end

rm_rf ROOT_DIR
mkdir_p ROOT_DIR

# puts "Cloning repo #{REPO}"
# git = Git.clone(REPO, 'source', path: ROOT_DIR, depth: 1)

def build(target)
  root = "#{ROOT_DIR}/#{target[:os]}-#{target[:compiler]}"
  source = "#{root}/source"
  build = "#{root}/build"
  package = "#{root}/package"

  mkdir_p build
  mkdir_p package
  target.logfile = File.new("#{root}/build.log", 'w')

  # cp_r "#{ROOT_DIR}/source", "#{root}"
  cp_r '/home/zohar/Projects/Doom64EX/.', source

  if target[:extern]
    if target[:extern].end_with? '.zip'
      target.system("wget #{target[:extern]} -O #{source}/extern.zip")
      target.system("unzip #{source}/extern.zip -d #{source}/extern")
    elsif target[:extern].end_with? '.tar.gz'
      target.system("wget #{target[:extern]} -O #{source}/extern.tar.gz")
      target.system("tar xvf #{source}/extern.tar.gz -C #{source}/extern")
    end
  end

  toolchain = target[:toolchain] ? "-DCMAKE_TOOLCHAIN_FILE=#{source}/build_scripts/#{target[:toolchain]}" : ''
  defines = target.fetch(:defines, []).map { |k,v| "-D#{k}=#{v} " }.join
  env = target.fetch(:env, []).map { |k,v| "#{k}=#{v} " }.join
  target.system("env #{env} cmake #{toolchain} -H#{source} -B#{build} #{defines}")
  target.system("cmake --build #{build} --target doom64ex --config Release -- -j4")

  cp_r Dir.glob("#{source}/extern/#{target[:extern_copy_libs]}"), package if target[:extern_copy_libs]
  cp "#{build}/src/engine/#{target[:binary]}", package
end

threads = []
targets.each do |t|
  next if ARGV.fetch(0, t[:os]) != t[:os]
  threads << Thread.new { build(t) }
end
threads.each { |t| t.join }
