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
  ReadCulList = lambda do |path|
    ParseCulList[File.read(path)]
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
  SetDifferences = lambda do |s1, s2, case_matters=false|
    if s1 == s2
      nil
    elsif case_matters
      {
        :in_1st_not_2nd => s1 - s2,
        :in_2nd_not_1st => s2 - s1
      }
    else
      f = lambda {|m, item| m.merge({item.downcase => item})}
      m1 = s1.inject({}, &f)
      m2 = s2.inject({}, &f)
      s1_ = Set.new(m1.keys)
      s2_ = Set.new(m2.keys)
      if s1_ == s2_
        nil
      else
        {
          :in_1st_not_2nd => Set.new((s1_ - s2_).map {|k| m1[k]}),
          :in_2nd_not_1st => Set.new((s2_ - s1_).map {|k| m2[k]})
        }
      end
    end
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
  RecordInputSetDifferences = lambda do |ris1, ris2, case_matters=false|
    if ris1 == ris2
      nil
    else
      g = lambda {|x,y|{records_in_1st_not_2nd: x, records_in_2nd_not_1st: y}}
      ks1 = Set.new(ris1.keys)
      ks2 = Set.new(ris2.keys)
      key_diffs = SetDifferences[ks1, ks2, case_matters]
      out = nil
      if key_diffs
        out = g[key_diffs[:in_1st_not_2nd], key_diffs[:in_2nd_not_1st]]
      end
      # check fields
      f = lambda {|m, item| m.merge({item.downcase => item})}
      m1 = ks1.inject({}, &f)
      m2 = ks2.inject({}, &f)
      ks = case_matters ? (ks1 & ks2) : (Set.new(m1.keys) & Set.new(m2.keys))
      ks.each do |k|
        k1 = case_matters ? k : m1[k]
        k2 = case_matters ? k : m2[k]
        diffs = SetDifferences[ris1[k1], ris2[k2], case_matters]
        if diffs
          if out
            out[k] = diffs
          else
            out = g[nil, nil]
            out[k] = diffs
          end
        end
      end
      out
    end
  end
end
