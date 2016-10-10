# CSE User Manual Build Scripts

## Dependencies for Website Generation

- the [Pandoc] markup language converter (we are using Pandoc 1.17.2, but any recent version should work)
- (optional) for PDF output, you will need to install a LaTeX system (see below)
- (optional) for compression of HTML/CSS, a recent version of [Node.js]
- a recent version of the [Ruby] programming language (we are using Ruby 2.0.0p648)
- (optional) an internet connection (for installing node dependencies)

For PDF generation, Pandoc creates LaTeX which is further processed to PDF. For Windows, [MiKTeX] is recommended. For Mac OS and Linux, please follow the instructions for recommended (La)TeX systems at [Pandoc's Install] page.

[Pandoc]: http://pandoc.org/
[Ruby]: https://www.ruby-lang.org/en/
[Node.js]: https://nodejs.org/en/
[MiKTeX]: https://miktex.org/
[Pandoc's Install]: http://pandoc.org/installing.html 

With the above installed, the scripts in this directory should be able to download and install all other dependencies.

The specific dependencies for [Node.js] are listed in the `package.json` file.

## Instructions for Building the Documentation

Using [Node.js] enables us to compress the HTML and CSS for the website. However, it is an optional dependency. In order to use [Node.js], change the value of `use-node?:` in the file `config.yaml` to `true`.

Your primary interaction with the build system will be through the [Rake] build tool (from the [Ruby] programming folks). [Rake] is a "make for Ruby".

[Rake]: https://github.com/ruby/rake

[Rake] comes installed with most [Ruby] installers. To see the build commands available, (after [Ruby] has been installed), type:

    > rake -T

from this directory to see a list of commands.

The default command is `build` which can be invoked either by:

    > rake build

or, since it is the default command:

    > rake

## Pushing Built Documents to the Server

Documents are built by default into `doc/build/output`. This directory is a check-out of the remote repository's `gh-pages` branch which uses GitHub's [GitHub Pages] functionality. The remote repository URL can be configured in the `config.yaml` file under the key `remote-repo-url`.

[GitHub Pages]: https://pages.github.com/

Once you have built the documents and are satisfied with how they look, you can push to the remote repository for the github site you want to push to (by default, `origin` is set to the value in `config.yaml` under the key `remote-repo-url`) and then push up to the `gh-pages` branch on that remote.
