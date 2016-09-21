# CSE User Manual Build Scripts

## Dependencies for Website Generation

- the [Pandoc](http://pandoc.org/) markup language converter (we are using Pandoc 1.17.2, but any recent version should work)
- (optional) a recent version of [node.js](https://nodejs.org/en/)
- a recent version of the [Ruby programming language](https://www.ruby-lang.org/en/) (we are using Ruby 2.0.0p648)
- an internet connection (for installing node dependencies)

With the above installed, the scripts in this directory should be able to download and install all other dependencies.

The specific dependencies for Node.js are listed in:

- package.json (for Node.js)

## Dependencies for Generating Probes Documenation

In addition to the dependencies above, probles documentation requires the availability of a C Preprocessor. Which we assume is called `cpp`.
