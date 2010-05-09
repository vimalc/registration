#!/usr/bin/env ruby

require 'fileutils'
include FileUtils

IMAGES_DIR = '../../images'
ORIGINALS_DIR = IMAGES_DIR + '/originals'
DOWNSAMPLES_DIR = IMAGES_DIR + '/downsamples'
CONFIG_DIR = '../config'

def all_files
  File.read("#{CONFIG_DIR}/all_files.txt").split
end

def downsampled_files
  # this code is repeated in
  Dir[DOWNSAMPLES_DIR + '/*.bmp'].map {|f| File.basename f, '.bmp' }
end

def files_ready_to_be_downsampled
  # this is the same code as fully_downloaded_files
  Dir[ORIGINALS_DIR + '/*.bmp'].map {|f| File.basename f, '.bmp' }
end

def error_files
  File.read(CONFIG_DIR + "/error_files.txt").split
end

def files_to_downsample
  all_files - downsampled_files - error_files
end

def downsample_file(basename)
  print "Downsampling #{basename}.bmp..."
  `../itk/ShrinkImage '#{ORIGINALS_DIR}/#{basename}.bmp' '#{DOWNSAMPLES_DIR}/#{basename}.bmp'`
  if $? == 0
    puts "done."
  else
    File.open("#{CONFIG_DIR}/error_files.txt", "a") {|f| f.puts basename }
    puts "Downsampling failed. Filename has been appended to 'error_files.txt'."
  end
  print "Removing hi-res copy of #{basename}.bmp..."
  rm "#{ORIGINALS_DIR}/#{basename}.bmp"
  puts "done.\n\n"
end

until files_to_downsample.empty?
  unless files_ready_to_be_downsampled.empty?
    downsample_file(files_ready_to_be_downsampled[0])
  else
    puts "No files ready to downsample yet, sleeping..."
    sleep 10
  end
end

puts "All files have been downsampled!"