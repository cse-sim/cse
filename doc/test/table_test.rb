require_relative 'test_helper'
require 'table'

class TableTest < Minitest::Unit::TestCase
  include Table
  def test_context
    t1 = Main.new(:template=>"Hi <%= m %> and <%= n %>", :context=>{"m"=>"a", "n"=>"b"})
    assert_equal(t1.m, "a")
    assert_equal(t1.n, "b")
    assert(t1.inspect != "c")
    t2 = Main.new(:template=>"Hi <%= m %> and <%= n %>", :context=>{"m"=>"a", "n"=>"b", "inspect"=>"c"})
    assert_equal(t2.m, "a")
    assert_equal(t2.n, "b")
    assert_equal(t2.inspect, "c")
  end
  def test_render
    expected = "Hi a and b"
    actual = Main.new(:template=>"Hi <%= m %> and <%= n %>", :context=>{"m"=>"a", "n"=>"b"}).render
    assert_equal(actual, expected)
    expected = "Hi a"
    actual = Main.new(:template=>"Hi <%= inspect %>", :context=>{"inspect"=>"a"}).render
    assert_equal(actual, expected)
  end
end
