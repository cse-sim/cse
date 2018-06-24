require_relative 'slug'

module MD
  # String -> Bool
  # Returns true if the line is an ATX style header
  SelectHeader = lambda {|s| s =~ /^#+[[:space:]]+.*$/ }
  
  # String -> String
  # Retreive the header string from an ATX style header
  NameFromHeader = lambda do |line|
    name_match = line.match(/^#+[[:space:]]+((?:(?![[:space:]]+\{[#\.]).)*)/)
    if name_match.nil? then "" else name_match[1].strip end
  end

  # String -> String
  NameAndSlug = lambda do |line|
    name = NameFromHeader[line]
    custom_slug_match = line.match(/\{#([^\}]*)\}\s*$/)
    if custom_slug_match.nil?
      slug = Slug::Slugify[name]
    else
      slug = custom_slug_match[1]
    end
    [name, slug]
  end
end
