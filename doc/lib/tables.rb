# Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file.

module Tables
  INTERCOL_SPACE = 2
  PAGE_MAX_COLS = 80

  TxtToTable = lambda do |txt|
    table = []
    txt.split(/\n/).each do |row|
      next if row == ""
      row = row.strip
      cols = row.split(/\|/)
      cols.map! {|c| c.strip}
      table << cols
    end
    table
  end

  MinColumnWidths = lambda do |table|
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

  MaxColumnWidths = lambda do |table|
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

  ColWidths = lambda do |mins, maxs|
    num_cols = mins.length
    spacing = 2 * (num_cols - 1)
    extra_space = PAGE_MAX_COLS - (mins.inject(0, &:+) + spacing)
    extra_space = 0 if extra_space < 0
    total_max = maxs.inject(0, &:+).to_f
    width_fracs = maxs.map {|m| m / total_max}
    extra_space_alloc = width_fracs.map {|w| (w * extra_space).to_i}
    widths = mins.zip(extra_space_alloc).map {|m,e| m+e}
    widths
  end

  WriteHeader = lambda do |widths|
    cols = []
    widths.each do |w|
      cols << ('-' * w)
    end
    cols.join('  ')
  end

  WriteFooter = lambda do |widths|
    num_cols = widths.length
    spacing = 2 * (num_cols - 1)
    width = widths.inject(0, &:+)
    '-' * (width + spacing)
  end

  # String Number -> (Array String)
  ChunkCol = lambda do |col, width|
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
  WriteRow = lambda do |cols, widths|
    col_chunks = []
    num_lines = 0
    cols.each_with_index do |col, idx|
      the_chunk = ChunkCol[col, widths[idx]]
      col_chunks << the_chunk
      num_lines = [the_chunk.length, num_lines].max
    end
    lines = [""] * num_lines
    num_cols = widths.length
    num_lines.times do |line_idx|
      #puts "line index: #{line_idx}"
      col_chunks.each_with_index do |chunks, col_idx|
        #puts "column index: #{col_idx}"
        cm = if col_idx == (num_cols - 1) then 0 else 1 end
        if chunks.length > line_idx
          makeup_space = [widths[col_idx] - chunks[line_idx].length, 0].max
          lines[line_idx] += chunks[line_idx] + " " * makeup_space + " " * (INTERCOL_SPACE * cm)
        else
          lines[line_idx] += " " * widths[col_idx] + " " * (INTERCOL_SPACE * cm)
        end
      end
    end
    lines.join("\n")
  end

  WriteContent = lambda do |table, widths|
    content = []
    table.each do |row|
      content << WriteRow[row, widths]
    end
    content.join("\n\n")
  end

  WriteTable = lambda do |table, header=false|
    mins = MinColumnWidths[table]
    maxs = MaxColumnWidths[table]
    widths = ColWidths[mins, maxs]
    parts =  []
    if header
      parts << WriteFooter[widths]
      parts << WriteContent[[table[0]], widths]
      parts << WriteHeader[widths]
      parts << WriteContent[table[1..-1], widths]
      parts << WriteFooter[widths]
    else
      parts << WriteHeader[widths]
      parts << WriteContent[table, widths]
      parts << WriteFooter[widths]
    end
    parts.join("\n")
  end

  Main = lambda do |txt|
    table = TxtToTable[txt]
    WriteTable[table]
  end

  #example = <<END
  #AMBIENT | Exterior surface is exposed to the "weather" as read from the weather file. Solar gain is calculated using solar geometry, sfAzm, sfTilt, and sfExAbs.
  #
  #SPECIFIEDT | Exterior surface is exposed to solar radiation as in AMBIENT, but the dry bulb temperature is calculated with a user specified function (sfExT). sfExAbs can be set to 0 to eliminate solar effects.
  #
  #ADJZN | Exterior surface is exposed to another zone, whose name is specified by sfAdjZn. Solar gain is 0 unless gain is targeted to the surface with SGDIST below.
  #
  #ADIABATIC | Exterior surface heat flow is 0. Thermal storage effects of delayed surfaces are modeled.
  #END

  #puts(main(example.strip))
end
