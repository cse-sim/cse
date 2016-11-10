
module LineProc
  # String Init Reducer -> Init
  ReduceLines = lambda do |str, init, reducer|
    val = init
    str.split("\n").each {|line|
      val = reducer.call(val, line)
    }
    val
  end

  # (Vec String) Init Reducer (Array (Tuple Regex String)) -> Init
  ReduceOverFiles = lambda do |files, init, reducer, gsubs=nil|
    val = init
    files.each do |f|
      val = val.merge(Hash[:file_url,f])
      open(f, 'r') do |fin|
        str = fin.read()
        if gsubs
          gsubs.each do |regex, sub|
            str = str.gsub(regex, sub)
          end
        end
        val = ReduceLines[str, val, reducer]
      end
    end
    val
  end
end
