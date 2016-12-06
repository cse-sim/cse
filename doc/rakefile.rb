# Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file.
########################################
# Build Documentation and Website
########################################
require 'fileutils'
require 'json'
require 'yaml'
require 'set'
require 'pathname'
require 'time'
require "rake/testtask"
require_relative 'lib/template'
require_relative 'lib/pandoc'
require_relative 'lib/tables'
require_relative 'lib/toc'
require_relative 'lib/section_index'
require_relative 'lib/verify_links'
require_relative 'lib/coverage_check'
require_relative 'lib/xlink'

########################################
# Globals
########################################
class EvalContext
  def test(name)
    begin
      if name[0] != "_" and (name[0] == name[0].upcase)
        puts("Bad context variable name: #{name}")
        puts("Cannot use uppercase letter to start context variable name (Ruby limitation)")
        puts("Either use a lowercase letter to start the name or start the name with an '_'")
        exit(1)
      end
      eval("#{name} = true")
    rescue
      puts("issue with context key \"#{name}\" in #{CONFIG_FILE}'s context section")
      puts("all context variables must be valid Ruby variables:")
      puts("   a lowercase letter or underscore followed by any\n" +
           "   combination of upper/lowercase letters, underscores, and digits")
      puts("please fix the variable in question in #{CONFIG_FILE} before continuing")
      exit
    end
  end
end
DEFAULT_CONFIG_FILE = "config/defaults.yaml"
CONFIG_FILE = "config.yaml"
LOG = STDOUT
ERB_OUTPUT_FILE = "erb-out.txt"
CONFIG = lambda do
  f = lambda {|path| if File.exist?(path) then YAML.load_file(path) else {} end}
  defaults = f[DEFAULT_CONFIG_FILE]
  local = f[CONFIG_FILE]
  config = defaults.merge(local)
  default_context = if defaults.include?("context") then defaults["context"] else {} end
  local_context = if local.include?("context") then local["context"] else {} end
  context = default_context.merge(local_context)
  ec = EvalContext.new
  context.each {|k, _| ec.test(k)}
  config['context'] = context
  #puts(config.inspect)
  config
end.call
THIS_DIR = File.expand_path(File.dirname(__FILE__))
BUILD_DIR = CONFIG.fetch("build-dir")
SRC_DIR = CONFIG.fetch("src-dir")
LOCAL_REPO = File.expand_path(File.join('..', '.git'), THIS_DIR)
REMOTE_REPO = CONFIG.fetch("remote-repo-url")
RUN_COVERAGE = CONFIG.fetch("coverage?")
REFERENCE_DIR = File.expand_path(
  CONFIG.fetch("reference-dir") , THIS_DIR
)
RECORD_INDEX_EXCEPTIONS_FILE = File.join(
  REFERENCE_DIR, 'record-index-exceptions.yaml'
)
DATE = nil # "February 23, 2016"
DRAFT = CONFIG.fetch("draft?") # true means, it is a draft
HEADER = "CSE User's Manual"
FOOTER = "Generated: #{Time.now.strftime("%FT%T%:z")}"
TOC_DEPTH = 4
WEB_SITE_MANIFEST_PATH = File.join(SRC_DIR, "web-page.yaml")
CSE_USER_MANUAL_MANIFEST_PATH = File.join(SRC_DIR, 'cse-user-manual.yaml')
BUILD_PDF = CONFIG.fetch("build-pdf?")
USE_GHPAGES = CONFIG.fetch("use-ghpages?")
USE_NODE = CONFIG.fetch("use-node?")
NODE_BIN_DIR = File.expand_path('node_modules', THIS_DIR)
DOCS_BRANCH = CONFIG.fetch("docs-branch")
HTML_OUT_DIR = CONFIG.fetch("output-dir")
VERBOSE = CONFIG.fetch("verbose?", false)
VERIFY_LINKS = CONFIG.fetch("verify-links?", false)
VERIFY_MANIFEST = CONFIG.fetch("verify-manifest?", false)
PANDOC_GENERAL_OPTIONS = [
  "--parse-raw",
  "--standalone",
  "--wrap=none",
].join(' ')
PANDOC_MD_OPTIONS = PANDOC_GENERAL_OPTIONS + " " + [
  "--atx-headers",
  "--normalize",
  "--to markdown",
  "--from markdown",
].join(' ')
CSS_NANO_EXE = File.expand_path(
  CONFIG.fetch("cssnano-exe"), THIS_DIR
)
HTML_MIN_EXE = File.expand_path(
  CONFIG.fetch("html-minifier-exe"), THIS_DIR
)
PREPROCESSOR_CONTEXT = CONFIG.fetch("context", {})

########################################
# Helper Functions
########################################
# Note: section index must be generated *after* the preprocessing
# source-paths can (should?) be a glob, i.e., some-path/**/* or
# some-path/**/*.md
GenerateSectionIndex = lambda do |config|
  src_path_glob = config.fetch("source-paths")
  outpath = config.fetch("output-path")
  merge_values = config.fetch("merge-values", nil)
  parent = File.dirname(outpath)
  FileUtils.mkdir_p(parent) unless File.exist?(parent)
  lambda do
    srcpaths = Dir[src_path_glob]
    if !FileUtils.uptodate?(outpath, srcpaths)
      si = SectionIndex::Generate[srcpaths]
      si = si.merge(merge_values) if merge_values
      File.write(outpath, si.to_yaml)
    end
  end
end

# String String -> String
# Given a full path and a root path, return the relative path from root
RelativePath = lambda do |path, root_path|
  Pathname.new(path).relative_path_from(Pathname.new(root_path)).to_s
end

# String -> Nil
# Given a path to a directory where manifest yaml files can be found, report on
# whether the manifests represent all files on disk
CheckManifests = lambda do |src_dir|
  src = Pathname.new(src_dir)
  md_file_set = Set.new(
    Dir[File.join(src_dir, "**", "*.md")].map do |p|
      Pathname.new(p).relative_path_from(src).to_s
    end
  )
  unknown_files = Set.new
  Dir[File.join(src_dir, "**", "*.yaml")].each do |path|
    puts("... checking #{File.basename(path)}")
    man = YAML.load_file(path)
    man["sections"].each do |_, section_file|
      if md_file_set.include?(section_file)
        md_file_set.delete(section_file)
      else
        unknown_files << section_file
      end
    end
  end
  puts("Verifying manifest")
  if md_file_set.empty? and unknown_files.empty?
    puts("No problems detected")
    return
  end
  puts("Problems detected:")
  if !md_file_set.empty?
    puts("  Files on disk but not in any manifest: ")
    md_file_set.sort.each {|f| puts("   - #{f}")}
  end
  if !unknown_files.empty?
    puts("  Files in manifest but not on disk: ")
    unknown_files.sort.each {|f| puts("   - #{f}")}
  end
end

# (Array String) -> (Map String String)
# Given an array of basefile names, return a record index array based on the
# "typical rules".
FilesToRecordIndex = lambda do |paths|
  rec_idx = {}
  paths.each do |path|
    base_no_ext = File.basename(path, ".*")
    base_html = base_no_ext + ".html"
    rec_idx[base_no_ext.upcase] = "#{base_html}##{base_no_ext.downcase}"
  end
  rec_idx
end

# (Map String String) (Map String (Map String String)) -> (Map String String)
# Given a record index of record name to index value (file and hashtag), and a
# map of known exceptions representing "false record name" to correct index entry
# to merge, update the record index with the exceptions. Example:
#
# UpdateRecordIndex[
#   {
#     "TOP-MEMBERS"=>"top-members.html#top-members",
#     "RSYS"=>"rsys.html#rsys"
#   },
#   {
#     "TOP-MEMBERS" => {
#       "TOP" => "top-members.html#top-members"
#     }
#   }
# ]
# =>
#   {
#     "TOP"=>"top-members.html#top-members",
#     "RSYS"=>"rsys.html#rsys"
#   }
UpdateRecordIndex = lambda do |ri, updates|
  new_ri = ri.dup
  updates.each do |k, v|
    if new_ri.include?(k)
      new_ri.delete(k)
      new_ri.merge!(v)
    end
  end
  new_ri
end

# Problem: need to build the record index on the fly based on what is left
# AFTER preprocessing. We also need to account for exceptions to the basic
# rule. The basic rule seems to be:
#
# 1. one record per file in doc/src/records
# 2. the index of that file is <basefilename with ext changed to
#    html>#<basefilename without ext downcased>
# 3. the record name seems to be <basefilename without ext upcase>
# 4. put the above in a map (i.e., Ruby hash table, Python dictionary)
#
# String String String -> Nil
# Given the path to the records/ directory, a path to save the record index at,
# and a file with known exceptions, read the record directory and create the
# record index and save it at the given path.
CreateRecordIndex = lambda do |rec_dir, outpath, exc_path=nil|
  paths = Dir[File.join(rec_dir, '*.md')]
  ri = FilesToRecordIndex[paths]
  exceptions = if exc_path then YAML.load_file(exc_path) else {} end
  ri = UpdateRecordIndex[ri, exceptions]
  File.write(outpath, ri.to_yaml)
  nil
end

InstallNodeDeps = lambda do
  unless File.exist?(NODE_BIN_DIR)
    `npm install`
  end
end

# String -> Nil
# Removes all files under the given path
Clean = lambda do |path|
  FileUtils.rm_rf(Dir[File.join(path, '*')])
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
      `git clone "#{LOCAL_REPO}" "#{HTML_OUT_DIR}"`
      Dir.chdir(HTML_OUT_DIR) do
        `git remote rm origin`
        `git remote add origin #{REMOTE_REPO}`
        `git fetch --all`
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

EnsureExists = lambda do |path|
  FileUtils.mkdir_p(path) unless File.exist?(path)
end

EnsureAllExist = lambda do |paths|
  paths.each {|p| EnsureExists[p]}
end

Run = lambda do |cmd, working_dir=nil|
  working_dir ||= Dir.pwd
  Dir.chdir(working_dir) do
    begin
      result = `#{cmd}`
      raise "Error running command" if $?.exitstatus != 0
      if !result or result.empty?
        if VERBOSE
          LOG.write("executed command: `#{cmd}`\n")
          LOG.write("... in directory: #{working_dir}\n")
          LOG.flush
        else
          LOG.write(".")
          LOG.flush
        end
      else
        if VERBOSE
          LOG.write("executed command: `#{cmd}`\n")
          LOG.write("... in directory: #{working_dir}\n")
          LOG.write("... result:\n#{result}\n")
          LOG.flush
        else
          LOG.write(".")
          LOG.flush
        end
      end
    rescue
      if VERBOSE
        LOG.write("Error running command: `#{cmd}`\n")
        LOG.write("... from directory: #{working_dir}\n")
        LOG.write("... failure:\n #{result}")
        LOG.flush
      else
        LOG.write("X")
        LOG.flush
      end
    end
  end
end

CopyFile = lambda do |path, out_path, dummy1=nil, dummy2=nil|
  FileUtils.cp(path, out_path)
end

# (Array String) (Map String *) -> Bool
# Returns true if the configuration has the given keys
CheckConfigHasKeys = lambda do |config, keys|
  keys.each do |k|
    if !config.include?(k)
      msg = (
        "KeyNotFound Error:\n" +
        "Configuration missing expected key #{k}\n" +
        "config: #{config.inspect}"
      )
      raise msg
    end
  end
  true
end

# (String String Int (Map String *) -> nil) ?(Or Nil (Array String)) ->
#   ((Map String *) -> ((Array String) -> (Array String)))
# Setup up a function to take an array of file paths and process their contents
# to an output path while wrapping in a context of parameters.
# The initial arguments are a function to do the work of transforming from
# source path to target path and configuration keys to check.
#
# This function has signature:
#   String String Int (Map String *) -> nil
# That is, it takes a source path, a target path, the index of which path it is
# in the overall process (a manifest is an array of source paths, so the index
# is the index into that array), and finally a map from string to any value (*)
# that is for configuation purposes. This file doesn't return any meaningful
# results (or if it does, they are not captured) and, instead, is run for its
# side-effects: creating a transformed file at the ouptut path location based
# on applying the function to the source path contents (presumably).
#
# The configuration keys is an array of strings to ensure exist in the
# configuration parameters fed into the next step of this function.
#
# Feeding MapOverManifest these two first arguments returns another function
# which takes a HashTable of configuration parameters which are optionally
# checked for values from the check_keys array.
#
# Feeding that next function the configuration parameters returns the
# final function which takes an array of source file paths (the "manifest")
# and processes them to the output directory (with the same basename as the
# input).
MapOverManifest = lambda do |fn, check_keys=nil|
  lambda do |config|
    CheckConfigHasKeys[config, check_keys] unless check_keys.nil?
    out_dir = config.fetch("output-dir")
    EnsureExists[out_dir]
    other_deps = config.fetch("paths-to-other-dependencies", [])
    if config.fetch("disable?", false)
      #MapOverManifest[CopyFile][config.merge("disable?" => false)]
      lambda {|manifest| manifest}
    else
      lambda do |manifest|
        new_manifest = []
        manifest.each_with_index do |path, idx|
          out_path = nil
          if config.fetch("relative-root-path", nil)
            out_path = File.join(
              out_dir,
              RelativePath[path, config["relative-root-path"]]
            )
            EnsureExists[File.dirname(out_path)]
          else
            out_path = File.join(out_dir, File.basename(path))
          end
          new_manifest << out_path
          if !FileUtils.uptodate?(out_path, [path] + other_deps)
            fn[path, out_path, idx, config]
          end
        end
        new_manifest
      end
    end
  end
end

# (String (Map String *) -> String) ?(Or Nil (Array String)) ->
#   ((Map String *) -> ((Array String) -> (Array String)))
# This function is very similar to MapOverManifest but instead
# works only on the manfiest path names themselves (and doesn't
# touch the actual file content). This can be used to modify
# path names.
MapOverManifestPaths = lambda do |fn, check_keys=nil|
  lambda do |config|
    CheckConfigHasKeys[config, check_keys] unless check_keys.nil?
    if config.fetch('disable?', false)
      lambda do |manifest|
        manifest
      end
    else
      lambda do |manifest|
        new_manifest = []
        manifest.map do |path|
          new_manifest << fn[path, config]
        end
        new_manifest
      end
    end
  end
end

# (Record 'output-dir' String 'context' (Map String *))
#   -> ((Array String) -> (Array String))
# This function takes an array of source files and preprocesses each with ERB,
# saving the rendered result with the same basename in the designated
# output-dir, creating the output-dir if necessary.
PreprocessManifest = MapOverManifest[
  Template::PreprocFile,
  ['output-dir', 'context']
]

# (Map "reference-dir" String ...) -> ((Array String) -> (Array String))
# Configuring with a reference directory, expand all manifest paths and
# return them.
ExpandPathsFrom = MapOverManifestPaths[
  lambda do |path, config|
    reference_dir = config.fetch("reference-dir")
    File.expand_path(path, reference_dir)
  end,
  ["reference-dir"]
]

TapManifest = lambda do |msg|
  lambda do |manifest|
    puts(msg)
    puts("manifest = #{manifest.inspect}")
    manifest
  end
end

NormalizeMarkdown = MapOverManifest[
  lambda do |path, out_path, _, _|
    Run["pandoc #{PANDOC_MD_OPTIONS} -o \"#{out_path}\" \"#{path}\""]
  end
]

#AdjustMarkdownLevels = MapOverManifest[
#  lambda do |path, out_path, idx, config|
#    levels = config.fetch("levels")
#  end,
#  ["levels"]
#]
AdjustMarkdownLevels = lambda do |config|
  levels = config.fetch("levels")
  out_dir = config.fetch("output-dir")
  EnsureExists[out_dir]
  lambda do |manifest|
    new_manifest = []
    if manifest.length != levels.length
      puts("length of manifest doesn't equal length of levels:")
      puts("... manifest length: #{manifest.length}")
      puts("... levels length: #{levels.length}")
      exit(1)
    end
    manifest.zip(levels).each do |path, level|
      out_path = File.join(out_dir, File.basename(path))
      new_manifest << out_path
      if level == 1
        FileUtils.cp(path, out_path)
      else
        adjustment = "#" * (level - 1)
        md = File.read(path, :encoding=>"UTF-8")
        File.open(out_path, 'w') do |f|
          md.lines.each do |line|
            if line =~ /^#+\s+/
              f.write(adjustment + line)
            else
              f.write(line)
            end
          end
        end
      end
    end
    new_manifest
  end
end

AddFiles = lambda do |config|
  new_paths = config.fetch("paths")
  out_dir = config.fetch("output-dir")
  add_to_manifest = config.fetch("append-to-manifest?", false)
  EnsureExists[out_dir]
  lambda do |manifest|
    new_manifest = []
    manifest.each do |path|
      out_path = File.join(out_dir, File.basename(path))
      new_manifest << out_path
      if !FileUtils.uptodate?(out_path, [path])
        FileUtils.cp(path, out_path)
      end
    end
    new_paths.each do |path|
      out_path = File.join(out_dir, File.basename(path))
      new_manifest << out_path if add_to_manifest
      if !FileUtils.uptodate?(out_path, [path])
        FileUtils.cp(path, out_path)
      end
    end
    new_manifest
  end
end

CopyToDir = lambda do |config|
  out_dir = config.fetch("output-dir")
  EnsureExists[out_dir]
  lambda do |manifest|
    new_manifest = []
    manifest.each do |path|
      out_path = File.join(out_dir, File.basename(path))
      new_manifest << path
      if !FileUtils.uptodate?(out_path, [path])
        FileUtils.cp(path, out_path)
      end
    end
    new_manifest
  end
end

CopyByGlob = lambda do |config|
  from_glob = config.fetch("from-glob")
  out_dir = config.fetch("output-dir")
  EnsureExists[out_dir]
  lambda do
    new_manifest = []
    Dir[from_glob].each do |path|
      out_path = File.join(tgt_dir, File.basename(path))
      new_manifest << out_path
      if not FileUtils.uptodate?(out_path, [path])
        FileUtils.cp(path, out_path)
      end
    end
    new_manifest
  end
end

PassThroughWithSideEffect = lambda do |config|
  fn = config.fetch("function")
  disable = config.fetch("disable?", false)
  lambda do |x|
    fn[] unless disable
    x
  end
end

UpdateOutlineCount = lambda do |count, level_at|
  new_count = []
  mod_idx = level_at - 1
  0.upto(mod_idx).each do |idx|
    c = count.fetch(idx, 0)
    if idx == mod_idx
      new_count << c+1
    elsif idx < mod_idx
      if idx > (count.length-1)
        new_count << c+1
      else
        new_count << c
      end
    else
      next
    end
  end
  new_count
end

OutlineCountToStr = lambda do |count|
  count.map(&:to_s).join(".")
end

NumberMd = lambda do |config|
  if config.fetch("disable", false)
    lambda do |manifest|
      manifest
    end
  else
    out_dir = config.fetch("output-dir")
    levels = config.fetch("levels", nil)
    starting_count = config.fetch("starting-count", [0])
    exceptions = config.fetch("exceptions", [])
    EnsureExists[out_dir]
    unnum = lambda do |line|
      line =~ /{-}\s*$/ || line =~ /{[^}]*?\.unnumbered[^}]*?}\s*$/
    end
    lambda do |manifest|
      new_manifest = []
      count = starting_count
      manifest.each_with_index do |path, idx|
        bn = File.basename(path)
        out_path = File.join(out_dir, bn)
        new_manifest << out_path
        if exceptions.include?(bn)
          if !FileUtils.uptodate?(out_path, [path])
            FileUtils.cp(path, out_path)
          end
          next
        end
        level_adjustment = levels ? (levels[idx] - 1) : 0
        md = File.read(path, :encoding=>"UTF-8")
        if !FileUtils.uptodate?(out_path, [path])
          File.open(out_path, 'w') do |f|
            md.lines.each do |line|
              if line =~ /^#+\s+/ and !unnum[line]
                m = line.match(/^(#+)\s+(.*)$/)
                level_at = m[1].length + level_adjustment
                count = UpdateOutlineCount[count, level_at]
                outline_number = OutlineCountToStr[count]
                f.write(m[1] + ' ' + outline_number + ' ' + m[2].chomp + "\n")
              else
                f.write(line)
              end
            end
          end
        end
      end
      new_manifest
    end
  end
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

# Config (and defaults)
#   "record-index-path"
#   "output-dir"
#   "log" => nil
#   "verbose" => false
#   "disable?" => false
#   "paths-to-other-dependencies" => []
#   "relative-root-path" => nil
XLinkMarkdown = MapOverManifest[
  XLink::OverFileOrig,
  ["record-index-path"]
]

#XLinkMarkdown = lambda do |config|
#  disable = config.fetch('disable?', false)
#  if disable
#    lambda do |manifest|
#      manifest
#    end
#  else
#    out_dir = config.fetch('output-dir')
#    EnsureExists[out_dir]
#    lambda do |manifest|
#      oi = if config.include?('object-index-path')
#             YAML.load_file(config['object-index-path'])
#           else
#             config.fetch('object-index')
#           end
#      ons = Set.new(oi.keys)
#      new_manifest = []
#      manifest.each do |path|
#        bn = File.basename(path)
#        out_path = File.join(out_dir, bn)
#        new_manifest << out_path
#        next if FileUtils.uptodate?(out_path, [path])
#        f = lambda do |tag, the_content, state|
#          g = lambda do |inline|
#            if inline['t'] == 'Str'
#              index = state[:oi]
#              ons = state[:ons]
#              word = inline['c']
#              word_only = word.scan(/[a-zA-Z:]/).join
#              word_only_singular = DePluralize[word_only]
#              if ons.include?(word_only) or ons.include?(word_only_singular)
#                if index.include?(word_only) or index.include?(word_only_singular)
#                  w = ons.include?(word_only) ? word_only : word_only_singular
#                  state[:num_hits] += 1
#                  ref = index[w].scan(/(\#.*)/).flatten[0]
#                  {'t' => 'Link',
#                   'c' => [
#                     ["", [], []],
#                     [{'t'=>'Str','c'=>word}],
#                     [ref, '']]}
#                else
#                  puts("WARNING! ObjectNameSet includes #{word_only}|#{word_only_singular}; Index doesn't")
#                  inline
#                end
#              else
#                inline
#              end
#            elsif inline['t'] == 'Emph'
#              new_ins = inline['c'].map(&g)
#              {'t'=>'Emph', 'c'=>new_ins}
#            elsif inline['t'] == 'Strong'
#              new_ins = inline['c'].map(&g)
#              {'t'=>'Strong', 'c'=>new_ins}
#            else
#              inline
#            end
#          end
#          if tag == "Para"
#            new_inlines = the_content.map(&g)
#            {'t' => 'Para',
#             'c' => new_inlines}
#          else
#            nil
#          end
#        end
#        content = File.read(path, :encoding=>"UTF-8")
#        doc = JSON.parse(Pandoc::MdToJson[content])
#        state = {
#          num_hits: 0,
#          oi: oi,
#          ons: ons
#        }
#        new_doc = Pandoc::Walk[doc, f, state]
#        new_content = Pandoc::JsonToMd[new_doc]
#        if VERBOSE
#          LOG.write("Crosslinked #{bn}; #{state[:num_hits]} found\n")
#          LOG.flush
#        else
#          LOG.write(".")
#          LOG.flush
#        end
#        File.write(out_path, new_content)
#      end
#      new_manifest
#    end
#  end
#end

JoinManifestToString = lambda do |manifest|
  manifest.map{|f| "\"#{f}\""}.join(' ')
end

RunPandoc = lambda do |config|
  opts = config.fetch('options')
  out_path = config.fetch('output-path')
  out_dir = File.dirname(out_path)
  working_dir = config.fetch('working-dir', out_dir)
  EnsureExists[out_dir]
  lambda do |input|
    cmd = "pandoc #{opts} -o \"#{out_path}\" #{input}"
    Run[cmd, working_dir]
    out_path
  end
end

RunPandocOverEach = lambda do |config|
  opts = config.fetch('options')
  out_dir = config.fetch('output-dir')
  working_dir = config.fetch('working-dir', nil)
  do_nav = config.fetch('do-navigation?', false)
  top = config.fetch('top-url', nil)
  toc = config.fetch('toc-url', nil)
  EnsureExists[out_dir]
  lambda do |manifest|
    new_manifest = []
    num_files = manifest.length
    manifest.each_with_index do |path, idx|
      out_path = File.join(out_dir, File.basename(path, '.md') + '.html')
      new_manifest << out_path
      working_dir = File.dirname(path) if working_dir.nil?
      if !FileUtils.uptodate?(out_path, [path])
        nav_opts = []
        prev = File.basename(manifest[(idx-1)%num_files], '.md') + '.html'
        the_next = File.basename(manifest[(idx+1)%num_files], '.md') + '.html'
        nav_opts << "--variable prev=\"#{prev}\"" if num_files > 1
        nav_opts << "--variable next=\"#{the_next}\"" if num_files > 1
        nav_opts << "--variable top=\"#{top}\"" if top
        nav_opts << "--variable nav-toc=\"#{toc}\"" if toc
        nav_opts << "--variable do-nav=true" if do_nav
        all_opts = opts + ' ' + nav_opts.join(' ')
        cmd = "pandoc #{all_opts} -o \"#{out_path}\" \"#{path}\""
        Run[cmd, working_dir]
      end
    end
    new_manifest
  end
end

GenTOC = lambda do |config|
  max_level = config.fetch('max-level')
  out_dir = config.fetch('output-dir')
  toc_name = config.fetch('toc-name', 'index.md')
  levels = config.fetch('levels')
  disable = config.fetch('disable?', false)
  if disable
    lambda do |manifest|
      manifest
    end
  else
    EnsureExists[out_dir]
    lambda do |manifest|
      new_manifest = []
      toc_out_path = File.join(out_dir, toc_name)
      new_manifest << toc_out_path
      if !FileUtils.uptodate?(toc_out_path, manifest)
        toc_content = TOC::GenTableOfContentsFromFiles[
          max_level,
          levels.zip(manifest)
        ]
        File.write(toc_out_path, toc_content)
      end
      manifest.each do |path|
        out_path = File.join(out_dir, File.basename(path))
        new_manifest << out_path
        if !FileUtils.uptodate?(out_path, [path])
          FileUtils.cp(path, out_path)
        end
      end
      new_manifest
    end
  end
end

Listify = lambda {|x| [x]}

JoinFunctions = lambda do |fs|
  lambda do |x|
    fs.each do |f|
      x = f[x]
    end
    x
  end
end

NewManifest = lambda do |manifest|
  lambda do |_|
    manifest
  end
end

CompressCSS = lambda do |config|
  disable = config.fetch("disable?", false)
  out_dir = config.fetch("output-dir")
  EnsureExists[out_dir]
  if disable
    lambda do |manifest|
      new_manifest = []
      manifest.each do |path|
        out_path = File.join(out_dir, File.basename(path))
        new_manifest << out_path
        if !FileUtils.uptodate?(out_path, [path])
          FileUtils.cp(path, out_path)
        end
      end
      new_manifest
    end
  else
    working_dir = config.fetch("working-dir", out_dir)
    cssnano = config.fetch("path-to-cssnano")
    lambda do |manifest|
      new_manifest = []
      manifest.each do |path|
        out_path = File.join(out_dir, File.basename(path))
        new_manifest << out_path
        if !FileUtils.uptodate?(out_path, [path])
          cmd = "#{cssnano} < #{path} > #{out_path}"
          Run[cmd, working_dir]
        end
      end
      new_manifest
    end
  end
end

CompressHTML = lambda do |config|
  out_dir = config.fetch("output-dir")
  disable = config.fetch("disable?", false)
  EnsureExists[out_dir]
  if disable
    lambda do |manifest|
      new_manifest = []
      manifest.each do |path|
        out_path = File.join(out_dir, File.basename(path))
        new_manifest << out_path
        if !FileUtils.uptodate?(out_path, [path])
          FileUtils.cp(path, out_path)
        end
      end
      new_manifest
    end
  else
    html_minifier = config.fetch("path-to-html-minifier")
    working_dir = config.fetch("working-dir", out_dir)
    opts = config.fetch("options", "")
    lambda do |manifest|
      new_manifest = []
      manifest.each do |path|
        out_path = File.join(out_dir, File.basename(path))
        new_manifest << out_path
        if !FileUtils.uptodate?(out_path, [path])
          if html_minifier
            cmd = "#{html_minifier} #{opts} -o #{out_path} #{path}"
            Run[cmd, working_dir]
          else
            FileUtils.cp(path, out_path)
          end
        end
      end
      new_manifest
    end
  end
end

BuildProbesAndCopyIntoManifest = lambda do |config|
  disable = config.fetch("disable?", false)
  if disable
    lambda do |manifest|
      manifest
    end
  else
    probes_dir = config.fetch('probes-build-dir')
    out_dir = config.fetch('output-dir')
    insert_after = config.fetch('insert-after-file')
    EnsureAllExist[[probes_dir, out_dir]]
    probes_input = config.fetch('path-to-probes-input')
    probes = YAML.load_file(probes_input)
    probes_md_path = File.join(probes_dir, 'probes.md')
    File.open(probes_md_path, 'w') do |f|
      f.write("# Probe Definitions\n\n")
      probes.keys.sort_by {|k| k.downcase}.each do |k|
        table = [["Name", "Input?", "Runtime?", "Type", "Variability", "Description"]]
        flds = probes[k][:fields]
        next if flds.empty?
        name = k
        array_txt = if probes[k][:array] then "[1..]" else "" end
        title = "\\@#{name}#{array_txt}."
        owner = probes[k][:owner]
        owner_txt = if owner == "--" then "" else " (owner: #{owner})" end
        f.write("## #{title}#{owner_txt}\n\n")
        if probes[k].include?(:description)
          f.write(probes[k][:description] + "\n\n")
        end
        flds.each do |fld|
          table << [
            fld[:name],
            if fld[:input] then "X" else "--" end,
              if fld[:runtime] then "X" else "--" end,
              fld[:type],
              fld[:variability],
              fld.fetch(:description, "--")
          ]
        end
        f.write(Tables::WriteTable[ table, true ])
        f.write("\n\n\n")
      end
    end
    lambda do |manifest|
      new_manifest = []
      manifest.each do |path|
        bn = File.basename(path)
        out_path = File.join(out_dir, bn)
        new_manifest << out_path
        if bn == insert_after
          probes_out_path = File.join(out_dir, File.basename(probes_md_path))
          new_manifest << probes_out_path
          if !FileUtils.uptodate?(probes_out_path, [probes_md_path])
            FileUtils.cp(probes_md_path, probes_out_path)
          end
        end
        if !FileUtils.uptodate?(out_path, [path])
          FileUtils.cp(path, out_path)
        end
      end
      new_manifest
    end
  end
end

ReLinkHTML = lambda do |config|
  out_dir = config.fetch("output-dir")
  tags_fname_map = config.fetch("tags-filename-map")
  EnsureExists[out_dir]
  lambda do |manifest|
    if tags_fname_map.respond_to?(:call)
      tags_fname_map = tags_fname_map.call
    end
    new_manifest = []
    m = manifest.length
    n = 0
    manifest.each do |path|
      bn = File.basename(path)
      out_path = File.join(out_dir, bn)
      new_manifest << out_path
      if !FileUtils.uptodate?(out_path, [path])
        n += 1
        content = File.read(path, :encoding=>"UTF-8")
        new_content = tags_fname_map.to_a.inject(content) do |nc, tag_fname|
          tag = tag_fname[0]
          fname = tag_fname[1]
          if fname == bn
            nc
          else
            nc.gsub(/<a href="#{tag}">([^<]*)<\/a>/,
                    "<a href=\"#{fname}#{tag}\">\\1</a>")
          end
        end
        File.write(out_path, new_content)
      end
    end
    if VERBOSE
      LOG.write("Updated #{n}/#{m} files for relinking\n")
      LOG.flush
    else
      LOG.write(".")
      LOG.flush
    end
    new_manifest
  end
end

CheckCoverage = lambda do |config|
  tag = config.fetch("tag")
  this_dir = config.fetch('this-dir', THIS_DIR)
  build_dir = config.fetch("build-dir", "build")
  md_dir = config.fetch("md-dir", "md")
  context = config.fetch("preproc-context", PREPROCESSOR_CONTEXT)
  JoinFunctions[[
    ExpandPathsFrom[
      "reference-dir" => File.expand_path('src')
    ],
    PreprocessManifest[
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "preprocessed"), this_dir
      ),
      "relative-root-path" => File.expand_path('src'),
      "context" => context
    ],
    NormalizeMarkdown[
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "normalize"), this_dir
      )
    ]
  ]]
end

BuildSinglePageHTML = lambda do |config|
  tag = config.fetch("tag")
  levels = config.fetch("levels")
  this_dir = config.fetch('this-dir', THIS_DIR)
  build_dir = config.fetch("build-dir", "build")
  md_dir = config.fetch("md-dir", "md")
  html_dir = config.fetch("html-dir", "html")
  rsrc_dir = config.fetch("resource-dir", "resources")
  out_dir = config.fetch("output-dir", "build/output")
  out_file = config.fetch("output-file-name", "out.html")
  disable_probes = config.fetch("disable-probes?", false)
  disable_toc = config.fetch("disable-toc?", false)
  disable_xlink = config.fetch("disable-xlink?", false)
  disable_compression = config.fetch("disable-compression?", !USE_NODE)
  disable_numbering = config.fetch("disable-numbering?", false)
  do_navigation = config.fetch("do-navigation?", false)
  title = config.fetch("title", "CSE User's Manual")
  subtitle = config.fetch("subtitle", "California Simulation Engine")
  date = config.fetch("date", nil)
  draft = config.fetch("draft?", true)
  context = config.fetch("preproc-context", PREPROCESSOR_CONTEXT)
  section_index = config.fetch(
    "section-index-path",
    File.expand_path(
      File.join(build_dir, tag, rsrc_dir, "section-index.yaml"), this_dir
    )
  )
  record_index_file = config.fetch(
    "record-index-file",
    File.expand_path(
      File.join(build_dir, tag, rsrc_dir, "record-index.yaml"), this_dir
    )
  )
  JoinFunctions[[
    ExpandPathsFrom[
      "reference-dir" => File.expand_path('src')
    ],
    PreprocessManifest[
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "preprocessed"), this_dir
      ),
      "relative-root-path" => File.expand_path('src'),
      "context" => context
    ],
    PassThroughWithSideEffect[
      "function" => GenerateSectionIndex[
        "source-paths"=> File.expand_path(
          File.join(build_dir, tag, md_dir, "preprocessed", "**", "*.md"), this_dir
        ),
        "output-path" => section_index,
        "merge-values" => {"#probe-definitions"=>"probes.html"}
      ]
    ],
    PassThroughWithSideEffect[
      "function" => lambda do
        rec_dir = File.expand_path(
          File.join(build_dir, tag, md_dir, "preprocessed", "records"), this_dir
        )
        outpath = record_index_file
        parent = File.dirname(outpath)
        FileUtils.mkdir_p(parent) unless File.exist?(parent)
        exc_path = RECORD_INDEX_EXCEPTIONS_FILE
        deps = [exc_path] + Dir[File.join(rec_dir, '*.md')]
        if !FileUtils.uptodate?(outpath, deps)
          CreateRecordIndex[rec_dir, outpath, exc_path]
        end
      end
    ],
    NormalizeMarkdown[
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "normalize"), this_dir
      )
    ],
    XLinkMarkdown[
      "record-index-path" => record_index_file,
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "xlink"), this_dir
      ),
      "log" => STDOUT,
      "disable?" => disable_xlink
    ],
    AdjustMarkdownLevels[
      "output-dir" => File.join(build_dir, tag, md_dir, "adjusted-headers"),
      "levels" => levels
    ],
    BuildProbesAndCopyIntoManifest[
      "disable?" => disable_probes,
      "probes-build-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "probes"), this_dir
      ),
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "all-with-probes"), this_dir
      ),
      "insert-after-file" => "output-reports.md",
      "path-to-probes-input" => File.expand_path(
        File.join("config", "reference", "probes_input.yaml"), this_dir
      )
    ],
    AddFiles[
      "paths" => [File.expand_path(
        File.join("config", "template", "site-template.html"), this_dir
      )],
      "append-to-manifest?" => false,
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "all-with-probes-and-template"), this_dir
      )
    ],
    JoinManifestToString,
    RunPandoc[
      "options" => [
        "--parse-raw",
        "--standalone",
        "--wrap=none",
        "--to html5",
        "--from markdown",
        "--mathjax",
        disable_numbering ? "" : "--number-sections",
        "--css=css/base.css",
        disable_toc ? "" : "--table-of-contents",
        disable_toc ? "" : "--toc-depth=#{TOC_DEPTH}",
        "--smart",
        "--variable header=\"#{HEADER}\"",
        "--variable footer=\"#{FOOTER}\"",
        "--variable do-nav=#{do_navigation}",
        "--variable top=\"index.html\"",
        "--template=site-template.html",
        title ? "--variable title=\"#{title}\"" : "",
        subtitle ? "--variable subtitle=\"#{subtitle}\"" : "",
        date ? "--variable date=\"#{date}\"" : "",
        draft ? "--variable draft=true" : "",
      ].join(' '),
      "output-path" => File.expand_path(
        File.join(build_dir, tag, html_dir, "generated-singlepage", out_file), this_dir
      ),
      "working-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "all-with-probes-and-template"), this_dir
      ),
    ],
    Listify,
    CompressHTML[
      "path-to-html-minifier" => HTML_MIN_EXE,
      "options" => [
        "--minify-css",
        "--minify-js",
        "--remove-comments"
      ].join(' '),
      "output-dir" => File.expand_path(out_dir, this_dir),
      "disable?" => disable_compression
    ],
    NewManifest[
      Dir[
        File.expand_path(File.join("config", "css", "*.css"), this_dir)
      ]
    ],
    CompressCSS[
      "path-to-cssnano" => CSS_NANO_EXE,
      "output-dir" => File.expand_path(
        File.join(out_dir, "css"), this_dir
      ),
      "disable?" => disable_compression
    ],
    NewManifest[
      Dir[File.expand_path(File.join("src", "media", "*"), this_dir)]
    ],
    CopyToDir[
      "output-dir" => File.expand_path(
        File.join(out_dir, "media"), this_dir
      )
    ],
  ]]
end

BuildMultiPageHTML = lambda do |config|
  tag = config.fetch("tag")
  levels = config.fetch("levels")
  build_dir = config.fetch("build-dir", "build")
  md_dir = config.fetch("md-dir", "md")
  html_dir = config.fetch("html-dir", "html")
  this_dir = config.fetch("this-dir", THIS_DIR)
  out_dir = config.fetch("output-dir", "build/output")
  rsrc_dir = config.fetch("resource-dir", "resources")
  disable_probes = config.fetch("disable-probes?", false)
  disable_toc = config.fetch("disable-toc?", false)
  disable_xlink = config.fetch("disable-xlink?", false)
  disable_compression = config.fetch("disable-compression?", !USE_NODE)
  disable_numbering = config.fetch("disable-numbering?", false)
  do_navigation = config.fetch("do-navigation?", false)
  title = config.fetch("title", "CSE User's Manual")
  subtitle = config.fetch("subtitle", "California Simulation Engine")
  date = config.fetch("date", nil)
  draft = config.fetch("draft?", true)
  context = config.fetch("preproc-context", PREPROCESSOR_CONTEXT)
  section_index = config.fetch(
    "section-index-path",
    File.expand_path(
      File.join(build_dir, tag, rsrc_dir, "section-index.yaml"), this_dir
    )
  )
  record_index_file = config.fetch(
    "record-index-file",
    File.expand_path(
      File.join(build_dir, tag, rsrc_dir, "record-index.yaml"), this_dir
    )
  )
  JoinFunctions[[
    ExpandPathsFrom[
      "reference-dir" => File.expand_path('src')
    ],
    PreprocessManifest[
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "preprocessed"), this_dir
      ),
      "relative-root-path" => File.expand_path('src'),
      "context" => context
    ],
    PassThroughWithSideEffect[
      "function" => GenerateSectionIndex[
        "source-paths"=> File.expand_path(
          File.join(build_dir, tag, md_dir, "preprocessed", "**", "*.md"), this_dir
        ),
        "output-path" => section_index,
        "merge-values" => {"#probe-definitions"=>"probes.html"}
      ]
    ],
    PassThroughWithSideEffect[
      "function" => lambda do
        rec_dir = File.expand_path(
          File.join(build_dir, tag, md_dir, "preprocessed", "records"), this_dir
        )
        outpath = record_index_file
        parent = File.dirname(outpath)
        FileUtils.mkdir_p(parent) unless File.exist?(parent)
        exc_path = RECORD_INDEX_EXCEPTIONS_FILE
        deps = [exc_path]+Dir[File.join(rec_dir,'*.md')]
        if !FileUtils.uptodate?(outpath, deps)
          CreateRecordIndex[rec_dir, outpath, exc_path]
        end
      end
    ],
    NormalizeMarkdown[
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "normalize"), this_dir
      )
    ],
    XLinkMarkdown[
      "record-index-path" => record_index_file,
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "xlink"), this_dir
      ),
      "log" => STDOUT,
      "disable?" => disable_xlink
    ],
    BuildProbesAndCopyIntoManifest[
      "probes-build-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "probes"), this_dir
      ),
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "all-with-probes"), this_dir
      ),
      "insert-after-file" => "output-reports.md",
      "path-to-probes-input" => File.expand_path(
        File.join("config", "reference", "probes_input.yaml"), this_dir
      ),
      "disable?" => disable_probes
    ],
    NumberMd[
      "output-dir" => File.join(build_dir, tag, md_dir, "number"),
      "levels" => levels + (disable_probes ? [] : [1]),
      "disable?" => disable_numbering
    ],
    GenTOC[
      "max-level" => TOC_DEPTH,
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "toc"), this_dir
      ),
      "toc-name" => "index.md",
      "levels" => (levels + (disable_probes ? [] : [1])).map do |lev|
        lev - 1
      end,
      "disable?" => disable_toc
    ],
    AddFiles[
      "paths" => [File.expand_path(
        File.join("config", "template", "site-template.html"), this_dir
      )],
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "ready-to-build"), this_dir
      )
    ],
    RunPandocOverEach[
      "options" => [
        "--parse-raw",
        "--standalone",
        "--wrap=none",
        "--to html5",
        "--from markdown",
        "--mathjax",
        "--css=css/base.css",
        "--smart",
        "--template=site-template.html",
        "--variable header=\"#{HEADER}\"",
        "--variable footer=\"#{FOOTER}\"",
        title ? "--variable title=\"#{title}\"" : "",
        subtitle ? "--variable subtitle=\"#{subtitle}\"" : "",
        date ? "--variable date=\"#{date}\"" : "",
        draft ? "--variable draft=true" : "",
      ].join(' '),
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, html_dir, "generated-multipage"), this_dir
      ),
      "top-url" => "../index.html",
      "toc-url" => "index.html",
      "do-navigation?" => do_navigation
    ],
    ReLinkHTML[
      "tags-filename-map" => lambda do
        index = YAML.load_file(record_index_file)
        index.to_a.sort_by do |e|
          e[0]
        end.inject(YAML.load_file(section_index)) do |m, e|
          fname_tag = e[1].scan(/([^\#]*)(\#.*)/).flatten
          m.merge(Hash[fname_tag[1], fname_tag[0]])
        end
      end,
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, html_dir, "relinked"), this_dir
      )
    ],
    CompressHTML[
      "path-to-html-minifier" => HTML_MIN_EXE,
      "options" => [
        "--minify-css",
        "--minify-js",
        "--remove-comments"
      ].join(' '),
      "output-dir" => File.expand_path(out_dir, this_dir),
      "disable?" => disable_compression
    ],
    NewManifest[
      Dir[
        File.expand_path(File.join("config", "css", "*.css"), this_dir)
      ]
    ],
    CompressCSS[
      "path-to-cssnano" => CSS_NANO_EXE,
      "output-dir" => File.expand_path(
        File.join(out_dir, "css"), this_dir
      ),
      "disable?" => disable_compression
    ],
    NewManifest[
      Dir[File.expand_path(File.join("src", "media", "*"), this_dir)]
    ],
    CopyToDir[
      "output-dir" => File.expand_path(
        File.join(out_dir, "media"), this_dir
      )
    ],
  ]]
end

BuildPDF = lambda do |config|
  tag = config.fetch("tag")
  levels = config.fetch("levels")
  this_dir = config.fetch('this-dir', THIS_DIR)
  build_dir = config.fetch("build-dir", "build")
  md_dir = config.fetch("md-dir", "md")
  out_dir = config.fetch("output-dir", "build/output")
  out_file = config.fetch("output-file-name", "out.pdf")
  rsrc_dir = config.fetch("resource-dir", "resources")
  disable_probes = config.fetch("disable-probes?", false)
  disable_toc = config.fetch("disable-toc?", false)
  disable_xlink = config.fetch("disable-xlink?", false)
  title = config.fetch("title", "CSE User's Manual")
  subtitle = config.fetch("subtitle", "California Simulation Engine")
  date = config.fetch("date", nil)
  draft = config.fetch("draft?", true)
  context = config.fetch("preproc-context", PREPROCESSOR_CONTEXT)
  section_index = config.fetch(
    "section-index-path",
    File.expand_path(
      File.join(build_dir, tag, rsrc_dir, "section-index.yaml"), this_dir
    )
  )
  record_index_file = config.fetch(
    "record-index-file",
    File.expand_path(
      File.join(build_dir, tag, rsrc_dir, "record-index.yaml"), this_dir
    )
  )
  JoinFunctions[[
    ExpandPathsFrom[
      "reference-dir" => File.expand_path('src')
    ],
    PreprocessManifest[
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "preprocessed"), this_dir
      ),
      "relative-root-path" => File.expand_path('src'),
      "context" => context
    ],
    PassThroughWithSideEffect[
      "function" => GenerateSectionIndex[
        "source-paths"=> File.expand_path(
          File.join(build_dir, tag, md_dir, "preprocessed", "**", "*.md"), this_dir
        ),
        "output-path" => section_index,
        "merge-values" => {"#probe-definitions"=>"probes.html"}
      ]
    ],
    PassThroughWithSideEffect[
      "function" => lambda do
        rec_dir = File.expand_path(
          File.join(build_dir, tag, md_dir, "preprocessed", "records"), this_dir
        )
        outpath = record_index_file
        parent = File.dirname(outpath)
        FileUtils.mkdir_p(parent) unless File.exist?(parent)
        exc_path = RECORD_INDEX_EXCEPTIONS_FILE
        deps = [exc_path]+Dir[File.join(rec_dir,'*.md')]
        if !FileUtils.uptodate?(outpath, deps)
          CreateRecordIndex[rec_dir, outpath, exc_path]
        end
      end
    ],
    NormalizeMarkdown[
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "normalize"), this_dir
      )
    ],
    XLinkMarkdown[
      "record-index-path" => record_index_file,
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "xlink"), this_dir
      ),
      "log" => STDOUT,
      "disable?" => disable_xlink
    ],
    AdjustMarkdownLevels[
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "adjusted-headers"), this_dir
      ),
      "levels" => levels
    ],
    BuildProbesAndCopyIntoManifest[
      "disable?" => disable_probes,
      "probes-build-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "probes"), this_dir
      ),
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "all-with-probes"), this_dir
      ),
      "insert-after-file" => "output-reports.md",
      "path-to-probes-input" => File.expand_path(
        File.join("config", "reference", "probes_input.yaml"), this_dir
      )
    ],
    AddFiles[
      "paths" => [File.expand_path(
        File.join("config", "template", "template.tex"), this_dir
      )],
      "append-to-manifest?" => false,
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "all-ready-to-build"), this_dir
      )
    ],
    AddFiles[
      "paths" => Dir[
        File.expand_path(File.join("src", "media", "*"), this_dir)
      ],
      "append-to-manifest?" => false,
      "output-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "all-ready-to-build", "media"), this_dir
      )
    ],
    JoinManifestToString,
    RunPandoc[
      "options" => [
        "--parse-raw",
        "--standalone",
        "--wrap=none",
        '--variable geometry="margin=1in"',
        '--variable urlcolor=cyan',
        "--latex-engine=xelatex",
        disable_toc ? "" : "--table-of-contents",
        disable_toc ? "" : "--toc-depth=#{TOC_DEPTH}",
        "--number-sections",
        "--smart",
        "--template=template.tex",
        "--listings",
        "--from markdown",
        "--variable header=\"#{HEADER}\"",
        "--variable footer=\"#{FOOTER}\"",
        title ? "--variable title=\"#{title}\"" : "",
        subtitle ? "--variable subtitle=\"#{subtitle}\"" : "",
        date ? "--variable date=\"#{date}\"" : "",
        draft ? "--variable draft=true" : "",
      ].join(' '),
      "output-path" => File.expand_path(
        File.join(out_dir, out_file), this_dir
      ),
      "working-dir" => File.expand_path(
        File.join(build_dir, tag, md_dir, "all-ready-to-build"), this_dir
      ),
    ],
  ]]
end

########################################
# Tasks
########################################
def time_it(&block)
  start_time = Time.now
  block.call
  puts("Elapsed time: #{Time.now - start_time} seconds")
end

desc "Setup gh-pages and node if used"
task :setup do
  SetupGHPages[] if USE_GHPAGES
  InstallNodeDeps[] if USE_NODE
end

desc "Build single-page HTML"
task :build_html_single => [:setup] do
  time_it do
    puts("#"*60)
    puts("Build Single-Page HTML")
    tag = "cse-user-manual-single-page"
    processed_manifest_path = File.join(BUILD_DIR, tag, 'md', 'preprocessed')
    context = PREPROCESSOR_CONTEXT.merge({'build_type'=>'build_html_single'})
    PreprocessManifest[
      'output-dir' => processed_manifest_path,
      'context' => context
    ][[CSE_USER_MANUAL_MANIFEST_PATH]]
    doc = YAML.load_file(
      File.join(
        processed_manifest_path,
        File.basename(CSE_USER_MANUAL_MANIFEST_PATH)
      )
    )
    manifest = doc["sections"]
    levels = manifest.map {|level, _| level}
    files = manifest.map {|_, path| path} 
    BuildSinglePageHTML[
      "tag" => tag,
      "levels" => levels,
      "draft?" => DRAFT,
      "date" => DATE,
      "output-dir" => "build/output",
      "output-file-name" => "cse-user-manual.html",
      "do-navigation?" => true,
      "disable-compression?" => !USE_NODE,
      "context" => context,
    ][files]
    puts("\nSingle-Page HTML DONE!")
    puts("^"*60)
    CheckManifests[processed_manifest_path] if VERIFY_MANIFEST
  end
end

desc "Build multi-page html"
task :build_html_multi => [:setup] do
  time_it do
    puts("#"*60)
    puts("Build Multi-Page HTML")
    tag = "cse-user-manual-multi-page"
    processed_manifest_path = File.join(BUILD_DIR, tag, 'md', 'preprocessed')
    context = PREPROCESSOR_CONTEXT.merge({'build_type'=>'build_html_multi'})
    PreprocessManifest[
      'output-dir' => processed_manifest_path,
      'context' => context
    ][[CSE_USER_MANUAL_MANIFEST_PATH]]
    doc = YAML.load_file(
      File.join(
        processed_manifest_path,
        File.basename(CSE_USER_MANUAL_MANIFEST_PATH)
      )
    )
    manifest = doc["sections"]
    levels = manifest.map {|level, _| level}
    files = manifest.map {|_, path| path} 
    BuildMultiPageHTML[
      "tag" => tag,
      "levels" => levels,
      "draft?" => DRAFT,
      "date" => DATE,
      "output-dir" => "build/output/cse-user-manual",
      "do-navigation?" => true,
      "disable-compression?" => !USE_NODE,
      "context" => context,
    ][files]
    puts("\nMulti-Page HTML DONE!")
    puts("^"*60)
    CheckManifests[processed_manifest_path] if VERIFY_MANIFEST
  end
end

desc "Build PDF"
task :build_pdf => [:setup] do
  time_it do
    puts("#"*60)
    puts("Building PDF...(note: can take up to several minutes)")
    tag = "cse-user-manual-pdf"
    processed_manifest_path = File.join(BUILD_DIR, tag, 'md', 'preprocessed')
    context = PREPROCESSOR_CONTEXT.merge({'build_type'=>'build_pdf'})
    PreprocessManifest[
      'output-dir' => processed_manifest_path,
      'context' => context
    ][[CSE_USER_MANUAL_MANIFEST_PATH]]
    doc = YAML.load_file(
      File.join(
        processed_manifest_path,
        File.basename(CSE_USER_MANUAL_MANIFEST_PATH)
      )
    )
    manifest = doc["sections"]
    levels = manifest.map {|level, _| level}
    files = manifest.map {|_, path| path} 
    BuildPDF[
      "tag" => tag,
      "levels" => levels,
      "output-dir" => "build/output/pdfs",
      "output-file-name" => "cse-user-manual.pdf",
    ][files]
    puts("\nPDF DONE!")
    puts("^"*60)
    CheckManifests[processed_manifest_path] if VERIFY_MANIFEST
  end
end

desc "Build Site"
task :build_site => [:setup] do
  time_it do
    puts("#"*60)
    puts("Build Site HTML")
    tag = "web-site"
    processed_manifest_path = File.join(BUILD_DIR, tag, 'md', 'preprocessed')
    context = PREPROCESSOR_CONTEXT.merge({'build_type'=>'build_site'})
    PreprocessManifest[
      'output-dir' => processed_manifest_path,
      'context' => context
    ][[WEB_SITE_MANIFEST_PATH]]
    doc = YAML.load_file(
      File.join(
        processed_manifest_path,
        File.basename(WEB_SITE_MANIFEST_PATH)
      )
    )
    manifest = doc["sections"]
    levels = manifest.map {|level, _| level}
    files = manifest.map {|_, path| path} 
    BuildMultiPageHTML[
      "tag" => tag,
      "levels" => levels,
      "date" => nil,
      "draft?" => true,
      "subtitle" => nil,
      "title" => "California Simulation Engine",
      "do-navigation?" => false,
      "output-dir" => "build/output",
      "disable-toc?" => true,
      "disable-xlink?" => true,
      "disable-compression?" => !USE_NODE,
      "context" => context,
    ][files]
    puts("\nSite HTML DONE!")
    puts("^"*60)
    CheckManifests[processed_manifest_path] if VERIFY_MANIFEST
  end
end

all_builds = [:build_html_single, :build_html_multi, :build_site]
all_builds << :build_pdf if BUILD_PDF
all_builds << :verify_links if VERIFY_LINKS
all_builds << :coverage if RUN_COVERAGE

desc "Build everything"
task :build_all => all_builds

desc "Check links for issues"
task :verify_links do
  puts("Checking links")
  problems = VerifyLinks::CheckLinks[HTML_OUT_DIR]
  problems.each do |p|
    puts("  - #{p}")
  end
end

desc "Build everything (alias for build_all)"
task :build => [:build_all]

desc "Removes the entire build directory. Note: you will loose build cache!"
task :clean_all do
  Clean[BUILD_DIR]
end

desc "Alias for clean_all"
task :clean do
  Clean[BUILD_DIR]
end

desc "Alias for clean_all"
task :reset do
  Clean[BUILD_DIR]
end

desc "Check manifests for missing/misspelled files"
task :check_manifests do
  CheckManifests[SRC_DIR]
end

desc "Render file specified with FILE env variable to #{ERB_OUTPUT_FILE}"
task :erb do
  if ENV.include? "FILE"
    Template::PreprocFile[
      ENV["FILE"],
      ERB_OUTPUT_FILE,
      1,
      "context" => PREPROCESSOR_CONTEXT
    ]
  else
    puts("You must define the environment variable `FILE` so we\n" +
         "know which file to test preprocessing with")
    exit(1)
  end
end

desc "Run documentation coverage checker"
task :coverage do
  time_it do
    puts("#"*60)
    puts("Check CSE User Manual's Documentation Coverage")
    tag = "cse-user-manual-coverage"
    processed_manifest_path = File.join(BUILD_DIR, tag, 'md', 'preprocessed')
    context = PREPROCESSOR_CONTEXT.merge({'build_type'=>'build_html_single'})
    PreprocessManifest[
      'output-dir' => processed_manifest_path,
      'context' => context
    ][[CSE_USER_MANUAL_MANIFEST_PATH]]
    doc = YAML.load_file(
      File.join(
        processed_manifest_path,
        File.basename(CSE_USER_MANUAL_MANIFEST_PATH)
      )
    )
    manifest = doc["sections"]
    files = manifest.map {|_, path| path} 
    CheckCoverage[
      "tag" => tag,
      "context" => context,
    ][files]
    ris1 = CoverageCheck::ReadCulList[
      File.join('config','reference','cullist.txt')
    ]
    files = Dir[File.join(BUILD_DIR, tag, 'md', 'preprocessed', 'records', '*.md')].map do |path|
      File.join(BUILD_DIR, tag, 'md', 'normalize', File.basename(path))
    end
    ris2 = CoverageCheck::ReadAllRecordDocuments[files]
    ris3 = CoverageCheck::DropNameFieldsIfNotInRef[CoverageCheck::AdjustMap[ris2], ris1]
    diffs = CoverageCheck::RecordInputSetDifferences[ris1, ris3, false]
    diff_report = CoverageCheck::RecordInputSetDifferencesToString[
      diffs, "CSE", "Documentation"
    ]
    puts("\n\n"+diff_report)
    File.write("documentation-coverage-report.txt", diff_report)
    puts("\nCoverage Check DONE!")
    puts("^"*60)
  end
end

Rake::TestTask.new(:test) do |t|
  t.libs << "test"
  t.libs << "lib"
  t.test_files = FileList['test/**/*_test.rb']
end

task :default => [:build_all]
