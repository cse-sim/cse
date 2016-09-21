# Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file.
require 'tmpdir'
require 'tempfile'
require 'yaml'

module DefParser
  # This is the starting state for the parser
  THIS_DIR = File.expand_path(File.dirname(__FILE__))
  LoadCnRecsOrig = lambda do
    src_dir = File.expand_path(File.join(THIS_DIR, '..', '..', 'src'), THIS_DIR)
    cnrecs_path = File.join(src_dir, 'CNRECS.DEF')
    File.read(cnrecs_path)
  end
  LoadCnRecs = lambda do
    output = nil
    Tempfile.open('CNRECS.DEF') do |f|
      # Call the c preprocessor
      src_dir = File.expand_path(File.join(THIS_DIR, '..', '..', 'src'), THIS_DIR)
      cnrecs_path = File.join(src_dir, 'CNRECS.DEF')
      tgt_path = f.path
      `cpp -I#{src_dir} -xc++ #{cnrecs_path} #{tgt_path}`
      output = File.read(f.path)
    end
    output
  end
  MkRecord = lambda do |id, name, fields|
    {
      :id => id,
      :name => name,
      :fields => fields
    }
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
        if reference
          m = ref_lines[idx].match(/^.*?\/\/(.*)$/)
          field[:description] = m[1].strip if m
        else
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
            puts("!!! Ignoring #{t}")
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
          output[record_id][:fields] += record_fields
        else
          output[record_id] = MkRecord[record_id, record_name, record_fields]
        end
        record_id = nil
        record_name = nil
        record_fields = []
      end
    end
    output
  end
  ParseCnRecs = lambda {Parse[LoadCnRecs[], LoadCnRecsOrig[]]}
  # String -> Nil
  # Given the path to an output file, write the output file in YAML format.
  ParseCnRecsToYaml = lambda {|p| File.write(p, ParseCnRecs[].to_yaml)}
end
