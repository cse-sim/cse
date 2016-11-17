module CoverageCheck
  # String -> (Or Nil String)
  # Given a line of markdown, recognize an ATX header and return the header
  # itself or return nil
  MdHeader = lambda do |line|
    m = line.match(/^#+\s+(.*)$/)
    if m
      m[1].strip
    else
      nil
    end
  end

  # String -> (Or Nil String)
  # Given a line of markdown, recognize a data item header and return it, or
  # return nil
  DataItemHeader = lambda do |line|
    m = line.match(/^\*\*(.*)\*\*$/)
    if m
      m[1].split(/=/)[0].gsub(/\*/,'')
    else
      nil
    end
  end

  # String -> (Map String (Set String))
  # Given the content of a markdown document of a record and its input, return
  # a RecordInputSet for the records and input fields found in that document
  ParseRecordDocument = lambda do |content|
    output = {}
    current_header = nil
    content.lines.map(&:chomp).each do |line|
      h = MdHeader[line]
      if h
        current_header = h
        output[current_header] = Set.new unless output.include?(h)
      else
        input_hdr = DataItemHeader[line]
        if input_hdr
          output[current_header] << input_hdr
        end
      end
    end
    output
  end

  # String -> (Map String (Set String))
  # Given a file path to a markdown document containing record documentation,
  # return a RecordInputSet for the records and input fields found in that
  # document.
  ReadRecordDocument = lambda do |path|
    ParseRecordDocument[File.read(path)]
  end

  # (Array String) -> (Map String (Set String))
  # Given an array of file paths to record documentation, consecutively
  # construct a RecordInputSet from all of the documents listed. Note: it is an
  # error for a document to define the same Record Name, Record Input Field
  # combination.
  ReadAllRecordDocuments = lambda do |paths|
    output = {}
    paths.each do |path|
      new_output = ReadRecordDocument[path]
      new_output.keys.each do |k|
        if output.include?(k)
          raise "Duplicate keys found: #{k} at #{path}"
        end
      end
      output.merge!(new_output)
    end
    output
  end

  # String -> (Map String (Set String))
  # Given the string content of the results of `cse -c > cullist.txt`,
  # parse that into a RecordInputSet
  # Examples:
  #   content = <<DOC
  #
  #   CSE 0.816 for Win32 console
  #   Command line: -c
  #
  #   Top
  #      doAutoSize
  #      doMainSim
  #
  #
  #   material    Parent: Top
  #      matThk
  #      matCond
  #
  #   DOC
  #   ParseCulList[content]
  #     =>
  #     {
  #       "Top" => Set.new(["doAutoSize", "doMainSim"]),
  #       "material" => Set.new(["matThk", "matCond"])
  #     }
  ParseCulList = lambda do |content|
    output = {}
    current_record = nil
    content.lines.map(&:chomp).each_with_index do |line, idx|
      next unless idx > 3
      record = line.match(/^(\S+).*$/)
      if record
        current_record = record[1].strip
        output[current_record] = Set.new unless output.include?(current_record)
      elsif !current_record.nil?
        field = line.match(/^   (\S+).*$/)
        if field
          output[current_record] << field[1].strip
        end
      end
    end
    output
  end

  # String -> (Map String (Set String))
  # Given the path to a cullist.txt file (i.e., the result of `cse -c >
  # cullist.txt`), read the file and parse it into a RecordInputSet
  # Examples:
  #   # In file "a.txt"
  #
  #   CSE 0.816 for Win32 console
  #   Command line: -c
  #
  #   Top
  #      doAutoSize
  #      doMainSim
  #
  #
  #   material    Parent: Top
  #      matThk
  #      matCond
  #
  #   # End file "a.txt"   
  #   path = "a.txt"
  #   ReadCulList[path]
  #     =>
  #     {
  #       "Top" => Set.new(["doAutoSize", "doMainSim"]),
  #       "material" => Set.new(["matThk", "matCond"])
  #     }
  ReadCulList = lambda do |path|
    {}
  end

  # (Set String) (Set String) ?Bool ->
  #   (Or Nil
  #       (Record :in_1st_not_2nd (Or Nil (Set String))
  #               :in_2nd_not_1st (Or Nil (Set String))))
  # Given two sets and an optional flag (defaults to false) that, if true,
  # compares on case, if false, doesn't; check for differences between the
  # contents of set 1 and the contents of set 2, reporting items in the first
  # set (but not in the second) and items in the second set (but not in the
  # first)
  # Examples:
  #   s1 = Set.new(["A", "B", "C"])
  #   s2 = Set.new(["B", "C", "D"])
  #   SetDifferences[s1, s2, false]
  #     =>
  #     {
  #       :in_1st_not_2nd => Set.new(["A"]),
  #       :in_2nd_not_1st => Set.new(["D"])
  #     }
  #   s1 = Set.new(["1","2","3"])
  #   s2 = Set.new(["1","2","3"])
  #   SetDifferences[s1, s2, false]
  #     =>
  #     nil
  SetDifferences = lambda do |s1, s2, case_matters=false|
    nil
  end

  # (Map String (Set String)) (Map String (Set String)) ?Bool ->
  # (Or Nil
  #     (Record :records_in_1st_not_2nd (Or Nil (Set String))
  #             :records_in_2nd_not_1st (Or Nil (Set String))
  #             :field_set_differences
  #             (Or Nil
  #                 (Map String
  #                      (Record :in_1st_not_2nd (Or Nil (Set String))
  #                              :in_2nd_not_1st (Or Nil (Set String)))))))
  # Given two RecordInputSet objects and an optional flag (defaulting
  # to false) which, if true, compares based on case, compare the two
  # RecordInputSet objects, returning any differences. If no differences,
  # return nil.
  # Examples:
  #   RecordInputSetDifferences[ris1, ris2, false]
  #     =>
  #     
  RecordInputSetDifferences = lambda do |ris1, ris2, case_matters=false|
    nil
  end

  # (Map String *) (Map String String) -> (Map String *)
  # Given a map from string to any and a map from string to string, create a
  # new map with any keys in the first map also in the second map renamed to
  # the second map's keys
  # Examples:
  #   RenameMap[m, names]
  #     =>
  #
  RenameMap = lambda do |m, names|
    m
  end
end
