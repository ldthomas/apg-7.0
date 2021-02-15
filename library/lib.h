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
/** \dir library
 * \brief The parser library required of all APG parsers..
 */

/** \file lib.h
 * \brief This header "#include"s all publid lib headers and other standard headers needed by most objects.
 */

#ifndef LIB_LIB_H_
#define LIB_LIB_H_

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>

#include "./apg.h"

/** \struct apg_phrase
 * \brief Defines a pointer to an `achar` array plus its length. That is, a phrase as is often used by APG.
 *
 * ABNF grammars define phrases of the grammar's alphabet characters.
 * APG must deal with them specifically and often.
 * A phrase is nothing more than an array of type `achar` characters.
 * Since all values may be present in the array it is not possible to use a special character as an array ending
 * character as is done with the C-language strings of `char` characters.
 * Also, the length of the array may easily be longer than
 * the maximum `achar` character, APG_MAX_ACHAR. Therefore, a special structure is required
 * to hold the array and length information.
 */
typedef struct{
    const achar* acpPhrase; ///< \brief Pointer to an array of type `achar` APG alphabet characters.
    aint uiLength; ///< \brief The number of characters in the array
} apg_phrase;

/** \struct u32_phrase
 * \brief Defines a pointer to an array of 32-bit unsigned integers plus its length. Typically needed by Unicode applications.
 *
 * This type of phrase is often necessary when dealing with Unicode, but can be used for any
 * array of 32-bit unsigned integers. Since the array may hold characters of all possible values,
 * it is not possible to use a special character to signal the end of the array. Therefore,
 * the structure includes the array length.
 */
typedef struct{
    const uint32_t* uipPhrase; ///< \brief Pointer to an array of 32-bit unsigned integers.
    uint32_t uiLength; ///< \brief The number of integers in the array.
} u32_phrase;

#include "./exception.h"
#include "./memory.h"
#include "./vector.h"
#include "./trace.h"
#include "./stats.h"
#include "./ast.h"
#include "./parser.h"
#include "./tools.h"

#endif /* LIB_LIB_H_ */

/** \page library The Parsing Library
 * ## Overview
 * The APG library contains the basic parsing functions used by the parser generator and all of
 * the parsers it generates. The library is highly configurable though a variety of macros (see \ref apg.h),
 * meaning that different macros are usually defined differently for each application.
 * The utility function vUtilApgInfo() will display which macros are defined with a brief explanation of each.
 * Because of this, the library is usually compiled along with the application that uses it. That is not to say that a static or dynamically-linked library couldn't
 * be built to service several applications using the same configuration. But as a rule, and in all of the applications
 * and examples
 * given here, the library will always be compiled along with the application that uses it.
 *
 * The library is designed to be I/O free. It does not include the `<stdio.h>` library as long as the
 * tracing and/or statistics options are not used.
 * Input and output are left to the applications, but a number of helpers are available in the [utilities](\ref utilities) library.
 *
 * APG Version 7.0 has been written using data encapsulation and exception throwing techniques, emulating those
 * object-oriented aspects of C++. Most of the library's facilities,
 * as well as most all of the utilities,
 * are implemented as "classes" and "objects".
 * A discussion of exceptions, classes and objects, as used by APG, is given in [Appendix A.](\ref objects)
 *
 * At the lowest library level, all APG objects and facilities will make use of
 * [exceptions](\ref exception.c), [memory](\ref memory.c) and [vectors](\ref vector.c).
 *
 * ### Exceptions
 * `exception.h & exception.c` <br>
 * *All APG facilities &ndash; functions, objects, applications &ndash; report all fatal errors using exceptions.*
 * Any application, function or other program entity designed to use any APG library facility,
 * must declared and initialize and exception structure at its beginning scope.
 * Initialization is done with a "constructor"-type function, but since it is the application owns structure
 * there is no corresponding destructor. Any attempt to use a non-initialized exception structure will
 * result in the application's exit with a \ref BAD_CONTEXT exit code.
 * Once initialized, it can be used to create "try" and "catch" blocks of code. Use the \ref XCTOR() macro to simplify and standardize this.
 *
 * All APG objects will take a pointer to an initialized exception structure as a constructor argument
 * and all fatal errors are reported with "thrown exceptions" to the application's catch block.
 *
 * ### Memory
 *  `memory.h & memory.c`<br>
 * *All APG objects create and keep a memory object to handle all memory allocations.*
 * The memory object serves the purpose of controlled memory allocation. While it does not do automatic garbage collection
 * it does keep track of all memory allocated and frees it all when the memory object is destroyed.
 * Individual allocations can be freed on the fly, of course, but the memory object's primary feature is
 * keeping track of all of a parent object's allocations and freeing them in single step. Memory leaks are still entirely possible
 * and any application should use still a memory checking tool such as [valgrind](https://www.valgrind.org/) to check,
 * but the memory object should make leaks less likely.
 *
 * ### Vectors
 *  `vector.h & vector.c`<br>
 *  Second only to memory, the vector class is heavily used in APG. It is an open-ended array that will grow
 *  as necessary to hold all of the data added to it.
 *  The vector object is one of only a few APG objects that does not create and maintain its own memory object.
 *  It takes a memory object context pointer as a constructor argument and uses it for all memory allocation.
 *  While the vector object does have a destructor to free all memory that is has used,
 *  an application need not use it. Destruction of a memory object will also destroy all
 *  vectors that have been created using it.
 *
 *  NOTE: The memory and vector classes are the workhorses of APG. However, they are not speed efficient.
 *  Frequent allocations and frees should not be allowed in the inner loops or other bottlenecks of a working application.
 *  The idea is to allocate sufficient memory at set up to get the application running with none or only a few
 *  allocations and reallocations necessary as the application executes.
 *
 * ### Parser
 * `parser.h, parserp.h, parser.c, parser-get-init.c & parser-translate-init.c`<br>
 * The parser class is the primary purpose of the library. Everything else in the library is there to support the parser.
 *
 * The a parser object is generated from an SABNF grammar. Executed against an input string of alphabet characters it will
 * 1) determine if the input string satisfies the root, or start rule, of the SABNF grammar and
 * 2) identifies all, if any, of the sub-phrases of the input string which match the rules
 * and UDT operators referenced by the start rule.
 *
 * During the parser's recursive-descent journey through the complete parse tree, the user may request a trace of its path,
 * a set of statistics counting the number and type of nodes it visits and the construction of an Abstract Syntax Tree (AST)
 * which preserves a requested set of sub-phrases for later examination and/or translation.
 *
 * ### Operators
 * `operators.h, operators-abnf.c, operators-sabnf.c & operators-bkr.c`<br>
 * These are the functions that perform the node operations. There is a function for each of the 15 node types.
 * Each function interprets the data in the respective opcode and acts accordingly.
 * Here they are broken into separate files for a logical grouping of the operations.
 * The strictly RFC8254 ABNF operations are all in operators-abnf.c.
 * The back referencing operators are in operators-bkr.c.
 * All of the remaining operators are in operators-sabnf.c.
 *
 * ### Back Referencing - Universal Mode
 * `backref.h, backrefu.h & backrefu.c`<br>
 * To accommodate back referencing, the parser must call a number of functions at various points in the parsing process.
 * For universal-mode back referencing, these functions are all here.
 *
 * ### Back Referencing - Parent Mode
 * `backref.h, backrefp.h & backrefp.c`<br>
 * To accommodate back referencing, the parser must call a number of functions at various points in the parsing process.
 * For parent-mode back referencing, these functions are all here.
 *
 * ### AST - The Abstract Syntax Tree
 * `ast.h, astp.h & ast.c`<br>
 * One of the primary reasons for parsing a string of alphabet characters is to perform some application-specific transformation
 * of the sub-phrases found which depends on both the characters found *and* their position in the parse tree.
 * This can be done in real time during the parse with rule and UDT call back functions.
 * (See \ref vParserSetRuleCallback(), \ref vParserSetUdtCallback(), \ref parser_callback and \ref callback_data.)
 * However, real-time translations have a couple of problems that can be solved with
 * an Abstract Syntax Tree (AST):
 *  - back tracking will force a real-time translator to be able to undo successful nodes on a parse tree branch that ultimately fails
 *  - when traversing down through a node, a real-time translator doesn't know
 *    - if a sub-phrase will be matched, or
 *    - what the sub-phrase will be if it is discovered
 *
 * An AST saves a subset of the full parse tree for translation in a separate, later operation.
 * Only the needed rules are saved, all rule nodes correspond to successfully matched nodes
 * and the matched sub-phrase is known at all stages, up or down, of the traversal.
 *
 * If an Abstract Syntax Tree is needed, the facilities for capturing it and traversing (translating) it are here.
 *
 * See this [note](\ref astcallback) for a discussion of this distinction between
 * AST call back functions during the parsing and translation processes.
 *
 * ### Parser Tracing
 * `trace.h, tracep.h, trace.c, trace-config.c & trace-out.c`<br>
 * The trace object will display a record of information for every node the parser visits in its traversal
 * through the parse tree.
 * It is highly configurable. The user can display records for the full set of all nodes visited
 * or any subset thereof.
 * The display can be in plain ASCII or HTML format.
 *
 * Tracing is the primary debugging tool for the parser. If the parser does not produce the expected results,
 * the grammar. the input string or both may be in error.
 * A look at what the parser sees at each node will usually quickly identify the problem.
 *
 * To allow tracing the `APG_TRACE` macro must be defined when the application is compiled
 * ( e.g. gcc -DAPG_TRACE ...).
 * Since the trace is displayed to a file, this necessarily brings `<stdio.h>` into the application.
 *
 * ### Statistics
 * `stats.h, statsp.h & stats.c`<br>
 * This facility, enabled with the `APG_STATS` macro, collects node-hit statistics. These can be useful when
 * optimizing a grammar or parser.
 *
 * ### Miscellaneous Tools
 * `tools.h, tools.c`<br>
 * There are a few tools that are implemented as simple functions and used in multiple places around in APG.
 * They have been collected here.
 *
 */

