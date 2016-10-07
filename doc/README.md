# CSE User Manual Build Scripts

## Dependencies for Website Generation

- the [Pandoc] markup language converter (we are using Pandoc 1.17.2, but any recent version should work)
- (optional) a recent version of [Node.js]
- a recent version of the [Ruby] programming language (we are using Ruby 2.0.0p648)
- an internet connection (for installing node dependencies -- optional)

[Pandoc]: http://pandoc.org/
[Ruby]: https://www.ruby-lang.org/en/
[Node.js]: https://nodejs.org/en/

With the above installed, the scripts in this directory should be able to download and install all other dependencies.

The specific dependencies for [Node.js] are listed in the `package.json` file.

## Instructions for Building the Documentation

Using [Node.js] enables us to compress the HTML and CSS for the website. However, it is an optional dependency. In order to use [Node.js], change the value of `use-node?:` in the file `config.yaml` to `true`.

[Node.js]: https://nodejs.org/en/

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

Documents are built by default into `doc/build/output`. This directory is a check-out of the local repository's `gh-pages` branch which uses GitHub's [GitHub Pages] functionality.

[GitHub Pages]: https://pages.github.com/

Once you have built the documents and are satisfied with how they look. You will need to add the remote repository for the github site you want to push to and then push up to the `gh-pages` on that remote.
