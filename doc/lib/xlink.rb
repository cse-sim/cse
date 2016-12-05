require 'set'
require_relative 'pandoc'

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
#
#     OverString :: Markdown FilePath FileLevel NoLinkLevels RecordIndex SectionPath -> (Tuple Markdown SectionPath)
#
# ### Purpose Statement
#
# #### XLinkMarkdownNoSelflinks
#
# The XLinkMarkdownNoSelflinks block is designed to be configured and return a
# closure around a function that takes a manifest of files to process,
# processes them, and returns the manifest of the newly processed files.
#
# #### OverString
#
# Given a markdown string, a file path corresponding to the markdown string,
# the file level of the path, the levels to check for no-linking, a record
# index and a section path, cross-link the given markdown string and return
# it along with any updates to the section path. Note: the FilePath has to be
# equivalent to what is used in the values of the RecordIndex.
#
# ### Header
#
# We should utilize MapOverManifest (and pull that into library code while
# we're at it) for XLinkMarkdownNoSelflinks.
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

  # String RJson (Record :rec_idx RecordIndex :rec_name_set (Set String) :sec_path (Array String))
  Walker = lambda do |tag, the_content, state|
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

  # Markdown FilePath FileLevel NoLinkLevels RecordIndex SectionPath -> (Tuple Markdown SectionPath)
  OverString = lambda do |md_str, path, hdr_lvl, nolink_lvl, rec_idx, sec_path|
    rjson = JSON.parse(Pandoc::MdToJson[md_str])
    state = {
      num_hits: 0,
      rec_idx: rec_idx,
      rec_name_set: Set.new(rec_idx.keys),
      sec_path: sec_path.dup
    }
    # ...
    new_rjson = Pandoc::Walk[rjson, Walker, state]
    new_md_str = Pandoc::JsonToMd[new_rjson]
    [new_md_str, state[:sec_path]]
  end

end
