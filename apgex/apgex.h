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
/** \dir ./apgex
 * \brief The APG pattern-matching engine..
 */

/** \file apgex.h
 * \brief Header file for the apgex object.
 */
#ifndef APGEX_H_
#define APGEX_H_

#include "../api/api.h"

/** \struct apgex_phrase
 * \brief The representation of a matched phrase.
 */
typedef struct{
    apg_phrase sPhrase; ///< \brief The matched phrase.
    aint uiPhraseOffset; ///< \brief Offset into the source string where the matched phrase begins.
} apgex_phrase;

/** \struct apgex_rule
 * \brief Information about each rule or UDT in the SABNF pattern.
 *
 * Note that even though UDTs are terminal nodes, they have in common with rule nodes that they are named.
 * Therefore, the phrases for matched UDTs are presented together with the named rule phrases.
 * Note that recursive rules and rules that appear in the SABNF grammar in more than one place
 * may match multiple phrases.
 */
typedef struct{
    const char* cpRuleName; ///< \brief The rule or UDT name.
    apgex_phrase* spPhrases; ///< \brief The list of matched phrases. Any given rule or UDT may have multiple matched sub-phrases.
    aint uiPhraseCount; ///< \brief The number of matched sub-phrases for this rule/UDT.
    aint uiIndex; ///< \brief The rule or UDT grammar index.
} apgex_rule;

/** \struct apgex_result
 * \brief The phrase matching results.
 *
 * Note that all data pointers in this structure are, in general, valid only until the next function call on the `apgex` object.
 * If the application needs to retain any phrases or other pointer data for future use it must
 * make a copy of it into its own memory space.
 */
typedef struct{
    apgex_phrase* spResult; ///< \brief The matched phrase. NULL if no match.
    apgex_phrase* spLeftContext; /**< \brief The phrase prefix.

    This is the portion of the source string preceding the matched phrase. NULL if no match. */
    apgex_phrase* spRightContext; /**< \brief The phrase suffix.

    This is the portion of the source string following the matched phrase. NULL if no match. */
    apgex_rule* spRules; ///< \brief The phrases matched by all enabled rules and/or UDTs. NULL if no match.
    aint uiLastIndex; ///< \brief The last index following the last pattern match attempt.
    aint uiRuleCount; ///< \brief The number of combined rules and UDTs in the pattern.
    aint uiNodeHits; ///< \brief The number of parser node hits.
    aint uiTreeDepth; ///< \brief The maximum parsing tree depth.
} apgex_result;

/** \struct apgex_properties
 * \brief Detailed information about the `apgex` object after vApgexPattern() has been called.

 * Note that all data pointers in this structure are, in general, valid only until the next function call on the `apgex` object.
 * If the application needs to retain any phrases or other pointer data for future use it must
 * make a copy of it into its own memory space.
 */
typedef struct{
    void* vpParser; ///< \brief Pointer to the parser object context.
    void* vpAst; ///< \brief Pointer to the AST object context. NULL unless the `a` flag is used.
    void* vpTrace; ///< \brief Pointer to the trace object context. NULL unless the `t` flag is used.
    const char* cpFlags; ///< \brief The original string of flags from vApgexPattern().
    const char* cpPattern; /**< \brief Internally preserve copy of the SABNF grammar defining the string to match.
    NULL if the pattern is defined with vApgexPatternParser(). */
    apg_phrase sOriginalSource; ///< \brief The original source or input string as a phrase.
    apg_phrase sLastSource; ///< \brief The last source or input string as a phrase -
                            /// may be different from original if sApgexReplace() or sApgexReplaceFunc() has been called.
    apgex_phrase sLastMatch; /**< \brief The last-matched phrase. Same as `spResult`
    from the last call to sApgexExec(). */
    apgex_phrase sLeftContext; ///< \brief The left context of the last match - that is the phrase prefix to the matched phrase.
    apgex_phrase sRightContext; ///< \brief The right context of the last match - that is the phrase suffix to the matched phrase.
    aint uiLastIndex; /**< \brief The index of the character in the input string where the attempted pattern match begins.
    Use vApgexSetLastIndex() to manually override the defaults. */
    abool bDefaultMode; ///< \brief True if the `cpFlags` parameter in vApgexPattern() is NULL or empty.
    abool bGlobalMode; ///< \brief  True if the `g` flag is set prior to any occurrence of `y` in the `cpFlags` string.
    abool bPpptMode; ///< \brief  True if the `p` flag is set. The parser will use Partially-Predictive Parsing Tables.
    abool bStickyMode; ///< \brief True if the `y` flag is set prior to any occurrence of `g` in the `cpFlags` string.
    abool bTraceMode; ///< \brief True if the `t` flag is set in the `cpFlags` string.
    abool bTraceHtmlMode; ///< \brief True if the `th` flags ar set for HTML trace output in the `cpFlags` string.
} apgex_properties;

/** \typedef pfn_replace
 * \brief Prototype for the replacement function used by sApgexReplaceFunc().
 * \param spResult Pointer to a pattern-matching result - same as return from sApgexExec().
 * \param spProperties Pointer to a pattern-matching properties - same as returned from sApgexProperties().
 * \param vpUser Pointer to user-supplied data. Same user-supplied data passed to sApgexReplaceFunc().
 * \return The transformed phrase. That is, the source string with the specified matched phrase replacements.
 */
typedef apg_phrase (*pfn_replace)(apgex_result* spResult, apgex_properties* spProperties, void* vpUser);

/** @name Construction and Destruction */
///@{
void* vpApgexCtor(exception* spEx);
void vApgexDtor(void* vpCtx);
///@}

/** @name Pattern Definition */
///@{
void vApgexPattern(void* vpCtx, const char* cpPattern, const char* cpFlags);
void vApgexPatternFile(void* vpCtx, const char* cpFileName, const char* cpFlags);
void vApgexPatternParser(void* vpCtx, void* vpParser, const char* cpFlags);
///@}

/** @name Pattern Matching Configuration */
///@{
void vApgexEnableRules(void* vpCtx, const char* cpNames, abool bEnable);
void vApgexDefineUDT(void* vpCtx, const char* cpName, parser_callback pfnUdt);
void vApgexSetLastIndex(void* vpCtx, aint uiLastIndex);
///@}

/** @name Pattern Matching */
///@{
apgex_result sApgexExec(void* vpCtx, apg_phrase* spSource);
apg_phrase sApgexReplace(void* vpCtx, apg_phrase* spSource, apg_phrase* spReplacement);
apg_phrase sApgexReplaceFunc(void* vpCtx, apg_phrase* spSource, pfn_replace pfnFunc, void* vpUser);
apg_phrase* spApgexSplit(void* vpCtx, apg_phrase* spSource, aint uiLimit, aint* uipCount);
abool bApgexTest(void* vpCtx, apg_phrase* spSource);
apgex_properties sApgexProperties(void* vpCtx);
void* vpApgexGetAst(void* vpCtx);
void* vpApgexGetTrace(void* vpCtx);
void* vpApgexGetParser(void* vpCtx);
///@}

/** @name Display Helpers */
///@{
void vApgexDisplayResult(void* vpCtx, apgex_result* spResult, const char* cpFileName);
void vApgexDisplayPhrase(void* vpCtx, apgex_phrase* spPhrase, const char* cpFileName);
void vApgexDisplayProperties(void* vpCtx, apgex_properties* spProperties, const char* cpFileName);
void vApgexDisplayPatternErrors(void* vpCtx, const char* cpFileName);
///@}

void vApgexBkrCheck(exception* spEx);
/**
\page apgex apgex - An APG Pattern-Matching Engine
﻿
\anchor apgex_anchor_top
[Introduction](\ref apgex_anchor_intro)<br>
[Construction](\ref apgex_anchor_const)<br>
[Defining the SABNF Pattern](\ref apgex_anchor_pattern)<br>
[Configuring the Pattern Parser](\ref apgex_anchor_config)<br>
[Execution](\ref apgex_anchor_exec)<br>
 - [Finding a Matched Phrase](\ref apgex_anchor_find)<br>
 - [Test for a Matched Phrase](\ref apgex_anchor_test)<br>
 - [Replace the Matched Phrase](\ref apgex_anchor_replace)<br>
 - [Split - Matched Phrases as Delimiters](\ref apgex_anchor_split)<br>

[Properties](\ref apgex_anchor_props)<br>
[Trace - the Debugger](\ref apgex_anchor_trace)<br>
[Abstract Syntax Tree - the Matched Phrase AST](\ref apgex_anchor_ast)<br>
[Display Helpers](\ref apgex_anchor_display)<br>

\anchor apgex_anchor_intro
### Introduction
`apgex` is a regex-like pattern-matching engine which uses SABNF as the pattern-defining syntax and
APG as the pattern-matching parser.
While regex has a long and storied history and is heavily integrated into modern programming languages and practice,
`apgex` offers the full pattern-matching power of APG and the reader-friendly SABNF syntax.
It is fully recursive, meaning nested parenthesis matching,
and introduces a new [parent mode of back referencing](\ref parent_mode), enabling,
for example, the matching of names in nested start and end HTML tags.

Though not specifically designed and built to address the `regex` issues discussed [here by Larry Wall](https://raku.org/archive/doc/design/apo/A05.html),
creator of the Perl language, `apgex` does seem to go a long way toward addressing many of those issues.
Most notably back referencing and nested patterns, but other issues as well.

First introduced in JavaScript APG
[[1]](https://github.com/ldthomas/apg-js2-exp)
[[2]](https://www.npmjs.com/package/apg-exp)
 as an alternative to
[RegExp](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/RegExp) in 2017,
this extends it to the C-language and the updated APG version 7.0.
The full documentation can be found in the files apgex.h and apgex.c.<br>
[&uarr;top](\ref apgex_anchor_top)

\anchor apgex_anchor_const
### Construction/Destruction
`apgex` follows the [object](\ref object_model) model of APG.
Pattern-matching objects are created and destroyed with the constructor and destructor functions,
 - \ref vpApgexCtor()
 - \ref vApgexDtor().

[&uarr;top](\ref apgex_anchor_top)

\anchor apgex_anchor_pattern
### Defining the SABNF Pattern
There are three functions for defining the SABNF pattern.
The `apgex` pattern-matching parser can be generated on the fly from an SABNF pattern string or file
or from a previously constructed SABNF grammar parser.
  - From a pattern string \ref vApgexPattern()<br>
  A valid SABNF grammar is simply defined in a null-terminated string and used to define the pattern.
  - From a pattern file \ref vApgexPatternFile()<br>
  Same as above, but the SABNF grammar exists in a file rather than a string.
  - From a previously constructed parser \ref vApgexPatternParser()<br>
  An APG parser object may be created in advance elsewhere in the application and used here
  to define the pattern. Any method can be used to generate the parser in advance -
  from an APG-generated file or on the fly from the APG API.

[&uarr;top](\ref apgex_anchor_top)

\anchor apgex_anchor_config
### Configuring the Pattern Parser
`apgex` offers several ways to configure the pattern-matching parser prior to execution.
 - flags - See vApgexPattern() for a complete discussion of the available flag options
 and their effect on the operation of the parser.
 - vApgexEnableRules() - `apgex` has the ability to capture the sub-phrases matched by the individual
 SABNF grammr rules and UDTs. By default, all such capture is disabled.
 Use this function to enable or disable any or all rules and UDTs.
 - vApgexDefineUDT() - If there are any UDTs in the SABNF pattern grammar they must have callback functions assigned
 prior to any phrase-matching attempt. Use this function to define them.
 - vApgexSetLastIndex() - See the discussion of flags in vApgexPattern() for an explanation of the
 default values and use of `uiLastIndex`. The default values can be overridden prior to any phrase-matching attempt
 with this function.
 - sApgexProperties() - If special configuration of the tracing object is needed,
 the properties includes a pointer to the trace object. See vTraceConfig() for details.
 If the AST is requested it may need to be configured before use.
 The properties includes a pointer to the AST object. See \ref ast.c for details.

[&uarr;top](\ref apgex_anchor_top)

\anchor apgex_anchor_exec
### Execution
Once the `apgex` object is constructed and the pattern has been defined and configured,
the search for a matched phrase must be executed.
There are four functions which will execute a search. These will be discussed individually in the next four sub-sections.<br>
[&uarr;top](\ref apgex_anchor_top)

\anchor apgex_anchor_find
#### Finding a Matched Phrase
 sApgexExec()<br>
 This is the primary execution function which will
 generate detailed information about the matched phrase if successful.
 The results include, in addition to the matched phrase, the "left context", the "right context"
 and the sub-phrases captured for each of the enabled rule/UDT names.
 For a detailed description of the results, see the \ref apgex_result structure.<br>
[&uarr;top](\ref apgex_anchor_top)

\anchor apgex_anchor_test
#### Test for a Matched Phrase
 bApgexTest()<br>
 This will execute the parse just the same as sApgexExec() except that no matched phrases are captured.
 The return is simply `true` if a match is found and `false` if not.<br>
[&uarr;top](\ref apgex_anchor_top)

\anchor apgex_anchor_replace
#### Replace the Matched Phrase
 sApgexReplace() and sApgexReplaceFunc()<br>
 This will perform a phrase match similar to sApgexExec() except that instead of returning the matched phrase,
 the source string is returned with the matched phrase replaced by the defined replacement string.
 There is considerable flexibility in the definition of the replacement string.
 Anything from a simple string, a string that includes phrases from the matched results, or even
 a user-defined function with full access to the result and the current state properties.
 See the function descriptions for details.<br>
[&uarr;top](\ref apgex_anchor_top)

\anchor apgex_anchor_split
#### Split - Matched Phrases as Delimiters
spApgexSplit()<br>
 * This function is modeled after the JavaScript function
 * [str.split([separator[, limit]])](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String/split)
 * when using a regular expression.
 It will use the matched phrases as delimiters to split the input, source string into an array of sub-strings.
 See the function description for details.<br>
[&uarr;top](\ref apgex_anchor_top)

\anchor apgex_anchor_props
### Properties
Properties are the current state of the `apgex` object. In addition to the last phrase match, if any,
there are the flags, the original input string and SABNF grammar,
pointers to the parser, trace and AST object contexts and other information.
See the \ref apgex_properties structure for the properties details.
Use sApgexProperties() to get a copy of the properties.<br>
[&uarr;top](\ref apgex_anchor_top)

\anchor apgex_anchor_trace
### Trace - the Debugger
When a phrase match doesn't go as expected the problem could be an SABNF grammar error, an error in the input source string
or both. The primary debugging tool is a trace &ndash; a detailed map of the parser's path through the parser tree
with a display of the results for each tree node visit. APG provides just such a tool and it can be activated for the
phrase-matching parser. This is done by simply specifying the "t" or "th" flags and defining the macro APG_TRACE when compiling the application.
Use vpApgexGetTrace() to get a pointer to the trace object's context.
This pointer can then be used to configure the trace. See vTraceConfig()<br>
[&uarr;top](\ref apgex_anchor_top)

\anchor apgex_anchor_ast
### Abstract Syntax Tree - the Matched Phrase AST
It may be that a more complex translation of the matched phrases is needed than that provided by the
[replacement functions.](\ref apgex_anchor_replace)
The AST is the ultimate translation tool as discussed in the [library](\ref library) section.
The full capabilities of the AST library are available for the translation or manipulation of the matched phrases.
Use vpApgexGetAst() to get a pointer to the AST object's context.<br>
[&uarr;top](\ref apgex_anchor_top)

\anchor apgex_anchor_display
### Display Helpers
The matched results and properties have a lot of information &ndash;
sometimes in lists, and even in lists of lists. `apgex` offers three display helpers for a quick, easy look this data.
Phrases and other strings of alphabet characters are displayed as simple, ASCII strings if possible.
If they contain non-ASCII characters a [format object](\ref utils) is used for a hexadecimal display.
 - vApgexDisplayResult() - a quick look at the matched result.
 - vApgexDisplayProperties() - a quick look at the object properties.
 - vApgexDisplayPhrase() - a quick look at a single `apgex` phrase.

[&uarr;top](\ref apgex_anchor_top)
   */

#endif /* APGEX_H_ */
