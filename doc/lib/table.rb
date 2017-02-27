# Problem we're trying to solve:
# - Markdown ascii tables are hard to write and maintain
# - Existing tables do not (always) generate correctly in PDF
require 'erb'
require 'set'
require 'csv'

module Table
  CheckKeys = lambda do |expected_keys, args|
    unexpected_keys = Set.new(args.keys) - expected_keys
    if not unexpected_keys.empty?
      raise "Unexpected keys: #{unexpected_keys.to_a}"
    end
  end
  # (Map String String) -> Binding
  MakeBinding = lambda do |base_path=nil|
    lambda do |ctxt|
      Main.new(:context => ctxt, :base_path => base_path).get_binding
    end
  end
  # The Main class's responsibility is to serve as the binding context for
  # an ERB template rendering. As such, it must expose the methods that
  # correspond to the table language and, additionally, it must be able
  # to define methods corresponding to <root>/doc/config.yaml context variables. To
  # allow for the possibility of different "back end" targets, there are stubs
  # for HTML, LaTeX, and Markdown backends. For now, we just transform our
  # table language to Markdown and let the markdown toolchain transform to HTML
  # and/or LaTeX.
  class Main
    def initialize(args=nil)
      @_verbose = false 
      args = args || {}
      expected_keys = Set.new(
        [:writer, :base_path, :allow_shadowing, :context])
      CheckKeys.(expected_keys, args)
      @writer = args.fetch(:writer, MdWriter.new)
      @base_path = args.fetch(:base_path, nil)
      # if true, context can overwrite some or all method names
      @allow_shadowing = args.fetch(:allow_shadowing, true)
      local_methods = methods
      args.fetch(:context, {}).each do |k, v|
        sym = k.to_sym
        if local_methods.include?(sym)
          msg = "Key \"#{k}\" will be shadowed by a local method"
          if @allow_shadowing
            puts "WARNING! #{msg}" if @_verbose
          else
            raise ArgumentError, msg
          end
        end
        define_singleton_method(sym) { v }
      end
    end
    def get_binding
      binding
    end
    def csv_table(csv_str, opts=nil)
      opts = opts || {}
      expected_keys = Set.new([:row_header, :col_header])
      CheckKeys.(expected_keys, opts)
      cells = CSV.new(csv_str).read
      @writer.array_of_array_to_table(cells, opts)
    end
    def csv_table_from_file(csv_path, opts=nil)
      path = if @base_path then File.join(@base_path, csv_path) else csv_path end
      csv_str = File.read(path)
      csv_table(csv_str, opts)
    end
    def member_table(args)
      expected_keys = Set.new([:units,:legal_range,:default,:required,:variability])
      CheckKeys.(expected_keys, args)
      @writer.member_table(args)
    end
  end

  class HtmlWriter
    def array_of_array_to_table(cells, opts=nil)
      opts = opts || {}
      row_header = opts.fetch(:row_header, true)
      col_header = opts.fetch(:col_header, false)
      table_str = "<table>\n"
      cells.each_with_index do |line, row_idx|
        if row_header and row_idx == 0
          table_str += "<thead>\n"
        elsif row_header and row_idx == 1
          table_str += "<tbody>\n"
        end
        attr_str = if row_header and row_idx == 0 then ' class="header"' else '' end
        table_str += "<tr#{attr_str}>\n"
        line.each_with_index do |col, col_idx|
          if row_header and row_idx == 0
            table_str += "<th>\n<strong>#{col}\n</strong>\n</th>\n"
          elsif col_header and col_idx == 0
            table_str += "<th>\n<strong>#{col}\n</strong>\n</th>\n"
          else
            table_str += "<td>#{col}\n</td>\n"
          end
        end
        table_str += "</tr>\n"
        if row_header and row_idx == 0
          table_str += "</thead>\n"
        end
      end
      if row_header
        table_str += "</tbody>\n"
      end
      table_str += "</table>\n"
      table_str
    end
    def member_table(args)
      nil_mark = "&mdash;"
      units= args.fetch(:units, nil_mark)
      legal_range = args.fetch(:legal_range, nil_mark)
      default = args.fetch(:default, nil_mark)
      required= args.fetch(:required, "No")
      variability=args.fetch(:variability, "constant")
      "<table class=\"member-table\">\n" +
        "<thead>\n" +
        "<tr class=\"header\">\n" +
        ["Units","Legal Range","Default","Required","Variability"].map do |c|
          "<th>\n<strong>#{c}\n</strong>\n</th>\n"
        end.join("") +
        "</tr>\n" +
        "</thead>\n" +
        "<tbody>\n" +
        "<tr>\n" + 
        [units, legal_range, default, required, variability].map do |c|
          "<td>#{c}\n</td>\n"
        end.join("") +
        "</tr>\n" +
        "</tbody>\n" +
        "</table>\n"
    end
  end

  class LatexWriter
    def array_of_array_to_table
    end
    def member_table
    end
  end

  class MdWriter
    def initialize(args=nil)
      args = args || {}
      @intercol_space = args.fetch(:intercol_space, 1)
    end
    def min_column_widths(table)
      min_widths = [2] * table[0].length
      table.each do |row|
        row.each_with_index do |cols, idx|
          words = cols.split
          longest_word = words.map {|w| w.length}.max
          longest_word = 0 if longest_word.nil?
          min_widths[idx] = [longest_word, min_widths.fetch(idx, 2)].max
        end
      end
      min_widths
    end
    def max_column_widths(table)
      max_widths = [0] * table[0].length
      table.each do |row|
        row.each_with_index do |col, idx|
          if not col.match(/^-+$/)
            max_widths[idx] = [col.length, max_widths.fetch(idx, 0)].max
          end
        end
      end
      max_widths
    end
    def col_widths(mins, maxs)
      num_cols = mins.length
      spacing = 2 * (num_cols - 1)
      extra_space = 80 - (mins.inject(0, &:+) + spacing)
      total_max = maxs.inject(0, &:+).to_f
      width_fracs = maxs.map {|m| m / total_max}
      extra_space_alloc = width_fracs.map {|w| (w * extra_space).to_i}
      widths = mins.zip(extra_space_alloc).map {|m,e| m+e}
      widths
    end
    def write_header(widths)
      cols = []
      widths.each do |w|
        cols << ('-' * w)
      end
      cols.join(' '*@intercol_space)
    end
    def write_footer(widths)
      num_cols = widths.length
      spacing = 2 * (num_cols - 1)
      width = widths.inject(0, &:+)
      '-' * (width + spacing)
    end
    # String Number -> (Array String)
    def chunk_col(col, width)
      words = col.split
      first_word = true
      len = 0
      chunks = []
      chunk = []
      words.each do |w|
        if first_word
          first_word = false
          len += w.length
        else
          len += w.length + 1
        end
        if len > width
          chunks << chunk
          len = w.length
          chunk = [w]
        else
          chunk << w
        end
      end
      chunks << chunk unless chunk.empty?
      chunks.map {|c| c.join(" ")}
    end
    # (Array String) (Array Number) -> String
    def write_row(cols, widths)
      col_chunks = []
      num_lines = 0
      cols.each_with_index do |col, idx|
        the_chunk = chunk_col(col, widths[idx])
        col_chunks << the_chunk
        num_lines = [the_chunk.length, num_lines].max
      end
      lines = [""] * num_lines
      num_cols = widths.length
      num_lines.times do |line_idx|
        #puts "line index: #{line_idx}"
        col_chunks.each_with_index do |chunks, col_idx|
          #puts "column index: #{col_idx}"
          cm = if col_idx == (num_cols - 1)
                 0
               else
                 1
               end
          if chunks.length > line_idx
            makeup_space = [widths[col_idx] - chunks[line_idx].length, 0].max
            lines[line_idx] += chunks[line_idx] + " " * makeup_space + " " * (@intercol_space * cm)
          else
            lines[line_idx] += " " * widths[col_idx] + " " * (@intercol_space * cm)
          end
        end
      end
      lines.join("\n")
    end
    def write_content(table, widths)
      content = []
      table.each do |row|
        content << write_row(row, widths)
      end
      content.join("\n\n")
    end
    def write_table(table)
      mins = min_column_widths(table)
      maxs = max_column_widths(table)
      widths = col_widths(mins, maxs)
      hdr = write_header(widths)
      [
        hdr,
        write_content(table, widths),
        hdr
      ].join("\n")
    end
    def write_border_line(widths)
      "-"*(widths.inject(0,&:+) + (widths.length - 1)*@intercol_space)
    end
    def write_table_with_header(table)
      mins = min_column_widths(table)
      maxs = max_column_widths(table)
      widths = col_widths(mins, maxs)
      bl = write_border_line(widths)
      [
        bl,
        write_content([table[0]], widths),
        write_header(widths),
        write_content(table[1..-1], widths),
        bl,
      ].join("\n")
    end
    def array_of_array_to_table(cells, opts=nil)
      opts = opts || {}
      row_header = opts.fetch(:row_header, true)
      if row_header
        write_table_with_header(cells)
      else
        write_table(cells)
      end
    end
    def member_table(args)
      nil_mark = "--"
      units= args.fetch(:units, nil_mark)
      legal_range = args.fetch(:legal_range, nil_mark)
      default = args.fetch(:default, nil_mark)
      required= args.fetch(:required, "No")
      variability=args.fetch(:variability, "constant")
      headers = ["**Units**","**Legal** **Range**","**Default**","**Required**","**Variability**"]
      content = [units, legal_range, default, required, variability]
      table = [headers, content]
      write_table_with_header(table)
    end
  end
end
