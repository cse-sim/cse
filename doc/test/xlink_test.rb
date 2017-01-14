require_relative 'test_helper'
require 'xlink'
require 'set'

class XLinkTest < Minitest::Unit::TestCase
  include XLink
  def test_OverString
    rec_idx = {"STUFF"=>"index.html#stuff","JUNK"=>"index.html#junk"}
    sec_path = []
    s = <<END
# STUFF

There is a lot of STUFF available in the world, including a lot of JUNK.

# JUNK

JUNK is a kind of STUFF.
END
    expected_str = <<END
# STUFF

There is a lot of STUFF available in the world, including a lot of [JUNK.](#junk)

# JUNK

JUNK is a kind of [STUFF.](#stuff)
END
    file_basename = "index.html"
    actual = OverString[s, file_basename, 1, 1, rec_idx, sec_path]
    expected = [expected_str, ["index.html#junk"]]
    assert_equal(expected[1], actual[1])
    assert_equal(expected[0], actual[0])
  end

  def test_InSectionPath
    sec_path = ["a","b","c"]
    expected = true
    actual = InSectionPath[sec_path, "c", 2]
    assert_equal(expected, actual)
    expected = false
    actual = InSectionPath[sec_path, "d", 20]
    assert_equal(expected, actual)
    expected = false
    actual = InSectionPath[sec_path, "a", 2]
    assert_equal(expected, actual)
    expected = true
    actual = InSectionPath[sec_path, "a", 3]
    assert_equal(expected, actual)
    expected = true
    actual = InSectionPath[sec_path, "a", 4]
    assert_equal(expected, actual)
    expected = false
    actual = InSectionPath[sec_path, "c", 0]
    assert_equal(expected, actual)
  end

  def test_UpdateSectionPath
    id = "a.html#hello"
    sec_path = []
    lvl = 1
    expected = [id]
    actual = UpdateSectionPath[sec_path, id, lvl]
    assert_equal(expected, actual)
    expected = [id, "a.html#and-goodbye"]
    actual = UpdateSectionPath[["a.html#hello","a.html#there","a.html#you"], "a.html#and-goodbye", 2]
    assert_equal(expected, actual)
  end
end
