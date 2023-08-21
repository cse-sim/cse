# Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file.
require_relative('command')

module CSE
  # Create an Alias
  C = Command

  cse_path = ENV.has_key?('CSE_EXE_PATH') ? ENV['CSE_EXE_PATH'] : '..\\builds\\cse'

  # (Or (Array String) String) ?String -> String
  # Given an array of strings of options to cse and an optional input to
  Run = lambda do |opts, input=nil|
    C::Run[cse_path, opts, input]
  end

  CullList = lambda do
    Run[["-c", "> #{File.join('config','reference','cullist.txt')}"]]
  end

  ProbesList = lambda do
    Run[["-p", "> #{File.join('config','reference','probes.txt')}"]]
  end

end
