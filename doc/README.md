# CSE User Manual Build Scripts

The files in this folder are concerned with building the documentation and website for CSE; specifically, the CSE User Manual. The build process heavily leverages the [Ruby] programming language, the [Pandoc] documentation processing tool, and, optionally, [Node.js] for doing compression of html/css. The majority of the build process is contained in the `rakefile.rb` located in this directory with supporting code in the `lib` subdirectory. The source code for the documentation is written in [Pandoc-flavored Markdown] with the manifest of files located in [YAML] files, both in the `src` directory. Additional support files are located in the `config` directory. The `test` directory is test code specifically for the documentation build system -- it can be run using `rake test` from the command line.

The source files including both manifest files (in YAML) and markdown files (ending with a `.md` extension) are preprocessed by "embedded Ruby" or [ERB] prior to running through the rest of the documentation build pipeline.

[Pandoc-flavored Markdown]: http://pandoc.org/MANUAL.html#pandocs-markdown
[YAML]: http://yaml.org/
[ERB]: http://ruby-doc.org/stdlib-2.3.1/libdoc/erb/rdoc/ERB.html

## Dependencies for Website Generation

- the [Pandoc] markup language converter (we are using Pandoc 1.17.2, but any recent version should work)
- (optional) for PDF output, you will need to install a LaTeX system (see below)
- (optional) for compression of HTML/CSS, a recent version of [Node.js]
- a recent version of the [Ruby] programming language (we are using Ruby 2.0.0p648)
- (optional) an internet connection (for installing node dependencies)
- the git version control manager (we are using 2.10.2)

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

**Note**: due to limitations with Ruby, your context variables need to start with either a lowercase letter or an underscore ("_"). A great way to test that ERB is doing the right thing for you is to use the `rake erb` task:

    set FILE=path\to\file.md & rake erb

or, if you happen to be on a Unix, GNU/Linux, or Mac OS computer:

    rake erb FILE=path/to/file.md

This will preprocess the file using [ERB] and write the results to `doc/erb-out.txt` for your perusal using the context variables you've defined in your local `doc/config.yaml` (which are merged with the context variables defined in `doc/config/defaults.yaml` -- your local context variables override the defaults).

We recommend looking at an [ERB Tutorial] to get up to speed on using [ERB].

[ERB Tutorial]: http://www.stuartellis.name/articles/erb/

## Updating the Documents: Support Files

The `doc/config/` directory contains various support files used to help build the documents. The `defaults.yaml` holds default configuration parameters that are discussed in the [configuration section](#the-configuration-file). Specifically, the css used for html styling is in the `css` directory; the templates used for both HTML and LaTeX (a precursor for a pdf build) are located in the `template` directory; and finally we have a `reference` directory which holds the following files:

- `probes.txt` is a file dump from the `cse -p` command
- `probes_input.yaml` is a yaml file of the above processed to include comments
- `record-index-exceptions.yaml` allows one to override settings when a record's "canonical location" in the documentation does **not** follow the "typical convention". The "typical convention" is that a record will have its own file located in the `doc/src/records` folder with a file name corresponding to the record's name (all in lowercase). Within that file, there will be a header with the same name as the record and this section header would be unique in all of the documentation sections. In other words, we would normally expect to find a record named "SOMETHING" in file `doc/src/records/something.md` where, opening that file, we would expect a header of `# SOMETHING` (Markdown). If that is **not** the case, then this file is where you would explicitly say where to locate the Record's "canonical location" in the documentation.

The format of the `record-index-exceptions.yaml` file is:

    AUTOGENERATED RECORD NAME: { "NAME TO REPLACE IT WITH": "filename.html#tag-name" }

In the above, filename.html should be named with an `.html` extension (even though the "source" is `.md`) and we still expect to find it in the `doc/src/records` folder.

## Updating the Documents: Adding a new Section or Record

When adding a new section or record you generally just open up your text editor and write the new file and place it under `doc/src`. However, the following list a few "gotchas" to look out for:

- be sure to save the file extension as `.md` -- the build tooling assumes markdown files are named with this extension
- write your file as if it were at the "top level". That is, even if you know the document will be nested several levels deep, make your top-most header a first-level header.
- be sure to enter the file's name and *actual* level in the document in the appropriate manifest.yaml file -- otherwise it won't get included in the documentation when you build!
- be sure to add your file to the version control sytem
- you don't need to explicitly link *any* RECORD names. If you uppercase record names per the document convention, the build process will automatically detect the name as a record name and cross-link the name to the "canonical location" where the record is defined
- all non-RECORD cross-links including section links **do** need to be added explicitly. The target for a section can be formed using [some straightforward rules] and would be written as `[link text](#identifier)`. Note: only use the `#identifier` syntax -- you don't need to specify the filename; even when the cross-link is in another physical file. The build process detects and fills in the filename depending on the type of build (pdf, single-page html, multi-page html, etc.)

[some straightforward rules]: http://pandoc.org/MANUAL.html#extension-auto_identifiers

## Pushing Built Documents to the Server

Documents are built by default into `doc/build/output`. This directory is a check-out of the remote repository's `gh-pages` branch which uses GitHub's [GitHub Pages] functionality. The remote repository URL can be configured in the `config.yaml` file under the key `remote-repo-url`.

[GitHub Pages]: https://pages.github.com/

Once you have built the documents and are satisfied with how they look, you can push to the remote repository's `gh-pages` branch for the github site you're interested in (by default, `origin` is set to the value in `config.yaml` under the key `remote-repo-url`).
