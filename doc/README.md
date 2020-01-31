# CSE User Manual Build Scripts

The files in this folder are concerned with building the documentation and website for CSE; specifically, the CSE User Manual. The build process heavily leverages the [Ruby] programming language, the [Pandoc] documentation processing tool, and, [Node.js] for doing compression of html/css. The majority of the build process is contained in the `rakefile.rb` located in this directory with supporting code in the `lib` subdirectory. The "source code" for the documentation is written in [Pandoc-flavored Markdown] with the manifest of files located in [YAML] files, both in the `src` directory. Additional support files are located in the `config` directory. The `test` directory is test code specifically for the documentation build system -- it can be run using `rake test` from the command line.

The source files including both manifest files (in YAML) and markdown files (ending with a `.md` extension) are preprocessed by "embedded Ruby" or [ERB] prior to running through the rest of the documentation build pipeline.

[Pandoc-flavored Markdown]: http://pandoc.org/MANUAL.html#pandocs-markdown
[YAML]: http://yaml.org/
[ERB]: http://ruby-doc.org/stdlib-2.3.1/libdoc/erb/rdoc/ERB.html

## Dependencies for Website Generation

- the [Pandoc] markup language converter (use Pandoc version 1.17.2; other versions are not supported)
- (optional) for PDF output, you will need to install a LaTeX system (see below)
- (optional) for compression of HTML/CSS, a recent version of [Node.js]
- a recent version of the [Ruby] programming language (use any Ruby NOT at end-of-life, currently Ruby 2.4+)
- (optional) an internet connection (for installing node dependencies)
- the git version control manager (we are using 2.10.2)
- Microsoft C++ Build Tools (for Visual Studio 2017 or 2019)

Pandoc version 1.17.2 can be downloaded from [here](https://github.com/jgm/pandoc/releases). If you do not have version 1.17.2 installed, the build tool will stop immediately with instructions on how to install Pandoc 1.17.2.

One way to make it easier to set up Pandoc 1.17.2 (especially in the presence of potentially a newer Pandoc version), is to use the cross-platform [conda] package, dependency, and environment manager. To do this, install the [miniconda] tool and then create an environment called `cse-docs`:

    # this only needs to be done once:
    conda create --name cse-docs
    # on   windows: activate cse-docs
    # on mac/linux: source activate cse-docs
    conda install -c edurand pandoc=1.17.2

This will install a managed [Pandoc] 1.17.2 on your computer. You can activate the [conda] environment and, therefore, the [Pandoc] version that comes with it by calling:

    # this needs to be done each time you want to activate Pandoc
    source activate cse-docs

[conda]: https://conda.io/docs/
[miniconda]: https://conda.io/miniconda.html

For PDF generation, Pandoc creates LaTeX which is further processed to PDF. For Windows, [MiKTeX] is recommended. For Mac OS and Linux, please follow the instructions for recommended (La)TeX systems at [Pandoc's Install] page. Note that to build the PDF, you must change the value of the "build-pdf?" key to true in the `config.yaml` file in this directory. This configuration file is discussed more below.

[Pandoc]: http://pandoc.org/
[Ruby]: https://www.ruby-lang.org/en/
[Node.js]: https://nodejs.org/en/
[MiKTeX]: https://miktex.org/
[Pandoc's Install]: http://pandoc.org/installing.html

With the above installed, the scripts in this directory should be able to download and install all other dependencies.

The specific dependencies for [Node.js] are listed in the `package.json` file.

With regard to the Microsoft Build Tools, the C++ preprocessor is required in the full build process to parse parameter names from the CSE source code.
This is accomplished with the `cl` program.
If you type `where cl` at your prompt, then `cl` is installed and reachable.
If not, the best way to get this is to set up your Visual Studio with C++ tools (you should have this if you are able to build CSE).
Then, to get a shell with `cl` and other tools on the path, go to `Start > Visual Studio 20XX > Developer Command Prompt for VS XX`.
You should have access to `cl` from that prompt.
If not, please check this [cl] article from Microsoft.

[cl]: https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=vs-2019

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

Configuration parameters can be used to change how the documentation builds. The config defaults are located in `doc/config/defaults.yaml`. If a file named `doc/config.yaml` is placed in the `doc` directory, it will override the corresponding value in the defaults.yaml file. By default, the `doc/config.yaml` file is ignored by the version control system (see `doc/.gitignore`) allowing developers to customize the build process for their needs. The `doc/config/defaults.yaml` file is written in [YAML] format and is heavily documented with comments to help clarify the role of each parameter.

A typical `doc/config.yaml` file for a developer planning to build the documentation for [GitHub Pages] would be as follows:

    use-node?: true
    build-pdf?: true
    context:
      build_all: true

Note in particular that the "context" attribute contains a dictionary (i.e., map or hash table) of values to define and pass into the embedded ruby ([ERB]) preprocessor. These attributes must be lower-case as discussed below.

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

## Inserting File Content

A special directive is available that allows one to insert any file directly into markdown documents (to avoid repetition) by specifying the file's relative path from the repository root. For example, to add the file: `<cse-repo>\doc\src\enduses.md`, one would write:

    <%= insert_file('doc/src/enduses.md') %>

Note: the `insert_file` method could be replaced by a direct call to `File.read(path_to_read)`. However, direct calls to `File.read` are fragile since the absolute paths won't build between multiple machines and the question with relative paths is what directory will be used. In contrast, the `insert_file` method always uses a path relative to the repository root.

## Table Pre-processing Directives

By setting the config variable "use-table-lang?" to `true` (note: it is `true` by default), we enable [ERB] to use a table pre-processing language. At the time of this writing, these table directives are used to transform imbedded CSV data (CSV = comma-separated value), in-file CSV data, or special tables to the "ascii-art" multi-line tables required by [Pandoc-flavored Markdown]. For general tables, external CSV files can be used (recommended). In this case, the source files may be edited with a spreadsheet program and saved to disk (in CSV format). By default, tables are stored in the directory relative to the `doc` directory as specified by the "table-path" variable specified in the `doc/config/defaults.yaml` file. At the time of this writing, that variable defaults to `doc/src/tables`.

The three table directives and their options and syntax are shown below:

### member\_table -- special syntax for Record Member Tables

The `member_table` has the following syntax:

    <%= member_table(
      units: "1/^o^F",
      legal_range: "no restrictions",
      default: "-0.0026",
      required: "No",
      variability: "constant") %>

This writes a Markdown table similar to the following:

    ---------------------------------------------------------------
    **Units** **Legal**    **Default** **Required** **Variability**
              **Range**                                            
    --------- ------------ ----------- ------------ ---------------
    1/^o^F    no           -0.0026     No           constant       
              restrictions                                         
    ---------------------------------------------------------------

The arguments to `member_table` are a [Ruby] hash (also called a hash-table, dictionary, or map). An alternate valid [Ruby] syntax for the same thing is:

    <%= member_table(
      :units => "1/^o^F",
      :legal_range => "no restrictions",
      :default => "-0.0026",
      :required => "No",
      :variability => "constant") %>

or alternately to be very explicit (note the curly brackets `{` and `}` below):

    <%= member_table({
      :units => "1/^o^F",
      :legal_range => "no restrictions",
      :default => "-0.0026",
      :required => "No",
      :variability => "constant"}) %>

In the call to `member_table`, each of the keys is checked for correct spelling. Keys that are elided are replaced with `--`, the markdown symbol for "--".

### csv\_table -- inline csv tables

CSV tables provide an alternate syntax for describing a multi-line table. Cell contents should be written in [Pandoc-flavored Markdown] to indicate boldness, links, etc.

A simple CSV Table appears below:

    <%= csv_table(<<END, :row_header => false)
    A,B,C
    1,2,3
    END
    %>

Here, we use [Ruby] syntax for ["here docs"](http://blog.jayfields.com/2006/12/ruby-multiline-strings-here-doc-or.html). Essentially, the CSV string gets escaped and placed where `<<END` appears in the call. Note that the end marker (which can be any identifier, but `END` is recommended) must be flush-left. The above code yields the following markdown table:

    ---------------------- ---------------------- ----------------------
    A                      B                      C                     

    1                      2                      3                     
    ---------------------- ---------------------- ----------------------

Note the option for having a `row_header` or not. If we indicate `true` for `row_header`

    <%= csv_table(<<END, :row_header => true)
    A,B,C
    1,2,3
    END
    %>
    
... we get:

    --------------------------------------------------------------------
    A                      B                      C                     
    ---------------------- ---------------------- ----------------------
    1                      2                      3                     
    --------------------------------------------------------------------

But what if we want to use a comma (",") in our strings? In this case, we must quote (i.e., surround with `"`) all of our cells. But what if we want to use a literal `"`? Then we double up the `"` character. See this example:

    <%= csv_table(<<END, :row_header => true)
    "Property","Value"
    "*Units*","units of measure (lb., ft, Btu, etc.) where applicable"
    "*Legal*","limits of valid range for numeric inputs; valid ""choices"""
    END
    %>

This yields:

    --------------------------------------------------------------------
    Property       Value                                                
    -------------- -----------------------------------------------------
    *Units*        units of measure (lb., ft, Btu, etc.) where          
                   applicable                                           

    *Legal*        limits of valid range for numeric inputs; valid      
                   "choices"                                            
    --------------------------------------------------------------------

So, all in all, it is relatively straight-forward to embed a csv table inline with the markdown. However, we recommend using the next directive for handling CSV tables: `csv_table_from_file`

### csv\_table\_from\_file

This last directive loads a CSV file from the file system and processes it into a multi-line Markdown table per [Pandoc-flavored Markdown].

An example of calling this directive lies below:

    <%= csv_table_from_file("input-data--member-table-definition.csv") %>

This yields:

    --------------------------------------------------------------------
    *Units*       units of measure (lb., ft, Btu, etc.) where applicable
    ------------- ------------------------------------------------------
    *Legal*       limits of valid range for numeric inputs; valid       
                  choices                                               

    *Range*       for *choice* members, etc.                            

    *Default*     value assumed if member not given; applicable only if 
                  not required                                          

    *Required*    YES if you must give this member                      

    *Variability* how often the given expression can change: hourly,    
                  daily, etc. See sections on                           
                  [expressions](#expressions-overview),                 
                  [statements](#member-statements), and [variation      
                  frequencies](#variation-frequencies-revisited)        
    --------------------------------------------------------------------

Note that by default (same as `csv_table`), the value of `:row_header` is `true`. To turn that off, we use:

    <%= csv_table_from_file(
      "input-data--member-table-definition.csv", row_header: false) %>

which is equivalent to:

    <%= csv_table_from_file(
      "input-data--member-table-definition.csv", :row_header => false) %>

Both of the above yield:

    ------------- ------------------------------------------------------
    *Units*       units of measure (lb., ft, Btu, etc.) where applicable

    *Legal*       limits of valid range for numeric inputs; valid       
                  choices                                               

    *Range*       for *choice* members, etc.                            

    *Default*     value assumed if member not given; applicable only if 
                  not required                                          

    *Required*    YES if you must give this member                      

    *Variability* how often the given expression can change: hourly,    
                  daily, etc. See sections on                           
                  [expressions](#expressions-overview),                 
                  [statements](#member-statements), and [variation      
                  frequencies](#variation-frequencies-revisited)        
    ------------- ------------------------------------------------------

The exciting thing about using `csv_table_from_file` is that the table data can live on the file system and be edited with a spreadsheet tool such as *Microsoft Excel* or *LibreOffice Calc*.

## Updating the Documents: Support Files

The `doc/config/` directory contains various support files used to help build the documents. The `defaults.yaml` holds default configuration parameters that are discussed in the [configuration section](#the-configuration-file). Specifically, the css used for html styling is in the `css` directory; the templates used for both HTML and LaTeX (a precursor for a pdf build) are located in the `template` directory; and finally we have a `reference` directory which holds the following files:

- `probes.txt` is a file dump from the `cse -p` command
- `probes_input.yaml` is a yaml file of the above processed to include comments
- `record-index-exceptions.yaml` allows one to override settings when a record's "canonical location" in the documentation does **not** follow the "typical convention". The "typical convention" is that a record will have its own file located in the `doc/src/records` folder with a file name corresponding to the record's name (all in lowercase). Within that file, there will be a header with the same name as the record and this section header would be unique in all of the documentation sections. In other words, we would normally expect to find a record named "SOMETHING" in file `doc/src/records/something.md` where, opening that file, we would expect a header of `# SOMETHING` (Markdown). If that is **not** the case, then this file is where you would explicitly say where to locate the Record's "canonical location" in the documentation source.

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
- the [YAML] manifests for each document are located in the source. The section you will typically be adjusting is the `sections:` area. This is a list of two-tuples: the first is the "file level" and the second is the relative path from the "src/" directory. A "file level" indicates the level of indentation of that file in the documentation tree. "File levels" should **not** increase more than one at a time.

[some straightforward rules]: http://pandoc.org/MANUAL.html#extension-auto_identifiers

## Pushing Built Documents to the Server

Documents are built by default into `doc/build/output`. This directory is a check-out of the remote repository's `gh-pages` branch which uses GitHub's [GitHub Pages] functionality. The remote repository URL can be configured in the `config.yaml` file under the key `remote-repo-url`.

[GitHub Pages]: https://pages.github.com/

Once you have built the documents and are satisfied with how they look, you can push to the remote repository's `gh-pages` branch for the github site you're interested in (by default, `origin` is set to the value in `config.yaml` under the key `remote-repo-url`).
