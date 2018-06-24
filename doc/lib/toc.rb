# Generate Table of Contents
require 'yaml'
require 'set'
require_relative 'md'
require_relative 'slug'
require_relative 'lineproc'

module TOC
  # TocData String -> TocData
  # where TocData = (spec/keys :req [:file_url :contents])
  # (spec/def :contents (spec/cat :level :name :link))
  # (spec/def :file_url string?)
  # (spec/def :level integer?) 
  # (spec/def :name string?)
  # (spec/def :link string?)
  TocReducer = lambda do |toc, line|
    if MD::SelectHeader[line]
      level = line.match(/^#+/).to_s.length
      name, slug = MD::NameAndSlug[line]
      final_slug = slug
      idx = 1
      while toc[:slug_set].include?(final_slug)
        final_slug = "#{slug}-#{idx}"
        idx += 1
      end
      toc[:slug_set] << final_slug
      src_basename = File.basename(toc[:file_url])
      ext = File.extname(src_basename)
      file_url = src_basename.gsub(ext, '.html')
      link = file_url + "##{final_slug}"
      new_contents = toc[:contents] + [[level, name, link]]
      if name.strip == ''
        # we have a 'blank' header... don't bother recording it
        toc
      else
        toc.merge({:contents => new_contents})
      end
    else
      toc
    end
  end

  # Integer (Array (Tuple Integer FilePath)) -> String
  # Generate the table of contents file from an integer specifying the maximum
  # level of depth and an array of tuples of file level and file path. Returns
  # a string of the table of contents markdown file.
  GenTableOfContentsFromFiles = lambda do |max_level, files|
    r = TocReducer
    init = {:contents=>[], :slug_set=>Set.new}
    toc = LineProc::ReduceOverFiles[
      files.map {|f| f[1]},
      init,
      r,
      [[/<!--(.*?)-->/m, '']]
    ]
    out = ""
    last_level0 = 1
    idx0 = 0
    last_fname0 = toc[:contents][0][2].gsub(/\#.*$/,'')
    toc[:contents].inject([last_level0, idx0, last_fname0]) do |s, c|
      last_level = s[0]
      # remove {#foo} at the end of headers
      name = c[1].gsub(/{[^}]*}\Z/, '').strip
      url = c[2]
      fname = url.gsub(/\#.*$/, '')
      last_fname = s[2]
      idx = if fname==last_fname then s[1] else s[1]+1 end
      file_level = files[idx][0]
      # level is the minimum of the actual header level or one more than
      # the last level -- prevents jumps in indentation of more than one
      # unit
      level = [c[0] + file_level, last_level + 1].min
      if level <= max_level
        indent = (' ' * ((level - 1) * 4)) + '- '
        out += indent + "[#{name}](#{url})\n"
      end
      [level, idx, fname]
    end
    out
  end

  # Integer FilePath FilePath -> Nil
  # Generate a Table of Contents (TOC) for a Markdown Document
  # - *max_level*: Integer >= 1, the maximum header depth to use in TOC
  # - *manifest_path*: String, path to the manifest file in YAML
  # - *toc_path*: String, path to the table of contents file to write out in markdown
  GenTableOfContents = lambda do |max_level, manifest_path, toc_path|
    man_dir = File.dirname(manifest_path)
    data = YAML.load_file(manifest_path)
    fs = data.fetch('sections', [])
    puts "Warning!!! No files in manifest at #{manifest_path}" if fs.empty?
    # make level 0-based to ease algorithm below;
    # expand file path to full path from the manifest directory
    files = fs.map {|level, path| [level - 1, File.expand_path(path, man_dir)]}
    File.write(toc_path, GenTableOfContentsFromFiles[max_level, files])
  end
end

if false
  THIS_DIR = File.expand_path(File.dirname(__FILE__))
  MAN_PATH = File.expand_path(File.join('..','src','cse-user-manual.yaml'), THIS_DIR)
  TOC_PATH = File.join(THIS_DIR, 'junk-toc.md')
  TOC::GenTableOfContents[3, MAN_PATH, TOC_PATH]
end
