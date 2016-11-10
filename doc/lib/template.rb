require 'ostruct'
require 'erb'

module Template
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
  RenderWithErb = lambda do |template, vars|
    ERB.new(template,0,'>').result(Namespace.new(vars).get_binding)
  end

  # String String Int (Map String *) -> nil
  # Given a source file path, an output file path (assumed to be created up to
  # the parent directory), an integer for the processing file number (not
  # used), and a map containing a key called 'context' which contains the
  # rendering context (i.e., a map from string to value to use in the
  # template), process the input file to the output file and save.
  PreprocFile = lambda do |path, out_path, _, config|
    text = File.read(path)
    result = Template::RenderWithErb[text, config.fetch('context')]
    File.write(out_path, result)
  end
end
