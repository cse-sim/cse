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

  MakeBinding = lambda do |ctxt|
    Namespace.new(ctxt).get_binding
  end

  # String Binding -> String
  # Render the given string template with Embedded Ruby (ERB) using the binding
  # context passed in by the second parameter.
  # See http://stackoverflow.com/a/5462069
  RenderWithErb = lambda do |template, b|
    ERB.new(template,0,'>').result(b)
  end

  # String String Int (Map String String) -> nil
  # Given a source file path, an output file path (assumed to be created up to
  # the parent directory), an integer for the processing file number (not
  # used), and a context hash to create a binding context for template
  # rendering, process the input file to the output file and save.
  PreprocFile = lambda do |fn=MakeBinding|
    lambda do |path, out_path, _, config|
      b = fn[config.fetch("context")]
      text = File.read(path)
      result = RenderWithErb[text, b]
      File.write(out_path, result)
    end
  end
end
