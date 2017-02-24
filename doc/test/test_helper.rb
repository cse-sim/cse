$LOAD_PATH.unshift File.expand_path('../../lib', __FILE__)
require 'minitest/autorun'
begin
  TC = Minitest::Test
rescue
  # for older versions of Ruby
  TC = Minitest::Unit::TestCase
end
