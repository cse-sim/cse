# Copyright (c) 1997-2018 The CSE Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file.
require 'tmpdir'
require 'tempfile'
require 'yaml'
require 'set'
require 'irb'

require_relative 'command'

module DefParser
  # This is the starting state for the parser
  THIS_DIR = File.expand_path(File.dirname(__FILE__))
  IsWindows = !((ENV['OS'] =~ /win/i).nil?)
  LoadFromReference = lambda do |fname|
    ref_dir = File.expand_path(File.join(THIS_DIR, '..', 'config', 'reference'), THIS_DIR)
    path = File.join(ref_dir, fname)
    File.read(path)
  end
  LoadProbes = lambda {LoadFromReference["probes.txt"]}
  LoadFromSrc = lambda do |fname|
    src_dir = File.expand_path(File.join(THIS_DIR, '..', '..', 'src'), THIS_DIR)
    path = File.join(src_dir, fname)
    File.read(path)
  end
  LoadCnRecsOrig = lambda {LoadFromSrc['CNRECS.DEF']}
  LoadCnFieldsOrig = lambda {LoadFromSrc['CNFIELDS.DEF']}
  LoadCndTypesOrig = lambda {LoadFromSrc['CNDTYPES.DEF']}
  # String -> String
  # Load via the C Pre-processor
  LoadViaCpp = lambda do |fname, with_comments=false|
    output = nil
    Tempfile.open('temp_file') do |f|
      # Call the c preprocessor
      src_dir = File.expand_path(File.join(THIS_DIR, '..', '..', 'src'), THIS_DIR)
      path = File.join(src_dir, fname)
      tgt_path = f.path
      if IsWindows
        opts = []
        opts << "/I\"#{src_dir}\""
        opts << "/C" if with_comments
        opts << "/EP"
        opts << "/TP"
        opts << "\"#{path}\""
        output = Command::Run["cl", opts, nil, false]
      else
        `cpp -I#{src_dir} -xc++ #{path} #{tgt_path}`
        output = File.read(f.path)
      end
    end
    output
  end
  LoadCnRecs = lambda {LoadViaCpp['CNRECS.DEF']}
  LoadCnRecsWithComments = lambda{LoadViaCpp['CNRECS.DEF', true]}
  LoadCndTypes = lambda {LoadViaCpp['CNDTYPES.DEF']}
  MkRecord = lambda do |id, name, fields|
    {
      :id => id,
      :name => name,
      :fields => fields
    }
  end
  ReplaceMultiLineCommentsKeepSpace = lambda do |txt|
    txt.gsub(/\/\*[^*]*\*\//m) {|m| m.gsub(/[^\n]/, ' ')}
  end
  # Parser for CNFIELDS.DEF
  ParseUnits = lambda do |input|
    data = {}
    ReplaceMultiLineCommentsKeepSpace[input].lines.map(&:chomp).each do |line|
      next if line =~/^\s*$/
      next if line =~ /^\s*\/\/.*$/
      toks = line.split(/\s+/)
      comment_match = line.match(/^.*?\/\/(.*)$/)
      data[toks[0]] = {
        :typename => toks[0],
        :datatype => toks[1],
        :limits   => toks[2],
        :units    => toks[3],
      }
      if comment_match
        data[toks[0]][:description] = comment_match[1].strip
      end
    end
    data
  end
  ParseCnFields = lambda {ParseUnits[LoadCnFieldsOrig[]]}
  ParseTypes = lambda do |input|
    
  end
  ParseCnTypes = lambda {ParseTypes[LoadCndTypes[]]}
  ParseCseDashP = lambda do |input|
    records = {}
    fields = []
    name = nil
    input.lines.map(&:chomp).each do |line|
      if line.start_with?("@")
        re = Regexp.new(
          /^@(?<name>[a-zA-Z_0-9]+)/.source +
          /(?<array>\[1\.\.\])?\./.source +
          /(?<input>\s+I)?/.source +
          /(?<runtime>\s+R)?/.source +
          /(?<owner>\s+owner:\s+[a-zA-Z_0-9]+)?.*$/.source
        )
        m = line.match(re)
        if m
          # add fields from last record
          records[name][:fields] = fields if name
          fields = []
          name = m["name"]
          records[name] = {}
          records[name][:input] = ! m["input"].nil?
          records[name][:runtime] = ! m["runtime"].nil?
          if m["owner"].nil?
            records[name][:owner] = "--"
          else
            records[name][:owner] = m["owner"].strip.split(/\s+/)[1]
          end
          records[name][:array] = ! m["array"].nil?
        end
      elsif line =~ /^\s*$/ or name.nil?
        next
      else
        field = {
          :name => line[0..20].strip,
          :input => line[21..24].strip == "I", 
          :runtime => line[25..28].strip == "R",
          :type => line[29..49].strip,
          :variability => line[50..-1].strip
        }
        fields << field
      end
    end
    # add fields that may not have been added
    records[name][:fields] = fields unless records[name].include?(:fields)
    records
  end
  ParseProbesTxt = lambda {ParseCseDashP[LoadProbes[]]}
  VERBOSE = true
  CleanStr = lambda do |s|
    s.encode('UTF-8',
             :invalid => :replace,
             :undef => :replace,
             :replace => '')
  end
  # String (Array String) Int -> (Tuple (Or Nil String) Int)
  # Given a line, an array of reference lines, and an integer for current
  # position in reference lines, return a tuple of the comment description for
  # the given line and the new position, or nil and the current position if
  # nothing is found.
  FindDescription = lambda do |line, ref_lines, current_ref_idx|
    stripped_line = line.strip
    next_ref_idx = current_ref_idx
    ref_line = nil
    ref_lines.each_with_index do |rl, ref_idx|
      next if ref_idx < current_ref_idx
      rl_ = rl.gsub(/\t/, ' ').gsub(/\/\//, '  ').gsub(/\/\*/, '  ') + " "
      if rl_.include?(stripped_line + " ")
        next_ref_idx = ref_idx
        ref_line = rl
        break
      end
    end
    if ref_line
      puts("matching:\n\t#{line.strip[0..40]}\n with\n\t#{ref_line.strip[0..40]}\n") if VERBOSE
      m = ref_line.match(/^.*?\/\/(.*)$/)
      if m
        # match // ... lines
        description = CleanStr[m[1].strip]
        puts("found field!: #{description[0..40]}") if VERBOSE
        [description, next_ref_idx]
      else
        # match /* ... lines
        n = ref_line.match(/^.*?\/\*(.*)$/)
        if n
          description = CleanStr[n[1].strip.gsub(/\*\//, '')]
          puts("found field!: #{description[0..40]}") if VERBOSE
          [description, next_ref_idx]
        else
          [nil, current_ref_idx]
        end
      end
    else
      [nil, current_ref_idx]
    end
  end
  Parse = lambda do |input, reference=nil|
    in_record = false
    record_id = nil
    record_name = nil
    record_fields = []
    output = {}
    variability = Set.new([
      "e" ,# field varies at end of interval.
      "f" ,# value available after input, before setup; before re-setup after autosize.
      "r" ,# runly (start of run) variability, including things set by input check/setup
      "y" ,# runly (end of run) variability, including things set by input check/setup
      "s" ,# subhour
      "h" ,# hour
      "mh",# month-hour
      "d" ,# day
      "m" ,# month
      "z" ,# constant
      "i" ,# after input, before checking/setup
    ])
    ref_lines = reference.lines.map(&:chomp) if reference
    current_ref_idx = 0
    input.lines.map(&:chomp).each_with_index do |line, idx|
      if line =~ /^\s*RECORD/
        m = line.match(/^\s*RECORD\s+([a-zA-Z_0-9-]*)\s*\"([^"]*)\"\s+\*(RAT|STRUCT).*$/)
        if m && (m[3] == "RAT")
          in_record = true
          record_id = m[1]
          record_name = m[2]
        end
      elsif line =~ /^\s*\*declare.*$/
        next
      elsif line =~ /^\s*$/
        next
      elsif line =~ /^\s*\*.*$/ && !(line =~ /^\s*\*END.*$/)
        # take the last two items
        spec, name = line.split(/\s+/)[-2..-1]
        next if spec.nil? || name.nil? || spec.include?('*') || name.include?('*')
        field = {
          :spec => spec,
          :name => name.gsub(/;/,'')
        }
        if field[:name] = "nHrHeat" and record_name == "zone interval results sub"
          binding.irb
        end
        if reference
          description, current_ref_idx = FindDescription[
            line, ref_lines, current_ref_idx]
          field[:description] = description if description
        end
        toks = line.gsub(/#{spec}\s+#{name}\s*$/, '')
          .strip
          .split(/\*/)
          .map(&:strip)
          .reject(&:empty?).map {|x| x.split(/\s+/)}
        hide = false
        toks.each do |tok|
          t = tok[0].downcase
          if t == "hide"
            hide = true
          elsif variability.include?(t)
            if field.include?(:variability)
              field[:variability] += [t]
            else
              field[:variability] = [t]
            end
          elsif t == "array"
            field[:array] = true
            begin
              field[:size] = tok[1].to_i
            rescue => e
              puts("!!! Attempt to read array size as an integer on line #{idx} failed;\n#{e.message}")
              puts("!!! tok=#{tok.inspect}")
              puts("!!! Storing and moving on...")
              field[:size] = tok[1]
            end
          elsif t == "noname"
            hide = true
          elsif t == "nest"
            hide = true
          else
            puts("!!! Ignoring #{t}; tok: #{tok}\n!!! line[#{idx}]: #{line}")
          end
        end
        record_fields << field unless hide
      elsif (line =~ /^\s*[a-zA-Z_0-9]+\s+[a-zA-Z_0-9]+\s*$/) && in_record
        # we have a two token field
        spec, name = line.strip.split(/\s+/)
        field = {
          :spec => spec,
          :name => name,
          :variability => ["constant"]
        }
        next if spec.nil? || name.nil? || spec.include?('*') || name.include?('*')
        record_fields << field
      elsif (line =~ /^\s*\*END/) && in_record
        in_record = false
        if output.include?(record_id)
          puts("Warning! Duplicate id found: #{record_id}")
          output[record_id.downcase][:fields] += record_fields
        else
          output[record_id.downcase] = MkRecord[record_id, record_name, record_fields]
        end
        record_id = nil
        record_name = nil
        record_fields = []
      end
    end
    output
  end
  ParseCnRecs = lambda {Parse[LoadCnRecs[], LoadCnRecsWithComments[]]}
  # String -> Nil
  # Given the path to an output file, write the output file in YAML format.
  ParseCnRecsToYaml = lambda {|p| File.write(p, ParseCnRecs[].to_yaml)}
end
