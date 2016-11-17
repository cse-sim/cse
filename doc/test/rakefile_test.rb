require_relative 'test_helper'
require_relative '../rakefile.rb'

class RakefileTest < Minitest::Unit::TestCase
  def test_UpdateOutlineCount
    assert_equal(UpdateOutlineCount[[0], 1], [1])
    assert_equal(UpdateOutlineCount[[1,1,3], 2], [1,2])
    assert_equal(UpdateOutlineCount[[1,1,3], 3], [1,1,4])
    assert_equal(UpdateOutlineCount[[1,1,3], 1], [2])
    assert_equal(UpdateOutlineCount[[1,1,3], 4], [1,1,3,1])
    assert_equal(UpdateOutlineCount[[1,1,3], 5], [1,1,3,1,1])
  end
  def test_OutlineCountToStr
    assert_equal(OutlineCountToStr[[1,1,3]], "1.1.3")
  end
end
