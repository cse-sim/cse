# Build the Documents
require('fileutils')
require('time')
require('json')
require('yaml')
require_relative('lib/pandoc')

########################################
# Globals
CONFIG_FILE = 'config.yaml'
CONFIG = YAML.load_file(CONFIG_FILE)
# ... input paths
THIS_DIR = File.expand_path(File.dirname(__FILE__))
SRC_DIR = File.expand_path(CONFIG.fetch("src-dir"), THIS_DIR)
CONTENT_DIR = SRC_DIR
MEDIA_DIR = File.expand_path(CONFIG.fetch("media-dir"), THIS_DIR)
CSS_DIR = File.expand_path(CONFIG.fetch("css-dir"), THIS_DIR)
TEMPLATE_DIR = File.expand_path(CONFIG.fetch("template-dir"), THIS_DIR)
REFERENCE_DIR = File.expand_path(CONFIG.fetch("reference-dir"), THIS_DIR)
PANDOC_HTML_OPTIONS = (
  CONFIG.fetch("options").fetch("pandoc").fetch("*") +
  CONFIG.fetch("options").fetch("pandoc").fetch("html")
).join(' ')
PANDOC_MD_OPTIONS = (
  CONFIG.fetch("options").fetch("pandoc").fetch("*") +
  CONFIG.fetch("options").fetch("pandoc").fetch("md")
).join(' ')
RECORD_NAME_FILE = File.join(REFERENCE_DIR, 'known-records.txt')
NODE_BIN_DIR = File.expand_path('node_modules', THIS_DIR)
# ... utility programs
USE_NODE = CONFIG.fetch("use-node?")
NANO_EXE = CONFIG.fetch("cssnano-exe")
HTML_MIN_EXE = CONFIG.fetch("html-minifier-exe")
# ... output paths
BUILD_DIR = File.expand_path(
  CONFIG.fetch("build-dir"), THIS_DIR
)
MD_TEMP_DIR = File.expand_path(
  File.join(CONFIG.fetch("md-temp-dir"), 'step-1-normalize'), THIS_DIR
)
MD_TEMP2_DIR = File.expand_path(
  File.join(CONFIG.fetch("md-temp-dir"), 'step-2-xlink'), THIS_DIR
)
HTML_TEMP_DIR = File.expand_path(
  CONFIG.fetch("html-temp-dir"), THIS_DIR
)
PDF_TEMP_DIR = File.expand_path(
  CONFIG.fetch("pdf-temp-dir"), THIS_DIR
)
HTML_OUT_DIR = File.expand_path(
  CONFIG.fetch("html-output-dir"), THIS_DIR
)
HTML_CSS_OUT_DIR = File.join(HTML_OUT_DIR, 'css')
HTML_MEDIA_OUT_DIR = File.join(HTML_OUT_DIR, 'media')
PDF_OUT_DIR = File.expand_path(
  CONFIG.fetch("pdf-output-dir"), THIS_DIR
)
LOG_DIR = File.expand_path(
  CONFIG.fetch("log-dir"), THIS_DIR
)

########################################
# Helper Functions
EnsureExists = lambda do |path|
  FileUtils.mkdir_p(path) unless File.exist?(path)
end

ReduceLines = lambda do |str, init, reducer|
  val = init
  str.split("\n").each {|line|
    val = reducer.call(val, line)
  }
  val
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

SelectHeader = lambda {|s| s =~ /^#+[[:space:]]+.*$/ }

HeaderLevel = lambda do |s|
  m = /^(#+)[^.#].*$/.match(s)
  if m.length == 2
    m[1].length
  else
    0
  end
end

# String -> String
# Given a string of markdown source that depicts a header in ATX style syntax,
# return the header
NameFromHeader = lambda do |line|
  name_match = line.match(/^#+[[:space:]]+((?:(?![[:space:]]+\{[#\.]).)*)/)
  if name_match.nil?
    ""
  else
    name_match[1].strip
  end
end

SlugifyName = lambda do |s|
  val = s.strip.downcase
    .gsub(/^[\d\s]*/, '')
    .gsub(/[:)(\/]/, '')
    .gsub(/---/, '')
    .gsub(/--/, '')
    .gsub(/[^\w-]+/, '-')
    .gsub(/^-+/, '')
    .gsub(/-+$/, '')
  if val == ''
    'section'
  else
    val
  end
end

# (Array Int) -> (State Line -> State)
ObjectIndexReducer = lambda do |target_levels|
  lambda do |state, line|
    debug = false
    if SelectHeader[line]
      level = HeaderLevel[line]
      next_state = state
      if target_levels.include?(level)
        name_set = state[:name_set]
        words = NameFromHeader[line].strip.gsub(/,/,'').split
        if words.length > 1
          # Not just the name of an IDD object
          # This prevents false matches such as "Building Controls Virtual Test Bed"
          # matching "Building"
          state
        else
          words.each do |w|
            if name_set.include?(w)
              slug = SlugifyName[NameFromHeader[line]]
              file_basename = File.basename(state[:file_url])
              puts "Object Index: matching #{w} with #{file_basename}" if debug
              file_extension = File.extname(file_basename)
              file_html = file_basename.gsub(file_extension, '.html')
              file_url =  file_html + '#' + slug
              # only take the first instance of w in a header
              unless next_state[:index].has_key?(w)
                next_state = next_state.merge(
                  Hash[:index, next_state[:index].merge(Hash[w,file_url])])
              end
            end
          end
        end
      end
      next_state
    else
      state
    end
  end
end

ReduceOverFiles = lambda do |files, init, reducer|
  val = init
  files.each do |f|
    val = val.merge(Hash[:file_url, f])
    open(f, 'r') do |fin|
      str = fin.read()
      val = ReduceLines[str, val, reducer]
    end
  end
  val
end

CreateRecordIndex = lambda do |name_set, files, levels=nil, verbose=false|
  levels ||= (1..6).to_a
  puts "Indexing objects..." if verbose
  out = ReduceOverFiles[
    files,
    {
      :name_set => name_set,
      :index => {},
      :file_url => nil},
      ObjectIndexReducer[levels]
  ]
  out[:index]
end

# String -> Bool
# Returns true if the first letter is capitalized
IsCapitalized = lambda do |word|
  (!word.nil?) and (!word.empty?) and (!word[0].match(/[A-Z]/).nil?)
end

DePluralize = lambda do |word|
  if word.end_with?("s")
    if word.length > 1
      word[0..-2]
    end
  else
    word
  end
end

# ObjectIndex ObjectNameSet String -> (Tuple String Int)
# Cross-links the markdown; that is, finds all objects in the text in the
# object name set. Works over the given string. Returns a tuple of the
# number of files processed, the total number evaluated, and the number of
# hits found
XLinkMarkdown = lambda do |oi, ons, content|
  f = lambda do |tag, the_content, state|
    g = lambda do |inline|
      if inline['t'] == 'Str'
        index = state[:oi]
        ons = state[:ons]
        word = inline['c']
        word_only = word.scan(/[a-zA-Z:]/).join
        word_only_singular = DePluralize[word_only]
        if ons.include?(word_only) or ons.include?(word_only_singular)
          if index.include?(word_only) or index.include?(word_only_singular)
            w = ons.include?(word_only) ? word_only : word_only_singular
            state[:num_hits] += 1
            ref = index[w].scan(/(\#.*)/).flatten[0]
            {'t' => 'Link',
             'c' => [
               ["", [], []],
               [{'t'=>'Str','c'=>w}],
               [ref, '']]}
          else
            puts("WARNING! ObjectNameSet includes #{word_only}|#{word_only_singular}; Index doesn't")
            inline
          end
        else
          inline
        end
      elsif inline['t'] == 'Emph'
        new_ins = inline['c'].map(&g)
        {'t'=>'Emph', 'c'=>new_ins}
      elsif inline['t'] == 'Strong'
        new_ins = inline['c'].map(&g)
        {'t'=>'Strong', 'c'=>new_ins}
      else
        inline
      end
    end
    if tag == "Para"
      new_inlines = the_content.map(&g)
      {'t' => 'Para',
       'c' => new_inlines}
    else
      nil
    end
  end
  doc = JSON.parse(Pandoc::MdToJson[content])
  state = {
    num_hits: 0,
    oi: oi,
    ons: ons
  }
  new_doc = Pandoc::Walk[doc, f, state]
  new_content = Pandoc::JsonToMd[new_doc]
  [new_content, state[:num_hits]]
end

BuildCrossLinkedNormalizedMD = lambda do |log|
  dirs = [
    MD_TEMP_DIR, MD_TEMP2_DIR,
  ]
  EnsureAllExist[dirs]
  record_name_set = Set.new(
    File.read(RECORD_NAME_FILE).split(/\n/).map {|x|x.strip}
  )
  log["... copying all markdown source to temp directory for processing"]
  md_src_files = Dir[File.join(CONTENT_DIR, '*.md')]
  md_tmp_files = []
  md_src_files.each do |path|
    out_path = File.join(MD_TEMP_DIR, File.basename(path))
    if FileUtils.uptodate?(out_path, [path])
      log["... #{out_path} up to date"]
    else
      log["... normalizing #{path} => #{out_path}"]
      RunAndLog[log, "pandoc #{PANDOC_MD_OPTIONS} -o #{out_path} #{path}"]
      md_tmp_files << out_path
    end
  end
  rec_idx = CreateRecordIndex[record_name_set, md_tmp_files, levels=[1,2,3]]
  md_tmp2_files = []
  md_tmp_files.each do |path|
    out_path = File.join(MD_TEMP2_DIR, File.basename(path))
    if FileUtils.uptodate?(out_path, [path])
      log["... already processed #{path}"]
    else
      md_tmp2_files << out_path
      # cross link
      log["... cross linking #{path} => #{out_path}"]
      content = File.read(path)
      new_content, num_hits = XLinkMarkdown[rec_idx, record_name_set, content]
      if content.start_with?('---')
        m = content.match(/\A(---.*?^---$).*/m)
        log["... re-adding yaml metadata block"] if m
        new_content =  m[1] + "\n\n" + new_content unless m.nil?
      end
      File.write(out_path, new_content)
      log["... cross linked #{path} => #{out_path}; #{num_hits} links found!"]
    end
  end
  md_tmp2_files
end

BuildHTML = lambda do
  dirs = [
    HTML_TEMP_DIR, HTML_CSS_OUT_DIR, HTML_MEDIA_OUT_DIR,
    LOG_DIR,
  ]
  EnsureAllExist[dirs]
  record_name_set = Set.new(
    File.read(RECORD_NAME_FILE).split(/\n/).map {|x|x.strip}
  )
  File.open(File.join(LOG_DIR, TimeStamp[] + ".txt"), 'w') do |f|
    log = MkLogger[f]
    log["Compressing CSS and moving it to destination"]
    Dir[File.join(CSS_DIR, '*.css')].each do |path|
      out_path = File.join(HTML_CSS_OUT_DIR, File.basename(path))
      log["... compressing #{path} => #{out_path}"]
      if USE_NODE
        RunAndLog[log, "#{NANO_EXE} < #{path} > #{out_path}"]
      else
        log["... node.js not used (see USE_NODE in #{__FILE__}: copying to destination vs compressing"]
        FileUtils.cp(path, out_path)
      end
    end
    log["Copying #{MEDIA_DIR} to #{HTML_MEDIA_OUT_DIR}"]
    FileUtils.cp_r(Dir[File.join(MEDIA_DIR, '*')], HTML_MEDIA_OUT_DIR)
    log["... copied"]
    md_src_files = Dir[File.join(CONTENT_DIR, '*.md')]
    log["Build HTML from Markdown, Add Cross-Links, and Compress"]
    md_tmp_files = BuildCrossLinkedNormalizedMD[log]
    md_tmp_files.each do |path|
      html_base = File.basename(path, '.md') + '.html'
      out_path = File.join(HTML_TEMP_DIR, html_base)
      log["... generating #{path} => #{out_path}"]
      RunAndLog[log, "pandoc #{PANDOC_HTML_OPTIONS} -o #{out_path} #{path}"]
      out_compress = File.join(HTML_OUT_DIR, html_base)
      if FileUtils.uptodate?(out_compress, [out_path])
        log["... already compressed  #{out_path} => #{out_compress}"]
      else
        if USE_NODE
          log["... compressing  #{out_path} => #{out_compress}"]
          RunAndLog[log, "#{HTML_MIN_EXE} -o #{out_compress} #{out_path}"]
        else
          log["... node.js not used (see USE_NODE in #{__FILE__}; copying to destination vs compressing"]
          FileUtils.cp(out_path, out_compress)
        end
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
desc "Build the HTML"
task :html do
  InstallNodeDeps[]
  BuildHTML[]
  puts("Done!")
end

desc "Build the PDFs"
task :pdf do
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
