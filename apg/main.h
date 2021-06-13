/*  *************************************************************************************
    Copyright (c) 2021, Lowell D. Thomas
    All rights reserved.
    
    This file is part of APG Version 7.0.
    APG Version 7.0 may be used under the terms of the BSD 2-Clause License.
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    
    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    
*   *************************************************************************************/
#ifndef APG_MAIN_H_
#define APG_MAIN_H_

/** \file main.h
\brief The home page information.

This file defines the [documentation outline](\ref maintop) and several of the documentation sections.

 */
/*
WHERE STUFF IS.
   reference                - \page location
1. \subpage intro           - apg/main.h
2. \subpage abnf            - apg/main.h
    1. \subpage sabnf       - apg/main.h
3. \subpage generator       - apg/main.h
    1. \subpage api         - api/api.h
    2. \subpage pppt        - apg/main.h
4. \subpage library         - library/lib.h
5. \subpage utils           - utilities/utilities.h
6. \subpage apps            - apg/main.h
    1. \subpage apgex       - apgex/apgex.h
    2. \subpage json        - json/json.h
    3. \subpage xml         - xml/xml.h
7. \subpage notes           - apg/main.h
    1. \subpage special     - apg/main.h
    2. \subpage hungarian   - apg/main.h
    3. \subpage objects     - apg/main.h
8. \subpage examples        - apg/main.h
9. \subpage abnfsabnf       - apg/main.h
   \page abnf_tree          - apg/main.h
10. \subpage license        - apg/main.h
*/

// ///////////////////////////////////////////////////////////
// MAIN PAGE - DOCUMENTATION TABLE OF CONTENTS
// ///////////////////////////////////////////////////////////
/** \mainpage APG - an ABNF Parser Generator, Version 7.0
## Documentation
\anchor maintop
1. \subpage intro
2. \subpage abnf
    1. [ABNF as Tree of Node Operations](\ref abnf_tree)
    2. [Superset ABNF (SABNF)](\ref sabnf)
3. \subpage generator
    1. \subpage api
    2. \subpage pppt
4. \subpage library
5. \subpage utils
6. \subpage apps
    1. \subpage apgex
    2. \subpage json
    3. \subpage xml
7. \subpage notes
    1. \subpage special
    2. \subpage hungarian
    3. \subpage objects
8. \subpage examples
9. \subpage abnfsabnf
10. \subpage license

*/

// ///////////////////////////////////////////////////////////
// INTRODUCTION
// ///////////////////////////////////////////////////////////
/** \page intro Introduction
 <i>Disclamer: I am not a Computer Scientist and what follows here and throughout is not a study in Formal Language Theory.
 It simply follows the definition of  ABNF to a natural means of parsing the phrases it defines.</i>

APG &ndash; an <strong>A</strong>BNF <strong>P</strong>arser <strong>G</strong>enerator &ndash;
was originally designed to generate recursive-descent parsers directly from
the [ABNF](https://tools.ietf.org/html/rfc5234) grammar defining the sentences or phrases to be parsed.
The approach is to recognize that ABNF defines a [tree](\ref abnf_tree) with seven types of nodes and
that each node represents an operation that can guide a
[depth-first traversal] (https://en.wikipedia.org/wiki/Tree_traversal) of the tree &ndash;
that is, a recursive-descent parse of the tree.

However, APG has since evolved from parsing the strictly Context-Free languages described by ABNF in a number of significant ways.

The first is through [disambiguation](\ref disambiguation).
A Context-Free language sentence may have more than one parse tree that correctly parses the sentence.
That is, different ways of phrasing the same sentence.
APG disambiguates, always selecting only a single, well-defined parse tree from the multitude.

From here it was quickly realized that this method of defining a tree of node operations did not
in any way require that the nodes correspond to the ABNF-defined tree. They could be expanded and enhanced in any way that might be convenient for the problem at hand.
The first expansion was to add the ["look ahead"](https://en.wikipedia.org/wiki/Syntactic_predicate) nodes.
That is, operations that look ahead for a specific phrase and then continue or not depending on whether the phrase is found.
Next nodes with user-defined operations were introduced. That is, a node operation that is hand-written by the user for a specific phrase-matching problem.
Finally, to develop an ABNF-based [pattern-matching engine](\ref apgex)
similar to [regular expressions](https://en.wikipedia.org/wiki/Regular_expression), `regex`,
a number of new node operations have been added: look behind, back referencing, and begin and end of string anchors.

These additional node operations enhance the original ABNF set but do not change them.
Rather they form a superset of ABNF,  or as is referred to here, [SABNF](\ref sabnf).

Today, APG is a versatile, well-tested generator of parsers. And because it is based on ABNF, it is especially well suited to parsing the languages of many
Internet technical specifications.
In fact, it is the parser of choice for several large Telecom companies.
Versions of APG have been developed to generate parsers in
[C/C++](https://github.com/ldthomas/apg-6.3),
[Java](https://github.com/ldthomas/apg-java) and
[JavaScript](https://github.com/ldthomas/apg-js2).

Version 7.0 is a complete re-write, adding a number of new features.
 - written entirely in C but with [objects and exception handling](\ref objects)
 - generates parsers for 8-, 16, 32-, and 64-bit wide alphabet characters
 - exposes an Application Programming Interface, [API](\ref api), which allows parser generation on the fly within the user's custom code
 - speeds parsing with Partially-Predictive Parsing Tables
 ([PPPT](\ref pppt)) &ndash; first introduced in version 5.0
in 2007, but never continued in future versions or languages
 - adds a pattern-matching engine, [apgex](\ref apgex), with more features and more power than regex
   - full recursion for matching deeply nested pairs
   - two modes of back referencing (introduces a new [parent mode](\ref parent_mode) back referencing)
   - handwritten pattern-matching code snippets for difficult, non-context-free phrases
   - named phrases for easy identification and referencing
   - replaces cryptic regex syntax with easy-to-read SABNF grammars
   - exposes the parsed AST for complex phrase translations
   - optionally traces the parse tree for debugging grammars and phrases
 - includes an [RFC8259](https://tools.ietf.org/html/rfc8259)-compliant [JSON](\ref json) parser and builder
 - includes a [standards](https://www.w3.org/TR/REC-xml/)-compliant [XML](\ref xml), non-validating parser
 - includes a number of [utilities](\ref utils), commonly needed and used by parsing applications:
   - data encoding and decoding &ndash; base64, UTF-8, UTF-16, UTF-32
   - display of unprintable, non-ASCII data in  [hexdump](https://www.man7.org/linux/man-pages/man1/hexdump.1.html)-like format
   - line separation and handling for ASCII and Unicode data
   - a message logging facility
   - plus a large tool chest of utility functions for system information, pretty printing of APG information
   and more
 - includes a large set of [examples](\ref examples) demonstrating all aspects of use

*/

// ///////////////////////////////////////////////////////////
// THE GENERATOR - APG
// ///////////////////////////////////////////////////////////
/**
﻿\page generator The Parser Generator

As mentioned in the introduction, APG begins with the observation that [RFC 5234](https://tools.ietf.org/html/rfc5234.html)
defines seven different types of phrase-describing or phrase-matching operations which conveniently form a tree
suitable for recursive-descent parsing. This is illustrated in [figure 1.](\ref abnf_tree) of the ABNF discussion.
The job of the generator is to specialize this general tree to a specific grammar, equipping the resulting nodes
with the grammar-specific information needed to carry out it operation. For example, a literal string node would need
to be equipped with the specific literal string specified in the grammar.
This is illustrated in [figure 2.](\ref abnf_tree) of the ABNF discussion.

RFC 5234 defines the ABNF rules with an ABNF grammar. That is, [ABNF for ABNF](https://tools.ietf.org/html/rfc5234.html#page-10).
Therefore, the APG parser generator is, in fact, an APG parser itself.
However, in this case, there is an easy answer to the "chicken or the egg" question.
The functions that perform the node operations and the format for the node data come first.
The parser that generates the node data comes second.
(Although, as any programmer will know, they both need to be debugged simultaneously.)
Also, the first time through, a bootstrap generator needs to be hand-written.
Later, when everything is working, it is then possible to generate an APG parser to replace the bootstrap
making it all self-contained. The parser generator and the parsers it produces both require the same [library](\ref library)
of parser and operator functions.

APG Version 7.0 comes with both a stand-alone, command-line function for generation the grammar files from which parsers
are constructed and an API that can be used to build custom applications for generating grammar files or
parsers directly on the fly without the need for intermediate grammar files.

For the command-line function, see the documentation details in \ref config.h and functions referred to therein.
To build the application from the `./apg` directory
  - include all source code from the directories
    - ./
    - ../../api
    - ../../library
    - ../../utilities
  - define the macro
    - APG_AST

<i>(For step-by-step instructions using Eclipse for `C/C++` see the file [Installation.md](Installation.md).)</i>

For the generator API see the [The Parser Generator API](\ref api).

APG version 7.0 also reintroduces the option to generate Partially-Predictive Parsing Tables (PPPT)
which have the ability to greatly improve the performance of the generated parsers.
See [PPPT](\ref pppt) for a discussion and documentation.

 */

// ///////////////////////////////////////////////////////////
// PPPT
// ///////////////////////////////////////////////////////////
/**
 * \page pppt Partially-Predictive Parsing Tables (PPPT)
While the generator is parsing an SABNF grammar there is much that it can determine about the generated parser's
performance independent of what input source string it might be parsing. From an examination of all terminals &ndash;
strings and ranges &ndash; it can determine the set of alphabet characters that the grammar-defined language will accept.
It is also possible for each terminal node
to build a map of whether or not it accepts or rejects each character. Each map entry has four states:
 - MATCH, (M), the node accepts the single character as a full match to the node
 - NOMATCH, (N), the node does not accept the character
 - EMPTY, (E), the node does not accept the character but does accept the empty string
 - ACTIVE, (A), the node will accept the character as the first character of a longer phrase, but the parser
 must continue to discover whether that full phrase is or is not matched

Additionally, once the maps for the terminal nodes are known, the generator can walk the parse tree
for the specified grammar and build maps for all nodes in the branches above the terminals.
Often this results in a determinate parse from a non-terminal node higher in the parse tree.
Sometimes as high as the root node &ndash; the start rule.

Consider the simple grammar:
<pre>
S = "a" / "bc" CRLF
</pre>
\dot
digraph pppt1 {
  size = "2.0";
  ratio = "fill";
      node [fontsize=14];
      label="Figure 1.\nS = \"a\" / \"bc\" CRFL";
      start [ label="RNM\n(S)"];
      alt [ label="ALT"];
      tls1 [ label="TLS\n\"a\""];
      tls2 [ label="TLS\n\"b\""];
      start->alt
      alt->{tls1 tls2}
  }
\enddot

It is simple to see from observation that the grammar's character set is [a, A, b, B, c, C].
  - the TLS("a") map is [M, M, N, N, N, N]
  - the TLS("bc") map is [N, N, A, A, N, N]
  - the ALT map is [M, M, A, A, N, N]
  - the RNM map is [M, M, A, A, N, N]

For the input string "a" or "A" the parser never needs to descend the parse tree at all.
From the PPPT map for the start rule these two characters are accepted.
However, for the characters "b" and "B" the parser knows that there is a possibility of a match and must
fully descend the parse tree to find out.
For characters "c", "C" or any other character not in the alphabet set, the start rule and the parse fail
immediately without need of descent.

This simple example illustrates how PPPT maps work. The generator will build the maps
and the parser will use them for a single-character look ahead to improve parser performance.

Experience has shown PPPT maps will reduce the number of node hits on average of about 68%.
However, the map lookups are not free and the actual reduction in parsing times are more like 50%.
That is, parsing times are cut in half.

__Limitations__<br>
The trade off with PPPT maps is, of course, memory.
Since there are only four PPPT states per node, only 2 bits are required per state. In theory, the PPPT states
for four nodes could be squeezed into each byte. In practice, the bit packing and unpacking slows the map look up and
a full byte is actually used for each state and node.

Large grammars produce an even larger number of parse tree nodes.
But the real problem is with the character set.
Consider, for example, a grammar that accepts the full range of UTF-32 characters.
With over a million characters in the alphabet character set, this would mean a megabyte for each parse tree node.
It's not unusual for a grammar to generate 1000 or more nodes, in which case the PPPT maps for this character set would
require a gigabyte or more of data storage.

Therefore, memory considerations are necessary before choosing to generate PPPT data for a parser.
As an aid in this consideration is the function \ref vApiPpptSize().
It can be called any time after \ref vApiOpcodes() which is where the alphabet character set range
and other relevant sizes are computed.



<div style="clear: both"></div>

 *
 */

// ///////////////////////////////////////////////////////////
// APPLICATIONS
// ///////////////////////////////////////////////////////////
/**
 * \page apps Applications
 * In addition to the utilities, bundled with APG Version 7.0 are several fully-developed applications.
 * These are parser applications that serve as both relatively complex examples of APG usage
 * and as useful applications to have available in their own right.
 * All are developed as APIs using the same [object](\ref object_model) model as
 * for the APG API and most accompanying utilities.
    1. \subpage apgex <br>
    apgex functions similarly to `regex`[**[1]**](https://en.wikipedia.org/wiki/Regular_expression)
    [**[2]**](https://www.regular-expressions.info/reference.html)
    [[3]](\ref regex_book),
    but uses SABNF as the pattern-defining syntax giving it more matching power and, for some at least,
    a more accessible syntax.
    2. \subpage json <br>
    The JSON parser object is a fully [RFC8259](https://tools.ietf.org/html/rfc8259)-compliant JSON parser and builder.
    3. \subpage xml <br>
    The XML parser object is a [standards](https://www.w3.org/TR/REC-xml/)-compliant, non-validating XML parser.

 */
/**
    \page regex_book
    Friedl, Jeffrey E. F. (2006), *Mastering Regular Expressions*. O'Reilly
 */

// ///////////////////////////////////////////////////////////
// CODING CONVENTIONS
// ///////////////////////////////////////////////////////////
/** \page notes Appendix A. Coding Conventions
1. \subpage special
2. \subpage hungarian
3. \subpage objects

*/

// ///////////////////////////////////////////////////////////
// SPECIAL VARIABLES
// ///////////////////////////////////////////////////////////
/**
\page special Special Variable Types
APG uses a few special character types. In particular is the alphabet character type. Depending on the SABNF grammar
the character size may need to be only 8 bits wide or as big as 64 bits wide or any value, 16 or 32, in between.
Therefore a special alphabet character type, `achar` is defined and is configurable in \ref apg.h.
The special types are:
<pre>
achar - the alphabet character, configurable in \ref apg.h
aint  - the working unsigned integer<sup>*</sup>, configurable in \ref apg.h
luint - the longest possible unsigned integer<sup>**</sup>
abool - the APG boolean
</pre>

<sup>*</sup>
Pronounce AY-int, not ain't. This is the basic APG computational unit.
I would have preferred to use <kbd>uint</kbd> here, but <kbd>uint</kbd> has a conflict with old compatibility names.

<sup>**</sup>
There are a couple of places where the size of `achar` and `aint` need to be known at coding time but
aren't known until compile time because they are configurable. One example is in the generator where space for alphabet characters
must be allocated before their size is known.
Another is when displaying them with a `printf()`-like statement. The print formats often require knowing the integer size in advance.
`luint` is used to solve this problem with statements such as the following:

<pre>
aint uiUnknown = 1;
printf("int of unknown size is %"PRIuMAX"\n", (luint)uiUnknown);
</pre>

*/


// ///////////////////////////////////////////////////////////
// HUNGARIAN NOTATION
// ///////////////////////////////////////////////////////////
/**
\page hungarian Hungarian Notation
####Hungarian Notation
Hungarian notation has its pros and cons. It is not necessarily self-consistent and the compiler, of course,
pays no attention to it. It is a matter of preference but I have found it very useful for the development of APG
and have followed the conventions below fairly consistently. For variables this notation indicates the variable type,
for functions it indicates the return value type.

The basic types use the prefixes:

<pre>
prefix - type             - example
c      - char             - char cChar;
ac     - achar            - achar acAlphabetChar;
ui     - unsigned int     - aint uiApgInteger; uint32_t uiUnicode;
b      - abool            - abool bIsTrue(){return 1;}
v      - void             - vNoReturn(){};
s      - struct/union     - struct {aint uiValue;} sMyStruct;
pfn    - function pointer - void (*pfnMyFunc)(void);
</pre>

A <kbd>p</kbd> is added to indicate a pointer and an <kbd>a</kbd> is added to indicate an array. For example:
<pre>
prefix - type
cp     - char* cpString = "string";
ca     - char caChar[5] = {65,66,67,68,69};
</pre>

The pre-prefix <kbd>s_</kbd>is used for static, globals within a file.

`static char* s_cpCommonMsg = "This message is used a lot.";`

####Additional Conventions

Macros are always in upper case with underscores.
<pre>
#define MY_DEFINE 1
</pre>

Type definitions are lower case with underscores
<pre>
typedef struct{aint uiValue;} my_special_type;
</pre>

Variables and functions are camel case with Hungarian notation for the variable type or function return type.
<pre>
aint uiMyFavoriteVariable = 1;
achar* acpMyFavoriteFunc(achar* acpCharacters){
    // do something to the characters
    return acpCharacters;
}
</pre>


*/
/**
 * \anchor object_model
\page objects Exceptions and Objects

APG version 7.0 is written entirely in C. C, of course, is not an object-oriented language. However, using data encapsulation is relatively easy
and a crude exception handler can be fashioned from the `setjmp()/longjmp()` facility.

#### Exception Handling
APG's exception handling is described in detail elsewhere (\ref exception.h and \ref exception.c).
But briefly, any application using an APG parser, or any other of the version 7.0 components, needs to be aware that
all fatal error conditions are handled with exceptions and to know how to deal with them.
Here is a simple "Hello World" example demonstrating the basic application set up.

\code
#include <stdio.h>
#include "./library/lib.h"
int main(int argc, char** argv){
    exception e;
    XCTOR(e);
    if(e.try){
        //try block
        printf("Hello, World\n");
        // report an error
        XTHROW(&e, "Hello, Exception!!!");
    }else{
        //catch block - handle exceptions here
        printf("oops\n");
        printf("%s:%s(%u):%s\n", e.caFile, e.caFunc, e.uiLine, e.caMsg);
    }
    return 0;
}
\endcode
Assuming this code is in the file `ex.c` which resides in the APG repository directory,
to compile and run with the `gcc` compiler,

<pre>
gcc %library/exception.c ex.c -o /tmp/ex
/tmp/ex
</pre>

The output should look like
<pre>
Hello, World
oops
ex.c:%main(10):Hello, Exception!!!
</pre>


The macro `XCTOR(e)` calls `setjmp()`, saving the call-stack state of the `%main()` function
and dropping into the user's try block.
Fatal errors in APG use the macro \ref XTHROW() to report the error location along with a descriptive
message. `XTHROW` calls `longjmp()`
which will restore the call stack state back the the `%main()` function's %XCTOR() call and drop into the caller's catch block.

#### Objects - Data Encapsulation
Data encapsulation is quite easily accomplished in C and APG makes heavy use of it. Most of APG's features, applications, tools and utilities are
implemented as data-encapsulated objects, each with its own constructor and destructor. As in the exception handling example above,
the object constructor returns a context pointer. That is, a pointer to an opaque chunk of data that holds the state of the object.
The destructor will then close any files and free all memory associated with the object, including the context itself.

A <i>constructor</i> function name always ends with `Ctor()`.<br>
The function almost always takes a pointer to a valid, initialized exception structure as a first argument.

A <i>destructor</i> function name always ends with `Dtor()`.<br>
It always takes its context pointer as a single argument.

In lieu of namespaces, all members of an object's "class" use the object name as a function name prefix.
For example, the parser "class" has member functions (ignoring arguments):
<pre>
void* vpParserCtor();
void  vParserDtor();
abool bParserValidate();
void  vParserParse();
void  vParserSetRuleCallback();
void  vParserSetUdtCallback();
</pre>

*/


// ///////////////////////////////////////////////////////////
// APPENDIX B. EXAMPLES
// ///////////////////////////////////////////////////////////
/** \dir ./examples
 * \brief Examples of using APG parsers and the accompanying tool chest of utilities and applications..
 */
/**
 * \page examples Appendix B. Examples
 * Included with the release of APG Version 7.0 are a number of examples of its usage
 * ranging from simple setups to complex applications.
 * They are designed to illustrate the main features of APG 7.0 an how to put them into practice.
 *
 * Since all samples are built as desktop, command-line applications with display capabilities,
 * they will all use the `utilities.h` header file, included explicitly or implicitly through one of the other headers,
 * such as `api.h`.
 * For each example, the required header files, source code directories and defined macros
 * required for compilation will be listed.
 * Each example will have one or more cases and the application will require a case number as a command line argument.
 * Executing the application with no arguments will present a synopsis of the example cases.
 *
 * **Input:**<br>
 * Many examples require pre-built input files, grammars and other data,
 * and all of these input files are collected in the directory `./examples/input`.
 *
 * **Output:**<br>
 * Some examples generate output which is considered temporary, "throw-away" data.
 * For this purpose an output directory, `./examples/output`, must exist.
 * If it doesn't already exist it must be created before running these tests.
 * Since files in this directory are temporary files which can be and should be
 * cleaned out periodically, it is convenient on Linux and similar systems
 * to create a
 * symbolic link to the temporary directory.
 * On a Linux system, for example, from the `./examples` directory,
 * <pre>
 * rm -r output
 * ln -s /tmp output
 * </pre>
 *
 * **The Examples:**
 - \subpage exbasic
 - \subpage exapi
 - \subpage extrace
 - \subpage exast
 - \subpage exunicode
 - \subpage exconv
 - \subpage exformat
 - \subpage exlines
 - \subpage exmsglog
 - \subpage exapgex
 - \subpage exjson
 - \subpage exxml
 - \subpage exsip
 - \subpage exodata
 *
 */

// ///////////////////////////////////////////////////////////
// APPENDIX C. THE ABNF GRAMMAR FOR SABNF
// ///////////////////////////////////////////////////////////
/** \anchor bottom
 *
 */
/** \page abnfsabnf Appendix C. The ABNF Grammar for SABNF

APG is a parser generator. From a language definition, or grammar, it generates a parser for the defined language.
That is, the parser generator is, itself, a parser of the language definition's grammar.
For this reason parser generators are often referred to as [compiler-compilers](https://en.wikipedia.org/wiki/Compiler-compiler).
In the case of ABNF, an ABNF grammar [can be and is](https://tools.ietf.org/html/rfc5234#page-10) used to define ABNF itself.
Similarly, an ABNF grammar can be used to define the full superset SABNF and that is given here.

See `./apg/abnf-for-sabnf.abnf` for the currently used grammar.

<pre>
;
; ABNF for SABNF (APG 7.0)
; RFC 5234 with some restrictions and additions.
; Updated 11/24/2015 for RFC 7405 case-sensitive literal string notation
;  - accepts %%s"string" as a case-sensitive string
;  - accepts %%i"string" as a case-insensitive string
;  - accepts "string" as a case-insensitive string
;  - accepts 'string' as a case-sensitive string
;
; Some restrictions:
;   1. Rules must begin at first character of each line.
;      Indentations on first rule and rules thereafter are not allowed.
;   2. Relaxed line endings. CRLF, LF or CR are accepted as valid line ending.
;   3. Prose values, i.e. <prose value>, are accepted as valid grammar syntax.
;      However, a working parser cannot be generated from them.
;
; Super set (SABNF) additions:
;   1. Look-ahead (syntactic predicate) operators are accepted as element prefixes.
;      & is the positive look-ahead operator, succeeds and backtracks if the look-ahead phrase is found
;      ! is the negative look-ahead operator, succeeds and backtracks if the look-ahead phrase is NOT found
;      e.g. &%%d13 or &rule or !(A / B)
;   2. User-Defined Terminals (UDT) of the form, u_name and e_name are accepted.
;      'name' is alpha followed by alpha/num/hyphen just like a rule name.
;      u_name may be used as an element but no rule definition is given.
;      e.g. rule = A / u_myUdt
;           A = "a"
;      would be a valid grammar.
;   3. Case-sensitive, single-quoted strings are accepted.
;      e.g. 'abc' would be equivalent to %%d97.98.99
;      (kept for backward compatibility, but superseded by %%s"abc")
; New 12/26/2015
;   4. Look-behind operators are accepted as element prefixes.
;      && is the positive look-behind operator, succeeds and backtracks if the look-behind phrase is found
;      !! is the negative look-behind operator, succeeds and backtracks if the look-behind phrase is NOT found
;      e.g. &&%%d13 or &&rule or !!(A / B)
;   5. Back reference operators, i.e. \rulename, \u_udtname, are accepted.
;      A back reference operator acts like a TLS or TBS terminal except that the phrase it attempts
;      to match is a phrase previously matched by the rule 'rulename'.
;      There are two modes of previous phrase matching - the parent-frame mode and the universal mode.
;      In universal mode, \rulename matches the last match to 'rulename' regardless of where it was found.
;      In parent-frame mode, \rulename matches only the last match found on the parent's frame or parse tree level.
;      Back reference modifiers can be used to specify case and mode.
;      \A defaults to case-insensitive and universal mode, e.g. \A === \\%%i%%uA
;      Modifiers %%i and %%s determine case-insensitive and case-sensitive mode, respectively.
;      Modifiers %%u and %%p determine universal mode and parent frame mode, respectively.
;      Case and mode modifiers can appear in any order, e.g. \\%%s%%pA === \\%%p%%sA.
;   7. String begin anchor, ABG(%^) matches the beginning of the input string location.
;      Returns EMPTY or NOMATCH. Never consumes any characters.
;   8. String end anchor, AEN(%$) matches the end of the input string location.
;      Returns EMPTY or NOMATCH. Never consumes any characters.
;
File            = *(BlankLine / Rule / RuleError)
BlankLine       = *(%%d32/%%d9) [comment] LineEnd
Rule            = RuleLookup owsp Alternation ((owsp LineEnd)
                / (LineEndError LineEnd))
RuleLookup      = RuleNameTest owsp DefinedAsTest
RuleNameTest    = RuleName/RuleNameError
RuleName        = alphanum
RuleNameError   = 1*(%%d33-60/%%d62-126)
DefinedAsTest   = DefinedAs / DefinedAsError
DefinedAsError  = 1*2%%d33-126
DefinedAs       = IncAlt / Defined
Defined         = %%d61
IncAlt          = %%d61.47
RuleError       = 1*(%%d32-126 / %%d9  / LineContinue) LineEnd
LineEndError    = 1*(%%d32-126 / %%d9  / LineContinue)
Alternation     = Concatenation *(owsp AltOp Concatenation)
Concatenation   = Repetition *(CatOp Repetition)
Repetition      = [Modifier] (Group / Option / BasicElement / BasicElementErr)
Modifier        = (Predicate [RepOp])
                / RepOp
Predicate       = BkaOp
                / BknOp
                / AndOp
                / NotOp
BasicElement    = UdtOp
                / RnmOp
                / TrgOp
                / TbsOp
                / TlsOp
                / ClsOp
                / BkrOp
                / AbgOp
                / AenOp
                / ProsVal
BasicElementErr = 1*(%%d33-40/%%d42-46/%%d48-92/%%d94-126)
Group           = GroupOpen  Alternation (GroupClose / GroupError)
GroupError      = 1*(%%d33-40/%%d42-46/%%d48-92/%%d94-126) ; same as BasicElementErr
GroupOpen       = %%d40 owsp
GroupClose      = owsp %%d41
Option          = OptionOpen Alternation (OptionClose / OptionError)
OptionError     = 1*(%%d33-40/%%d42-46/%%d48-92/%%d94-126) ; same as BasicElementErr
OptionOpen      = %%d91 owsp
OptionClose     = owsp %%d93
RnmOp           = alphanum
BkrOp           = %%d92 [bkrModifier] bkr-name
bkrModifier     = (cs [um / pm]) / (ci [um / pm]) / (um [cs /ci]) / (pm [cs / ci])
cs              = '%%s'
ci              = '%%i'
um              = '%%u'
pm              = '%%p'
bkr-name        = uname / ename / rname
rname           = alphanum
uname           = %%d117.95 alphanum
ename           = %%d101.95 alphanum
UdtOp           = udt-empty
                / udt-non-empty
udt-non-empty   = %%d117.95 alphanum
udt-empty       = %%d101.95 alphanum
RepOp           = (rep-min %%d42 rep-max)
                / (rep-min %%d42)
                / (%%d42 rep-max)
                / %%d42
                / rep-min-max
AltOp           = %%d47 owsp
CatOp           = wsp
AndOp           = %%d38
NotOp           = %%d33
BkaOp           = %%d38.38
BknOp           = %%d33.33
AbgOp           = %%d37.94
AenOp           = %%d37.36
TrgOp           = %%d37 ((Dec dmin %%d45 dmax) / (Hex xmin %%d45 xmax) / (Bin bmin %%d45 bmax))
TbsOp           = %%d37 ((Dec dString *(%%d46 dString)) / (Hex xString *(%%d46 xString)) / (Bin bString *(%%d46 bString)))
TlsOp           = [TlsCase] TlsOpen TlsString TlsClose
TlsCase         = ci / cs
TlsOpen         = %%d34
TlsClose        = %%d34
TlsString       = *(%%d32-33/%%d35-126/StringTab)
StringTab       = %%d9
ClsOp           = ClsOpen ClsString ClsClose
ClsOpen         = %%d39
ClsClose        = %%d39
ClsString       = 1*(%%d32-38/%%d40-126/StringTab)
ProsVal         = ProsValOpen ProsValString ProsValClose
ProsValOpen     = %%d60
ProsValString   = *(%%d32-61/%%d63-126/StringTab)
ProsValClose    = %%d62
rep-min         = rep-num
rep-min-max     = rep-num
rep-max         = rep-num
rep-num         = 1*(%%d48-57)
dString         = dnum
xString         = xnum
bString         = bnum
Dec             = (%%d68/%%d100)
Hex             = (%%d88/%%d120)
Bin             = (%%d66/%%d98)
dmin            = dnum
dmax            = dnum
bmin            = bnum
bmax            = bnum
xmin            = xnum
xmax            = xnum
dnum            = 1*(%%d48-57)
bnum            = 1*%%d48-49
xnum            = 1*(%%d48-57 / %%d65-70 / %%d97-102)
;
; Basics
alphanum        = (%%d97-122/%%d65-90) *(%%d97-122/%%d65-90/%%d48-57/%%d45)
owsp            = *space
wsp             = 1*space
space           = %%d32
                / %%d9
                / comment
                / LineContinue
comment         = %%d59 *(%%d32-126 / %%d9)
LineEnd         = %%d13.10
                / %%d10
                / %%d13
LineContinue    = (%%d13.10 / %%d10 / %%d13) (%%d32 / %%d9)
</pre>

*/

// ///////////////////////////////////////////////////////////
// APPENDIX D. THE LICENSE
// ///////////////////////////////////////////////////////////
/** \anchor bottom
 *
 */
/** \page license Appendix D. The License

<pre>

    Copyright (c) 2021, Lowell D. Thomas
    All rights reserved.

    APG Version 7.0 may be used under the terms of the 2-Clause BSD License.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    </pre>

*/

// ///////////////////////////////////////////////////////////
// ABNF AND SABNF
// ///////////////////////////////////////////////////////////
/**
\page abnf Augmented Backus-Naur Form (ABNF)
## ABNF

ABNF is a syntax to describe phrases, a phrase being any string of integer character codes.
Because the character codes so often represent the [ASCII](http://www.asciitable.com/) character set there are special
ABNF features to accommodate an easy description of ASCII strings. However, the meaning and range of the
character code integers are entirely up to the user.
The complete ABNF syntax description of a phrase
is called a grammar and the terms "grammar" and "ABNF syntax" will be used synonymously here.

What follows is description of the salient features of ABNF. If there are any differences between what is represented here and in
[RFC 5234](https://tools.ietf.org/html/rfc5234.html), the RFC naturally prevails.

__Rules__<br>
Phrases are described with named rules.
A rule name is alphanumeric with hyphens allowed after the first character.
Rule names are case insensitive.
A rule definition   has the form:
<pre>
name = elements CRLF
</pre>
where the equal sign, `=`, separates the name from the phrase definition.
Elements are made up of terminals, non-terminals and other rule names, as described below.
Each rule must end with a carriage return, line feed combination, CRLF.
A rule definition may be continued with continuation lines, each of which begins with a space or tab.

__Terminals__<br>
Rules resolve into a string of terminal character codes.
ABNF provides several means of representing terminal characters and strings of characters explicitly.

_single characters_<br>
<pre>
%%d32     - represents the decimal integer character code 32
%%x20     - represents the hexidecimal integer character code 20 or decimal 32
%%b100000 - represents the binary integer character code 100000 or decimal 32
</pre>
_strings of characters_
<pre>
%%d13.10     - represents the line-ending character string CRLF.
%%x0D.0A     - represents the line-ending character string CRLF.
%%b1101.1010 - represents the line-ending character string CRLF.
</pre>
_range of characters_<br>
<pre>
%%d48-57         - represents any single character code in the decimal range 48 through 57
                  that is, any ASCII digit 0, 1, 2, 3 ,4, 5, 6, 7, 8 or 9
%%x30-39         - represents any single character code in the hexidecimal range 30 through 39
                  (also any ASCII digit)
%%b110000-111001 - represents any single character code in the binary range 110000 through 111001
                  (also any ASCII digit)
</pre>
_literal strings of characters_<br>
Because of their frequency of use, there is also a notation for literal strings of printing, 7-bit ASCII characters.
<pre>
"ab"   - represents the case-insensitive string "ab" and would match
         %%d97.98, %%d65.98, %%d97.66 or %%d65.66 ("ab", "Ab", "aB" or "AB")
%%i"ab" - defined in RFC 7405, is a case-insensitive literal string (identical to "ab")
%%s"ab" - defined in RFC 7405, is a case-sensitive literal string (identical to %%d97.98)
</pre>
Tab characters, 0x09, are not allowed in literal strings.

_prose values_<br>
When all else fails, ABNF provides a means for the grammar's author to simply provide a prose
explanation of the phrase in the form of a spoken, as opposed to formal, language.
The notation is informative and the parser generator will recognize it as valid ABNF.
However, since there are no formal specifics, the generator will halt without generating a parser.
<pre>
<phrase description in prose>
</pre>
Tab characters, 0x09, are not allowed in the prose values.

__Non-Terminals__<br>
_concatenation_<br>
A space between elements in a rule definition represents a concatenation of the two elements.
For example, consider the two rules,
<pre>
AB1 = "a" "b" CRLF
AB2 = "ab" CRLF
</pre>
The space between the two elements `"a"` and `"b"` acts as a concatenation operator.
The effect in this case is that rule `AB1` defines the same phrase as rule `AB2`.

_alternatives_<br>
The forward slash, `/`, is the alternative operator. The rule
<pre>
AB = "a" / "b" CRLF
</pre>
would match either the phrase `a` or the phrase `b`.

_incremental alternatives_<br>
While not a new operation, incremental alternatives are a sometimes-convenient means of adding alternatives to a rule.
<pre>
alt1 = "a" / "b" / "c" CRLF
alt2 = "a" CRLF
      / "b" CRLF
      / "c" CRLF
alt3 = "a" / "b" CRLF
alt3 =/ "c" CRLF
</pre>
Rules `alt1`, `alt2` and `alt3` have identical definitions. The incremental alternative, `=/`, allows for adding additional
alternatives to a rule at a later date. As seen in `alt2`, the same affect can be achieved with line continuations.
However, in some cases, it may be convenient or even essential to add additional
alternatives later in the grammar. For example, if the grammar is broken into two or more files.
In such a case, line continuations would not be possible and the incremental alternative becomes an essential
syntactic addition.

_repetitions_<br>
An element modifier of the general form `n*m (0 <= n <= m)` can be used to indicate a repetition of the element
a minimum of `n` times and a maximum of `m` times. For example, the grammar
<pre>
number = 2*3digit CRLF
digit  = %%d48-57  CRLF
</pre>
would define a phrase that could be any number with 2 or 3 ASCII digits.
There are a number of shorthand variations of the repetition operator.
<pre>
 *= 0*infinity (zero or more repetitions)
n*= n*infinity (n or more repetitions)
*m= 0*m        (zero to m repetitions)
n = n*n        (exactly n repetitions)
</pre>

__Groups__<br>
Elements may be grouped with enclosing parentheses. Grouped elements are then treated as a single element
within the full context of the defining rule. Consider,
<pre>
phrase1 = elem (foo / bar) blat CRLF
phrase2 = elem foo / bar blat CRLF
phrase3 = (elem foo) / (bar blat) CRLF
</pre>
`phrase1` matches `elem foo blat` or `elem bar blat`, whereas `phrase2` matches `elem foo` or `bar blat`.
A word of caution here. Concatenation has presidence over (tighter binding than) alternation so that `phrase2`
is the same as `phrase3` and not `phrase1`.
It can be confusing. Use parentheses liberally to keep the grammar meaning clear.

Another useful way to think of groups is as anonymous rules. That is, given
<pre>
phrase1 = elem (foo / bar) blat CRLF
phrase2 = elem anon blat CRLF
anon    = foo /bar CRLF
</pre>
phrase1 and phrase2 are identical. Only phrase2 utilizes the explicit rule `anon` for the parenthesized grouping.
In phrase1, the parenthesized grouping anonymously defines the same rule as `anon`.

__Optional Groups__<br>
Elements grouped with square brackets, `[]`, are optional groups. Consider,
<pre>
phrase1 = [elem foo] bar blat CRLF
phrase2 = 0*1(elem foo) bar blat CRLF
</pre>
Both phrases are identical and will match either `elem foo bar blat` or `bar blat`.

__Comments__<br>
Comments begin with a semicolon, `;`, and continue to the end of the current line.
For example, in the following rule definition, everything from the semicolon to CRLF is considered white space.
<pre>
phrase = "abc"; any comment can go here   CRLF
</pre>
In this implementation empty lines and comment-only lines are accepted as white space,
but any line beginning with one or more space/tab characters and having text not beginning
with a semicolon will be rejected as an ABNF syntax error.
Consider the lines,
<pre>
1:CRLF
2:    CRLF
3:;comment CRLF
4:     ; comment CRLF
5:   comment CRLF
</pre>
Lines `1:` through `4:` are valid blank lines. Line `5:` would be regarded as a syntax error.

__Bringing it all together now__<br>
Here is an example of a complete ABNF grammar representing the general definition of a floating point number.
<pre>
float    = [sign] decimal [exponent]
sign     = "+" / "-"
decimal  = integer [dot [fraction]]
           / dot fraction
integer  = 1*%%d48-57
dot      = "."
fraction = 1*%%d48-57
exponent = "e" [esign] exp
esign    = "+" / "-"
exp      = 1*%%d48-57
</pre>


### Restrictions

This APG implementation imposes a several
restrictions and changes to the strict ABNF described above.
These are minor changes except for the disambiguation rules.

__Indentations__<br>
RFC 5234 specifies that a rule may begin in any column, so long as all rules begin in the same column.
This implementation restricts the rules to the first column.

__Line Endings__<br>
RFC 5234 specifies that a line ending must be the carriage return/line feed pair, CRLF.
This implementation relaxes that and accepts CRLF, LF or CR as a valid line ending.
However, the last line must have a line ending or a fatal error is generated.
(_Forgetting a line ending on the last line is a common and annoying error,
but keeping the line ending requirement has been a conscious design decision._)

__Case-Sensitive Strings__<br>
This implementation allows case-sensitive strings to be defined with single quotes.
<pre>
phrase1 = 'abc'      CRLF
phrase2 = %%s"abc"    CRLF
phrase3 = %%d97.98.99 CRLF
</pre>
All three of the above phrases defined the identical, case-sensitive string `abc`. The single-quote
notation for this was introduced in 2011 prior to publication of RFC 7405.
The SABNF single-quote notation is kept for backward compatibility.

__Empty Strings__<br>
As will be seen later, some rules may accept empty strings. That is, they match a string with 0 characters.
To represent an empty string explicitly, two possibilities exist.
<pre>
empty-string = 0*0element ; zero repetitions
empty-string = ""         ; empty literal string
</pre>
In this implementation only the literal string is allowed.
Zero repetitions will halt the parser generator with a grammar error.

\anchor disambiguation
__Disambiguation__<br>
The ALT operation allows the parser to follow multiple pathways through the parse tree.
It can be and often is the case that more than one of these pathways will lead to a successful phrase match.
The question of what to do with multiple matches was answered early in the development of APG with the simple
rule of always trying the alternatives left to right as they appear in the grammar
and then simply accepting the first to succeed. This "first success" disambiguation rule may break
a strictly context-free aspect of ABNF, but it not only solves the problem
of what to do with multiple matches, at least on a personally subjective level, it actually makes the grammars
easier to write. That is, easier to arrange the alternatives to achieve the desired phrase definitions.

Related to disambiguation is the question of how many repetitions to accept. Consider the grammar
<pre>
reps = *"a" "a" CRLF
</pre>
A strictly context-free parser should accept any string a<sup>n</sup>, n>0.
But in general this requires some trial and error with back tracking.
Instead, repetitions in APG always accept the longest match possible. That would mean that APG would fail to match
the example above. However, a quick look shows that a simple rewrite would fix the problem.
<pre>
reps = 1*"a" CRLF
</pre>
Longest-match repetitions rarely lead to a serious problem. Again, knowing in advance exactly how the parser will handle
repetitions allows for easy writing of a correct grammar.


\anchor abnf_tree
### ABNF as a Tree of Node Operations
APG was originally developed with the recognition that the ABNF syntax elements could be represented
with a tree of node operations. That is, every ABNF grammar can be
represented as a tree of seven types of node operations, three terminal and four non-terminal.
 - TLS - The TLS, or terminal literal string, operation is simply to match or capture the alphabet characters
 in the input, source string as defined in the grammar.
 The match is done in a case-insensitive manner for ASCII alphabetic characters.
 - TBS - The TBS, or terminal binary string, operation.
 Same as TLS except that the characters are matched character-for-character with no special ASCII considerations.
 - TRG - The TRG, or terminal range, operation matches, or captures, any alphabet character in the grammar-defined range.
 - ALT - The ALT node provides a list of alternate paths for the parser, one for each child node.
 - CAT - The CAT node dictates concatenating the results of all child nodes.
 - REP - The REP node, within grammar-defined limits, dictates that the child node be repeated,
 the results of each repetition being concatenated to the previous result.
 - RNM - The RNM, or rule, node operation is to substitute it with the node operations defined by the named rule.

\dot
digraph abnf_tree {
  size = "5,5";
  ratio = "fill";
      node [fontsize=14];
      label="Figure 1. ABNF as a tree of 7 node operations -\nALT CAT REP RNM TLS TBS TRG";
      start [ label="Start Rule\nRNM"];
      alt1 [ label="ALT"];
      alt2 [ label="ALT"];
      cat1 [label="CAT"];
      cat2 [label="CAT"];
      cat3 [label="CAT"];
      rep1 [label="REP"];
      rep2 [label="REP"];
      rep3 [label="REP"];
      grp [label="group" style=dotted];
      opt [label="option" style=dotted];
      pros [label="<prosval>"];
      rnm [ label="RNM"];
      tls [ label="TLS"];
      tbs [ label="TBS"];
      trg [ label="TRG"];
      start->alt1
      alt1->cat1
      alt1->cat3
      alt1->cat2
      cat2->rep1
      cat2->rep3
      cat2->rep2
      rep2->alt2
      alt2->grp
      alt2->opt
      alt2->pros
      alt2->rnm
      alt2->trg
      alt2->tbs
      alt2->tls
  }
  \enddot

\dot
digraph parse_tree {
  size = "4.0";
  ratio = "fill";
      node [fontsize=14];
      label="Figure 2. Opcodes for the grammar:\nsample = right / \"left\"\nright = 1*%d48-57 %d65";
      start [ label="RNM\n(sample)"];
      alt [ label="ALT"];
      cat [label="CAT"];
      rep [label="REP\n(1 - inf)"];
      rnm [ label="RNM\n(right)"];
      tls [ label="TLS\n\"left\""];
      tbs [ label="TBS\n%d65"];
      trg [ label="TRG\n%d48-57"];
      start->alt
      alt->tls
      alt->rnm
      rnm->cat
      cat->tbs
      cat->rep
      rep->trg
  }
  \enddot

<div style="clear: both"></div>
The ABNF syntax defines a tree of general node operations as illustrated in figure 1.
An ABNF grammar then reduces this to a specific tree of node operations
as illustrated in figure 2 for the sample grammar below.
A [depth-first](https://en.wikipedia.org/wiki/Tree_traversal)
traversal of the grammar-specific tree, with the node operations as the guide, then is the APG recursive-descent parser.
Figure 2 illustrates the grammar-specific tree of opcodes for the sample grammar
<pre>
sample = right / "left"
right  = 1*%%d48-57 %%d65
</pre>

\anchor sabnf
## Superset ABNF (SABNF)
In addition to the seven original node operations defined by ABNF, APG recognizes an addition 8 operations.
Since these do not alter the original seven operations in any way, these constitute a super set of the original set.
Hence the designation <strong>S</strong>uperset
<strong>A</strong>ugmented <strong>B</strong>ackus-<strong>N</strong>aur <strong>F</strong>orm, or SABNF.

The user-defined terminals and look ahead operations have been carried over from previous versions of APG.
Look behind, anchors and back references have been developed to replicate the phrase-matching power of various
flavors of `regex`. However, the parent mode of back referencing is, to my knowledge, a new APG development
with no previous counterpart in other parsers or phrase-matching engines.

__User-Defined Terminals__<br>
In addition to the ABNF terminals above, APG allows for User-Defined Terminals (UDT).
These allow the user to write any phrase he or she chooses as a code snippet. The syntax is,
<pre>
phrase1 = u_non-empty CRLF
phrase2 = e_possibly-empty CRLF
</pre>
UDTs begin with `u_` or `e_`. The underscore is not used in the ABNF syntax, so the parser can easily
distinguish between UDT names and rule names. The difference between the two forms is that a UDT
beginning with `u_` may not return an empty phrase. If it does the parser will throw an exception.
Only if the UDT name begins with `e_` is an empty phrase return accepted. The difference has to do with
the [rule attributes](\ref attrs) and will not be discussed here further.

Note that even though UDTs are terminal phrases, they are also named phrases and share some named-phrase
qualities with rules.

__Look Ahead__<br>
The look ahead operators are modifiers like repetitions. They are left of and adjacent to the phrase
that they modify.
<pre>
phrase1 = &"+" number CRLF
phrase2 = !"+" number CRLF
number  = ["+" / "-"] 1*%%d48-75 CRLF
</pre>
`phrase1` uses the positive look ahead operator. If `number` begins with a `"+"` then `&"+"` returns the
empty phrase and parsing continues. Otherwise, `&"+"` return failure and `phrase1` fails to find a match.
That is, `phrase1` accepts only numbers that begin with `+`. e.g. +123.

`phrase2` uses the negative look ahead operator. It works just as described above except that it succeeds if
`"+"` is *not* found and fails if it is.
That is, `phrase2` accepts only numbers with no sign or with a negative sign. e.g. -123 or 123.

A good discussion of the origin of these operators can be found in this
[Wikipedia article.](https://en.wikipedia.org/wiki/Syntactic_predicate)

__Look Behind__<br>
The look behind operators are  modifiers very similar to the look ahead operators, the difference, as the name implies, is that they operate on phrases behind the current string index instead of ahead of it.
<pre>
phrase1 = any-text &&line-end text CRLF
phrase2 = any-text !!line-end text CRLF
text = *%%d32-126 CRLF
any-text = *(%%d10 / %%d13 / %%d32-126) CRLF
line-end = %%d13.10 / %%d10 / %%d13 CRLF
</pre>
`phrase1` will succeed only if `text` is preceded by a `line-end`.
`phrase2` will succeed only if `text` is *not* preceded by a `line-end`.

Look behind was introduced specifically for [apgex](\ref apgex), the phrase-matching engine.
It may have limited use outside this application.

__Back References__<br>
Back references are terminal strings similar to terminal literal and binary strings.
The difference is that terminal literal and binary strings
are predefined in the grammar syntax whereas back reference strings are dynamically defined with a previous
rule name or UDT matched phrase.
The basic notation is a backslash, `\`, followed by a rule or UDT name.
<pre>
phrase1 = A \A CRLF
phrase2 = A \\%%iA CRLF
phrase3 = A \\%%sA CRLF
phrase4 = u_myudt \u_myudt
A       = "abc" / "xyz" CRLF
</pre>
The back reference `\A` will attempt a case-insensitive match to a previously matched by `A`-phrase.
(The notation works equally for rule names and UDT names.)
Therefore, `phrase1` would match `abcabc` or `abcABC`, etc., but not `abcxyz`. The `%%i` and `%%s` notation
is used to indicate case-insensitive and case-sensitive matches, just as specified in RFC 7405
for literal strings. Therefore, `phrase3` would match `xYzxYz` but not `xYzxyz`.

These back reference operations were introduced specifically for [apgex](\ref apgex)
to match the parsing power of various flavors of the `regex` engines. However, it was soon
recognized that another mode of back referencing was possible.
The particular problem to solve was, how to use back referencing to match tag names in opening and closing HTML and XML tags.
This led to the development of a new type of back referencing, which to my knowledge, is unique to APG.

I'll refer to the original definition of back referencing above as "universal mode". The name "universal" being
chosen to indicate that the back reference `\%%uA` matches the last occurrence of `A` universally. That is, regardless of where in
the input source string or parse tree it occurs.

I'll refer to the new type of back referencing as "parent mode". The name "parent" being chosen to indicate
that `\%%pA` matches the last occurrence of `A` on a sub-tree of the parse tree with the same parent node.
A more detailed explanation is given [here](\ref parent_mode).

Case insensitive and universal mode are the defaults unless otherwise specified.
The complete set of back references with modifiers is:
<pre>
\A     = \\%%iA   = \\%%uA = \\%%i%%uA = \\%%u%%iA
\\%%sA   = \\%%s%%uA = \\%%u%%sA
\\%%pA   = \\%%i%%pA = \\%%p%%iA
\\%%s%%pA = \\%%p%%sA
</pre>

__Anchors__<br>
Again, to aid the pattern matching engine [apgex](\ref apgex),
SABNF includes two specific anchors, one for the beginning of the input source string
and one for the end.
<pre>
phrase1 = %^ text     CRLF
phrase2 = text %$     CRLF
phrase3 = %^ "abc" %$ CRLF
text    = *%d32-126   CRLF
</pre>
Anchors match a location, not a phrase. `%^` returns an empty string match if the input string character index
is zero and fails otherwise. Likewise, `%$` returns an empty string match if the input string character index
equals the string length and fails otherwise.
The notations `%^` and `%$` have been chosen to be similar to their familiar `regex` counterparts.

In the examples above, `phrase1` will match `text` only if it starts at the beginning of the string.
`phrase2` will match `text` only if it ends at the end of a string. `phrase3` will match `abc`
only if it is the entire string. This may seem self evident in this context, but
APG 7.0 allows parsing of sub-strings of the full input source string.
Therefore, when parsing sub-strings it may not always be known
programmatically whether a phrase is at the beginning or end of a string.

### Operator Summary

\htmlonly
<table>
<caption><strong>Terminal SABNF operators.</strong></caption>
<tr>
<th>operator</th>
<th>notation</th>
<th>form</th>
<th>description</th>
</tr>
<tr>
<td>TLS</td>
<td>"string"</td>
<td>ABNF</td>
<td>terminal literal string</td>
</tr>
<tr>
<td>TBS</td>
<td>%d65.66.67</td>
<td>ABNF</td>
<td>terminal binary string</td>
</tr>
<tr>
<td>TRG</td>
<td>%d48-57</td>
<td>ABNF</td>
<td>terminal range</td>
</tr>
<tr>
<td>UDT</td>
<td>u_name or<br>e_name</td>
<td>SABNF</td>
<td>User-Defined Terminal</td>
</tr>
<tr>
<td>BKR</td>
<td>\name or<br>\u_name</td>
<td>SABNF</td>
<td>back reference</td>
</tr>
<tr>
<td>ABG</td>
<td>%$</td>
<td>SABNF</td>
<td>begin of string anchor</td>
</tr>
<tr>
<td>AEN</td>
<td>%^</td>
<td>SABNF</td>
<td>end of string anchor</td>
</tr>
</table>

<table>
<caption><strong>Non-Terminal SABNF operators.</strong></caption>
<tr>
<th>operator</th>
<th>notation</th>
<th>form</th>
<th>description</th>
</tr>
<tr>
<td>ALT</td>
<td>/</td>
<td>ABNF</td>
<td>alternation</td>
</tr>
<tr>
<td>CAT</td>
<td>space</td>
<td>ABNF</td>
<td>concatenation</td>
</tr>
<tr>
<td>REP</td>
<td>n*m</td>
<td>ABNF</td>
<td>repetition</td>
</tr>
<td>RNM</td>
<td>name</td>
<td>ABNF</td>
<td>rule name</td>
</tr>
<tr>
<td>AND</td>
<td>&</td>
<td>SABNF</td>
<td>positive look ahead</td>
</tr>
<tr>
<td>NOT</td>
<td>!</td>
<td>SABNF</td>
<td>negative look ahead</td>
</tr>
<tr>
<td>BKA</td>
<td>&&</td>
<td>SABNF</td>
<td>positive look behind</td>
</tr>
<tr>
<td>BKN</td>
<td>!!</td>
<td>SABNF</td>
<td>negative look behind</td>
</tr>
</table>
\endhtmlonly

\page parent_mode Universal vs Parent Mode Back Referencing
A universal mode back reference, `\%%uA`, matches the last occurrence of `A` regardless of where it occurs
in the input source string or on the parse tree. Consider a grammar for HTML-like tags using universal mode.
<pre>
U   = (%%d60 tag %%d62) U (%%d60.47 \\%%utag %%d62) / %%d45.45 CRLF
tag = 1*%%(d97-122 / %%d65-90) CRLF
</pre>
\dot
 strict digraph universal {
  size = "3.0";
  ratio = "fill";
      node [fontsize=14];
      label="Figure 1. Universal Mode Tree of Tags";
      root [ label="U" ordering=out];
      parent [label="U" ordering=out];
      parent ordering=out
      center [label="--"];
      tag1 [label="<TagA>"];
      tag2 [label="<TagB>"];
      match1 [label="</TagB>"];
      match2 [label="</TagB>"];
      root->tag1
      root->parent
      root->match1
      parent->tag2
      parent->center
      parent->match2
  }
\enddot
The input string
<pre>
<TagA><TagB>--</TagB></TagB>
</pre>
would have a parse tree figuratively like Figure 1. Notice that the last tag matched
was "TagB" at the bottom of the left side of the parse tree.
Since universal mode back referencing only matches that last occurrence of the rule "tag"
both back references can only match "TagB". This is not what we want for HTML tags.

<div style="clear: both"></div>

Let's try this again with parent mode back referencing. We will use the same grammar except use parent mode back references.
<pre>
P   = (%%d60 tag %%d62) P (%%d60.47 \\%%ptag %%d62) / %%d45.45 CRLF
tag = 1*%%(d97-122 / %%d65-90) CRLF
</pre>
\dot
 strict digraph parent {
  size = "3.0";
  ratio = "fill";
      node [fontsize=14];
      label="Figure 2. Parent Mode Tree of Tags";
      root [ label="U" ordering=out];
      parent [label="U" ordering=out];
      parent ordering=out
      center [label="--"];
      tag1 [label="<TagA>"];
      tag2 [label="<TagB>"];
      match1 [label="</TagA>"];
      match2 [label="</TagB>"];
      root->tag1
      root->parent
      root->match1
      parent->tag2
      parent->center
      parent->match2
  }
\enddot
The input string
<pre>
<TagA><TagB>--</TagB></TagA>
</pre>
would have a parse tree figuratively like Figure 2.
Since parent mode back referencing only matches the last occurrence of the rule "tag" having the same parent as the back reference
we get a symmetric matching across the left and right branches of the parse tree. This solves the problem
of matching not only the correct pairing of the HTML start and end tags, but the tag names as well.
<div style="clear: both"></div>

 */

#endif /* APG_MAIN_H_ */
