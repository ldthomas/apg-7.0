﻿# APG Version 7.0

[Overview](#overview)<br>
[Documentation](#doc)<br>
[Applications](#apps)<br>
[Libraries](#libs)<br>
[Installation](#install)<br>
[License](#license)

##  <a id='overview'></a> Overview 
APG is an acronym for **A**BNF **P**arser **G**enerator. It was originally designed to generate recursive-descent parsers directly from the [ABNF](https://tools.ietf.org/html/rfc5234) grammar defining the sentences or phrases to be parsed. The approach was to recognize that ABNF defines a tree with seven types of nodes and that each node represents an operation that can guide a [depth-first traversal](https://en.wikipedia.org/wiki/Tree_traversal) of the tree – that is, a recursive-descent parse of the tree.

However, it was quickly realized that this method of defining a tree of node operations did not in any way require that the nodes correspond to the ABNF-defined operators. They could be expanded and enhanced in any way that might be convenient for the problem at hand. The first expansion was to add the ["look ahead"](https://en.wikipedia.org/wiki/Syntactic_predicate) nodes. That is, operations that look ahead for a specific phrase and then continue or not depending on whether the phrase is found. Next, nodes with user-defined operations were introduced. That is, a node operation that is hand-written by the user for a specific phrase-matching problem. Finally, to develop an ABNF-based pattern-matching engine similar to [regex](https://en.wikipedia.org/wiki/Regular_expression), a number of new node operations have been added: look behind, back referencing, and begin and end of string anchors.

These additional node operations enhance the original ABNF set but do not change them. Rather they form a superset of ABNF, or as is referred to here, [SABNF](./SABNF.md).

Previous versions of APG have been developed to generate parsers in [C/C++](https://github.com/ldthomas/apg-6.3), [Java](https://github.com/ldthomas/apg-java) and [JavaScript](https://github.com/ldthomas/apg-js2). Version 7.0 is a complete re-write in C adding a number of new features.

-   It re-introduces **P**artially-**P**redictive **P**arsing **T**ables (PPPTs), first introduced in version 5.0 in 2007, but never continued in future versions or languages.
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
The documentation is included in the code as doxygen comments. To generate the documentation, install [Graphviz](https://graphviz.org/) and [doxygen](https://www.doxygen.nl/index.html). With the Linux distribution Ubuntu 20.04 this is as simple as:
> $sudo apt update<br>
> $sudo apt install graphviz<br>
> $sudo apt install doxygen

With other Linux distros the procedure is nearly the same. With Graphviz and doxygen installed, from the repository directory simply execute doxygen.
> $doxygen

The documentation home page can then be found at
> ./documentation/index.html

Or view it at the [sanbf.com website](https://sabnf.com/docs/doc7.0/index.html).

It is recommened that before beginning an application using APG, at a minimum,  **Appendix A. Coding Conventions** (especially Exceptions and Objects), and the basic parser application example in **Appendix B. Examples** in the documentation be consulted.

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
 - `./xml` - the XML parser

_Note that these libraries are not built once and for all and linked as static or dynamic libraries. The reason is that for each application, in general, the libraries are built with a different set of compiler defines. Therefore, for each application and example the necessary libraries with the required compiler defines are built from source._

##  <a id='install'></a> Installation 
APG Version 7.0 has been developed on a the Linux Ubuntu 20.04 operating system using the [Eclipse for C/C++](https://www.eclipse.org/downloads/packages/) IDE version 4.16. `CMakeLists.txt` files exist in the `apg` application directory as well as in each of the examples application directories. The script `build-release.sh` will use `cmake` to build any or all of the applications. Run 
>$./build-release.sh --help

 for exact usage.

This is handy to build the release version of the parser generator and you may want to copy it to a path directory for ease of use. e.g.
>$sudo cp Release/apg/apg70 /usr/local/bin<br>
>$apg70 --version

However, running the release versions of the examples is not very instructive. To study the examples and experiment with them they are best run in an IDE with a good code editor and debugger. `build-debug.sh` exists for this purpose. Run 
>build-debug.sh --help

for exact usage. `build-debug.sh` will generate Eclipse project files. Simply run
>$./build-debug.sh all

Open Eclipse. The workspace directory is not important, but a good practice might be to switch the workspace to the apg-7.0 repository directory. Once Eclipse is open, navigate to 
>Project->import->General->Existing Projects into Workspace

and in the `Select root directory` window navigate to (full path)
>./apg-7.0/Debug

All of the projects should appear already selected. Simply click `Finish` and the apg project and all of the example projects will be imported. At this point it should be possible to build, edit, run and debug any of the projects in the normal Eclipse manner.

For other Linux distros I would imagine the process to be similar as long as `cmake` version 3.20+ and Eclipse 4.16+ are installed. 

Unfortunately, although `cmake` theoretically works across many platforms, I have not been able to get it to work on Windows. 

Although considerably more tedious, Eclipse projects can also be created manually without the help of `cmake` and this is described in detail for the `apg` application in the [installation](installation.md) document. I have been successful in getting apg-7.0 running in Eclipse on Windows with this manual procedure.

I want to thank [hhaoao](https://github.com/hhaoao) for suggesting the advantages of `cmake` His `CMakeLists.txt` file in issue #2 was the model I used for all of the projects here.

##  <a id='license'></a> License 
APG Version 7.0 is licensed under the permissive,  Open Source Initiative-approved [2-clause BSD license](LICENSE.md). 

