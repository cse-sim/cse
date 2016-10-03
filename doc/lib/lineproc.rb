
module LineProc
  # String Init Reducer -> Init
  ReduceLines = lambda do |str, init, reducer|
    val = init
    str.split("\n").each {|line|
      val = reducer.call(val, line)
    }
    val
  end

  # (Vec String) Init Reducer -> Init
  ReduceOverFiles = lambda do |files, init, reducer|
    val = init
    files.each do |f|
      val = val.merge(Hash[:file_url,f])
      open(f, 'r') do |fin|
        str = fin.read()
        val = ReduceLines[str, val, reducer]
      end
    end
    val
  end
end
