require 'set'
require_relative 'pandoc'
require_relative 'slug'

# Below method of program design adapted from "How to Design Programs, 2nd
# Edition":
# http://www.ccs.neu.edu/home/matthias/HtDP2e/part_preface.html#%28part._.Systematic_.Program_.Design%29
#
# Problem Analysis and Data Definitions
# -------------------------------------
#
# ### Problem Analysis
#
# The current cross-linking algorithm does not know where it is in a
# markdown-based document. As such, it will link any term that matches criteria
# to its "canonical source link". What we would like is that if a term links to
# the "parent section" of the current documentation (and optionally arbitrarily
# high up the ancestor "path"), then we DO NOT want to cross-link it, otherwise
# we do. This will prevent a new section on Foo from getting multiple links to
# the term "Foo" in the opening paragraph which is just silly and causes visual
# clutter.
#
# The current cross-linking algorithm works but is a fairly large piece of code
# that could use some refactoring.
#
# ### Data Definitions
#
#     QuoteType :: (Case "SingleQuote" "DoubleQuote")
#     -------------------- 
#     Types of Quotes -- used in Pandoc JSON
#
#     Inline :: (Case
#       {'t' => 'Str', 'c' => String}
#       {'t' => 'Emph', 'c' => (Array Inline)}
#       {'t' => 'Strong', 'c' => (Array Inline)}
#       {'t' => 'Strikeout', 'c' => (Array Inline)}
#       {'t' => 'Superscript', 'c' => (Array Inline)}
#       {'t' => 'Subscript', 'c' => (Array Inline)}
#       {'t' => 'SmallCaps', 'c' => (Array Inline)}
#       {'t' => 'Quoted', 'c' => (Tuple QuoteType (Array Inline))}
#       {'t' => 'Cite', 'c' => (Tuple (Array Citation) (Array Inline))}
#       {'t' => 'Code', 'c' => (Tuple Attr String)}
#       {'t' => 'Space', 'c' => []}
#       {'t' => 'SoftBreak', 'c' => []}
#       {'t' => 'LineBreak', 'c' => []}
#       {'t' => 'Math', 'c' => (Tuple MathType String)}
#       {'t' => 'RawInline', 'c' => (Tuple Format String)}
#       {'t' => 'Link', 'c' => (Tuple Attr (Array Inline) Target)}
#       {'t' => 'Image', 'c' => (Tuple Attr (Array Inline) Target)}
#       {'t' => 'Note', 'c' => (Array Block)}
#       {'t' => 'Span', 'c' => (Tuple Attr (Array Inline))})
#     --------------------
#     Types of Inline constructs in Pandoc JSON
#
#     RecordIndex :: (Map String String)
#     --------------------
#
#     A mapping from the name of a record (e.g., "CONSTRUCTION") to its
#     location in the final rendered HTML file including both filename (e.g.,
#     "construction.html") AND bookmark (e.g., "#construction")
#
#     Example:
#     {
#        "CONSTRUCTION" => "construction.html#construction",
#        "DHWDAYUSE" => "dhwdayuse.html#dhwdayuse",
#        "DHWHEATER" => "dhwheater.html#dhwheater"
#     }
#
#     Markdown :: String
#     --------------------
#     
#     This is just text in Markdown format.
#
#     RJson :: Int | String | Bool | Float | (Map RJSON RJSON) | (Array RJSON)
#     --------------------
#
#     This is JSON (javascript object notation) represented in Ruby format.
#
#     JSON :: String
#     --------------------
#
#     A string in JSON format.
#
#     SectionPath :: (Array String)
#     --------------------
#
#     An array of strings of section headers to identify the section one is in at the moment.
#
#     FilePath :: String
#     --------------------
#
#     The path to a file.
#
#     Manifest :: (Array String)
#     -------------------- 
#
#     An array of full file paths constituting one entire "document".
#
#     FileLevel :: PositiveInt
#     -------------------- 
#
#     The level of the file in the overall document hierarchy. A level of "1"
#     indicates that there should be no adjustments to the headings in the
#     current file. A level of 2 or greater indicates that headings should be
#     shifted (FileLevel - 1) positions (i.e., an H1 would become an H3 if
#     FileLevel is 3 -- H1 + (3 - 1) = H3)
#
#     NoLinkLevels :: NonNegativeInteger
#     -------------------- 
#
#     The number of levels of hierarchy to screen for skipping potential
#     "self-links". Set to 0 to get the "link everywhere" behavior.
#
# Signature, Purpose Statement, Header
# ------------------------------------
#
# ### Signature
#
#     XLinkMarkdownNoSelflinks :: (Map String Any) ->
#       ((Array String) -> (Array String))
# The XLinkMarkdownNoSelflinks block is designed to be configured and return a
# closure around a function that takes a manifest of files to process,
# processes them, and returns the manifest of the newly processed files.
#
#     OverString :: Markdown FilePath FileLevel NoLinkLevels RecordIndex SectionPath -> (Tuple Markdown SectionPath)
# Given a markdown string, a file path corresponding to the markdown string,
# the file level of the path, the levels to check for no-linking, a record
# index and a section path, cross-link the given markdown string and return
# it along with any updates to the section path. Note: the FilePath has to be
# equivalent to what is used in the values of the RecordIndex.
#
# ### Notes
#
# - make the log a config variable, not a global; although, it could default to
#   STDOUT...
#
# Function Examples
# -----------------
#
# Record Index:
#
#     rec_idx = {"STUFF"=>"index.html#stuff","JUNK"=>"index.html#junk"}
#
# SectionPath:
#
#     sec_path = []
#
# Source String, `s`:
#
#     # STUFF
#
#     There is a lot of STUFF available in the world, including a lot of JUNK.
#
#     # JUNK
#
#     JUNK is a kind of STUFF.
#
# File name of source string:
#
#     file_basename = "index.html"
#
# This would be the result of running the source string through our processor:
#
#     out, new_sec_path = OverString[s, file_basename, 1, 1, rec_idx, sec_path]
#
# Contents of `out`:
#
#     # STUFF
#
#     There is a lot of STUFF available in the world, including a lot of [JUNK](#junk).
#
#     # JUNK
#
#     JUNK is a kind of [STUFF](#stuff).
#
# Contents of `new_sec_path`:
#
#     ["junk"]
#
# Calling with:
#
#     out, new_sec_path = OverString[s, "other.html", 1, 1, rec_idx, sec_path]
#
# Would result in `out` as:
#
#     # STUFF
#
#     There is a lot of [STUFF](index.html#stuff) available in the world, including a lot of [JUNK](index.html#junk).
#
#     # JUNK
#
#     [JUNK](index.html#junk) is a kind of [STUFF](#stuff).
#
# Contents of `new_sec_path`:
#
#     ["junk"]
#
module XLink
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

  # String RJson (Record :rec_idx RecordIndex :rec_name_set (Set String) :sec_path (Array String))
  Walker = lambda do |tag, the_content, state|
    g = lambda do |inline|
      if inline['t'] == 'Str'
        index = state[:rec_idx]
        rns = state[:rec_name_set]
        word = inline['c']
        word_only = word.scan(/[a-zA-Z:]/).join
        word_only_singular = DePluralize[word_only]
        if rns.include?(word_only) or rns.include?(word_only_singular)
          if index.include?(word_only) or index.include?(word_only_singular)
            w = rns.include?(word_only) ? word_only : word_only_singular
            state[:num_hits] += 1
            ref = index[w].scan(/(\#.*)/).flatten[0]
            {'t' => 'Link',
             'c' => [
               ["", [], []],
               [{'t'=>'Str','c'=>word}],
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

  OverFileOrig = lambda do |path, out_path, idx, config|
    content = File.read(path, :encoding=>"UTF-8")
    rec_idx_path = config.fetch("record-index-path")
    rec_idx = YAML.load_file(rec_idx_path)
    rec_name_set = Set.new(rec_idx.keys)
    state = {
      num_hits: 0,
      rec_idx: rec_idx,
      rec_name_set: rec_name_set
    }
    doc = JSON.parse(Pandoc::MdToJson[content])
    new_doc = Pandoc::Walk[doc, Walker, state]
    new_content = Pandoc::JsonToMd[new_doc]
    LogRun[path, config, state]
    File.write(out_path, new_content)
  end

  # String (Record "log"? (Or IO Nil) "verbose"? Bool) (Record :num_hits Int) -> Void
  # Prints a log to the "log" field of the config variable if exists. If "verbose" field
  # is set to true, logs a line stating the number of hits in the given file, otherwise "."
  LogRun = lambda do |path, config, state|
    log = config.fetch("log", nil)
    if config.fetch("verbose", false) and log
      log.write("CrossLinked #{File.basename(path)}; #{state[:num_hits]} found\n")
      log.flush
    elsif log
      log.write(".")
      log.flush
    end
  end

  # String RJson (Record :rec_idx RecordIndex :rec_name_set (Set String) :sec_path (Array String))
  # Walker that keeps track of the section it is in
  SectionWalker = lambda do |tag, the_content, state|
    g = lambda do |inline|
      if inline['t'] == 'Str'
        index = state[:rec_idx]
        rns = state[:rec_name_set]
        sp = state[:sec_path]
        word = inline['c']
        word_only = word.scan(/[a-zA-Z:]/).join
        word_only_singular = DePluralize[word_only]
        if rns.include?(word_only) or rns.include?(word_only_singular)
          if index.include?(word_only) or index.include?(word_only_singular)
            w = rns.include?(word_only) ? word_only : word_only_singular
            ref = index[w].scan(/(\#.*)/).flatten[0]
            id = index[w]
            if InSectionPath[sp, id, state[:nolink_lvl]]
              inline
            else
              state[:num_hits] += 1
              {'t' => 'Link',
               'c' => [
                 ["", [], []],
                 [{'t'=>'Str','c'=>word}],
                 [ref, '']]}
            end
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
    elsif tag == "Header"
      sp = state[:sec_path]
      h_adj = state[:hdr_adj]
      h_lvl, _, inlines = the_content
      h_adj = state[:hdr_adj]
      slug = Slug::Slugify[InlinesToText[inlines]]
      new_sp = UpdateSectionPath[
        sp,
        File.basename(state[:file_path],'.*') + '.html' + "#" + slug,
        h_lvl + h_adj
      ]
      state[:sec_path] = new_sp
      nil
    else
      nil
    end
  end

  InlinesToText_vtable = {
    'Str' => lambda {|c| c},
    'Emph' => lambda {|c| '*' + InlinesToText[c] + '*'},
    'Strong' => lambda {|c| '**' + InlinesToText[c] + '**'},
    'Strikeout' => lambda {|c| '~~' + InlinesToText[c] + '~~'},
    'Superscript' => lambda {|c| '^' + InlinesToText[c] + '^'},
    'Subscript' => lambda {|c| '~' + InlinesToText[c] + '~'},
    'SmallCaps' => lambda {|c| '[' + InlinesToText[c] + ']{style="font-variant:small-caps;"}'},
    'Quoted' => lambda do |c|
      q = (c[0] == "DoubleQuote" ? '"' : "'")
      q + InlinesToText[c[1]] + q
    end,
    'Space' => lambda {|_| " "},
  }

  # (Array Inline) -> String
  # Compute a (markdown) string from an array of inlines
  InlinesToText = lambda do |inlines|
    out = ""
    inlines.each do |inln|
      f = InlinesToText_vtable.fetch(inln['t'])
      out += f[inln['c']]
    end
    out
  end

  # String (Array String) PositiveInt (Array Inline) -> (Array String)
  # Updates the section path given the current file path, the existing section
  # path, the header level, and the header text
  UpdateSectionPath = lambda do |sec_path, id, lvl|
    new_sp = []
    0.upto(lvl).each do |idx|
      if idx == (lvl - 1)
        new_sp << id
        break
      else
        new_sp << sec_path[idx]
      end
    end
    new_sp
  end

  # (Array String) String NonNegativeInt -> Bool
  # Return true if `id` is in the first `search_levels` of section path, else false.
  InSectionPath = lambda do |sec_path, id, search_levels|
    p = sec_path.reverse.take(search_levels)
    p.include?(id)
  end

  # Markdown FilePath FileLevel NoLinkLevel RecordIndex SectionPath -> (Tuple Markdown SectionPath)
  # Add cross-linking to the given file and return. Don't cross-link when link
  # goes to a header in the direct path of the current location up to
  # NoLinkLevels up the ancestor chain.
  OverString = lambda do |md_str, path, hdr_lvl, nolink_lvl, rec_idx, sec_path|
    rjson = JSON.parse(Pandoc::MdToJson[md_str])
    state = {
      num_hits: 0,
      rec_idx: rec_idx,
      rec_name_set: Set.new(rec_idx.keys),
      hdr_adj: (hdr_lvl - 1),
      nolink_lvl: nolink_lvl,
      sec_path: sec_path.dup,
      file_path: path
    }
    new_rjson = Pandoc::Walk[rjson, SectionWalker, state]
    new_md_str = Pandoc::JsonToMd[new_rjson]
    [new_md_str, state[:sec_path], state[:num_hits]]
  end

  # FilePath FilePath Int (Record "record-index-path" String
  #   "section-path"? (Array String) "levels" (Array Int) "nolinklevel" Int
  #   "verbose"? Bool "log"? IO) -> Void
  # Assume that path is in the format to correspond to paths in config record-index
  OverFile = lambda do |path, out_path, idx, config|
    rec_idx_path = config.fetch("record-index-path")
    rec_idx = YAML.load_file(rec_idx_path)
    sec_path = config.fetch("section-path", [])
    nolink_level = config.fetch("nolinklevel", 1)
    levels = config.fetch("levels", nil)
    level = if levels.nil? then 1 else levels[idx] end
    md, new_sp, num_hits = OverString[
      File.read(path), path, level, nolink_level, rec_idx, sec_path
    ]
    config["section-path"] = new_sp
    LogRun[path, config, {num_hits: num_hits}]
    File.write(out_path, md)
  end
end
