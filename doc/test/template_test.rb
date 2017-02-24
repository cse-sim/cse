require_relative 'test_helper'
require 'template'
require 'fileutils'

class TestTemplate < TC
  include Template
  def setup
    @temp_path_in = "junk_in.txt"
    File.write(@temp_path_in, "Hi <%= a %>")
    @temp_path_out= "junk_out.txt"
  end
  def teardown
    FileUtils.rm(@temp_path_in) if File.exist?(@temp_path_in)
    FileUtils.rm(@temp_path_out) if File.exist?(@temp_path_out)
  end
  def test_rendering_a_template
    expected = "Hi b"
    actual = RenderWithErb.("Hi <%= a %>", MakeBinding.({"a"=>"b"}))
    assert_equal(expected, actual)
  end
  def test_preproc_file
    PreprocFile[].(@temp_path_in, @temp_path_out, nil, {"context"=>{"a"=>"b"}})
    expected = "Hi b"
    actual = File.read(@temp_path_out)
    assert_equal(expected, actual)
  end
end
