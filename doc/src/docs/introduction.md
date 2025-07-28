# Introduction

## Greetings

The purpose of this manual is to document the California Simulation Engine computer program, CSE. CSE is an hourly building and HVAC simulation program which calculates annual energy requirements for building space conditioning and lighting. CSE is specifically tailored for use as internal calculation machinery for compliance with the California building standards.

CSE is a batch driven program which reads its input from a text file. It is not intended for direct use by people seeking to demonstrate compliance. Instead, it will be used within a shell program or by technically oriented users. As a result, this manual is aimed at several audiences:

1.  People testing CSE during its development.

2.  Developers of the CSE shell program.

3.  Researchers and standards developers who will use the program to explore possible conservation opportunities.

Each of these groups is highly sophisticated. Therefore this manual generally uses an exhaustive, one-pass approach: while a given topic is being treated, *everything* about that topic is presented with the emphasis on completeness and accuracy over ease of learning.

Please note that CSE is under development and will be for many more months. Things will change and from time to time this manual may be inconsistent with the program.

## Manual Organization

This Introduction covers general matters, including program installation.

<!--

Next, [About CSE][about-cse] will describe the program and the calculation techniques used in it.

-->

[Operation][operation] documents the operational aspects of CSE, such as command line switches, file naming conventions, and how CSE finds files it needs.

[Input Structure][input-structure] documents the CSE input language in general.

[Input Data][input-data] describes all of the specific input language statements.

[Output Reports][output-reports] will describe the output reports.

Lastly, [Probe Definitions][probe-definitions] lists all available probes.

<!--

Finally, Appendix A gives an example CSE input file and the output it produces.

-->

## Installation

### Hardware and Software Requirements

CSE is a 32-bit Microsoft Windows console application. That is, it runs at the command prompt on Windows Vista, Windows 7, Windows 8, and Windows 10.  Memory and disk space requirements depend on the size of projects being modeled, but are generally modest.

To prepare input files, a text editor is required. Notepad will suffice, although a text editor intended for programming is generally more capable. Alternatively, some word processors can be used in "ASCII" or "text" or "non-document" mode.

<!--
### Installation Files

(To be written.)
-->

### Installation Procedure

Create a directory on your hard disk with the name \\CSE or some other name of your choice. Copy the files into that directory. Add the name of the directory to the PATH environment setting unless you intend to use CSE only from the CSE directory.

<!--

### Simple Test Run

Page break field here (2)

-->
