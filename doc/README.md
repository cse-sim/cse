# CSE User Manual

The files in this folder are concerned with building the documentation and website for CSE; specifically, the CSE User Manual. The documentation is built using Material for MkDocs, a popular theme for MkDocs, which is built with Python.

## uv and Python environment

This project uses [uv](https://docs.astral.sh/uv/) to manage dependencies and python environments.

See the [uv installation guide](https://docs.astral.sh/uv/getting-started/installation/) to get set up with uv. There are many options for installing uv, including pip/pipx, Homebrew, WinGet, and more.

## MkDocs CLI

MkDocs handles most of the functions that are required for developing and building the documentation. See the official [MkDocs documentation](https://www.mkdocs.org/user-guide/cli) for more detail on possible options/configuration when using the MkDocs CLI.

One option to highlight is the `-f` argument, which allows running MkDocs using the config specified in a location other than the current directory.

From `cse/doc/src`:

    > uv run mkdocs serve

From `cse`:

    > uv run mkdocs serve -f doc/src/mkdocs.yml



## Developing the Docs

MkDocs includes a dev server which can be helpful during development.

    > uv run mkdocs serve

The dev serve includes a file watcher, so the docs will be rebuilt when changes are saved.

To have MkDocs only rebuild dirty files, use the `--dirty` flag. WARNING: While the build process can be significantly faster, there are certain limitations to be aware of:
- Links and navigation headings may not work correctly.
- Content shared using the `{% include shared/file.md %}` directive might not trigger a rebuild and/or changes might not be reflected in the new build.


## Building the Docs

To build the documentation, use

    > uv run mkdocs build


## Inserting (Shared) File Content

To include external files within a page, use the include directive:

    {% include 'enduses.md' %}

## Tables

The three table directives and their options and syntax are shown below:

### member_table

The `member_table` uses the following syntax:

    {{
    member_table({
        "units": "1/^o^F",
        "legal_range": "no restrictions", 
        "default": "-0.0026",
        "required": "No",
        "variability": "constant" 
    })
    }}

This writes a Markdown table similar to the following:

    ---------------------------------------------------------------
    **Units** **Legal**    **Default** **Required** **Variability**
              **Range**                                            
    --------- ------------ ----------- ------------ ---------------
    1/^o^F    no           -0.0026     No           constant       
              restrictions                                         
    ---------------------------------------------------------------


### csv_table -- inline csv tables

CSV tables provide an alternate syntax for describing a multi-line table. A csv string is passed as the first argument. The second argument is a boolen for whether a header should be included. By default, no header is included in the table.

    {{
    csv_table(csv_string, header=False)
    }}

To add commas, we must (annoyingly) use the HTML comma entity, `&comma;`. To add quotation marks, first escape the character: `\"`.


### csv_table_from_file

`csv_table_from_file` is provided as a convenience wrapper for csv_table. This function takes a file_path as the first argument. The second arugment is a boolean flag for header, as in the regular csv_table. The file_path is expected to be a path to a csv file, so that when loaded, the content can be passed to csv_table as a csv string.

    {{
      csv_table(file_path, header=False)
    }}

### Coverage Report

The module `coverage.py` can be used to verify if the headings in the input-data directory contain exact matches (case-insensitive) to the output of `CSE -c`.

#### Running the coverage check

The python file can be run from anywhere in the cse directory by calling:

    > uv run path/to/coverage.py --path_to_cse <path-to-cse> [--path_to_input_data <path-to-input-data>]

The coverage checker will internally call `CSE -c` and capture the stdout to a string, which is then parsed and compared with the contents of the input-data directory. If no matches are found, a pass message will be printed. Otherwise, differences in either direction are printed. 

#### Class Variables

There are a few hard-coded constants (saved as class variables) specified at the top of the `coverage.py` file to be aware of.

**IGNORED_HEADERS** specifices headers (e.g., record names) that are ignored for the purpose of checking if the docs have full coverage of the `CSE -c` output.

**IGNORED_INPUT_DATA_FILE_PATHS** includes files in the input-data directory that are not analyzed as part of the coverage check. For example, it should be possible to define headings of any level in `index.md` without triggering a failure in the coverage check.

**SHARED** is a dictionary/map for record names that contain data members from files shared using the `{% include shared/file.md%}` directive. The coverage check analyzes the raw markdown headings before any rendering by Jinja or other steps in the build process (i.e., before shared content is injected), thus the SHARED class variable allows the coverage check to account for this shared content. The keys are record names and the value is a file in the `shared` directory whose data member headings will be associated with the key/record name for the purpose of the coverage check.

**ALIASES**: certain headings in the output of `CSE -c` are enumerated, but are simplified in the docs (e.g., **cpStage1, ..., cpStage7** in `CSE -c` output becomes **cpStageN** in the docs). Aliases account for these differences.


## Deploying Built Documents to GitHub Pages

The documentation can be deployed to GitHub Pages using either the built-in deploy command or by manually committing the built files.

### Deploy with MkDocs

The command:

    > uv run mkdocs gh-deploy

will automatically build and deploy the docs to GitHub pages.

### Deploy manually
Build the docs using `mkdocs build`, as described above. Then, paste the contents of the resulting `build` directory to the _gh-pages_ branch to manually deploy the latest version of the docs.
