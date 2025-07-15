# CSE User Manual

The files in this folder are concerned with building the documentation and website for CSE; specifically, the CSE User Manual. The documentation is built using Material for Mkdocs, a popular theme for Mkdocs, which is built with Python.


## Developing the Docs

Mkdocs includes a dev server which can be helpful during development.

    > uv run mkdocs serve

The dev serve includes a file watcher, so the docs will be rebuilt when changes are saved. However, because probe definitions are written as part of the on_config hook, the files in the probe-definitions folder are recreated. This can cause an infinite loop of rebuilding. Future work should explore fixing this. One solution might be adding a flag to enable only writing new probe definitions (i.e., skip existing files in the probe-definitions folder).


## Building the Docs

To build the documentation, use

    > uv run mkdocs build


## Inserting (Shared) File Content

To include external files within a page, you may use the include directive:


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

A coverage report has not yet been implemented for the current documentation system.


## Deploying Built Documents to GitHub Pages

The deploy process is not currently automated. A user must manually commit changes to the gh-pages branch to deploy the latest version of the docs. Future work could explore automating this process as well as leveraging mike to make different versions of the docs available to users.
