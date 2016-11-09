# This is a utility script used to split the monolithic CSE User Manual into
# smaller files.
require_relative 'pandoc'
require_relative 'slug'
require 'json'

THIS_DIR = File.expand_path(File.dirname(__FILE__))

def split_file_by(fname, levels=nil)
  levels ||= [1]
  src = File.expand_path(
    File.join('..', 'src', fname),
    THIS_DIR
  )
  md = File.read(src, :encoding=>"UTF-8")
  json = Pandoc::MdToJson[md]
  doc = JSON.parse(json)
  sections = Pandoc::SplitJsonByHeadingLevel[doc, levels]
  sections.each do |section|
    normalized_section = Pandoc::NormalizeHeadings[section]
    snippet = Pandoc::DocSnippetToMd[
      Pandoc::FirstObjOfTag[normalized_section, "Header"]
    ]
    name = Slug::Slugify[snippet.match(/^#+\s+(.*)$/)[1]]
    path = File.expand_path(File.join('..', 'src', "#{name}.md"), THIS_DIR)
    if File.exist?(path)
      puts("Warning! #{path} would be overwritten! Skipping...")
    else
      File.write(path, Pandoc::JsonToMd[normalized_section])
    end
  end
end

# This file can be used as a script to split markdown files. However, this
# scripting functionality is not enabled by default.
if false
  puts("Ready...")
  split_file_by('cse-user-manual.md', [1])
  split_file_by('input-data.md', [1, 2])
  split_file_by('top-members.md', [1, 2])
  split_file_by('construction.md', [1, 2])
  split_file_by('zone.md', [1, 2])
  split_file_by('heatplant.md', [1, 2])
  split_file_by('coolplant.md', [1, 2])
  puts("Done!")
end
