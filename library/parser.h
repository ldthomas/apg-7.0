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
#ifndef LIB_PARSER_H_
#define LIB_PARSER_H_

/** \file parser.h
 * \brief The SABNF parser's public header file.
 *
 * Defines only the macros, structures, typedefs, and function prototypes needed by the user of the parser.
 *
 */
/** \name ABNF Opcode Identifiers
 * These are the unique identifiers of the original 7 ABNF opcodes.
 */
/// \{
#define ID_ALT   1 /**< \brief alternation */
#define ID_CAT   2 /**< \brief concatenation */
#define ID_REP   3 /**< \brief repetition */
#define ID_RNM   4 /**< \brief rule name */
#define ID_TRG   5 /**< \brief terminal range */
#define ID_TBS   6 /**< \brief terminal binary string */
#define ID_TLS   7 /**< \brief terminal literal string */
/// \}
/** \name SABNF Superset Opcode Identifiers
 * These are the unique identifiers of the additional opcodes of the superset SABNF.
 */
/// \{
#define ID_UDT   8 /**< \brief user-defined terminal */
#define ID_AND   9 /**< \brief positive look ahead */
#define ID_NOT   10 /**< \brief negative look ahead */
#define ID_BKR   11 /**< \brief back reference to a previously matched rule or UDT name */
#define ID_BKA   12 /**< \brief positive look behind */
#define ID_BKN   13 /**< \brief negative look behind */
#define ID_ABG   14 /**< \brief anchor - beginning of string */
#define ID_AEN   15 /**< \brief anchor - end of string */
#define ID_GEN   19 /**< \brief general opcode (not SABNF). Serves to locate the ID in any opcode structure
and must be larger than all other opcode IDs.
One or more arrays may be created of size ID_GEN and data for all other opcodes must fit in the array.*/
/// \}
/** \name Parser State Indentifiers
 * These four identifiers are used to indicate the parser state
 * for any node operation in the parse tree.
 */
/// \{
#define ID_ACTIVE               20 /**< \brief indicates active parser state, parser has just entered the node and is moving down the parse tree */
#define ID_MATCH                21 /**< \brief indicates a matched phrase parser state on return from parse tree below this node */
#define ID_NOMATCH              22 /**< \brief indicates that no phrase was matched on return from parse tree below this node */
#define ID_EMPTY                23 /**< \brief indicates a matched empty phrase parser state on return from parse tree below this node */
/// \}
/** \name PPPT Map Values
 * These are the four possible values for a single character in a Partially-Predictive Parsing Table map.
 */
/// \{
#define ID_PPPT_NOMATCH         0 /**< \brief deterministic NOMATCH &ndash; there is no chance of a phrase match with this leading character */
#define ID_PPPT_MATCH           1 /**< \brief deterministic MATCH &ndash; this character constitutes a single character phrase match of length 1*/
#define ID_PPPT_EMPTY           2 /**< \brief deterministic EMTPY &ndash; this is an empty string match, the parse succeeds but the phrase length is 0 */
#define ID_PPPT_ACTIVE          3 /**< \brief non-deterministic &ndash; it is not possible to determine a match or not based on this character
                                     &ndash; the parser will have to do a full, normal parse to find out */
/// \}
/** \name Asymetric Syntax Tree (AST) States and Return Codes
 * These identifiers are used for the traversal direction and return codes in AST call back functions.
 */
/// \{
#define ID_AST_PRE              30 /**< \brief indicates pre-node-traversal AST callback state (down the tree) */
#define ID_AST_POST             31 /**< \brief indicates post-node-traversal AST callback state (up the tree) */
#define ID_AST_OK               32 /**< \brief normal AST callback function return */
#define ID_AST_SKIP             33 /**< \brief on return from AST callback, skip all nodes below (ignored on return from ID_AST_POST state) */
/// \}
/** \name Attribute Rule Types
 * These identifiers indicate the attribute classifications of the grammar's rules.
 */
/// \{
#define ID_ATTR_N               40 /**< \brief rule is non-recursive - never refers to itself */
#define ID_ATTR_R               41 /**< \brief rule is recursive - refers to itself, directly or indirectly, one or more times */
#define ID_ATTR_MR              42 /**< \brief rule is one of a mutually-recursive group (each rule in the group refers to itself and all others) */
/// \}
/** \name Look Ahead and Look Behind (Look Around) Indicators
 * The parser behaves somewhat differently when in a look around state.
 * That is, whether or not there is a look around opcode (AND(&), NOT(!), BKA(&&) or BKN(!!)) in the branch of the parse tree,
 */
/// \{
#define ID_LOOKAROUND_NONE      50 /**< \brief the parser presently is not in look around mode */
#define ID_LOOKAROUND_AHEAD     51 /**< \brief the parser presently is in look ahead mode */
#define ID_LOOKAROUND_BEHIND    52 /**< \brief the parser presently is in look behind mode */
/// \}
/** \name Back Reference Mode and Case Indicators
 * Identifies the case sensitivity and back reference mode of the back reference operator.
 * Back referencing can be either case-sensitive or insensitive and can be either universal- or parent-mode.
 */
/// \{
#define ID_BKR_MODE_U           60 /**< \brief the back reference is universal mode */
#define ID_BKR_MODE_P           61 /**< \brief the back reference is parent mode */
#define ID_BKR_CASE_S           62 /**< \brief the back reference is case sensitive */
#define ID_BKR_CASE_I           63 /**< \brief the back reference is case insensitive  */
/// \}

/** \struct callback_data
 * \brief The data struct passed to each callback function.
 *
 * It is the same for rule name (RNM)  and user-defined (UDT) callback functions.
 * Note that the callback function only sees the sub-string being parsed, not the entire string, if different.
 *
 * Only the variables designated [input/output] should be modified.
 * All other variables are read only.
 * The parser will fail or produce unpredictable results if any read-only variables are changed.
 */
typedef struct {
    // only the 3 members below are to be changed by user
    void* vpUserData;/**< \brief [input/output] User-defined data passed to to the parser in \ref parser_config.

    Ignored by Parser.*/
    aint uiCallbackState; /**< \brief [input/output]  Rule name (RNM) callback functions:
                        If ID_ACTIVE, the parser takes no action.
                        Otherwise the parser accepts this result and skips the sub-tree below the RNM operator.

                        User-defined (UDT) callback functions: Must be not be ID_ACTIVE.
                        The parser will throw an exception if the user returns ID_ACTIVE. */
    aint uiCallbackPhraseLength;/**< \brief [input/output] The phrase length of the matched phrase
                        if the callback function returns ID_MATCH.

                        If the callback function returns ID_ACTIVE, ID_EMPTY or ID_NOMATCH,
                        this value is ignored and assumed to be 0.
                        The parser will throw an exception if this value extends beyond the end of the string being parsed.

                        For UDT callback functions, the parser will throw an exception if the phrase length is 0
                        and the UDT is designated as non-empty (UDT name begins with `u_`). */

    // Parser will produce unpredictable results if user changes any of the values below.
    const achar* acpString; /**< \brief [read only] Pointer to the input sub-string, */
    aint uiStringLength; /**< \brief [read only] The input string length. */
    aint uiParserState; /**< \brief [read only] ID_ACTIVE if the parser is going down the tree.
                        ID_MATCH or ID_NOMATCH if coming up the tree. */
    aint uiParserOffset; /**< \brief  [read only] Offset from acpString to the first character to match */
    aint uiParserPhraseLength; /**< \brief [read only] The parser's matched phrase length if uiParserState is ID_MATCH or ID_NOMATCH.
                        0 otherwise. */
    aint uiRuleIndex; /**< \brief [read only] The rule index of this rule's callback. APG_UNDEFINED if this is a UDT callback. */
    aint uiUDTIndex; /**< \brief [read only] The UDT index of this UDT's callback. APG_UNDEFINED if this is a rule callback. */
    exception* spException; /**< \brief [read only] Use to throw exceptions back to the parser's catch block scope:
                    e.g. `XTHROW(spException, "my message")` */

    // For system use only.
    void* vpCtx; /**< \brief [read only] Do not use. For system use only.*/
    void* vpMem; /**< \brief [read only] Do not use. For system use only.*/
} callback_data;

/** \typedef parser_callback
 * \brief User-written callback function prototype.
 * \param spData Callback data supplied by the parser. See \ref callback_data for restrictions.
 * \return APG_SUCCESS on success, APG_FAILURE if any type of error occurs
 */
typedef void (*parser_callback)(callback_data* spData);

/** \struct parser_state
 * \brief The parser's final state.
 */
typedef struct{
    aint uiSuccess; /**< \brief True (>0) if the input string was matched *in its entirety*, false (0) otherwise. */
    aint uiState; /**< \brief One of \ref ID_EMPTY, \ref ID_MATCH or \ref ID_NOMATCH.
                Note that it is possible for the parser to match a phrase without matching the entire input string.
                In this case the state would be ID_MATCH or ID_EMPTY but uiSuccess would be false. */
    aint uiPhraseLength; /**< \brief Length of the matched phrase. */
    aint uiStringLength; /**< \brief Length of the input string. */
    aint uiMaxTreeDepth; /**< \brief The maximum tree depth reached during the parse. */
    aint uiHitCount; /**< \brief The number of nodes visited during the traversal of the parse tree. */
} parser_state;

/** \struct parser_config
 * \brief Defines the input string and other configuration parameters for the parser,
 *
 */
typedef struct {
    const achar* acpInput; ///< \brief Pointer to the input string.
    aint uiInputLength; ///< \brief Number of input string alphabet characters.
    aint uiStartRule; ///< \brief Index of the start rule. Any rule in the SABNF grammar may be used as the start rule.
    abool bParseSubString; ///< \brief If true (non-zero), only parse the defined  sub-string of the input string.
    aint uiSubStringBeg; /**< \brief The first character of the sub-string to parse.
                        Must be < uiInputLength or exception is thrown. */
    aint uiSubStringLength; /**< \brief The number of characters in the sub-string.
                        If 0, then the remainder of the string from uiSubStringBeg is used.
                        If uiSubStringBeg + uiSubStringLength > uiInputLength, then uiSubStringLength is truncated. */
    aint uiLookBehindLength; /**< \brief The maximum length to look behind for a match.
                        Use 0 or APG_INFINITE for infinite look behind. */
    void* vpUserData; /**< \brief Pointer to user data, if any.
                        Not examined or used by the parser in any way.
                        Presented to the user's callback functions. */
} parser_config;

void* vpParserCtor(exception* spException, void* vpParserInit);
void vParserDtor(void* vpCtx);
abool bParserValidate(void* vpCtx);
void vParserParse(void* vpCtx, parser_config* spConfig, parser_state* spState);
aint uiParserRuleLookup(void* vpCtx, const char* cpRuleName);
aint uiParserUdtLookup(void* vpCtx, const char* cpUdtName);
void vParserSetRuleCallback(void* vpCtx, aint uiRuleId, parser_callback pfnCallback);
void vParserSetUdtCallback(void* vpCtx, aint uiUdtId, parser_callback pfnCallback);

#endif /* LIB_PARSER_H_ */
