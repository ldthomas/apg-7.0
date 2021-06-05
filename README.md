## ﻿﻿APG Version 7.0

[Overview](#overview)<br>
[Documentation](#doc)<br>
[Applications](#apps)<br>
[Libraries](#libs)<br>
[Installation](#install)<br>
[License](#license)

_**Update:** The installation has been updated to include `cmake` files and scripts to automate the generation of executables and IDE project files for `apg` and its examples._

##  <a id='overview'></a> Overview 
APG is an acronym for **A**BNF **P**arser **G**enerator. It was originally designed to generate recursive-descent parsers directly from the [ABNF](https://tools.ietf.org/html/rfc5234) grammar defining the sentences or phrases to be parsed. The approach was to recognize that ABNF defines a tree with seven types of nodes and that each node represents an operation that can guide a [depth-first traversal](https://en.wikipedia.org/wiki/Tree_traversal) of the tree – that is, a recursive-descent parse of the tree.

However, it was quickly realized that this method of defining a tree of node operations did not in any way require that the nodes correspond to the ABNF-defined operators. They could be expanded and enhanced in any way that might be convenient for the problem at hand. The first expansion was to add the ["look ahead"](https://en.wikipedia.org/wiki/Syntactic_predicate) nodes. That is, operations that look ahead for a specific phrase and then continue or not depending on whether the phrase is found. Next, nodes with user-defined operations were introduced. That is, a node operation that is hand-written by the user for a specific phrase-matching problem. Finally, to develop an ABNF-based pattern-matching engine similar to [regex](https://en.wikipedia.org/wiki/Regular_expression), a number of new node operations have been added: look behind, back referencing, and begin and end of string anchors.

These additional node operations enhance the original ABNF set but do not change them. Rather they form a superset of ABNF, or as is referred to here, [SABNF](https://github.com/ldthomas/apg-7.0/blob/main/SABNF.md).

Previous versions of APG have been developed to generate parsers in [C/C++](https://github.com/ldthomas/apg-6.3), [Java](https://github.com/ldthomas/apg-java) and [JavaScript](https://github.com/ldthomas/apg-js). Version 7.0 is a complete re-write in C adding a number of new features.

-   It re-introduces **P**artially-**P**redictive **P**arsing **T**ables (PPPTs), first introduced in version 5.0 in 2007, but never continued until now in future versions or languages.
-   It exposes an Application Programming Interface (API) for on-the-fly parser generation.
-   It adds a pattern-matching engine, apgex, which has all the pattern-matching power of SABNF and includes a new, parent-mode, type of back referencing capable of matching the names in nested XML and HTML start and end tags. 
-   It includes an [RFC8259](https://tools.ietf.org/html/rfc8259)-compliant JSON parser and builder.
-   It includes a [standards](https://www.w3.org/TR/REC-xml/)-compliant, non-validating XML parser.
-   It includes a number of utilities commonly needed and used by parsing applications:
    -   data encoding and decoding
    -   display of unprintable, non-ASCII data in [hexdump](https://www.man7.org/linux/man-pages/man1/hexdump.1.html)-like format
    -   line separation and handling for ASCII data
    -   line separation and handling for Unicode data
    -   a message logging facility
    -   plus a large variety of utility functions for system information, pretty printing of APG information and more. 

##  <a id='doc'></a> Documentation 
The documentation is included in the code as doxygen comments. To generate the documentation, install [Graphviz](https://graphviz.org/) and [doxygen](https://www.doxygen.nl/index.html). 
> $sudo apt update<br>
> $sudo apt install graphviz<br>
> $sudo apt install doxygen<br>
> $doxygen

The documentation can then be found at
> ./documentation/index.html

Or view it at the [sanbf.com website](https://sabnf.com/docs/doc7.0/index.html).

Note: It is recommened that before beginning an application using APG, at a minimum,  **Appendix A. Coding Conventions** (especially Exceptions and Objects), and the basic parser application example in **Appendix B. Examples** in the documentation be consulted.

##  <a id='apps'></a> Applications 
This repository consists of several applications and libraries, each being in its own directory. The applications are detailed here. The libraries in the [next section](#libs). `./` indicates the repository directory.

 - `./apg` - the command-line parser generator application
 - `./examples/ex-apgex` - examples of using the pattern-matching engine, apgex
 - `./examples/ex-api` - examples of building parsers from the parser generator API
 - `./examples/ex-ast` - examples using the Abstract Syntax Tree
 - `./examples/ex-basic` - demonstration of the basics of using APG parsers
 - `./examples/ex-conv` - demonstration of using the data conversion utility
 - `./examples/ex-format` - demonstration of using the data formatting utility
 - `./examples/ex-json` - building and using a JSON parser
 - `./examples/ex-lines` - demonstration of using the line-parsing utilities
 - `./examples/ex-msgs` - demonstration of using the message logging utility
 - `./examples/ex-sip` - a real-world example - parsing Session Initiation Protocol messages
 - `./examples/ex-trace` - demonstration of using the trace facility - the primary debugging tool
 - `./examples/ex-wide` - parsing wide (32-bit) characters
 - `./examples/ex-xml` - building and using the XML non-validating parser

##  <a id='libs'></a> Libraries 
The library directories contain source code that is shared with many of the applications.

 - `./apgex` - the pattern-matching engine code
 - `./api` - the parser generator API
 - `./json` - the JSON parser and builder
 - `./library` - the basic library of node operations required of all APG parsers
 - `./utilities` - a tool chest of utilities for data conversion, formatting, pretty printing, etc.
 - `./xml` - the XML non-validating parser

_Note that these libraries cannot be conveniently built once and for all and used as static or dynamic-linked libraries. Each application is built, in general, with a different set of compiler directives. Therefore, the necessary libraries must be built from source along with the application that uses them._

##  <a id='install'></a> Installation 
[cmake](https://cmake.org/) files, `CMakeLists.txt`, exist in the `apg` application directory as well as in each of the example application directories. These can be used to simplify the building of both the application executables and IDE project files for editing and debugging. The bash script, [build.sh](https://github.com/ldthomas/apg-7.0/blob/main/build.sh) simplifies this for the case of Unix Makefiles and the [Eclipse C/C++ IDE](https://www.eclipse.org/downloads/packages/release/2021-03/r/eclipse-ide-cc-developers). For usage and suggestions for other build systems and IDEs run

>$./build.sh --help

The [installation](https://github.com/ldthomas/apg-7.0/blob/main/Installation.md) document gives detailed instructions for creating Eclipse projects for `apg` and all of the examples both manually and using the `./build.sh` script . 

The script, [run.sh](https://github.com/ldthomas/apg-7.0/blob/main/run.sh) is available to execute any of the applications that have been built. For usage execute:

> $./run.sh --help

For example

> $./build -r target<br>
> $./run.sh target

will execute any of the target applications. For the `apg` application note:

 1. If you will be needing this for other projects it may be convenient to move it to a PATH directory, e.g.<br> `$sudo cp Release/apg/apg70 /usr/local/bin`.
 2. The reason for naming it `apg70` is to distinguish it from the Automated Password Generator application, `apg`, which comes preinstalled on some systems.

For the example applications note that many of them require that a directory `./examples/output` exists. The files that go here are transitory and available only for inspection and verification that the example is working correctly. A good usage might be

> $ln -s /tmp ./examples/output

If the `./examples/output` directory does not exist, the script `build.sh` will create it the first time it is run.

These `cmake` files and scripts have been tested on Linux Ubuntu 20.04 and Eclipse C/C++, version 4.16. Hopefully, they will also prove helpful for your favorite OS and IDE as well.


I want to thank [hhaoao](https://github.com/hhaoao) for suggesting the advantages of `cmake`. His `CMakeLists.txt` file in issue #2 was the model I used for all of the projects here.

##  <a id='license'></a> License 
APG Version 7.0 is licensed under the permissive,  Open Source Initiative-approved [2-clause BSD license](LICENSE.md). 

