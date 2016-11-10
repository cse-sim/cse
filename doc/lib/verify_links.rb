require 'net/http'

module VerifyLinks
  # Problem: links don't always work in generated documentation due to a large
  # number of reasons. We would like to be able to confirm all links are valid
  # within a generated HTML document.

  # String -> Bool
  # See http://stackoverflow.com/a/18582395
  UrlGood = lambda do |url_string|
    begin
      url = URI.parse(url_string)
      req = Net::HTTP.new(url.host, url.port)
      req.use_ssl = (url.scheme == 'https')
      path = url.path unless url.path == ""
      res = req.request_head(path || '/')
      if res.kind_of?(Net::HTTPRedirection)
        # Go after any redirect and make sure you can access the redirected URL
        url_exist?(res['location'])
      else
        res.code[0] != "4" #false if http code starts with 4 - error on your side.
      end
    rescue Errno::ENOENT
      false #false if can't find the server
    end
  end

  # String -> (Array String)
  GatherLinks = lambda do |text|
    text.scan(/href\s*=\s*["']([^"']*)["']/).flatten
  end

  # (Array String) String String -> (Array String)
  LinkIssues = lambda do |links, path, root, display=true|
    dir = if File.directory?(path) then path else File.dirname(path) end
    problems = []
    links.each do |link|
      good = true
      if link.start_with?("http")
        # calling urls with Ruby via SSL not robust on windows...
        # skipping the check below for now...
        if false # !UrlGood[link]
          problems << link
          good = false
        end
      else
        file_path, id = link.split(/#/)
        full_path = if file_path.start_with?("/") then File.join(root, file_path) else File.join(dir, file_path) end
        exist = File.exist?(full_path)
        if !exist
          problems << "#{link} doesn't exist"
          good = false
        end
        if id and exist and File.file?(full_path)
          txt = File.read(full_path)
          if !(txt =~ /id\s*=\s*["']#{id}["']/)
            problems << "\"##{id}\" not found in file #{full_path}"
            good = false
          end
        end
      end
      if display
        STDOUT.write(good ? '.' : 'x')
        STDOUT.flush
      end
    end
    problems
  end

  # String -> (Array String)
  # Given the path to an html directory, check all links in all files and
  # return an array of all problems
  CheckLinks = lambda do |dir|
    problems = []
    num_paths = 0
    num_links = 0
    Dir[File.join(dir, '**', '*.html')].each do |path|
      links = GatherLinks[File.read(path)]
      problems += LinkIssues[links, path, dir].map {|lnk| "Source: #{path}; Issue: #{lnk}"}
      num_paths += 1
      num_links += links.length
    end
    puts("\nChecked #{num_paths} files for a total of #{num_links} links")
    puts("#{problems.length} link issues encountered")
    problems
  end
end
