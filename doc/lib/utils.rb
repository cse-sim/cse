require 'fileutils'

module Utils
  # String -> nil
  # Ensures the given path (a directory) exists
  EnsureExists = lambda do |path|
    FileUtils.mkdir_p(path) unless File.exist?(path)
  end

  # (Array String) -> nil
  # Ensures that all of the given paths exist.
  EnsureAllExist = lambda do |paths|
    paths.each {|p| EnsureExists[p]}
  end

  CopyIfStale = lambda do |from_path, to_path|
    if !FileUtils.uptodate?(to_path, [from_path])
      FileUtils.cp(from_path, to_path)
    end
  end
end
