require 'set'

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
  # return nil.
  DataItemHeader = lambda do |line|
    m = line.match(/^\*\*(.*)\*\*\\?\s*$/)
    if m
      header = m[1].split(/=/)[0].gsub(/\*/,'').strip
      if Ignore[header]
        nil
      else
        header
      end
    else
      nil
    end
  end
  IgnoredHeaders = Set.new(["Related Probes"].map(&:downcase))
  # String -> Bool
  # Return true if the given header should be ignored
  Ignore = lambda do |header|
    hdc = header.downcase
    IgnoredHeaders.each do |ih|
      return true if hdc.include?(ih) or ih.include?(hdc)
    end
    false
  end
  # String -> (Map String (Set String))
  # Given the content of a markdown document of a record and its input, return
  # a RecordInputSet for the records and input fields found in that document
  ParseRecordDocument = lambda do |content|
    output = {}
    current_header = nil
    content.lines.map(&:chomp).each do |line|
      h = MdHeader[line]
      if h and not Ignore[h]
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
  # Given two sets and an optional flag (defaults to false) that, if true,
  # compares on case, if false, doesn't; check for differences between the
  # contents of set 1 and the contents of set 2, reporting items in the first
  # set (but not in the second) and items in the second set (but not in the
  # first)
  # ARGUMENTS
  # s1: (Set String), the first set
  # s2: (Set String), the second set
  # case_matters: ?Bool, if true compare with case, else case-insensitive; default: false
  # keys_to_ignore: ?(Or (Set String) nil): a set of keys to ignore; default nil 
  # RETURNS
  #   (Or Nil
  #       (Record :in_1st_not_2nd (Or Nil (Set String))
  #               :in_2nd_not_1st (Or Nil (Set String))))
  SetDifferences = lambda do |s1, s2, case_matters=false, keys_to_ignore=nil|
    keys_to_ignore ||= Set.new
    if s1 == s2
      nil
    elsif case_matters
      s1_ = s1 - keys_to_ignore
      s2_ = s2 - keys_to_ignore
      if s1_ == s2_
        nil
      else
        {
          :in_1st_not_2nd => s1_ - s2_,
          :in_2nd_not_1st => s2_ - s1_,
        }
      end
    else
      keys_to_ignore_dc = Set.new(keys_to_ignore.map {|k|k.downcase})
      f = lambda {|m, item| m.merge({item.downcase => item})}
      m1 = s1.inject({}, &f)
      m2 = s2.inject({}, &f)
      s1_ = Set.new(m1.keys) - keys_to_ignore_dc
      s2_ = Set.new(m2.keys) - keys_to_ignore_dc
      if s1_ == s2_
        nil
      else
        {
          :in_1st_not_2nd => Set.new((s1_ - s2_ - keys_to_ignore_dc).map {|k| m1[k]}),
          :in_2nd_not_1st => Set.new((s2_ - s1_ - keys_to_ignore_dc).map {|k| m2[k]}),
        }
      end
    end
  end
  # (Map String (Set String)) -> (Map String (Set String))
  # Given a map from string to set of string, adjust the map so that fields
  # with spacing in them get collapsed to the record defined by the first word
  # through set union 
  AdjustMap = lambda do |m|
    n = {}
    m.each do |k, v|
      new_k = (k =~ /\s+/) ? k.split(/\s+/)[0] : k
      if n.include?(new_k)
        n[new_k] += v
      else
        n[new_k] = v
      end
    end
    n
  end
  # (Map String (Set String)) -> (Map String (Set String))
  # Adjust a record input set to drop all fields that end with "Name"
  DropNameFields = lambda do |ris|
    n = {}
    ris.each do |k, vs|
      n[k] = vs.reject {|v| v.end_with?("Name")}.to_set
    end
    n
  end
  # RecordInputSet -> RecordInputSet
  # Downcase a record input set
  DowncaseRecordInputSet = lambda do |ris|
    n = {}
    ris.each do |k, vs|
      n[k.downcase] = vs.map(&:downcase).to_set
    end
    n
  end
  # (Map String (Set String)) (Map String (Set String)) ?Bool ->
  #   (Map String (Set String))
  # Given a record input set and a reference record input set and an optional
  # boolean flag (defaulting to false) that, if true, compares case sensitive,
  # drop all fields that end with "Name" UNLESS they also appear in the
  # reference. Note: compares
  DropNameFieldsIfNotInRef = lambda do |ris, ref, case_sensitive=false|
    ref = case_sensitive ? ref : DowncaseRecordInputSet[ref]
    name = case_sensitive ? "Name" : "name"
    n = {}
    ris.each do |k, vs|
      kk = case_sensitive ? k : k.downcase
      n[k] = vs.reject {|v|
        w = case_sensitive ? v : v.downcase
        w.end_with?(name) && !(ref.include?(kk) && ref[kk].include?(w))
      }.to_set
    end
    n
  end
  # Given two RecordInputSet objects, an optional flag (defaulting to false)
  # which, if true, compares based on case, and optional records to ignore and
  # record fields to ignore, compare the two RecordInputSet objects, returning
  # any differences. If no differences, return nil.
  # ARGUMENTS:
  # - ris1: (Map String (Set String))
  # - ris2: (Map String (Set String))
  # - case_matters: ?Bool
  # - records_to_ignore: ?(Set String)
  # - record_fields_to_ignore: ?(Map String (Set String))
  # RETURN:
  # (Or Nil
  #     (Record :records_in_1st_not_2nd (Or Nil (Set String))
  #             :records_in_2nd_not_1st (Or Nil (Set String))
  #             String
  #             (Record :in_1st_not_2nd (Or Nil (Set String))
  #                     :in_2nd_not_1st (Or Nil (Set String)))))
  RecordInputSetDifferences = lambda do |ris1, ris2, case_matters=false, records_to_ignore=nil, record_fields_to_ignore=nil|
    records_to_ignore ||= Set.new
    record_fields_to_ignore ||= {}
    if !case_matters
      # if case doesn't matter, downcase all keys
      ris1 = ris1.keys.inject({}) {|m, k| m.merge({k.downcase => Set.new(ris1[k].map {|ris1k|ris1k.downcase})})}
      ris2 = ris2.keys.inject({}) {|m, k| m.merge({k.downcase => Set.new(ris2[k].map {|ris2k|ris2k.downcase})})}
      records_to_ignore = Set.new(records_to_ignore.map {|k| k.downcase})
      record_fields_to_ignore = record_fields_to_ignore.keys.inject({}) {|m, k| m.merge({k.downcase => record_fields_to_ignore[k]})}
    end
    if ris1 == ris2
      nil
    else
      g = lambda {|x,y|{records_in_1st_not_2nd: x, records_in_2nd_not_1st: y}}
      ks1 = Set.new(ris1.keys)
      ks2 = Set.new(ris2.keys)
      key_diffs = SetDifferences[ks1, ks2, case_matters, records_to_ignore]
      out = nil
      if key_diffs
        out = g[key_diffs[:in_1st_not_2nd], key_diffs[:in_2nd_not_1st]]
      end
      # check fields
      fsd = :field_set_differences
      ks = ks1 & ks2
      ks.each do |k|
        # ignore based on lookup that is both case sensitive and insensitive
        next if records_to_ignore.include?(k)
        field_ignores = record_fields_to_ignore.fetch(k, Set.new)
        diffs = SetDifferences[
          ris1.fetch(k, Set.new), ris2.fetch(k, Set.new),
          case_matters, field_ignores]
        if diffs
          if out
            out[fsd] = {} unless out.include?(fsd)
            out[fsd][k] = diffs
          else
            out = g[nil, nil]
            out[fsd] = {} unless out.include?(fsd)
            out[fsd][k] = diffs
          end
        end
      end
      out
    end
  end
  # (Map * *) String * * String String -> String
  # Given a map,  a string for a name of what the map represents (e.g.,
  # Records, Data Input Fields, etc.), two keys into the map, and two strings
  # to name the variations represented by the two keys, report off on the
  # differences as a string.
  DiffsToString = lambda do |diffs, name, k1, k2, a, b, indent=0|
    if diffs.nil?
      ""
    else
      ind = " "*indent
      s = ""
      s += "#{name}:\n" if !name.nil? and !diffs.nil? and (diffs[k1] or diffs[k2])
      [[k1,a,b], [k2,b,a]].each do |k, x, y|
        if diffs[k]
          s += "#{ind}in #{x} but not in #{y}:\n" unless diffs[k].nil? or diffs[k].empty?
          diffs[k].sort.each do |r|
            s += "#{ind}- #{r}\n"
          end
        else
          s += ""
        end
      end
      s
    end
  end
  # (Or Nil
  #     (Record :records_in_1st_not_2nd (Or Nil (Set String))
  #             :records_in_2nd_not_1st (Or Nil (Set String))
  #             :field_set_differences
  #             (Or Nil
  #                 (Map String
  #                      (Record :in_1st_not_2nd (Or Nil (Set String))
  #                              :in_2nd_not_1st (Or Nil (Set String)))))))
  # -> String
  # Given the output of RecordInputSetDifferences, format it and retun as a string
  RecordInputSetDifferencesToString = lambda do |diffs, a=nil, b=nil|
    a ||= "First"
    b ||= "Second"
    if diffs.nil?
      "No changes detected between #{a} and #{b}"
    else
      s = ""
      if diffs[:records_in_1st_not_2nd] or diffs[:records_in_1st_not_2nd]
        s += "RECORD INCONSISTENCIES:\n"
        s += DiffsToString[
          diffs,
          nil,
          :records_in_1st_not_2nd,
          :records_in_2nd_not_1st,
          a,
          b,
          4
        ]
      end
      if diffs[:field_set_differences]
        ks = diffs[:field_set_differences].keys.sort
        if ks and !ks.empty?
          s += "DATA FIELD INCONSISTENCIES:\n"
          ks.each do |k|
            s += DiffsToString[
              diffs[:field_set_differences][k],
              k,
              :in_1st_not_2nd,
              :in_2nd_not_1st,
              a,
              b,
              4
            ]
          end
        end
      end
      s
    end
  end
end
