# CSE User Manual Build Scripts

The files in this folder are concerned with building the documentation and website for CSE; specifically, the CSE User Manual. The build process heavily leverages the [Ruby] programming language, the [Pandoc] documentation processing tool, and, optionally, [Node.js] for doing compression of html/css. The majority of the build process is contained in the `rakefile.rb` located in this directory with supporting code in the `lib` subdirectory. The source code for the documentation is written in [Pandoc-flavored Markdown] with the manifest of files located in [YAML] files, both in the `src` directory. Additional support files are located in the `config` directory.

The source files including both manifest files (in yaml) and markdown files (ending with a `.md` extension) are preprocessed by "embedded Ruby" or [ERB] prior to running through the rest of the documentation build pipeline.

[Markdown]: http://pandoc.org/MANUAL.html#pandocs-markdown
[YAML]: http://yaml.org/
[ERB]: http://ruby-doc.org/stdlib-2.3.1/libdoc/erb/rdoc/ERB.html

## Dependencies for Website Generation

- the [Pandoc] markup language converter (we are using Pandoc 1.17.2, but any recent version should work)
- (optional) for PDF output, you will need to install a LaTeX system (see below)
- (optional) for compression of HTML/CSS, a recent version of [Node.js]
- a recent version of the [Ruby] programming language (we are using Ruby 2.0.0p648)
- (optional) an internet connection (for installing node dependencies)

For PDF generation, Pandoc creates LaTeX which is further processed to PDF. For Windows, [MiKTeX] is recommended. For Mac OS and Linux, please follow the instructions for recommended (La)TeX systems at [Pandoc's Install] page. Note that to build the PDF, you must change the value of the "build-pdf?" key to true in the `config.yaml` file in this directory. This configuration file is discussed more below.

[Pandoc]: http://pandoc.org/
[Ruby]: https://www.ruby-lang.org/en/
[Node.js]: https://nodejs.org/en/
[MiKTeX]: https://miktex.org/
[Pandoc's Install]: http://pandoc.org/installing.html 

With the above installed, the scripts in this directory should be able to download and install all other dependencies.

The specific dependencies for [Node.js] are listed in the `package.json` file.

## Instructions for Building the Documentation

Using [Node.js] enables us to compress the HTML and CSS for the website. It is an optional dependency but recommended if you plan to push to the live `gh-pages` branch. The [Node.js] dependencies are off by default: in order to use [Node.js], create a local `config.yaml` file in `doc/config.yaml` with the value of `use-node?:` as `true`:

    use-node?: true

Your primary interaction with the build system will be through the [Rake] build tool (from the [Ruby] programming folks). [Rake] is a "make for Ruby".

[Rake]: https://github.com/ruby/rake

[Rake] comes installed with most [Ruby] installers. To see the build commands available, (after [Ruby] has been installed), type:

    > rake -T

from this directory to see a list of commands.

The default command is `build` which can be invoked either by:

    > rake build

or, since it is the default command:

    > rake

## The Configuration File

Configuration parameters can be used to change how the documentation builds. The config defaults are located in `doc/config/defaults.yaml`. If a file named `doc/config.yaml` is placed in the `doc` directory, it will override the corresponding value in the defaults.yaml file. By default, the `doc/config.yaml` file is ignored (see `doc/.gitignore`) allowing developers to customize the build process for their needs. The `doc/config/defaults.yaml` file is written in [YAML] format and is heavily documented with comments to help clarify the role of each parameter.

A typical `doc/config.yaml` file for a developer planning to build the documentation for [GitHub Pages] would be as follows:

    use-node?: true
    build-pdf?: true
    context:
      build_all: true

Note in particular that the "context" attribute contains a dictionary (i.e., map or hash table) of values to define and pass into the embedded ruby ([ERB]) preprocessor. 


[YAML]: http://yaml.org/

## Preprocessing files with Embedded Ruby

Embedded Ruby or [ERB] is a technology originally developed for making web pages server-side "on the fly". Instead of a program consisting primarily of code with surrounding comments, [ERB] flips things around to where a file is string content with programming constructs embedded inside of it. This can be used for simple calculations and also for conditional inclusion of code:

    <% if dev_build %>
    # Experimental Stuff

    lorem ipsum dolar ...
    <% end %>

    # Stable stuff

    blah blah

In the above code, assuming that the variable `dev_build` was defined, this snippet of [ERB] would include the "Experimental Stuff" section depending on whether the value was `true` (yes, include) or `false` (skip). You can define variable flags such as `dev_build` by adding a "context" section to your local `config.yaml` file. For example, a local user's `config.yaml` file for the above snippet might look as follows:

    build-pdf?: true
    context:
      dev_build: true

**Note**: due to limitations with Ruby, your context variables need to start with either a lowercase letter or an underscore ("_").

We recommend looking at an [ERB Tutorial] to get up to speed on using [ERB].

[ERB Tutorial]: http://www.stuartellis.name/articles/erb/

## Pushing Built Documents to the Server

Documents are built by default into `doc/build/output`. This directory is a check-out of the remote repository's `gh-pages` branch which uses GitHub's [GitHub Pages] functionality. The remote repository URL can be configured in the `config.yaml` file under the key `remote-repo-url`.

[GitHub Pages]: https://pages.github.com/

Once you have built the documents and are satisfied with how they look, you can push to the remote repository's `gh-pages` branch for the github site you're interested in (by default, `origin` is set to the value in `config.yaml` under the key `remote-repo-url`).
