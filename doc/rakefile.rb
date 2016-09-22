# Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file.
########################################
# Build Documentation and Website
require('fileutils')
require('time')
require('json')
require('yaml')
require_relative('lib/pandoc')
require_relative('lib/command')
require_relative('lib/def_parser')
require_relative('lib/tables')

########################################
# Globals
CONFIG_FILE = 'config.yaml'
CONFIG = YAML.load_file(CONFIG_FILE)
# ... input paths
THIS_DIR = File.expand_path(File.dirname(__FILE__))
LOCAL_REPO = File.expand_path(File.join('..', '.git'), THIS_DIR)
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
PANDOC_PDF_OPTIONS = (
  CONFIG.fetch("options").fetch("pandoc").fetch("*") +
  CONFIG.fetch("options").fetch("pandoc").fetch("pdf")
).join(' ')
HTML_MIN_OPTIONS = (
  CONFIG.fetch("options").fetch("html-minifier").fetch("*")
).join(' ')
RECORD_NAME_FILE = File.join(REFERENCE_DIR, 'known-records.txt')
RECORD_INDEX_FILE = File.join(REFERENCE_DIR, 'record-index.yaml')
NODE_BIN_DIR = File.expand_path('node_modules', THIS_DIR)
# ... utility programs
USE_NODE = CONFIG.fetch("use-node?")
NANO_EXE = CONFIG.fetch("cssnano-exe")
HTML_MIN_EXE = CONFIG.fetch("html-minifier-exe")
# ... misc. constants
PDF_HEADER = eval(CONFIG.fetch("pdf-header"))
PDF_FOOTER = eval(CONFIG.fetch("pdf-footer"))
PDF_SKIP_LIST = CONFIG.fetch("pdf-skip-list")
# ... github integration
DOCS_BRANCH = CONFIG.fetch("docs-branch")
USE_GHPAGES = CONFIG.fetch("use-ghpages?")
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
GEN_PROBES = CONFIG.fetch("generate-probes-documentation?")
PROBES_TEMP_DIR = File.expand_path(
  CONFIG.fetch("probes-temp-dir"), THIS_DIR
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
  str.lines.map(&:chomp).each {|line|
    val = reducer.call(val, line)
  }
  val
end

EnsureAllExist = lambda do |paths|
  paths.each {|p| EnsureExists[p]}
end

# -> Nil
# The basic idea of this subroutine is to clone the current repository into the
# build output directory and checkout the gh-pages branch. We then clean all
# files out and (re-)generate into that git repository. The user can then manually
# pull back into the local gh-pages branch after inspection.
SetupGHPages = lambda do
  if USE_GHPAGES
    if ! File.exist?(File.join(HTML_OUT_DIR, '.git'))
      FileUtils.mkdir_p(File.dirname(HTML_OUT_DIR))
      `git clone #{LOCAL_REPO} #{HTML_OUT_DIR}`
      Dir.chdir(HTML_OUT_DIR) do
        `git checkout #{DOCS_BRANCH}`
      end
      puts("Removing existing content...")
      Dir[File.join(HTML_OUT_DIR, '*')].each do |path|
        unless File.basename(path) == '.git'
          puts("... removing: rm -rf #{path}")
          FileUtils.rm_rf(path)
        end
      end
    end
  end
end

# Get a time-stamp usable as a filename
TimeStamp = lambda do
  DateTime.now.strftime("%Y-%m-%d-%H-%M")
end

# FileID -> (Function String -> Nil)
# Create a logging file; Note: as a first action, the logger documents the
# config file being used
MkLogger = lambda do |f|
  logger = lambda do |msg|
    f.write("#{TimeStamp[]} :: #{msg}\n")
  end
  logger["Begin logging with settings from #{CONFIG_FILE}"]
  logger
end

# (Function String -> Nil) String -> Nil
# Run the given command using Ruby's Object.system(...) command. Checks the
# exit code to determine success/failure
RunAndLog = lambda do |log, cmd, working_dir=nil|
  log["Running command:\n#{cmd}"]
  working_dir ||= Dir.pwd
  Dir.chdir(working_dir) do
    begin
      result = `#{cmd}`
      if !result or result.empty?
        log["... success!"]
      else
        log["... success! result:\n#{result}"]
      end
    rescue => e
      log["... failure:\n#{e.message}"]
    end
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

# (Array Int) -> (Function State Line -> State)
# Given an array of heading levels to consider, create and return a reducing
# function for use in walking over a Markdown source file and determining index
# locations for certain headers.
ObjectIndexReducer = lambda do |target_levels|
  lambda do |state, line|
    debug = false
    if SelectHeader[line]
      level = HeaderLevel[line]
      next_state = state
      if target_levels.include?(level)
        name_set = state[:name_set]
        words = NameFromHeader[line].strip.gsub(/,/,'').split(/\s/)
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

# (Array String) State (Function State String -> State) -> State
# where State = (Record :name_set (Set String)
#                       :index (Map String String)
#                       :file_url String)
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

# (Set String) (Array String) ?(Array Int) ?Bool -> (Map String String)
# Determines the index of certain records within a markdown file. Note: assumes
# that the markdown has been authored with ATX style headers! Also, this
# algorithm will get confused if the record name being searched for is not an
# exact match with the header name (including case).
CreateRecordIndex = lambda do |name_set, files, levels=nil, verbose=false|
  levels ||= (1..6).to_a
  puts "Indexing objects..." if verbose
  out = ReduceOverFiles[
    files,
    {
      :name_set => name_set,
      :index => {},
      :file_url => nil
    },
    ObjectIndexReducer[levels]
  ]
  out[:index]
end

# (Array String) String ?(Fn [String] Nil) -> (Tuple Int Int (Array String))
# Given an array of file paths, a target directory path, and an optional
# logging function, copy all files to the target directory path (i.e., keep
# the same base name). Note: does not copy if the target directory already has
# a file that is newer than the one being copied.  Note: creates the target
# directory if it doesn't already exist.
CopyFilesExplicit = lambda do |paths, tgt, log_fn=nil|
  FileUtils.mkdir_p(tgt) unless File.exist?(tgt)
  copied_paths = []
  files_copied = 0
  paths.each do |path|
    out_path = File.join(tgt, File.basename(path))
    if ! FileUtils.uptodate?(out_path, [path])
      files_copied += 1
      copied_paths << out_path
      FileUtils.cp_r(path, out_path)
      log_fn["... copying #{path} => #{out_path}"] if log_fn
    else
      log_fn["... #{path} up-to-date at #{out_path}"] if log_fn
    end
  end
  [files_copied, paths.length, copied_paths]
end

# String String (Fn [String] Nil) -> (Tuple Int Int)
# Given a source path GLOB, a target path, and an optional logging function,
# copy all files matched by GLOB to the target directory. Note: does not copy
# if the file in the target directory is newer than the one in source.
CopyFilesGlob = lambda do |src_glob, tgt, log_fn=nil|
  CopyFilesExplicit[Dir[src_glob], tgt, log_fn]
end

# String -> Bool
# Returns true if the first letter is capitalized
IsCapitalized = lambda do |word|
  (!word.nil?) and (!word.empty?) and (!word[0].match(/[A-Z]/).nil?)
end

# String -> String
# Attempts to remove a plural "s" from a word if appropriate. Note: this is a
# very basic algorithm.
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

# (Fn [String] nil) (Array String) (Fn [String String] (Tuple String String))
#   (Fn [String] String) -> (Tuple Int Int (Array String))
# Given a logging function (taking a message and logging it), an array of
# source file paths to process, a function taking the path to process and
# output-path to process to and returning a 2-tuple of string command and
# working directory to switch to (can be nil for current directory), and a
# function taking the input path and returning the output path (which this code
# will ensure exists), run the given command over all input source files.
# Returns a tuple of the number of files processed, number of files considered,
# and an array of all the paths (in target folder) that were updated.
RunCmdOverEach = lambda do |log_fn, src_files, cmd_fn, tgt_fn|
  log_fn["Running RunCmdOverEach..."]
  changed_files = []
  num_changed = 0
  num_considered = 0
  src_files.each do |path|
    num_considered += 1
    out_path = tgt_fn[path]
    log_fn["... out_path = #{out_path}"]
    if FileUtils.uptodate?(out_path, [path])
      log_fn["... #{out_path} up to date"]
    else
      log_fn["... #{out_path} needs to be generated"]
      parent = File.dirname(out_path)
      FileUtils.mkdir_p(parent) unless File.exist?(parent)
      num_changed += 1
      changed_files << out_path
      c, working_dir = cmd_fn[path, out_path]
      log_fn["... running command \"#{c}\"\n... ... from #{path} => #{out_path}"]
      RunAndLog[log_fn, c, working_dir]
    end
  end
  [num_changed, num_considered, changed_files]
end

# (Function -> nil) -> (Array String)
# Build the cross-linked normalized markdown. Returns an array of file paths to
# all of the cross-linked normalized markdown files, regardless of whether they
# were uptodate or newly built.
BuildCrossLinkedNormalizedMD = lambda do |log|
  dirs = [
    MD_TEMP_DIR, MD_TEMP2_DIR,
  ]
  EnsureAllExist[dirs]
  record_name_set = Set.new(
    File.read(RECORD_NAME_FILE).lines.map {|x|x.strip}
  )
  log["... copying all markdown source to temp directory for processing"]
  md_src_files = Dir[File.join(CONTENT_DIR, '*.md')]
  md_tmp_files = []
  m = 0
  n = 0
  md_src_files.each do |path|
    n += 1
    out_path = File.join(MD_TEMP_DIR, File.basename(path))
    md_tmp_files << out_path
    if FileUtils.uptodate?(out_path, [path])
      log["... #{out_path} up to date"]
    else
      m += 1
      log["... normalizing #{path} => #{out_path}"]
      RunAndLog[log, "pandoc #{PANDOC_MD_OPTIONS} -o #{out_path} #{path}"]
    end
  end
  log["... copied #{m}/#{n} files."]
  rec_idx = YAML.load_file(RECORD_INDEX_FILE)
  md_tmp2_files = []
  md_tmp_files.each do |path|
    out_path = File.join(MD_TEMP2_DIR, File.basename(path))
    md_tmp2_files << out_path
    if FileUtils.uptodate?(out_path, [path])
      log["... already processed #{path}"]
    else
      # cross link
      log["... cross linking #{path} => #{out_path}"]
      content = File.read(path)
      new_content, num_hits = XLinkMarkdown[rec_idx, record_name_set, content]
      if content.start_with?('---')
        log["... re-adding yaml metadata block"]
        yaml_metadata = []
        markers_found = 0
        in_block = false
        content.lines.map(&:chomp).each do |line|
          break if markers_found == 2
          if line == "---" or line == "..."
            yaml_metadata << line
            markers_found += 1
            in_block = !in_block
            next
          end
          yaml_metadata << line if in_block
        end
        if !yaml_metadata.empty?
          new_content = yaml_metadata.join("\n") + "\n\n" + new_content
        end
      end
      File.write(out_path, new_content)
      log["... cross linked #{path} => #{out_path}; #{num_hits} links found!"]
    end
  end
  md_tmp2_files
end

UniquePath = lambda do |path|
  if File.exist?(path)
    parent = File.dirname(path)
    bn = File.basename(path)
    base, ext = bn.split(/\./)
    n = 0
    new_path = path
    while File.exist?(new_path)
      new_path = File.join(parent, base + ("-%03d"%n) + "." + ext)
    end
    new_path
  else
    path
  end
end

BuildHTML = lambda do
  dirs = [
    HTML_TEMP_DIR, HTML_CSS_OUT_DIR, HTML_MEDIA_OUT_DIR,
    LOG_DIR,
  ]
  SetupGHPages[]
  EnsureAllExist[dirs]
  log_file_path = UniquePath[File.join(LOG_DIR, TimeStamp[] + ".txt")]
  File.open(log_file_path, 'w') do |f|
    log = MkLogger[f]
    log["Building HTML"]
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
    num_media_copied, all_media = CopyFilesGlob[
      File.join(MEDIA_DIR, '*'), HTML_MEDIA_OUT_DIR
    ]
    log["... copied #{num_media_copied}/#{all_media} files"]
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
          RunAndLog[log, "#{HTML_MIN_EXE} #{HTML_MIN_OPTIONS} -o #{out_compress} #{out_path}"]
        else
          log["... node.js not used (see USE_NODE in #{__FILE__}; copying to destination vs compressing"]
          FileUtils.cp(out_path, out_compress)
        end
      end
    end
  end
end

# (Array String) (Array String) (Fn [String] Nil) -> (Array String)
# Given an array of paths and an array of file basenames, returns the list of
# paths which DO NOT match any of the basenames.
Skip = lambda do |path_list, skip_list, log_fn=nil|
  path_list.reject do |path|
    flag = skip_list.include?(File.basename(path))
    if flag
      log_fn["Skipping #{path}"] if log_fn
    else
      log_fn["Building #{path}"] if log_fn
    end
    flag
  end
end

BuildPDF = lambda do
  dirs = [
    PDF_TEMP_DIR, LOG_DIR, PDF_OUT_DIR
  ]
  SetupGHPages[]
  EnsureAllExist[dirs]
  log_file_path = UniquePath[File.join(LOG_DIR, TimeStamp[] + ".txt")]
  File.open(log_file_path, 'w') do |f|
    log = MkLogger[f]
    log["Building PDFs"]
    log["Copy media to pdf temporary directory"]
    num_copied, total, _ = CopyFilesGlob[
      File.join(MEDIA_DIR, '*'),
      File.join(PDF_TEMP_DIR, 'media'),
      log
    ]
    log["... #{num_copied}/#{total} files copied"]
    log["Cross-link Markdown and Build PDF"]
    md_tmp_files = BuildCrossLinkedNormalizedMD[log]
    num_files = md_tmp_files.length
    log["... #{md_tmp_files.length} files found"]
    md_tmp_files = Skip[md_tmp_files, PDF_SKIP_LIST, log]
    num_files_to_build = md_tmp_files.length
    log["... building #{num_files_to_build}/#{num_files} files"]
    log["Copying cross-linked markdown to #{PDF_TEMP_DIR}"]
    num_copied, total, _ = CopyFilesExplicit[md_tmp_files, PDF_TEMP_DIR, log]
    log["... #{num_copied}/#{total} files copied."]
    log["Copying template files to #{PDF_TEMP_DIR}"]
    num_copied, total, _ = CopyFilesGlob[
      File.join(TEMPLATE_DIR, '*.tex'), PDF_TEMP_DIR, log
    ]
    log["... #{num_copied}/#{total} files copied."]
    # Note: if we move to multiple markdown pages, we need to base the
    # following around a "manifest file" -- one manifest file per document.
    pdf_opts = PANDOC_PDF_OPTIONS + ' ' + [
      "--variable header=\"#{PDF_HEADER}\"",
      "--variable footer=\"#{PDF_FOOTER}\""
    ].join(' ')
    RunCmdOverEach[
      log,
      Dir[File.join(PDF_TEMP_DIR, '*.md')],
      lambda do |path, out_path|
        working_dir = File.dirname(path)
        ["pandoc #{pdf_opts} -o #{out_path} #{path}", working_dir]
      end,
      lambda do |path|
        File.join(PDF_OUT_DIR, File.basename(path, '.md') + '.pdf')
      end
    ]
  end
end

Clean = lambda do |path|
  FileUtils.rm_rf(Dir[File.join(path, '*')])
end

InstallNodeDeps = lambda do
  unless File.exist?(NODE_BIN_DIR)
    `npm install`
  end
end

RegenerateRecordIndex = lambda do
  record_name_set = Set.new(
    File.read(RECORD_NAME_FILE).lines.map {|x|x.strip}
  )
  md_files = Dir[File.join(CONTENT_DIR, '*.md')]
  rec_idx = CreateRecordIndex[record_name_set, md_files, levels=[1,2,3]]
  File.write(RECORD_INDEX_FILE, rec_idx.to_yaml)
end

EscapeForMd = lambda do |word|
  word
    .gsub(/_/, "\\_")
    .gsub(/\$/, "\\$")
    .gsub(/\*/, "\\*")
end

MkVariabilityString = lambda do |flags|
  fs = flags.map(&:downcase)
  s = []
  g = lambda do |flag, message|
    if fs.include?(flag)
      s << message
      fs.reject {|x| x == flag}
    end
  end
  g["e", "end of each"]
  g["r", "start of run"]
  g["y", "end of run"]
  g["s", "subhour"]
  g["h", "hourly"]
  g["mh", "month-hour"]
  g["d", "day"]
  g["m", "month"]
  g["z", "constant"]
  g["i", "input time"]
  s.join(' ')
end

BuildProbesDocs = lambda do
  dirs = [
    PROBES_TEMP_DIR
  ]
  EnsureAllExist[dirs]
  out_path = File.join(PROBES_TEMP_DIR, 'probes.yaml')
  DefParser::ParseCnRecsToYaml[out_path]
  probes = DefParser::ParseProbesTxt[]
  probes_alt = DefParser::ParseCnRecs[]
  md_path = File.join(PROBES_TEMP_DIR, 'probes.md')
  File.open(md_path, 'w') do |f|
    f.write("# Probe Definitions\n\n")
    probes.keys.sort_by {|k| k.downcase}.each do |k|
      rec_alt = nil
      if probes_alt.include?(k.downcase)
        #puts("Alternate record exists for #{k}")
        rec_alt = probes_alt[k.downcase]
      end
      table = [["Name", "Input?", "Runtime?", "Type", "Variability", "Description"]]
      flds = probes[k][:fields] #.sort_by {|x| x[:name]}
      next if flds.empty?
      name = k
      array_txt = if probes[k][:array] then "[1..]" else "" end
      title = "@#{name}#{array_txt}."
      owner = probes[k][:owner]
      owner_txt = if owner == "--" then "" else " (owner: #{owner})" end
      f.write("## #{title}#{owner_txt}\n\n")
      flds.each do |fld|
        desc = "--"
        if rec_alt
          flds_alt = rec_alt[:fields].select do |f|
            na = f[:name].downcase
            n1a = na
            n2a = na.gsub(/^[^_]*_/, '')
            nb = fld[:name].downcase 
            n1a == nb || n2a == nb
          end
          #puts("... matching #{fld[:name]}: #{flds_alt.inspect}") unless flds_alt.empty?
          if flds_alt.length == 1
            fld_alt = flds_alt[0]
            desc = fld_alt.fetch(:description, desc)
          end
        end
        table << [
          fld[:name],
          if fld[:input] then "X" else "--" end,
          if fld[:runtime] then "X" else "--" end,
          fld[:type],
          fld[:variability],
          desc
        ]
      end
      f.write(Tables::WriteTable[ table, true ])
      f.write("\n\n\n")
    end
  end
  puts("Probes markdown built!")
end

########################################
# Tasks
desc "Build the HTML"
task :html do
  InstallNodeDeps[] if USE_NODE
  BuildHTML[]
  puts("HTML Built!")
end

desc "Build the PDFs"
task :pdf do
  InstallNodeDeps[] if USE_NODE
  BuildPDF[]
  puts("PDF(s) Built!")
end

desc "[Internal] (re-)generate record index (overwrites file!)"
task :gen_rec_idx do
  RegenerateRecordIndex[]
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

desc "Removes the entire build directory. Note: you will loose build cache!"
task :clean_all do
  Clean[BUILD_DIR]
end

desc "Alias for clean_all"
task :reset do
  Clean[BUILD_DIR]
end

desc "Add generated content to gh-pages branch"
task :ghp => [:html, :pdf] do
  if USE_GHPAGES
    Dir.chdir(HTML_OUT_DIR) do
      `git add -A`
    end
    puts("Content added to index at #{HTML_OUT_DIR}, #{DOCS_BRANCH}")
    puts("Once you've reviewed the files at #{HTML_OUT_DIR}, type:\n")
    puts("- `git commit -m \"Your Commit Message\"`")
    puts("- `get remote add github https://github.com/cse-sim/cse.git`")
    puts("- `get push github gh-pages`")
  end
end

desc "Generate probes markdown documentation"
task :probes do
  BuildProbesDocs[]
end

task :default => [:html, :pdf]
