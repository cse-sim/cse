# Build the Documents
# TODO:
# - add xlinking (make caps only, handle plurals)
# - add report generation from source and from markdown
require('fileutils')
require('time')

########################################
# Globals
# ... input paths
THIS_DIR = File.expand_path(File.dirname(__FILE__))
SRC_DIR = File.join(THIS_DIR, 'src')
CONTENT_DIR = File.join(SRC_DIR, 'content')
STATIC_DIR = File.join(SRC_DIR, 'static')
MEDIA_DIR = File.join(STATIC_DIR, 'media')
CSS_DIR = File.join(STATIC_DIR, 'css')
TEMPLATE_DIR = File.join(SRC_DIR, 'template')
OPTIONS_DIR = File.join(SRC_DIR, 'options')
PANDOC_HTML_OPTIONS_FILES = [
  File.join(OPTIONS_DIR, 'pandoc.txt'),
  File.join(OPTIONS_DIR, 'pandoc-html.txt'),
]
NODE_MODULES_DIR = File.join(THIS_DIR, 'node_modules')
NODE_BIN_DIR = File.join(NODE_MODULES_DIR, '.bin')
# ... utility programs
PREFIX = ''
NANO_EXE = PREFIX + File.join(NODE_BIN_DIR, 'cssnano')
HTML_MIN_EXE = PREFIX + File.join(NODE_BIN_DIR, 'html-minifier')
# ... output paths
BUILD_DIR = File.join(THIS_DIR, 'build')
HTML_TEMP_DIR = File.join(BUILD_DIR, 'html')
PDF_TEMP_DIR = File.join(BUILD_DIR, 'pdf')
OUT_DIR = File.join(BUILD_DIR, 'output')
HTML_OUT_DIR = File.join(OUT_DIR, 'html')
HTML_CSS_OUT_DIR = File.join(HTML_OUT_DIR, 'css')
HTML_MEDIA_OUT_DIR = File.join(HTML_OUT_DIR, 'media')
LOG_DIR = File.join(BUILD_DIR, 'logs')

########################################
# Helper Functions
EnsureExists = lambda do |path|
  FileUtils.mkdir_p(path) unless File.exist?(path)
end

EnsureAllExist = lambda do |paths|
  paths.each {|p| EnsureExists[p]}
end

# Get a time-stamp usable as a filename
TimeStamp = lambda do
  DateTime.now.strftime("%Y-%m-%d-%H-%M")
end

MkLogger = lambda do |f|
  lambda do |msg|
    f.write("#{TimeStamp[]} :: #{msg}\n")
  end
end

OptionsFilesToOptionString = lambda do |paths|
  opts = []
  paths.each do |path|
    opts << File.read(path).split(/\n/).map {|x|x.strip}.join(' ')
  end
  opts.join(' ')
end

RunAndLog = lambda do |log, cmd|
  if system(cmd)
    log["... success!"]
  else
    log["... failure: #{$?}"]
  end
end

BuildHTML = lambda do
  dirs = [HTML_TEMP_DIR, LOG_DIR, HTML_CSS_OUT_DIR, HTML_MEDIA_OUT_DIR]
  EnsureAllExist[dirs]
  pandoc_options = OptionsFilesToOptionString[PANDOC_HTML_OPTIONS_FILES]
  File.open(File.join(LOG_DIR, TimeStamp[] + ".txt"), 'w') do |f|
    log = MkLogger[f]
    log["Compressing CSS and moving it to destination"]
    Dir[File.join(CSS_DIR, '*.css')].each do |path|
      out_path = File.join(HTML_CSS_OUT_DIR, File.basename(path))
      log["... compressing #{path} => #{out_path}"]
      RunAndLog[log, "#{NANO_EXE} < #{path} > #{out_path}"]
    end
    log["Copying #{MEDIA_DIR} to #{HTML_MEDIA_OUT_DIR}"]
    FileUtils.cp_r(Dir[File.join(MEDIA_DIR, '*')], HTML_MEDIA_OUT_DIR)
    log["... copied"]
    md_src_files = Dir[File.join(CONTENT_DIR, '*.md')]
    log["Build HTML from Markdown, Add Cross-Links, and Compress"]
    md_src_files.each do |path|
      html_base = File.basename(path, '.md') + '.html'
      out_path = File.join(HTML_TEMP_DIR, html_base)
      if FileUtils.uptodate?(out_path, [path])
        log["... already processed #{path}"]
      else
        log["... generating #{path} => #{out_path}"]
        RunAndLog[log, "pandoc #{pandoc_options} -o #{out_path} #{path}"]
        # cross link

      end
      out_compress = File.join(HTML_OUT_DIR, html_base)
      if FileUtils.uptodate?(out_compress, [out_path])
        log["... already compressed  #{out_path} => #{out_compress}"]
      else
        log["... compressing  #{out_path} => #{out_compress}"]
        RunAndLog[log, "#{HTML_MIN_EXE} -o #{out_compress} #{out_path}"]
      end
    end
  end
end

BuildPDF = lambda do
  EnsureAllExist[[PDF_TEMP_DIR, LOG_DIR, OUT_DIR]]
end

Clean = lambda do |path|
  FileUtils.rm_rf(Dir[File.join(path, '*')])
end

InstallNodeDeps = lambda do
  unless File.exist?(NODE_BIN_DIR)
    `npm install`
  end
end

########################################
# Tasks
task :build_html do
  InstallNodeDeps[]
  BuildHTML[]
  puts("Done!")
end

task :build_pdf do
  InstallNodeDeps[]
  BuildPDF[]
  puts("Done!")
end

desc "Remove all output"
task :clean_output do
  Clean[OUT_DIR]
end

desc "Clean output (same as 'clean_output')"
task :clean => [:clean_output]

desc "Remove logs"
task :clean_logs do
  Clean[LOG_DIR]
end

desc "Clean all -- Note: you will loose build cache!"
task :clean_all do
  Clean[BUILD_DIR]
end

task :default => [:build_html, :build_pdf]
