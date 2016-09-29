module Slug
  Slugify = lambda do |s|
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
end
