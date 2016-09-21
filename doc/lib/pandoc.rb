# Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file.
require_relative('command')

module Pandoc
  # Create an Alias
  C = Command

  STD_MD_OPTS = [
    '-t markdown',
    '--atx-headers',
    '--normalize',
    '--wrap=none',
    '+RTS', '-K128m', '-RTS'
  ]

  # (Or (Array String) String) ?String -> String
  # Given an array of strings of options to pandoc and an optional input to
  # feed into Pandoc, return Pandoc's output, erroring if Pandoc errors
  Run = lambda do |opts, input=nil|
    C::Run['pandoc', opts, input]
  end

  # MdToJson : MarkdownString -> JsonString
  # Transform a MarkdownString into a JsonString.
  # Note: to get Ruby data structures, run the output string through JSON.parse
  MdToJson = lambda do |md_str|
    Run[["-t json", "-f markdown"], md_str]
  end

  # GetDocMeta :: DocIR -> RubyJson
  # Given a Pandoc DocIR document, extract the meta-data portion and return it.
  GetDocMeta = lambda do |doc_ir|
    doc_ir[0]
  end

  # GetDocContent :: DocIR -> DocSnippet
  # Given a DocIR, extract the content portion
  GetDocContent = lambda do |doc_ir|
    doc_ir[1]
  end

  # GetItemTag :: DocItem -> String
  # Given a top item as from the items in the content portion of a
  # PandocRubyJson document, return the tag (a string)
  GetItemTag = lambda do |doc_item|
    doc_item.fetch("t")
  end

  # GetHeaderLevel :: HeaderItem -> Int
  # Given a RubyJson representation of a header as from a PandocRubyJson
  # document, return the heading level
  GetHeaderLevel = lambda do |h|
    h["c"][0].to_i
  end

  # SplitJsonByHeadingLevel :: DocIR (Array Int) -> (Array DocIR)
  # This method takes (1) DocIR document and (2) an array of heading levels to
  # split the document at and returns an array of DocIR documents split at the
  # given heading levels. Note: the original document's meta-data is inserted
  # verbatim into each subdocument as it's meta-data.
  SplitJsonByHeadingLevel = lambda do |doc_ir, heading_levels|
    docs = []
    meta = GetDocMeta[doc_ir]
    current_doc = []
    GetDocContent[doc_ir].each do |top_item|
      tag = GetItemTag[top_item]
      if tag == "Header" and heading_levels.include?(GetHeaderLevel[top_item])
        docs << [meta.clone, current_doc] unless current_doc == []
        current_doc = []
        current_doc << top_item
      else
        current_doc << top_item
      end
    end
    docs << [meta.clone, current_doc] unless current_doc == []
  end

  # ObjsOfTag :: DocIR String -> (Array DocItem)
  # Given a DocIR Document and the Tag (a string) to search for, return all
  # occurrences of the given tag in an array. Note: only searches the top
  # level.
  ObjsOfTag = lambda do |doc_ir, tag|
    items = []
    GetDocContent[doc_ir].each do |item|
      items << item if GetItemTag[item] == tag
    end
    items
  end

  # FirstObjOfTag :: DocIR String -> (Or DocItem nil)
  # Given a DocIR Document and a Tag (a string) to search for, return the
  # first occurrence of the given tag, a DocItem
  # Note: only searches the top level.
  FirstObjOfTag = lambda do |doc_ir, tag|
    GetDocContent[doc_ir].each do |item|
      return item if GetItemTag[item] == tag
    end
    nil
  end

  # MakeEmptyDocMeta :: () -> { unMeta :: EmptyMap }
  # Creates an empty document metadata object. Since Ruby doesn't have proper
  # immutable objects, it's safer just to make a new one each time with a
  # call to this function.
  MakeEmptyDocMeta = lambda do
    {"unMeta"=>{}}
  end

  # MakeDocIr :: (Or DocItem DocSnippet) ?DocMeta -> DocIR
  # Given a block DocItem or DocSnippet, create a proper DocIR given an
  # optional DocMeta. If no DocMeta is provided, a blank DocMeta will be used
  MakeDocIr = lambda do |snippet, meta=nil|
    meta = MakeEmptyDocMeta[] if meta.nil?
    content = if snippet.class == {}.class
                [snippet]
              elsif snippet.class == [].class
                snippet
              else
                raise ArgumentError, "unhandled object: #{snippet}"
              end
    [meta, content]
  end

  # DocSnippetToMd :: (Or Nil DocItem DocSnippet) -> MarkdownString
  # Given a part of a document (that must be a block element or array of
  # block elements), wrap it in such a way that we can create a Markdown
  # string from it and return that string. If snippet is nil, returns ""
  DocSnippetToMd = lambda do |snippet|
    return "" if snippet.nil?
    doc = MakeDocIr[snippet]
    json_str = JSON.generate(doc)
    Run[['-f json'] + STD_MD_OPTS, json_str]
  end

  # MakeAttr :: String ?(Array String) ?(Array (Tuple String String)) -> AttrItem
  # Creates a new attribute with the given ID (a string), the given classes
  # (optional), and the given Array of Key/Value pairs (both strings). Returns
  # the AttrItem
  MakeAttr = lambda do |id, classes=nil, key_val_pairs=nil|
    classes = [] if classes.nil?
    key_val_pairs = [] if key_val_pairs.nil?
    [id, classes, key_val_pairs]
  end

  # StrItem :: String -> StrItem
  # Creates a new string item. The String passed in should NOT contain
  # any spaces or special markup. It should essentially be just one
  # word or token. Returns the new StrItem for the given text.
  StrItem = lambda do |text|
    {"t"=>"Str",
     "c"=>text}
  end

  # SpaceItem :: () -> SpaceItem
  # Creates and returns a new SpaceItem
  SpaceItem = lambda do
    {"t"=>"Space",
     "c"=>[]}
  end

  # StrToDocItems :: String -> (Array DocItem)
  # Turns a string into an array of Str and Space DocInline items
  StrToDocItems = lambda do |s|
    items = []
    toks = s.split(/ /)
    num_toks = toks.length
    cnt = 0
    toks.each do |tok|
      items << StrItem[tok]
      cnt += 1
      items << SpaceItem[] unless cnt == num_toks
    end
    items
  end

  # MakeHeaderItem :: Int (Array DocInline) ?Attr -> HeaderItem
  # Create a header item given the header level, the header content
  # as an Array of DocInline, and an optional Attr object. Returns
  # the HeaderItem specified.
  MakeHeaderItem = lambda do |level, inlines, attr=nil|
    {"t"=>"Header",
     "c"=>[level,
           if attr.nil? then MakeAttr[SecureRandom.uuid] else attr end,
        inlines]}
  end

  # MakePara :: (Array DocInline) -> DocPara
  # Create a paragraph from an array of inlines
  MakePara = lambda do |inlines|
    {"t"=>"Para",
     "c"=>inlines}
  end

  # HeaderText :: DocHeader -> (Array DocInline)
  # Given a DocHeader, extract and return an array of DocInline items
  HeaderText = lambda do |h|
    h["c"][2]
  end

  # Tree (Fn Tag Content State -> Result) State -> Tree
  # Walk a tree, applying an action to every object and keeping a state.
  # Returns the modified tree.
  Walk = lambda do |x, f, state|
    if x.class == [].class
      array = []
      x.each do |item|
        if (item.class == {}.class && item.include?('t'))
          res = f.call(item['t'], item['c'], state)
          if res.nil?
            array << Walk.call(item, f, state)
          elsif res.class == [].class
            res.each {|z| array << Walk.call(z, f, state)}
          else
            array << Walk.call(res, f, state)
          end
        else
          array << Walk.call(item, f, state)
        end
      end
      state[:out] = array unless state.include? :out
      array
    elsif x.class == {}.class
      obj = {}
      x.keys.each do |k|
        obj[k] = Walk.call(x[k], f, state)
      end
      obj
    else
      x
    end
  end

  # DocIR -> Int
  # Given a DocIR document, return the minimum heading
  # level found within that document. If no headers exist, returns nil
  MinHeadingLevel = lambda do |doc|
    f = lambda do |tag, content, state|
      if tag == "Header"
        header_level = content[0]
        if state[:min_header_level].nil? or (header_level < state[:min_header_level])
          state[:min_header_level] = header_level
        end
      end
      nil
    end
    state = {min_header_level: nil}
    Walk.call(doc, f, state)
    state.fetch(:min_header_level)
  end

  # (Or DocItem DocInline DocSnippet DocIR) Int ->
  #   (Or DocItem DocInline DocSnippet DocIR)
  # Return a document with heading level adjusted by the given amount for
  # every heading level encountered
  AdjustHeading = lambda do |doc, adjustment|
    return doc if adjustment == 0
    f = lambda do |tag, content, state|
      if tag == "Header"
        header_level = content[0]
        new_header_level = header_level + adjustment
        new_content = [new_header_level] + content[1..-1]
        {"t" => "Header", "c" => new_content}
      else
        nil
      end
    end
    Walk.call(doc, f, {})
  end

  # DocIR -> DocIR
  #
  # Make it so the lowest heading number in a markdown file is 1 by moving
  # each heading's level by 1 - (min headingLevels)
  NormalizeHeadings = lambda do |doc|
    min_heading = MinHeadingLevel[doc]
    desired_min_heading = 1
    heading_adjust = desired_min_heading - min_heading
    AdjustHeading[doc, heading_adjust]
  end

  # MarkdownString (Array Int) Bool ->
  #     (Record slug: String, level: Int, doc: RubyJSON)
  #
  # Given a string of markdown, split it by the heading levels given in the
  # heading_levels array, and name the new split files according to the top
  # level heading in each file. Returns a map (record) with symbol keys: slug,
  # level, and doc where slug is the slugified name of the first heading (title
  # if missing), level is the heading level of the split document, and doc is
  # the actual document as RubyJSON datastructures
  #
  # Note: normalizes heading levels if normalize_headings set to true
  SplitMdByHeadingLevels = lambda do |md_str, heading_levels, normalize_headings=true|
    verbose = false
    doc_ir = JSON.parse(MdToJson[md_str])
    docs = SplitJsonByHeadingLevel.call(doc_ir, heading_levels)
    slug_set = Set.new
    docs.map do |x|
      h = FirstObjOfTag.call(x, "Header")
      if h.nil?
        txt = "title"
      else
        txt = DocSnippetToMd.call(MakePara.call(HeaderText.call(h)))
          .gsub(/<[^>]*>/, ' ').gsub(/[^a-zA-Z0-9]/, ' ').strip
      end
      slug = Slug::CreateUnique.call(Slug::SlugifyName.call(txt), slug_set)
      slug_set << slug
      xx = if normalize_headings then NormalizeHeadings.call(x) else x end
      new_doc = JsonToMd.call(xx)
      level = if h.nil? then 1 else GetHeaderLevel.call(h) end
      puts "#{slug} (#{level})" if verbose
      puts "... original header: #{txt}" if verbose
      puts "... doc: #{new_doc[0..80].inspect}" if verbose
      {slug: slug, level: level, doc: new_doc}
    end
  end

  # DocIR -> MarkdownString
  # Use Pandoc to convert a DocIR to MarkdownString. Converts the DocIR to
  # a JsonString and feeds that into Pandoc using the standard pandoc options
  # defined in STD_MD_OPTS
  JsonToMd = lambda do |doc_ir|
    json_str = JSON.generate(doc_ir)
    begin
      Pandoc::Run.call(['-f json'] + STD_MD_OPTS, json_str)
    rescue
      puts "Error running json_to_markdown over:\n#{json_str[0..1200]}"
      raise
    end
  end

  # ContentObject ?String -> String
  # Given the content field of a JSON representation of a document, tunnel
  # through until all content has been reduced to a string.
  FlattenToString = lambda do |content, s=nil|
    s ||= ""
    if content.class == [].class
      content.inject(s) do |new_s, c|
        FlattenToString[c, new_s]
      end
    elsif content.class == {}.class
      if content["t"] == "Space"
        s + " "
      elsif content["t"] == "Str"
        s + content["c"]
      elsif content["t"] == "SoftBreak"
        s + " "
      else
        FlattenToString[content["c"], s]
      end
    elsif content.class == "".class
      s += ""
    else
      s
    end
  end
end
