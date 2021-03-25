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
/** \dir ./api
 * \brief The parser generator Application Programming Interface..
 */

/** \file api.h
 *  \brief Public header file for the APG API suite of functions.
 *
 *  The command line parser generator, [apg](\ref config.h), is built from an
 *  Application Programming Interface (API) object. This object is available
 *  for custom applications to use as well. It's primary documentation is here
 *  and in \ref api.c but it has a lot of code which is spread over a number of files
 *  in the api directory. Refer to them here:
 *   - \ref api.c
 *   - \ref attributes.h
 *   - \ref attributes.c
 *   - \ref input.c
 *   - \ref output.c
 *   - \ref pppt.c
 *   - \ref rule-attributes.c
 *   - \ref rule-dependencies.c
 *   - \ref semantics.h
 *   - \ref semantics.c
 *   - \ref syntax.h
 *   - \ref syntax.c
 *
 */

#ifndef API_H_
#define API_H_

#include <limits.h>

#include "../utilities/utilities.h"

/** \struct api_attr
 * \brief The recursive attributes of a single SABNF grammra rule.
 */
typedef struct {
    abool bLeft; /**< \brief  APG_TRUE if the rule is left recursive */
    abool bNested; /**< \brief  APG_TRUE if the rule is nested recursive */
    abool bRight; /**< \brief  APG_TRUE if the rule is right recursive */
    abool bCyclic; /**< \brief  APG_TRUE if the rule is cyclic */
    abool bFinite; /**< \brief  APG_TRUE if the rule is finite */
    abool bEmpty; /**< \brief  APG_TRUE if the rule can be empty */
    const char *cpRuleName; /**< \brief  the rule name for these attributes */
    aint uiRuleIndex; /**< \brief  the index of the rule for these attributes */
    aint uiRecursiveType; /**< \brief  ID_ATTR_N, ID_ATTR_R, ID_ATTR_MR, ID_ATTR_NMR, or ID_ATTR_RMR */
    aint uiMRGroup; /**< \brief  the group number, if this is a member of a mutually-recursive group (there may be multiple groups) */
} api_attr;

/** \struct pppt_size
 * \brief Size information for the **P**artially-**P**redictive **P**arsing **T**ables (PPPT) data.
 */
typedef struct{
    luint luiAcharMin; ///< \brief The minimum character size in the grammar alphabet.
    luint luiAcharMax; ///< \brief The maximum character size in the grammar alphabet.
    luint luiMapSize; ///< \brief The size, in bytes, of a single PPPT table entry (map).
    luint luiMaps; ///< \brief The number of maps needed.
    luint luiTableSize; /**< \brief The memory requirement, in bytes, of the full table. */
} pppt_size;

// the API constructor and destructor
/** @name Construction/Destruction */
///@{
void* vpApiCtor(exception* spEx);
void vApiDtor(void *vpCtx);
abool bApiValidate(void *vpCtx);
void* vpApiGetErrorLog(void *vpCtx);
///@}

// the input API - get, validate and manipulate the SABNF grammar characters
/** @name Input - Getting the Grammar Source
 * This suite of functions is concerned with the input SABNF grammar file.
 * Facilities are available for files, strings, clearing for a new grammar
 * and display of the input grammar.
 */
///@{
void vApiInClear(void *vpCtx);
const char* cpApiInFile(void *vpCtx, const char *cpFileName);
const char* cpApiInString(void *vpCtx, const char *cpString);
void vApiInValidate(void *vpCtx, abool bStrict);
void vApiInToAscii(void *vpCtx, const char *cpFileName);
void vApiInToHtml(void *vpCtx, const char *cpFileName, const char *cpTitle);
///@}

// the syntax API - parsing the SABNF grammar
/** @name Syntax Analysis of the Grammar
 * The syntax is validated and if valid an Abstract Syntax Tree (AST)is generated.
 * */
///@{
void vApiSyntax(void *vpCtx, abool bStrict);
///@}

// the semantics API - translating the SABNF grammar AST to APG opcodes
/** @name Semantic Analysis of the Grammar
 * The AST from the syntax phase is translated into specific opcode.
 * That is, for each node in the syntax tree, the grammar-specific information
 * necessary to execute the node operations are generated.
 * */
///@{
void vApiOpcodes(void *vpCtx);
void vApiOpcodesToAscii(void *vpCtx, const char *cpFileName);
void vApiRulesToAscii(void *vpCtx, const char *cpMode, const char *cpFileName);
void vApiRulesToHtml(void *vpCtx, const char *cpFileName);
///@}

// the attributes API - analyzing the SABNF grammar's opcodes
/** @name The Rule Attributes
 * Evaluate the rule attributes (e.g. left recursive, cyclic) to identify
 * any fatal aspects or attributes of the grammar.
 * */
///@{
abool bApiAttrs(void *vpCtx);
//const api_attr* spApiAttrsErrors(void *vpCtx, aint* uipCount);
void vApiAttrsToAscii(void *vpCtx, const char *cpMode, const char *cpFileName);
void vApiAttrsErrorsToAscii(void *vpCtx, const char *cpMode, const char *cpFileName);
///@}

// the Partially-Predictive Parsing Tables (PPPT)
/** @name Evaluate the Partially-Predictive Parsing Tables (PPPT)
 * PPPT maps can significantly reduce the number of node visits necessary for a traversal of the parse tree.
 * The PPPT maps are generated here.
 * */
///@{
void vApiPppt(void *vpCtx, char **cppProtectedRules, aint uiProtectedRules);
void vApiPpptSize(void *vpCtx, pppt_size* spSize);
///@}

// full parser generation tools
/** @name One-Step Generation
  Composite functions which will perform all steps of generation with a single function call.
   - grammar input & character validation
   - syntax validation
   - semantic validation and opcode generation
   - attribute validation
   - PPPT map generation
 */
///@{
void vApiFile(void *vpCtx, const char *cpFileName, abool bStrict, abool bPppt);
void vApiString(void *vpCtx, const char *cpString, abool bStrict, abool bPppt);
///@}

// the generator API - generate a parser from the SABNF grammar
/** @name Parser Generation
 * Generator output can be in two forms.
 * A pair of grammar files can be generated from which a parser can be constructed.
 * Or a parser object, completely independent of the API, can be generated.
 * */
///@{
void vApiOutput(void *vpCtx, const char *cpOutput);
void* vpApiOutputParser(void* vpCtx);
///@}

/** \page api The Parser Generator API
 * The Application Programming Interface (API) follows the same [object model](\ref objects)
 * used throughout APG. Once an object has been created, the parser generation takes place
 * in six distinct phases with an optional, seventh phase for [PPPT](\ref pppt) generation.
 * Each phase checks to verify that all required previous phases have been completed before continuing.
 * If any fatal errors are discovered in any phase, an exception is thrown back to the application's catch block.
 * Access to the list of messages is available through it's [message log](\ref utils) with \ref vpApiGetErrorLog().
 *
 * Note that the generator uses the AST library feature and any application using this API will need
 * to be compiled with the macro APG_AST defined.
 *
### Input
Generation begins with an SABNF grammar. This may be obtained from a file, \ref cpApiInFile(),
or a string, \ref cpApiInString(). These functions may be called multiple times, interchangeably.
After the first call, each successive call concatenates its input to that of the previous calls.

### Character Set Validation
The first step is to validate the input grammar characters, \ref vApiInValidate().
SABNF is fully described by the printing ASCII character set, plus tab, line feed and carriage return.
That is, the character set [9, 10, 13, 32-126].
This step simply verifies that there are no input characters outside this set.

Note: If the last line of the grammar has no line ending, CRLF, CR or LF, this will generate a
character set validation error. Technically, this is a syntax error, but it is quickly
and most easily discovered here.

### Syntax Validation
The syntax phase parses the input SABNF grammar, verifies that the syntax is correct and generates an AST
for translation. A syntax error might be, for example, a malformed rule name or an invalid equal sign.
e.g.
<pre>
1bad_rule_name =: elements CRLF
</pre>


### Semantic Validation and Translation
The semantic phase checks for any semantic errors and translates the grammar syntax into
the `rule`, `UDT` and `opcode` information required by an APG parser. Some examples of
rules that are syntactically correct but have semantic errors would be
<pre>
rule1 = %%d57-48  CRLF
rule2 = 3*2"ABC" CRLF
rule3 = 0*0"ABC" CRLF
rule4 = 0"ABC"   CRLF
</pre>
`rule1` has an inverted range definition. The range must be from lowest to highest.<br>
`rule2` has an inverted minimum and maximum number of repetitions. minimum <= maximum is required.<br>
`rule3` and `rule4` are specifically disallowed by APG. In theory they could be interpreted as an empty string.
However, they have the wasteful feature of requiring a repetition operand that will never be used.
APG allows only the literal string, "", as an explicit designation of an empty string

 *### Attributes Validation
It is well known that recursive-descent parsers will fail if a rule is left recursive.
Besides left recursion, there are a couple of other fatal attributes that need to be disclosed as well.
There are several non-fatal attributes that are of interest also.
This module will determine six different attributes listed here with simple examples.

__fatal attributes__
<pre>
; left recursion    CRLF
S = S "x" / "y"     CRLF

; cyclic            CRLF
S = S               CRLF

; infinite          CRLF
S = "y" S           CRLF
</pre>

__non-fatal attributes__ (but nice to know)
<pre>
; nested recursion  CRLF
S = "a" S "b" / "y" CRLF

; right recursion   CRLF
S = "x" S / "y"     CRLF

; empty string      CRLF
S = "x" S / ""      CRLF
</pre>
Note that these are “aggregate” attributes,
in that if the attribute is true it only means that it can be true,
not that it will always be true for every input string.
In the simple examples above the attributes may be obvious and definite &ndash; always true or false.
However, for a large grammar with possibly hundreds of rules and parse tree branches,
it can be obscure which branches lead to which attributes.
Furthermore, different input strings will lead the parser down different branches.
It is for this reason that a given attribute may reveal itself or not.

The attribute phase will determine each of these six attributes for each rule in the grammar.
Additionally, it will identify rule dependencies and mutually-recursive groups. For example,
<pre>
S = "a" A "b" / "y" CRLF
A = "x"             CRLF
</pre>
`S` is dependent on `A` but `A` is not dependent on `S`.

<pre>
S = "a" A "b" / "c" CRLF
A = "x" S "y" / "z" CRLF
</pre>
`S` and `A` are dependent on one another and are mutually recursive.

A more detailed discussion of attributes and how they are computed can be found [here](\ref attrs).

__Attributes Optional__

The attribute phase can be bypassed if necessary. While there are at this time no known bugs in the
APG version 7.0 attributes algorithm
some, but not all, previous versions did have some problems with large grammars (400+ rules).
Therefore, if you think parser generation is being prevented with incorrect attribute evaluations,
the option to bypass is here.

_Bypass attributes at the risk of building a faulty parser._


 *### PPPT Generation (optional)
The parser generator has the ability to generate [Partially-Predictive Parsing Table](\ref pppt) (PPPT) maps
which can greatly reduce the work of a parser. Be sure to understand the limits on PPPTs.
Some exploratory work may be necessary to determine if they are right for any given grammar.
If PPPT maps are not needed, it is a good idea to also define the macro APG_NO_PPPT when compiling the application.
It will reduce the amount of code needed and used by the parser.

### Composite Generation
Without giving up too much specificity, it is possible to process all of the above phases with a single function.
\ref vApiFile() for a grammar file or \ref vApiString() for a grammar string.
Using these functions will impose a couple of restrictions:
 - The entire grammar must be in a single file or string.
 - It is not possible to skip the attributes phase.

But within these restrictions, these functions can save some application coding.

 *### Output
Once the grammar has been processed, whether explicitly or with one of the composite functions,
there are then two methods for generating a parser object, \ref vApiOutput() and \ref vpApiOutputParser().

__Grammar Files__<br>
\ref vApiOutput() will generate two grammar files. A base name is provided, say `mygrammar`, and two files
defining the rules, UDTs, opcodes and all necessary supporting data will be generated &ndash; `mygrammar.h` and `mygrammar.c`.
To create a parser object from these file, `mygrammar.h` must be included with the application and `mygrammar.c`
must be compiled with it. A parser can then be constructed using the data pointer `vpMygrammarInit` supplied in mygrammar.h.

Note that the base name, `mygrammar`, is used as a namespace, both with the initialization pointer and with
macros provided in `mygrammar.h` for easy access to the rule identifiers or indexes.
This allows multiple parsers to exist in a single application.

__Parser Object__<br>
\ref vpApiOutputParser() will return a pointer to a parser object directly. The parser object
has its own memory space and is completely independent of the API object that created it.
The API object may even be destroyed after parser construction without any effect on the generated parser object.

However, they do share a the single exception structure that the application used for the API object creation.
Fatal errors in the parser will throw exceptions back to the application's catch block .



 * */



#endif /* API_H_ */
