# This is a utility script used to split the monolithic CSE User Manual into
# smaller files.
require_relative 'pandoc'
require_relative 'slug'
require 'json'

THIS_DIR = File.expand_path(File.dirname(__FILE__))
src = File.expand_path(
  File.join('..', 'src', 'cse-user-manual.md'),
  THIS_DIR
)
md = File.read(src)
json = Pandoc::MdToJson[md]
doc = JSON.parse(json)
sections = Pandoc::SplitJsonByHeadingLevel[doc, [1]]
section_number = 1
sections.each do |section|
  normalized_section = Pandoc::NormalizeHeadings[section]
  snippet = Pandoc::DocSnippetToMd[
    Pandoc::FirstObjOfTag[normalized_section, "Header"]
  ]
  name = Slug::Slugify[snippet.match(/^#+\s+(.*)$/)[1]]
  path = File.expand_path(File.join('..', 'src', "#{name}.md"), THIS_DIR)
  File.write(path, Pandoc::JsonToMd[normalized_section])
end
