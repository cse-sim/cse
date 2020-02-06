require_relative 'test_helper'
require 'table'
require 'template'

class TableTest < TC
  include Table
  def test_context
    t1 = Main.new(:context=>{"m"=>"a", "n"=>"b"})
    assert_equal(t1.m, "a")
    assert_equal(t1.n, "b")
    assert(t1.inspect != "c")
    t2 = Main.new(:context=>{"m"=>"a", "n"=>"b", "inspect"=>"c"})
    assert_equal(t2.m, "a")
    assert_equal(t2.n, "b")
    assert_equal(t2.inspect, "c")
  end
  def test_context_is_only_on_instance
    t1 = Main.new(:context=>{"m"=>"a"})
    t2 = Main.new(:context=>{"n"=>"a"})
    assert(t1.respond_to?(:m))
    assert(!t1.respond_to?(:n))
    assert(!t2.respond_to?(:m))
    assert(t2.respond_to?(:n))
  end
  def test_render
    b1 = MakeBinding.().({"m"=>"a", "n"=>"b"})
    expected = "Hi a and b"
    actual = Template::RenderWithErb.("Hi <%= m %> and <%= n %>", b1)
    assert_equal(actual, expected)
    b2 = MakeBinding.().({"inspect"=>"a"})
    expected = "Hi a"
    actual = Template::RenderWithErb.("Hi <%= inspect %>", b2)
    assert_equal(actual, expected)
  end
end
