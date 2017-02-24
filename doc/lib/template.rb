require 'ostruct'
require 'erb'

module Template
  # An example class that can be used to return a binding context
  class Namespace
    def initialize(hash)
      hash.each do |key, value|
        singleton_class.send(:define_method, key) { value }
      end
    end
    def get_binding
      binding
    end
  end

  # String (Map String *) -> String
  # Render the given string template with Embedded Ruby (ERB) using the context
  # given by the second parameter which is a binding map.
  # See http://stackoverflow.com/a/5462069
  RenderWithErb = lambda do |template, binding|
    ERB.new(template,0,'>').result(binding)
  end

  # String String Int Binding -> nil
  # Given a source file path, an output file path (assumed to be created up to
  # the parent directory), an integer for the processing file number (not
  # used), and a class that has method get_binding which returns the binding
  # context for template rendering, process the input file to the output file
  # and save.
  PreprocFile = lambda do |path, out_path, _, binding|
    text = File.read(path)
    result = RenderWithErb[text, binding]
    File.write(out_path, result)
  end
end
