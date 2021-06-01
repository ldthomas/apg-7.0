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
/** \file apgex.c
 * \brief Source code for the apgex phrase-matching engine.
 *
 * Must be compiled with the macro APG_AST defined. e.g. gcc -DAPG_AST ...
 *
 * If the trace flag, "t" or "th", is set (see vApgexPattern()), then the macro APG_TRACE must also be defined.
 */

#include "./apgex.h"
#include "../library/parserp.h"
#include "../library/tracep.h"

/** @name Internal Use Only Macros */
///@{
#define BUF_SIZE    (PATH_MAX + 256)
#define DOLLAR  36
#define AMP     38
#define ACCENT  96
#define APOS    39
#define LANGLE  60
#define RANGLE  62
#define UNDER   95
///@}

/** \struct phrase_r
 * \brief For internal object use only. Defines a phrase as an offset into vpVecRelPhases.
 */
typedef struct{
    aint uiSourceOffset; ///< \brief Offset into the source array for the first character of the phrase.
    aint uiLength; ///< \brief The length or number of characters in the phrase.
    aint uiNext; ///< \brief The index to the next phrase in a singly-linked list. APG_UNDEFINED marks last phrase in list.
} phrase_r;

/** \struct rule_r
 * \brief For internal object use only. Relative offsets to phrase information for rules.
 *
 * One struct for each rule. Handles all information needed by this rule.
 */
typedef struct{
    const char* cpRuleName; ///< \brief Rule name.
    aint uiRuleIndex; ///< \brief Rule index or index
    aint uiPhraseCount; ///< \brief The number of matched phrases.
    aint uiFirstPhrase; ///< \brief Offset in vpVecRelPhases for the first phrase of a singly-linked list of phrases matched by this rule.
    ///<  APG_UNDEFINED if no phrases for this rule exist.
    aint uiLastPhrase; ///< \brief Offset into vpVecRelPhrases for the last matched phrase. APG_UNDEFINED if no phrases for this rule exist.
    ///< Used only during the building of the singly-linked list.
    abool bEnabled; ///< \brief True if this rule has been enabled for phrase capture.
} rule_r;

/** \struct udt_r
 * \brief For internal object use only. Relative offsets to phrase information for UDTs.
 *
 * One struct for each UDT. Handles all information needed by this UDT.
 * Note that since these are user-defined functions, one function must be supplied with a call to \ref vApgexDefineUDT()
 * for every UDT in the grammar. Disabling the UDT simply means that its matched phrase will not be saved.
 * The callback function is still needed for successful parsing of the input string.
 */
typedef struct{
    const char* cpUdtName; ///< \brief UDT name.
    parser_callback pfnUdt; ///< \brief The UDT callback, if this is a UDT. Cannot be NULL;
    ///< Because these are user-defined functions, they must be supplied with a call to \ref vApgexDefineUDT().
    aint uiUdtIndex; ///< \brief Rule or UDT index or index
    aint uiPhraseCount; ///< \brief The number of matched phrases.
    aint uiFirstPhrase; ///< \brief Offset in vpVecRelPhases for the first phrase match for this rule.
    aint uiLastPhrase; ///< \brief Index into vpVecRelPhrases for the next match or APG_UNDEFINED if this is the last block.
    abool bEnabled; ///< \brief True if this UDT has been enabled for phrase capture.
} udt_r;

/** \struct result_r
 * \brief For internal object use only. The phrase matching result in relative phrases.
 */
typedef struct{
    phrase_r* spResult; ///< \brief
    phrase_r* spLeftContext; ///< \brief
    phrase_r* spRightContext; ///< \brief
    rule_r* spRules; ///< \brief
    udt_r* spUdts; ///< \brief
} result_r;

/** \struct apgex
 * \brief For internal object use only. The phrase matching object context.
 */
typedef struct{
    const void* vpValidate; ///< \brief Must be the "magic number" to be a valid context.
    exception* spException; ///< \brief Pointer to the exception structure
                            /// for reporting errors to the application catch block.
    void* vpMem; ///< \brief Pointer to a memory object used for all memory allocations.
    void* vpVecSource; ///< \brief
    void* vpVecOriginalSource; ///< \brief Vector to hold the original source with no replacements.
    void* vpVecPattern; ///< \brief Vector for the pattern string, if any.
    void* vpVecFlags; ///< \brief Vector for the input flags string.
    void* vpVecRules; ///< \brief Vector of rule structures.
    void* vpVecUdts; ///< \brief Vector of UDT structures
    void* vpVecStrings; ///< \brief Vector for string scratch space.
    void* vpVecPhrases; ///< \brief Vector of matched phrases.
    void* vpVecRelPhrases; ///< \brief Vector of relative phrases - offsets rather than absolute pointers.
    void* vpVecRelRules; ///< \brief Vector of relative rules.
    void* vpVecRelUdts; ///< \brief Vector of relative UDTs.
    void* vpVecReplaceRaw; ///< \brief Vector for the original replacement string with no modifications.
    void* vpVecReplacement; ///< \brief Vector for the final form of the replacement string.
    void* vpVecSplitPhrases; ///< \brief Vector for the resulting phrases of spApgexSplit().
    void* vpParser; ///< \brief Pointer to the SABNF grammar parser.
    void* vpApi; ///< \brief Pointer to the API object used to generate the parser.
    void* vpFmt; ///< \brief Pointer to a [format](\ref format.c) object used for display.
    void* vpAst; ///< \brief Pointer to the AST object if any.
    void* vpTrace; ///< \brief Pointer to the trace object if any.
    void* vpExternalParser; ///< \brief Pointer to the externally-supplied parsed, if any.
    FILE* spDisplay; ///< \brief The open file for display - may be stdout.
    apgex_phrase* spLastMatch; ///< \brief Pointer to the last matched result.
    apgex_phrase* spLeftContext; ///< \brief Pointer to the left context of the last result.
    apgex_phrase* spRightContext; ///< \brief Pointer to the right context of the last result.
    rule_r* spRelRules; ///< \brief Pointer to the relative rules in the vector of relative rules.
    udt_r* spRelUdts; ///< \brief Pointer to the relative UDTs in the vector of relative UDTs.
    aint uiRuleCount; ///< \brief Number of rules in the SABNF pattern grammar.
    aint uiUdtCount; ///< \brief Number of UDTs in the SABNF pattern grammar.
    aint uiEnabledRuleCount; ///< \brief Number of enabled rules.
    aint uiEnabledUdtCount; ///< \brief Number of enabled UDTs.
    aint uiLastIndex; ///< \brief Last index - the offset to the first character to begin the search for a pattern match.
    aint uiNodeHits; ///< \brief Number of node hits in the pattern-matching parse.
    aint uiTreeDepth; ///< \brief Maximum tree depth reached in the pattern-matching parse.
    abool bReplaceMode; ///< \brief True if in replace mode.
    abool bDefaultMode; ///< \brief True if in default mode.
    abool bTraceMode; ///< \brief True if tracing is requested.
    abool bTraceHtmlMode; ///< \brief True if tracing in HTML mode is requested.
    abool bGlobalMode; ///< \brief True if in global mode.
    abool bPpptMode; ///< \brief True if PPPTs are used.
    abool bStickyMode; ///< \brief True if in sticky mode.
} apgex;

static const void* s_vpMagicNumber = (const void*)"apgex";
static const char* s_cpNoPattern = "No pattern or properties defined yet. This function call must be preceded by\n"
        "vApgexPattern(), vApgexPatternFile() or vApgexPatternParser()";
static char s_cZero = 0;
static char* s_cpExternalPattern = "<external>";
static const char* s_cpEmptySource = "source cannot be NULL or empty";
static char* s_cpEndian = "???";

static inline void vMakeAbsPhrase(apgex* spExp, phrase_r* spRelPhrase, apgex_phrase* spAbsPhrase);
static inline void vMakeRelPhrase(apgex* spExp, aint uiSourceOffset, aint uiLen, phrase_r* spPhrase);
static inline char* cpBool(abool bVal);
static void vClearForParse(apgex* spExp);
static void vClearForPattern(apgex* spExp);
static void vExecResult(apgex* spExp, apgex_result* spResult);
static abool bExecTest(apgex* spExp);
static void vMatchResult(apgex* spExp, parser_config* spConfig, parser_state* spState, apgex_result* spResult);
static void vMatchDefault(apgex* spExp, parser_config* spConfig, apgex_result* spResult);
static void vMatchGlobal(apgex* spExp, parser_config* spConfig, apgex_result* spResult);
static void vMatchSticky(apgex* spExp, parser_config* spConfig, apgex_result* spResult);
static abool bTestDefault(apgex* spExp, parser_config* spConfig);
static abool bTestGlobal(apgex* spExp, parser_config* spConfig);
static abool bTestSticky(apgex* spExp, parser_config* spConfig);
static void vReplacement(apgex* spExp, apgex_result* spResult);
static void vReplaceFunc(apgex* spExp, apgex_result* spResult, pfn_replace pfnFunc, void* vpUser);
static void vReplace(apgex* spExp, apgex_result* spResult);
static rule_r* spFindRule(apgex* spExp, const char* cpName);
static udt_r* spFindUdt(apgex* spExp, const char* cpName);
static aint pfnRuleCallback(ast_data* spData);
static aint pfnUdtCallback(ast_data* spData);
//static void pfnRuleCallback(callback_data* spData);
//static void pfnUdtCallback(callback_data* spData);
static void vPrintPhrase(apgex* spExp, apgex_phrase* spPhrase, FILE* spOut);
static void vDecodeFlags(apgex* spExp, const char* cpFlags);
static void vConstructParser(apgex* spExp);
static void vInitRules(apgex* spExp);
static void vInitCallbacks(apgex* spExp);
static void vResetCallbacks(apgex* spExp);
static abool bIsNameChar(char cChar);
static void vNamePhrase(apgex* spExp, achar* acpName, aint uiNameLen, apg_phrase* spPhrase);

/** @name Tracing Macros
 * These macros handle the tracing calls. They are implemented as macros so that
 * when tracing is not used all tracing code is removed - the macros are empty.
 * To enable tracing the macro APG_TRACE must be defined when the application is compiled.
 */
///@{
#ifdef APG_TRACE
#define TRACE_APGEX_HEADER(t) vTraceApgexHeader((t))
#define TRACE_APGEX_FOOTER(t) vTraceApgexFooter((t))
#define TRACE_APGEX_SEPARATOR(x)  vTraceApgexSeparator((x)->vpTrace, (x)->uiLastIndex)
#define TRACE_APGEX_CHECK(x)
#define TRACE_APGEX_OUTPUT(x) vTraceApgexOutput((x))
static void vTraceApgexOutput(apgex *spExp) {
    if (spExp->bTraceMode) {
        spExp->vpTrace = vpTraceCtor(spExp->vpParser);
        vTraceApgexType(spExp->vpTrace, TRACE_HEADER_APGEX);
        if (spExp->bTraceHtmlMode) {
            vTraceOutputType(spExp->vpTrace, TRACE_HTML);
        }
    }
}
#else
#define TRACE_APGEX_HEADER(t)
#define TRACE_APGEX_FOOTER(t)
#define TRACE_APGEX_SEPARATOR(x)
#define TRACE_APGEX_CHECK(x) vTraceApgexCheck((x))
#define TRACE_APGEX_OUTPUT(x)
static void vTraceApgexCheck(apgex* spExp){
    if(spExp->bTraceMode){
        XTHROW(spExp->spException, "to use the 't' or 'th' flag the application must be compiled with APG_TRACE defined");
    }
}
#endif
///@}

/** \brief The phrase-matching engine object constructor.
 * \param spEx Pointer to a valid exception structure initialized with \ref XCTOR().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Returns a pointer to the apgex object context.
 */
void* vpApgexCtor(exception* spEx){
    if(!bExValidate(spEx)){
        vExContext();
    }
    BKR_APGEX_CHECK(spEx);
    void* vpMem = vpMemCtor(spEx);
    apgex* spExp = (apgex*) vpMemAlloc(vpMem, sizeof(apgex));
    memset((void*) spExp, 0, sizeof(apgex));
    spExp->vpMem = vpMem;
    spExp->spException = spEx;
    spExp->vpApi = vpApiCtor(spExp->spException);
    spExp->vpVecStrings = vpVecCtor(vpMem, sizeof(char), 1024);
    spExp->vpVecSource = vpVecCtor(vpMem, sizeof(achar), 1024);
    spExp->vpVecOriginalSource = vpVecCtor(vpMem, sizeof(achar), 16);
    spExp->vpVecPattern = vpVecCtor(vpMem, sizeof(char), 4096);
    spExp->vpVecFlags = vpVecCtor(vpMem, sizeof(char), 32);
    spExp->vpVecRules = vpVecCtor(vpMem, sizeof(apgex_rule), 512);
    spExp->vpVecUdts = vpVecCtor(vpMem, sizeof(apgex_rule), 64);
    spExp->vpVecPhrases = vpVecCtor(vpMem, sizeof(apgex_phrase), 1024);
    spExp->vpVecRelPhrases = vpVecCtor(vpMem, sizeof(phrase_r), 1024);
    spExp->vpVecRelRules = vpVecCtor(vpMem, sizeof(rule_r), 512);
    spExp->vpVecRelUdts = vpVecCtor(vpMem, sizeof(rule_r), 64);
    spExp->vpVecReplaceRaw = vpVecCtor(vpMem, sizeof(achar), 1024);
    spExp->vpVecReplacement = vpVecCtor(vpMem, sizeof(achar), 1024);
    spExp->vpVecSplitPhrases = vpVecCtor(vpMem, sizeof(apg_phrase), 128);
    spExp->vpFmt = vpFmtCtor(spEx);
    spExp->bDefaultMode = APG_TRUE;
    spExp->vpValidate = s_vpMagicNumber;
    s_cpEndian = bIsBigEndian() ? "big" : "little";
    return (void*) spExp;
}

/** \brief Back referencing check.
 *
 * The back referencing macro APG_BKR must be defined when compiling the apgex object.
 * If not, a macro is defined in apg.h which calls this function,
 * whose only purpose is to throw an exception reminding the user do define the macro.
 * \param spEx Pointer to a valid exception structure.
 * If not valid application will silently exit with a \ref BAD_CONTEXT exit code.
 */
void vApgexBkrCheck(exception* spEx){
    if(!bExValidate(spEx)){
        vExContext();
    }
    XTHROW(spEx, "apgex must be compiled with the macro APG_BKR defined.");
}

/** \brief The phrase-matching engine object destructor.
 *
 * Frees all memory allocated to the `apgex` object and for good measure
 * zeros it out as insurance against reuse by a stale pointer.
 *
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * Silently ignores a NULL pointer, but exits the application with \ref BAD_CONTEXT exit code
 * if the pointer is non-NULL and not a valid apgex object context pointer.
 */
void vApgexDtor(void *vpCtx) {
    if(vpCtx){
        apgex* spExp = (apgex*) vpCtx;
        if(spExp->vpValidate != s_vpMagicNumber){
                vExContext(); // does not return
        }
        void* vpMem = spExp->vpMem;
        if(!spExp->vpExternalParser){
            // destroy the local parser, which destroys AST and trace object along with it
            vParserDtor(spExp->vpParser);
        }
        if(spExp->spDisplay && (spExp->spDisplay != stdout)){
            fclose(spExp->spDisplay);
            spExp->spDisplay = NULL;
        }
        vApiDtor(spExp->vpApi);
        vFmtDtor(spExp->vpFmt);
        memset(vpCtx, 0, sizeof(apgex));
        vMemDtor(vpMem);
    }
}

// /** \brief Validates an `apgex` context pointer.
// * \param vpCtx The pointer to validate.
// * \return Returns true if the pointer is valid, false otherwise.
// */
//abool bApgexValidate(void* vpCtx){
//    apgex* spExp = (apgex*) vpCtx;
//    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
//            return APG_FALSE;
//    }
//    return APG_TRUE;
//}

/** \brief Prepare a phrase-matching parser for the given pattern.
 *
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpPattern The complete SABNF grammar to define the strings to be matched.
 * Must be a complete grammar including line end characters (\\n, \\r\\n or \\r) after each line including the last.
 * \param cpFlags A string of flags that control the pattern-matching behavior.
 * The flag characters may appear in any order and may appear multiple times.
 * If "g" and "y" are both present, the first appearing will be honored.
 *  - NULL or empty string - default mode<br>
 *  The phrase matching starts at `uiLastIndex`<sup>&dagger;</sup> and searches forward until a match is found or the end of the source string is reached.
 *  `uiLastIndex` is then always reset to zero.
 *  - g - global mode<br>
 *  The phrase matching starts at `uiLastIndex` and searches forward until a match is found or the end of the source string is reached.
 *  If a match is found, `uiLastIndex` is then set to the next character after the matched phrase.
 *  Multiple calls to sApgexExec() in global mode will find all matched phrases in the source string.
 *  `uiLastIndex` is set to zero when the end of the source string is reached.
 *  If no match is found, `uiLastIndex` is set to zero, regardless of its original value.
 *  - y - sticky mode<br>
 *  Sticky mode is similar to global mode except that there is no searching for a matched phrase.
 *  It is either found at `uiLastIndex` or the match fails.
 *  In detail, the phrase matching starts at `uiLastIndex`.
 *  If a phrase match is found, `uiLastIndex` is set to the next character beyond the end of the matched phrase
 *  or zero if the end of the source string has been reached.
 *  If no match is found no further searching of the source string is done and `uiLastIndex` is reset to zero.
 *  Note that multiple calls to sApgexExec() will match multiple *consecutive* phrases, but only if there are no
 *  unmatched characters in between.
 *  - p - PPPT mode<br>
 *  The parser will use Partially-Predictive Parsing Tables.
 *  PPPTs are for the purpose of speeding the parsing process at the expense of some added memory for the tables.
 *  Sometimes a lot of memory and in fact sometimes a prohibitive amount of memory.
 *  Since speed is often not a major concern in pattern matching, PPPTs are not used by default.
 *  Make sure you understand the memory consequences and the fact that some rules will not appear in the
 *  matched rules list if using the "p" flag. If the "p" flag is not set, compiling the application
 *  with the macro APG_NO_PPPT defined will reduce the code footprint and save some PPPT checking calls.
 *  - t - trace mode<br>
 *  A trace of the pattern-matching parser will be generated in ASCII format.
 *  By default, the trace is displayed on the standard output, `stdout`.
 *  To change the output file, get a pointer to the trace object with vpApgexGetTrace() and
 *  use vTraceSetOutput().
 *  For additional trace configuration see also vTraceConfig() and vTraceConfigDisplay().
 *  To use the "t" flag, the application must be compiled with the macro APG_TRACE defined.
 *  - h - trace HTML mode<br>
 *  A trace of the pattern-matching parser will be generated in HTML format.
 *  This flag must be preceded by the "t" flag or an exception is thrown.
 *
 *  <sup>&dagger;</sup>By default, `uiLastIndex` begins at 0. However, it can be set to any valid value prior
 *  to the phrase matching attempt with a call to vApgexSetLastIndex()
 */
void vApgexPattern(void* vpCtx, const char* cpPattern, const char* cpFlags){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext(); // does not return
    }
    vClearForPattern(spExp);
    vDecodeFlags(spExp, cpFlags);

    // save a copy of the pattern & flags including the null terminator
    vpVecPushn(spExp->vpVecPattern, (void*)cpPattern, (aint)(strlen(cpPattern) + 1));

    // construct the parser
    vConstructParser(spExp);

    // initialize the rule list
    vInitRules(spExp);
}

/** \brief Reads the SABNF grammar defining the pattern from a file.
 *
 * Same as vApgexPattern() except the pattern grammar is read from a file.
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpFileName Name of the file with the SABNF grammar.
 * \param cpFlags A string of flags. See vApgexPattern().
 */
void vApgexPatternFile(void* vpCtx, const char* cpFileName, const char* cpFlags){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext(); // does not return
    }
    if(!cpFileName || (strlen(cpFileName) == 0)){
        XTHROW(spExp->spException, "cpFileName cannot be NULL or empty");
    }
    vClearForPattern(spExp);
    vDecodeFlags(spExp, cpFlags);

    // get the pattern
    aint uiSize = 1024;
    aint uiPatternSize = uiSize;
    vUtilFileRead(spExp->vpMem, cpFileName, NULL, &uiPatternSize);
    uint8_t* ucpPattern = (uint8_t*)vpVecPushn(spExp->vpVecPattern, NULL, (uiPatternSize + 1));
    vUtilFileRead(spExp->vpMem, cpFileName, ucpPattern, &uiPatternSize);
    ucpPattern[uiPatternSize] = 0;

    // construct the parser
    vConstructParser(spExp);

    // initialize the rule list
    vInitRules(spExp);
}

/** \brief Define the SABNF pattern with a user-created parser.
 *
 * The SABNF pattern is implicitly defined by a user-supplied parser.
 * This parser is independent of the `apgex` context and it's destructor is never called,
 * even by vApgexDtor(). The supplied parser may or may not have been created with the same
 * memory object, vpMem. It is the user's responsibility to have a properly defined `catch block`
 * to handle any exceptions thrown from the supplied parser.
 * In this case, the `apgex` properties will have an empty string for the unknown pattern.
 *
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param vpParser Pointer to a valid SABNF parser context.
 * \param cpFlags A string of flags. See vApgexPattern().
 */
void vApgexPatternParser(void* vpCtx, void* vpParser, const char* cpFlags){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext(); // does not return
    }
    if(!vpParser){
        XTHROW(spExp->spException, "vpParser cannot be NULL");
    }
    if(!bParserValidate(vpParser)){
        XTHROW(spExp->spException, "vpParser is not a pointer to a valid parser object context");
    }
    vClearForPattern(spExp);
    vDecodeFlags(spExp, cpFlags);

    // save an empty pattern
    vpVecPushn(spExp->vpVecPattern, s_cpExternalPattern, (aint)(strlen(s_cpExternalPattern) + 1));

    // use the specified parser
    spExp->vpExternalParser = vpParser;
    spExp->vpParser = vpParser;
    spExp->vpAst = vpAstCtor(spExp->vpParser);

    // initialize the rule list
    vInitRules(spExp);
}

/** \brief Attempt a pattern match on the source array of APG alphabet characters.
 *
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spSource Pointer to the source or input string as an APG phrase.
 * \return An apgex_result structure.
 * Note that the return is a structure and not a pointer to a structure.
 * The `spResult` element in the result structure will be NULL if no match was found.
 */
apgex_result sApgexExec(void* vpCtx, apg_phrase* spSource){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!spExp->vpParser){
        XTHROW(spExp->spException, s_cpNoPattern);
    }
    if(!(spSource && spSource->acpPhrase && spSource->uiLength)){
        XTHROW(spExp->spException, s_cpEmptySource);
    }
    apgex_result sResult = {};
    vClearForParse(spExp);
    vpVecPushn(spExp->vpVecSource, (void*)spSource->acpPhrase, spSource->uiLength);

    vInitCallbacks(spExp);
    vExecResult(spExp, &sResult);
    vResetCallbacks(spExp);
    return sResult;
}

/** \brief Replace the matched phrase with a specified phrase.
 *
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spSource Pointer to the source as a \ref apg_phrase.
 * \param spReplacement Pointer to the replacement phrase.
 * In default mode, only the first matched phrase is replaced.
 * In global or sticky mode, all possible matched phrases in those respective modes are replaced.
 * The search begins at `uiLastIndex` which is always set to zero on return.
 * The replacement phrase may have
 * some special characters for dynamic replacement possibilities.
 *  - no special characters<br>
 *  Each matched phrase is simply replaced with the specified replacement string.
 *  - $$<br>
 *  Escape sequence to insert a dollar sign, $, in the replacement string.
 *  - $_<br>
 *  Replace $_ with the full, original source string.
 *  - $&<br>
 *  Replace $& with the current matched phrase.
 *  - $\`<br>
 *  Replace $\` with the left context of the current matched phrase.
 *  - $'<br>
 *  Replace $' with the right context of the current matched phrase.
 *  - $<rulename><br>
 *  Replace $<rulename> with the matched phrase for the rule or UDT name "rulename".
 * \return The source phrase with replacements, if any, as an apg_phrase structure.
 * Note that the return is a structure and not a pointer to a structure.
 */
apg_phrase sApgexReplace(void* vpCtx, apg_phrase* spSource, apg_phrase* spReplacement){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!(spSource && spSource->acpPhrase && spSource->uiLength)){
        XTHROW(spExp->spException, s_cpEmptySource);
    }
    apg_phrase sPhrase = {};
    apgex_result sResult = {};
    vClearForParse(spExp);
    vInitCallbacks(spExp);
    spExp->bReplaceMode = APG_TRUE;
    vpVecPushn(spExp->vpVecSource, (void*)spSource->acpPhrase, spSource->uiLength);
    vpVecPushn(spExp->vpVecOriginalSource, (void*)(void*)spSource->acpPhrase, spSource->uiLength);
    if(spReplacement && spReplacement->acpPhrase && spReplacement->uiLength){
        vpVecPushn(spExp->vpVecReplaceRaw, (void*)spReplacement->acpPhrase, spReplacement->uiLength);
    }
    if(spExp->bDefaultMode){
        vExecResult(spExp, &sResult);
        if(sResult.spResult){
            vReplacement(spExp, &sResult);
            vReplace(spExp, &sResult);
        }
    }else{
        vExecResult(spExp, &sResult);
        while(sResult.spResult){
            vReplacement(spExp, &sResult);
            vReplace(spExp, &sResult);
            vExecResult(spExp, &sResult);
        }
    }
    spExp->uiLastIndex = 0;
    sPhrase.acpPhrase = (achar*)vpVecFirst(spExp->vpVecSource);
    sPhrase.uiLength = uiVecLen(spExp->vpVecSource);
    vResetCallbacks(spExp);
    return sPhrase;
}

/** \brief Replace the matched phrase with a user-generated phrase.
 *
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spSource Pointer to the source as a \ref apg_phrase.
 * \param pfnFunc Pointer to the replacement function. See \ref pfn_replace for the function prototype.
 * The returned phrase from this function will be used as the replacement for the matched phrase.
 * In default mode, only the first matched phrase is replaced.
 * In global or sticky mode, all possible matched phrases in those respective modes are replaced.
 * The search begins at `uiLastIndex` which is always set to zero on return.
 * \param vpUser Pointer to user-supplied data. This pointer will be passed to the above function, pfnFunc.
 * \return The source phrase with replacements, if any, as an apg_phrase structure.
 * Note that the return is a structure and not a pointer to a structure.
 */
apg_phrase sApgexReplaceFunc(void* vpCtx, apg_phrase* spSource, pfn_replace pfnFunc, void* vpUser){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!(spSource && spSource->acpPhrase && spSource->uiLength)){
        XTHROW(spExp->spException, s_cpEmptySource);
    }
    if(!pfnFunc){
        XTHROW(spExp->spException, "pfnFunc cannot be NULL");
    }
    vClearForParse(spExp);
    vInitCallbacks(spExp);
    spExp->bReplaceMode = APG_TRUE;
    vpVecPushn(spExp->vpVecSource, (void*)spSource->acpPhrase, spSource->uiLength);
    vpVecPushn(spExp->vpVecOriginalSource, (void*)(void*)spSource->acpPhrase, spSource->uiLength);
    apg_phrase sPhrase = {};
    apgex_result sResult = {};
    if(spExp->bDefaultMode){
        vExecResult(spExp, &sResult);
        if(sResult.spResult){
            vReplaceFunc(spExp, &sResult, pfnFunc, vpUser);
            vReplace(spExp, &sResult);
        }
    }else{
        vExecResult(spExp, &sResult);
        while(sResult.spResult){
            vReplaceFunc(spExp, &sResult, pfnFunc, vpUser);
            vReplace(spExp, &sResult);
            vExecResult(spExp, &sResult);
        }
    }
    spExp->uiLastIndex = 0;
    sPhrase.acpPhrase = (achar*)vpVecFirst(spExp->vpVecSource);
    sPhrase.uiLength = uiVecLen(spExp->vpVecSource);
    vResetCallbacks(spExp);
    return sPhrase;
}

/** \brief Split a phrase into an array of sub-phrases.
 *
 * This function is modeled after the JavaScript function
 * [str.split([separator[, limit]])](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String/split)
 * when using a regular expression.
 * The source phrase is searched for pattern matches.
 *  - If a single match is found, it's left context and right context become the two members of the array of sub-phrases returned.
 *  - If multiple matches are found the array of sub-phrases are those sub-phrases remaining after removing the matched characters.
 *  - If no match is found the array has a single member which is the original source phrase.
 *  - If the entire source phrase is matched a single, empty sub-phrase is returned (acpPhrase = NULL, uiLength = 0).
 *  - If the pattern is an empty string (pattern = ""\\n), each character in the source phrase is returned as a separate sub-phrase.

 * Also,
 *  - The flags, "gy", are ignored.
 *  - The flags "thp" are honored.
 *  - All rules and UDTs are disabled, even if previously enabled with vApgexEnableRules().
 *  - uiLastIndex is set to 0, even if previously set with vApgexSetLastIndex();
 *
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spSource A pointer to the source to search for pattern matches.
 * May not be NULL. If it is empty (uiLength = 0) the return will be a single empty phrase.
 * \param uiLimit Places a limit on the number of pattern matches to find.
 * If uiLimit = 0, all matches will be found. That is, uiLimit = 0 is shorthand for uiLimit = APG_MAX_AINT.
 * \param uipCount The number of sub-phrases in the returned array.
 * \return Returns a pointer to the first sub-string in the array.
 * The number of array members is returned in uipCount.
 */
apg_phrase* spApgexSplit(void* vpCtx, apg_phrase* spSource, aint uiLimit, aint* uipCount){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!(spSource && spSource->acpPhrase && spSource->uiLength)){
        XTHROW(spExp->spException, s_cpEmptySource);
    }
    if(!uipCount){
        XTHROW(spExp->spException, "the returned array count cannot be NULL");
    }

    // ensure all UDTs are defined
    aint ui;
    for(ui = 0; ui < spExp->uiUdtCount; ui++){
        if(!spExp->spRelUdts[ui].pfnUdt){
            char caBuf[BUF_SIZE];
            snprintf(caBuf, BUF_SIZE, "UDT \"%s\" not defined", spExp->spRelUdts[ui].cpUdtName);
            XTHROW(spExp->spException, caBuf);
        }
    }
    vClearForParse(spExp);
    parser_config sConfig = {};
    apgex_result sResult = {};
    apg_phrase* spReturn = NULL;
    apg_phrase sPhrase;
    aint uiLen;
    aint uiBegin = 0;
    spExp->uiLastIndex = 0;
    uiLimit = uiLimit ? uiLimit : APG_MAX_AINT;

    // save the source
    vpVecPushn(spExp->vpVecSource, (void*)spSource->acpPhrase, spSource->uiLength);

    // disable all rules/UDTs
    spExp->uiEnabledRuleCount = 0;
    spExp->uiEnabledUdtCount = 0;
    for(ui = 0; ui < spExp->uiRuleCount; ui++){
        spExp->spRelRules[ui].bEnabled = APG_FALSE;
    }
    for(ui = 0; ui < spExp->uiUdtCount; ui++){
        spExp->spRelUdts[ui].bEnabled = APG_FALSE;
    }

    if(spExp->vpTrace){
        // initalize the trace
        TRACE_APGEX_HEADER(spExp->vpTrace);
    }

    // initialize for the parser input
    sConfig.acpInput = (achar*)vpVecFirst(spExp->vpVecSource);
    sConfig.uiInputLength = uiVecLen(spExp->vpVecSource);
    sConfig.bParseSubString = APG_TRUE;
    sConfig.uiSubStringBeg = spExp->uiLastIndex;
    sConfig.vpUserData = (void*)spExp;

    // look for phrase matches
    sResult.spResult = (apgex_phrase*)1;
    while(uiLimit && sResult.spResult){
        sConfig.uiSubStringBeg = spExp->uiLastIndex;
        uiBegin = spExp->uiLastIndex;
        vMatchGlobal(spExp, &sConfig, &sResult);
        if(sResult.spResult){
            // push left context on the sub-phrase array
            // make a phrase from the interval [uiBegin, uiLastIndex)
            uiLen = sResult.spResult->sPhrase.uiLength ? sResult.spLeftContext->sPhrase.uiLength - uiBegin : 1;
            if(uiLen){
                sPhrase.acpPhrase = (achar*)vpVecFirst(spExp->vpVecSource) + uiBegin;
                sPhrase.uiLength = uiLen;
                vpVecPush(spExp->vpVecSplitPhrases, (void*)&sPhrase);
            }
        }else{
            // push right context on the sub-phrase array
            uiLen = sConfig.uiInputLength - uiBegin;
            if(uiLen){
                sPhrase.acpPhrase = (achar*)vpVecFirst(spExp->vpVecSource) + uiBegin;
                sPhrase.uiLength = uiLen;
                vpVecPush(spExp->vpVecSplitPhrases, (void*)&sPhrase);
            }
        }
        uiLimit--;
    }

    if(spExp->vpTrace){
        // finalize the trace
        TRACE_APGEX_FOOTER(spExp->vpTrace);
        TRACE_DTOR(spExp->vpTrace);
        spExp->vpTrace = NULL;
    }
    spReturn = (apg_phrase*)vpVecFirst(spExp->vpVecSplitPhrases);
    *uipCount = uiVecLen(spExp->vpVecSplitPhrases);
    vResetCallbacks(spExp);
    return spReturn;
}

/** \brief Report only success or failure on a pattern match.
 *
 * Similar to sApgexExec() in default mode, except that only success or failure is reported.
 * The matched phrase is not returned.
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spSource Pointer to the source or input string as an APG phrase.
 * \return True if a phrase match was found in the source, false otherwise.
 */
abool bApgexTest(void* vpCtx, apg_phrase* spSource){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!(spSource && spSource->acpPhrase && spSource->uiLength)){
        XTHROW(spExp->spException, s_cpEmptySource);
    }
    vClearForParse(spExp);
    vpVecPushn(spExp->vpVecSource, (void*)spSource->acpPhrase, spSource->uiLength);
    return bExecTest(spExp);
}

/** \brief Enable or disable specified rule and/or UDT names for phrase capture.
 *
 * By default, all rules and UDTs are disabled.
 * The result, which is equal to the start rule, is always captured, independent of these selections.
 * However, the start rule, like all other rules, is captured independently and only if it is enabled here.
 * Note that UDTs, if any, must always be defined with vApgexDefineUDT() prior to any matching function call.
 * Disabling them simply means that matched phrases are not saved.
 * They still must be defined for the parse to be performed.
 *
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpNames The name or names of the rules/UDTs to enable or disable. All names are case insensitive.
 * May not be NULL or empty.
 *  - "--all" Enable/disable all rules and UDTs.
 *  - "name[,name,...]" A comma-delimited list of one or more names to enabled/disabled.
 *  Rule and UDT names may be mixed and in any order. All names, including "--all", are case-insensitive.
 *  Only a single comma allowed between names - no spaces or other delimiters.
 * \param bEnable If true, the named rules/UDTS are enabled, meaning that their phrase will be captured.
 *  If false, the rules/UDTS will be disabled, meaning that their respective phrases will not be captured
 * \return Exception is thrown if any name is not a valid rule name.
 */
void vApgexEnableRules(void* vpCtx, const char* cpNames, abool bEnable){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!spExp->vpParser){
        XTHROW(spExp->spException, s_cpNoPattern);
    }
    if(!cpNames && !cpNames[0]){
        XTHROW(spExp->spException, "names list, cpNames, cannot be NULL or empty");
    }
    aint uiStrLen = (aint)strlen(cpNames) + 1;
    aint ui = 0;
    aint uiNameLen = 0;
    udt_r* spUdt;
    rule_r* spRule;
    char cChar;
    char caBuf[BUF_SIZE];
    bEnable = (bEnable > 0) ? APG_TRUE : APG_FALSE;
    vVecClear(spExp->vpVecStrings);
    char* cpNameBuf = (char*)vpVecPushn(spExp->vpVecStrings, NULL, (uiStrLen + 10));
    for(; ui < uiStrLen; ui++){
        cChar = cpNames[ui];
        if(bIsNameChar(cChar)){
            // create a new name
            cpNameBuf[uiNameLen++] = cChar;
        }else{
            if(uiNameLen > 0){
                // end a name
                cpNameBuf[uiNameLen] = (char)0;
                uiNameLen = 0;
                if(iStriCmp(cpNameBuf, "--all") == 0){
                    goto all;
                }

                // look up
                spRule = spFindRule(spExp, cpNameBuf);
                if(spRule){
                    spRule->bEnabled = bEnable;
                }else{
                    spUdt = spFindUdt(spExp, cpNameBuf);
                    if(spUdt){
                        spUdt->bEnabled = bEnable;
                    }else{
                        snprintf(caBuf, BUF_SIZE, "\"%s\" is not a valid rule or UDT name", cpNameBuf);
                        vMemFree(spExp->vpMem, cpNameBuf);
                        XTHROW(spExp->spException, caBuf);
                    }
                }
            } // else ignore dilimiters
        }
    }
    spExp->uiEnabledRuleCount = 0;
    spExp->uiEnabledUdtCount = 0;
    for(ui = 0; ui < spExp->uiRuleCount; ui++){
        if(spExp->spRelRules[ui].bEnabled){
            spExp->uiEnabledRuleCount++;
        }
    }
    for(ui = 0; ui < spExp->uiUdtCount; ui++){
        if(spExp->spRelUdts[ui].bEnabled){
            spExp->uiEnabledUdtCount++;
        }
    }
    return;
    all:;
    spExp->uiEnabledRuleCount = spExp->uiRuleCount;
    spExp->uiEnabledUdtCount = spExp->uiUdtCount;
    for(ui = 0; ui < spExp->uiRuleCount; ui++){
        spExp->spRelRules[ui].bEnabled = bEnable;
    }
    for(ui = 0; ui < spExp->uiUdtCount; ui++){
        spExp->spRelUdts[ui].bEnabled = bEnable;
    }
}

/** \brief Sets the index of the character in the source where the pattern-match search is to begin.
 *
 * uiLastIndex governs the starting point of the search in the next call to any of the pattern-matching functions.
 * It is initialized to 0 by default.
 * It's value on consecutive calls to a pattern-matching function are normally governed by the
 * mode (default, global, sticky) rules.
 * This function can be used prior to any pattern-matching call to override the default behavior.
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param uiLastIndex The source character index where the pattern match is to begin.
 * Must be lest than the source length or an exception will be thrown.
 */
void vApgexSetLastIndex(void* vpCtx, aint uiLastIndex){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    spExp->uiLastIndex = uiLastIndex;
}

/** \brief Get a copy of the object's properties.
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return A apgex_properties structure (not a pointer to a structure).
 * See which for the properties details.
 */
apgex_properties sApgexProperties(void* vpCtx){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!spExp->vpParser){
        XTHROW(spExp->spException, s_cpNoPattern);
    }
    apgex_properties sProps;
    memset(&sProps, 0, sizeof(apgex_properties));
    sProps.vpParser = spExp->vpParser;
    sProps.vpAst = spExp->vpAst;
    sProps.vpTrace = spExp->vpTrace;
    sProps.cpPattern = (const char*)vpVecFirst(spExp->vpVecPattern);
    sProps.cpFlags = (const char*)vpVecFirst(spExp->vpVecFlags);
    sProps.uiLastIndex = spExp->uiLastIndex;
    sProps.bDefaultMode = spExp->bDefaultMode;
    sProps.bGlobalMode = spExp->bGlobalMode;
    sProps.bStickyMode = spExp->bStickyMode;
    sProps.bPpptMode = spExp->bPpptMode;
    sProps.bTraceMode = spExp->bTraceMode;
    sProps.bTraceHtmlMode = spExp->bTraceHtmlMode;
    if(spExp->bReplaceMode){
        sProps.sOriginalSource.acpPhrase = (const achar*)vpVecFirst(spExp->vpVecOriginalSource);
        sProps.sOriginalSource.uiLength = uiVecLen(spExp->vpVecOriginalSource);
    }else{
        sProps.sOriginalSource.acpPhrase = (const achar*)vpVecFirst(spExp->vpVecSource);
        sProps.sOriginalSource.uiLength = uiVecLen(spExp->vpVecSource);
    }
    sProps.sLastSource.acpPhrase = (const achar*)vpVecFirst(spExp->vpVecSource);
    sProps.sLastSource.uiLength = uiVecLen(spExp->vpVecSource);
    if(spExp->spLastMatch){
        sProps.sLastMatch = *spExp->spLastMatch;
    }
    if(spExp->spLeftContext){
        sProps.sLeftContext = *spExp->spLeftContext;
    }
    if(spExp->spRightContext){
        sProps.sRightContext = *spExp->spRightContext;
    }
    return sProps;
}

/** \brief Get a pointer to the AST object's context.
 *
 * This function can be called after any of the phrase-matching functions.
 *  - sApgexExec()
 *  - sApgexReplace()
 *  - sApgexReplaceFunc()
 *  - bApgexTest()
 *  - spApgexSplit()
 *
 *  The AST object will reflect the results of the last successful phrase match.
 *  If the last phrase match was unsuccessful the pointer is valid but the AST will have no records.
 *  Be sure to understand the meaning of the [flags](\ref vApgexPattern())
 *  used and the result of the phrase-matching function called.
 *
 *  Following a successful phrase match, the AST object will have records for all rules and UDTs in the pattern
 *  but all call back functions will be NULL.
 *  Use vAstSetRuleCallback() and vAstSetUdtCallback() to set the translation functions
 *  specific to the application prior to translation with vAstTranslate().
 *
 *  See sApgexProperties() for alternate access to this pointer.
 *
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return A pointer to the AST object's context.
 * An exception is thrown if called with no pattern defined
 */
void* vpApgexGetAst(void* vpCtx){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!spExp->vpParser){
        XTHROW(spExp->spException, s_cpNoPattern);
    }
    return spExp->vpAst;
}

/** \brief Get a pointer to the trace object's context.
 *
 * This function can be called after any of the pattern-defining functions.
 *  - vApgexPattern()
 *  - vApgexPatternFile()
 *  - vApgexPatternParser()
 *
 *  The trace context pointer can be used to configure the trace prior to any of the phrase-matching functions.
 *
 *  See sApgexProperties() for alternate access to this pointer.
 *
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return A pointer to the trace object's context if the "t" flag was specified. NULL otherwise.
 * An exception is thrown if called with no pattern defined
 */
void* vpApgexGetTrace(void* vpCtx){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!spExp->vpParser){
        XTHROW(spExp->spException, s_cpNoPattern);
    }
    return spExp->vpTrace;
}

/** \brief Get a pointer to the parser object's context.
 *
 * This function can be called after any of the pattern-defining functions.
 *  - vApgexPattern()
 *  - vApgexPatternFile()
 *  - vApgexPatternParser()
 *
 *  See sApgexProperties() for alternate access to this pointer.
 *
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return A pointer to the parser object's context.
 * An exception is thrown if called with no pattern defined
 */
void* vpApgexGetParser(void* vpCtx){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!spExp->vpParser){
        XTHROW(spExp->spException, s_cpNoPattern);
    }
    return spExp->vpParser;
}

/** \brief Display the object's properties.
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spProperties Pointer to the properties to display.
 * This would be a pointer to the return value of sApgexProperties().
 * \param cpFileName The name of the file to write the display to.
 * If NULL, `stdout` is used. If non-NULL any directories in the pathname must exist.
 */
void vApgexDisplayProperties(void* vpCtx, apgex_properties* spProperties, const char* cpFileName){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!spExp->vpParser){
        XTHROW(spExp->spException, s_cpNoPattern);
    }
    spExp->spDisplay = stdout;
    if(cpFileName){
        spExp->spDisplay = fopen(cpFileName, "wb");
        if(!spExp->spDisplay){
            char caBuf[BUF_SIZE];
            snprintf(caBuf, BUF_SIZE, "can't open file %s for writing", cpFileName);
            XTHROW(spExp->spException, caBuf);
        }
    }
    FILE* spOut = spExp->spDisplay;
    apgex_phrase sOutPhrase = {};
    fprintf(spOut, "PROPERTIES:\n");
    fprintf(spOut, "Pattern:\n");
    fprintf(spOut, "%s", (char*)vpVecFirst(spExp->vpVecPattern));
    fprintf(spOut, "\n");
    fprintf(spOut, "          Flags: \"%s\"\n", (char*)vpVecFirst(spExp->vpVecFlags));
    fprintf(spOut, "        default: %s\n", cpBool(spProperties->bDefaultMode));
    fprintf(spOut, "         global: %s\n", cpBool(spProperties->bGlobalMode));
    fprintf(spOut, "         sticky: %s\n", cpBool(spProperties->bStickyMode));
    fprintf(spOut, "           pppt: %s\n", cpBool(spProperties->bPpptMode));
    if(spProperties->bTraceMode){
        if(spProperties->bTraceHtmlMode){
            fprintf(spOut, "          trace: yes(html)\n");
        }else{
            fprintf(spOut, "          trace: yes(ascii)\n");
        }
    }else{
        fprintf(spOut, "          trace: no\n");
    }
    if(spExp->bReplaceMode){
        fprintf(spOut, "Original Source: ");
        sOutPhrase.sPhrase = spProperties->sOriginalSource;
        vPrintPhrase(spExp, &sOutPhrase, spOut);
        fprintf(spOut, "Replaced Source: ");
        sOutPhrase.sPhrase = spProperties->sLastSource;
        vPrintPhrase(spExp, &sOutPhrase, spOut);
    }else{
        fprintf(spOut, "Original Source: ");
        sOutPhrase.sPhrase = spProperties->sOriginalSource;
        vPrintPhrase(spExp, &sOutPhrase, spOut);
    }
    fprintf(spOut, "     Last Index: %"PRIuMAX"\n", (luint)spExp->uiLastIndex);
    fprintf(spOut, "     Last Match: ");
    vPrintPhrase(spExp, &spProperties->sLastMatch, spOut);
    fprintf(spOut, "   Left Context: ");
    vPrintPhrase(spExp, &spProperties->sLeftContext, spOut);
    fprintf(spOut, "  Right Context: ");
    vPrintPhrase(spExp, &spProperties->sRightContext, spOut);
    if(spOut && (spOut != stdout)){
        fclose(spOut);
        spExp->spDisplay = NULL;
    }
}

/** \brief Display the object's properties.
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spPhrase Pointer to the apgex_phrase to display.
 * This would be a one of the members of an \ref apgex_result or \ref apgex_rule.
 * \param cpFileName The name of the file to write the display to.
 * If NULL, `stdout` is used. If non-NULL any directories in the pathname must exist.
 */
void vApgexDisplayPhrase(void* vpCtx, apgex_phrase* spPhrase, const char* cpFileName){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!spPhrase){
        XTHROW(spExp->spException, "phrase to display, spPhrase, cannot be NULL");
    }
    spExp->spDisplay = stdout;
    if(cpFileName){
        spExp->spDisplay = fopen(cpFileName, "wb");
        if(!spExp->spDisplay){
            char caBuf[BUF_SIZE];
            snprintf(caBuf, BUF_SIZE, "can't open file %s for writing", cpFileName);
            XTHROW(spExp->spException, caBuf);
        }
    }
    vPrintPhrase(spExp, spPhrase, spExp->spDisplay);
    if(spExp->spDisplay && (spExp->spDisplay != stdout)){
        fclose(spExp->spDisplay);
        spExp->spDisplay = NULL;
    }
}

/** \brief Display the complete results from a pattern match.
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spResult Pointer to the pattern matched result,
 * the return from sApgexExec();
 * \param cpFileName The file to display the output to. If NULL, stdout is used.
 */
void vApgexDisplayResult(void* vpCtx, apgex_result* spResult, const char* cpFileName){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!spResult){
        XTHROW(spExp->spException, "result to display, spResult, cannot be NULL");
    }
    spExp->spDisplay = stdout;
    if(cpFileName){
        spExp->spDisplay = fopen(cpFileName, "wb");
        if(!spExp->spDisplay){
            char caBuf[BUF_SIZE];
            snprintf(caBuf, BUF_SIZE, "can't open file %s for writing", cpFileName);
            XTHROW(spExp->spException, caBuf);
        }
    }
    FILE* spOut = spExp->spDisplay;
    fprintf(spOut, "RESULT:\n");
    fprintf(spOut, "result       : ");
    if(!spResult->spResult){
        fprintf(spOut, "no match\n");
    }else{
        vPrintPhrase(spExp, spResult->spResult, spOut);
        fprintf(spOut, "left context : ");
        vPrintPhrase(spExp, spResult->spLeftContext, spOut);
        fprintf(spOut, "right context: ");
        vPrintPhrase(spExp, spResult->spRightContext, spOut);
        fprintf(spOut, "uiLastIndex  : %"PRIuMAX"\n", (luint)spResult->uiLastIndex);
        fprintf(spOut, "node hits    : %"PRIuMAX"\n", (luint)spResult->uiNodeHits);
        fprintf(spOut, "tree depth   : %"PRIuMAX"\n", (luint)spResult->uiTreeDepth);
        if(spResult->uiRuleCount){
            aint ui, uj;
            fprintf(spOut, "\nRULES:\n");
            for(ui = 0; ui < spResult->uiRuleCount; ui++){
                if(ui > 0){
                    fprintf(spOut, "\n");
                }
                fprintf(spOut, "%s: phrases: %"PRIuMAX"\n", spResult->spRules[ui].cpRuleName, (luint)spResult->spRules[ui].uiPhraseCount);
                for(uj = 0; uj < spResult->spRules[ui].uiPhraseCount; uj++){
                    vPrintPhrase(spExp, &spResult->spRules[ui].spPhrases[uj], spOut);
                }
            }
        }
    }
    if(spOut && (spOut != stdout)){
        fclose(spOut);
        spExp->spDisplay = NULL;
    }
 }

void vApgexDisplayPatternErrors(void* vpCtx, const char* cpFileName){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    spExp->spDisplay = stdout;
    if(cpFileName){
        spExp->spDisplay = fopen(cpFileName, "wb");
        if(!spExp->spDisplay){
            char caBuf[BUF_SIZE];
            snprintf(caBuf, BUF_SIZE, "can't open file %s for writing", cpFileName);
            XTHROW(spExp->spException, caBuf);
        }
    }
    FILE* spOut = spExp->spDisplay;
    void* vpMsgs = vpApiGetErrorLog(spExp->vpApi);
    if(!bMsgsValidate(vpMsgs)){
        vExContext();
    }
    const char* cpMsg = cpMsgsFirst(vpMsgs);
    while(cpMsg){
        fprintf(spOut, "%s\n", cpMsg);
        cpMsg = cpMsgsNext(vpMsgs);
    }

    if(spOut && (spOut != stdout)){
        fclose(spOut);
        spExp->spDisplay = NULL;
    }
}

/** \brief Define the callback function for a User-Defined Terminal (UDT).
 *
 * If there are any UDTs in the SABNF pattern grammar, each one of them
 * must have a user-written callback function to do its pattern matching work.
 * This function will define a callback function to the apgex object for a single UDT.
 * This function must be called for each UDT appearing in the SABNF pattern grammar.
 * \param vpCtx A pointer to a valid apgex object context returned from vpApgexCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpName Name of the UDT.
 * \param pfnUdt The callback function for this UDT.
 */
void vApgexDefineUDT(void* vpCtx, const char* cpName, parser_callback pfnUdt){
    apgex* spExp = (apgex*) vpCtx;
    if(!vpCtx || (spExp->vpValidate != s_vpMagicNumber)){
            vExContext();
    }
    if(!spExp->vpParser){
        XTHROW(spExp->spException, s_cpNoPattern);
    }
    udt_r* spUdt = spFindUdt(spExp, cpName);
    if(!spUdt){
        char caBuf[BUF_SIZE];
        snprintf(caBuf, BUF_SIZE, "pattern has no UDT named %s", cpName);
        XTHROW(spExp->spException, caBuf);
    }
    vParserSetUdtCallback(spExp->vpParser, spUdt->uiUdtIndex, pfnUdt);
//    spExp->spRelUdts[spUdt->uiUdtIndex].pfnUdt = pfnUdt;
}

//  ///////////////////////////////////////////////////////////////////////////////
//      STATIC FUNCTIONS
//  ///////////////////////////////////////////////////////////////////////////////
static inline void vMakeAbsPhrase(apgex* spExp, phrase_r* spRelPhrase, apgex_phrase* spAbsPhrase){
    spAbsPhrase->sPhrase.acpPhrase = (achar*)vpVecFirst(spExp->vpVecSource) + spRelPhrase->uiSourceOffset;
    spAbsPhrase->sPhrase.uiLength = spRelPhrase->uiLength;
    spAbsPhrase->uiPhraseOffset = spRelPhrase->uiSourceOffset;
}

static inline void vMakeRelPhrase(apgex* spExp, aint uiSourceOffset, aint uiLen, phrase_r* spPhrase){
    spPhrase->uiSourceOffset = uiSourceOffset;
    spPhrase->uiLength = uiLen;
}

static inline char* cpBool(abool bVal){
    if(bVal){
        return "yes";
    }
    return "no";
}

static void vNamePhrase(apgex* spExp, achar* acpName, aint uiNameLen, apg_phrase* spPhrase){
    rule_r* spRule = NULL;
    udt_r* spUdt = NULL;
    char* cpNameStr;
    phrase_r* spRelPhrase;
    achar* acpSource = (achar*)vpVecFirst(spExp->vpVecSource);
    aint ui;
    char caBuf[BUF_SIZE];
    void* vpVecTmp = spExp->vpVecStrings;
    vVecClear(vpVecTmp);
    memset(spPhrase, 0, sizeof(*spPhrase));
    // look up name
    for(ui = 0; ui < uiNameLen; ui++){
        char cChar = (char)acpName[ui];
        vpVecPush(vpVecTmp, &cChar);
    }
    vpVecPush(vpVecTmp, &s_cZero);
    cpNameStr = (char*)vpVecFirst(vpVecTmp);
    spRule = spFindRule(spExp, (const char*)cpNameStr);
    if(spRule){
        if(spRule->bEnabled && spRule->uiFirstPhrase != APG_INFINITE){
            spRelPhrase = (phrase_r*)vpVecAt(spExp->vpVecRelPhrases, spRule->uiFirstPhrase);
            spPhrase->acpPhrase = &acpSource[spRelPhrase->uiSourceOffset];
            spPhrase->uiLength = spRelPhrase->uiLength;
        }
        goto found;
    }
    spUdt = spFindUdt(spExp, (const char*)cpNameStr);
    if(spUdt){
        if(spUdt->bEnabled && spUdt->uiFirstPhrase != APG_INFINITE){
            spRelPhrase = (phrase_r*)vpVecAt(spExp->vpVecRelPhrases, spUdt->uiFirstPhrase);
            spPhrase->acpPhrase = &acpSource[spRelPhrase->uiSourceOffset];
            spPhrase->uiLength = spRelPhrase->uiLength;
        }
        goto found;
    }
    snprintf(caBuf, BUF_SIZE, "replacement error: &<%s> not a valid rule or UDT name", (char*)cpNameStr);
    vVecClear(vpVecTmp);
    XTHROW(spExp->spException, caBuf);
    found:;
    vVecClear(vpVecTmp);
}
static void vReplaceFunc(apgex* spExp, apgex_result* spResult, pfn_replace pfnFunc, void* vpUser){
    vVecClear(spExp->vpVecReplacement);
    apgex_properties sProps = sApgexProperties(spExp);
    apg_phrase sPhrase = pfnFunc(spResult, &sProps, vpUser);
    if(sPhrase.acpPhrase && sPhrase.uiLength){
        vpVecPushn(spExp->vpVecReplacement, (void*)sPhrase.acpPhrase, sPhrase.uiLength);
    }
}

static void vReplacement(apgex* spExp, apgex_result* spResult){
    // the raw replacement achar array is in vpVecReplaceRaw
    // make the substituted replacement achar array is in vpVecReplacement
    // make the string version of the substituted replacement achar array is in vpVecReplaceString
    vVecClear(spExp->vpVecReplacement);
    achar* acpRaw = (achar*)vpVecFirst(spExp->vpVecReplaceRaw);
    aint uiRawLen = uiVecLen(spExp->vpVecReplaceRaw);
    aint ui, uii, uj, uiNameLen;
    apg_phrase *spMatch, *spLeft, *spRight;
    char caBuf[BUF_SIZE];
    if(uiRawLen > 1){
        ui = 0;
        uii = 1;
        spMatch = &spResult->spResult->sPhrase;
        spLeft = &spResult->spLeftContext->sPhrase;
        spRight = &spResult->spRightContext->sPhrase;
        while(ui < uiRawLen){
            while(APG_TRUE){
                if(acpRaw[ui] == DOLLAR){
                    if(uii >= uiRawLen){
                        XTHROW(spExp->spException, "replacement error: $ found at end of string - must be $`, $&, $', $$ or $<rulename>");
                    }
                    if(acpRaw[uii] == DOLLAR){
                        vpVecPush(spExp->vpVecReplacement, &acpRaw[ui]);
                        ui += 2;
                        uii += 2;
                        break;
                    }
                    if(acpRaw[uii] == UNDER){
                        if(spMatch->uiLength){
                            vpVecPushn(spExp->vpVecReplacement, vpVecFirst(spExp->vpVecOriginalSource), uiVecLen(spExp->vpVecOriginalSource));
                        }
                        ui += 2;
                        uii += 2;
                        break;
                    }
                    if(acpRaw[uii] == AMP){
                        if(spMatch->uiLength){
                            vpVecPushn(spExp->vpVecReplacement, (void*)spMatch->acpPhrase, spMatch->uiLength);
                        }
                        ui += 2;
                        uii += 2;
                        break;
                    }
                    if(acpRaw[uii] == ACCENT){
                        if(spLeft->uiLength){
                            vpVecPushn(spExp->vpVecReplacement, (void*)spLeft->acpPhrase, spLeft->uiLength);
                        }
                        ui += 2;
                        uii += 2;
                        break;
                    }
                    if(acpRaw[uii] == APOS){
                        if(spRight->uiLength){
                            vpVecPushn(spExp->vpVecReplacement, (void*)spRight->acpPhrase, spRight->uiLength);
                        }
                        ui += 2;
                        uii += 2;
                        break;
                    }
                    if(acpRaw[uii] == LANGLE){
                        for(uj = uii + 1; uj < uiRawLen; uj++){
                            if(acpRaw[uj] == RANGLE){
                                uiNameLen = uj - uii -1;
                                if(uiNameLen == 0){
                                    XTHROW(spExp->spException, "replacement error: &<> - no rule name");
                                }
                                apg_phrase sPhrase = {};
                                vNamePhrase(spExp, &acpRaw[uii + 1], uiNameLen, &sPhrase);
                                if(sPhrase.acpPhrase){
                                    vpVecPushn(spExp->vpVecReplacement, (void*)sPhrase.acpPhrase, sPhrase.uiLength);
                                }
                                // else if no matched phrase for this rule name replace &<rulename> with empty string
                                ui += uiNameLen + 3;
                                uii = ui + 1;
                                goto name;
                            }
                        }
                        XTHROW(spExp->spException, "replacement error: found &< but closing angle bracket, >, not found");
                        name:;
                        break;
                    }
                    snprintf(caBuf, BUF_SIZE, "replacement error: $ followed by %"PRIuMAX" - must be $`, $&, $', $$ or $<rulename>", (luint)acpRaw[uii]);
                    XTHROW(spExp->spException, caBuf);
                }else{
                    vpVecPush(spExp->vpVecReplacement, &acpRaw[ui]);
                    ui++;
                    uii++;
                }
                break;
            }
        }
    }
}
static void vReplace(apgex* spExp, apgex_result* spResult){
    aint uiReplaceLen;
    achar* acpRight = NULL;
    aint uiRight;
    apg_phrase *spMatch = &spResult->spResult->sPhrase;
    apg_phrase *spLeft = &spResult->spLeftContext->sPhrase;
    apg_phrase *spRight = &spResult->spRightContext->sPhrase;
    uiReplaceLen = uiVecLen(spExp->vpVecReplacement);
    if(spRight->uiLength){
        // save right context
        uiRight = uiVecLen(spExp->vpVecReplacement);
        vpVecPushn(spExp->vpVecReplacement, (void*)spRight->acpPhrase, spRight->uiLength);
        acpRight = (achar*)vpVecAt(spExp->vpVecReplacement, uiRight);
    }
    // pop off the phrase and its right context
    aint uiTmp = spMatch->uiLength + spRight->uiLength;
    if(uiTmp){
        vpVecPopi(spExp->vpVecSource, spLeft->uiLength);
    }
    // push on the replacement string
    if(uiReplaceLen){
        vpVecPushn(spExp->vpVecSource, vpVecFirst(spExp->vpVecReplacement), uiReplaceLen);
    }
    if(spRight->uiLength){
        // push on the right context
        vpVecPushn(spExp->vpVecSource, (void*)acpRight, spRight->uiLength);
    }
    spExp->uiLastIndex += uiReplaceLen;
    spExp->uiLastIndex -= spMatch->uiLength;
    spExp->spRightContext->uiPhraseOffset += uiReplaceLen;
    spExp->spRightContext->uiPhraseOffset -= spMatch->uiLength;
}

static void vExecResult(apgex* spExp, apgex_result* spResult){
    parser_config sConfig = {};
    while(APG_TRUE){
        sConfig.uiInputLength = uiVecLen(spExp->vpVecSource);
        if(spExp->uiLastIndex >= sConfig.uiInputLength){
            // no match - no substring to parse
            break;
        }

        sConfig.acpInput = (achar*)vpVecFirst(spExp->vpVecSource);
        sConfig.bParseSubString = APG_TRUE;
        sConfig.uiSubStringBeg = spExp->uiLastIndex;
        sConfig.vpUserData = (void*)spExp;
        if(spExp->vpTrace){
            TRACE_APGEX_HEADER(spExp->vpTrace);
        }
        if(spExp->bDefaultMode){
            vMatchDefault(spExp, &sConfig, spResult);
            spExp->uiLastIndex = 0;
        }else if(spExp->bGlobalMode){
            vMatchGlobal(spExp, &sConfig, spResult);
            if(!spResult->spResult){
                spExp->uiLastIndex = 0;
            }
        }else if(spExp->bStickyMode){
            vMatchSticky(spExp, &sConfig, spResult);
            if(!spResult->spResult){
                spExp->uiLastIndex = 0;
            }
        }
        spResult->uiLastIndex = spExp->uiLastIndex;
        if(spExp->vpTrace){
            TRACE_APGEX_FOOTER(spExp->vpTrace);
            TRACE_DTOR(spExp->vpTrace);
            spExp->vpTrace = NULL;
        }
        break;
    }
}

static abool bExecTest(apgex* spExp){
    abool bReturn = APG_FALSE;
    parser_config sConfig = {};
    while(APG_TRUE){
        sConfig.uiInputLength = uiVecLen(spExp->vpVecSource);
        if(spExp->uiLastIndex >= sConfig.uiInputLength){
            // no match - no substring to parse
            break;
        }

        // check that all UDTs are defined
        aint ui;
        for(ui = 0; ui < spExp->uiUdtCount; ui++){
            if(!spExp->spRelUdts[ui].pfnUdt){
                char caBuf[BUF_SIZE];
                snprintf(caBuf, BUF_SIZE, "UDT \"%s\" not defined", spExp->spRelUdts[ui].cpUdtName);
                XTHROW(spExp->spException, caBuf);
            }
        }
        sConfig.acpInput = (achar*)vpVecFirst(spExp->vpVecSource);
        sConfig.bParseSubString = APG_TRUE;
        sConfig.uiSubStringBeg = spExp->uiLastIndex;
        sConfig.vpUserData = (void*)spExp;
        if(spExp->vpTrace){
            TRACE_APGEX_HEADER(spExp->vpTrace);
        }
        if(spExp->bDefaultMode){
            bReturn = bTestDefault(spExp, &sConfig);
            spExp->uiLastIndex = 0;
        }else if(spExp->bGlobalMode){
            bReturn = bTestGlobal(spExp, &sConfig);
            if(!bReturn){
                spExp->uiLastIndex = 0;
            }
        }else if(spExp->bStickyMode){
            bReturn = bTestSticky(spExp, &sConfig);
            if(!bReturn){
                spExp->uiLastIndex = 0;
            }
        }
        if(spExp->vpTrace){
            TRACE_APGEX_FOOTER(spExp->vpTrace);
            TRACE_DTOR(spExp->vpTrace);
            spExp->vpTrace = NULL;
        }
        break;
    }
    return bReturn;
}

static void vDecodeFlags(apgex* spExp, const char* cpFlags){
    spExp->bDefaultMode = APG_TRUE;
    aint ui = 0;
    aint uiFlagsLen = 0;
    if(cpFlags){
        uiFlagsLen = (aint)strlen(cpFlags);
        if(uiFlagsLen){
            // save a copy of the original flags string
            vpVecPushn(spExp->vpVecFlags, (void*)cpFlags, (aint)(strlen(cpFlags) + 1));
            char caBuf[BUF_SIZE];
            for(; ui < uiFlagsLen; ui++){
                switch(cpFlags[ui]){
                case 'g':
                    if(!spExp->bStickyMode){
                        spExp->bGlobalMode = APG_TRUE;
                        spExp->bDefaultMode = APG_FALSE;
                    }
                    break;
                case 'y':
                    if(!spExp->bGlobalMode){
                        spExp->bStickyMode = APG_TRUE;
                        spExp->bDefaultMode = APG_FALSE;
                    }
                    break;
                case 't':
                    if(!spExp->bTraceMode){
                        spExp->bTraceMode = APG_TRUE;
                    }
                    break;
                case 'h':
                    if(spExp->bTraceMode){
                        spExp->bTraceHtmlMode = APG_TRUE;
                    }else{
                        XTHROW(spExp->spException, "%'h' flag (for HTML trace output) must follow 't' flag");
                    }
                    break;
                case 'p':
                    spExp->bPpptMode = APG_TRUE;
                    break;
                default:
                    snprintf(caBuf, BUF_SIZE, "%c unrecognized flag character, must be one or more of \"gyta\"", cpFlags[ui]);
                    XTHROW(spExp->spException, caBuf);
                    break;
                }
            }
        }
    }
    TRACE_APGEX_CHECK(spExp);
    if((cpFlags == NULL) || (uiFlagsLen == 0)){
        // save an empty string
        vpVecPush(spExp->vpVecFlags, &s_cZero);
    }
}

static void vConstructParser(apgex* spExp){
    char* cpPattern = (char*)vpVecFirst(spExp->vpVecPattern);
    aint uiPatternLen = (aint)strlen(cpPattern);
    if(!cpPattern || (uiPatternLen <= 1)){
        XTHROW(spExp->spException, "attmpting to construct the parser but the pattern is not yet defined");
    }
    vApiInClear(spExp->vpApi);
    cpApiInString(spExp->vpApi, cpPattern);
    vApiInValidate(spExp->vpApi, APG_FALSE);
    vApiSyntax(spExp->vpApi, APG_FALSE);
    vApiOpcodes(spExp->vpApi);
    bApiAttrs(spExp->vpApi);
    if(spExp->bPpptMode){
        vApiPppt(spExp->vpApi, NULL, 0);
    }
    spExp->vpParser = vpApiOutputParser(spExp->vpApi);
    TRACE_APGEX_OUTPUT(spExp);
    spExp->vpAst = vpAstCtor(spExp->vpParser);
}

static void vInitRules(apgex* spExp){
    if(!spExp->vpParser){
        XTHROW(spExp->spException, "attempting to initialize rules and UDTS but parser not defined");
    }

    // set all rule and UDT call backs, even if disabled
    // we want the AST to collect records for ALL rules and UDTs
    aint ui;
    parser* spParser = (parser*)spExp->vpParser;
    spExp->uiRuleCount = spParser->uiRuleCount;
    spExp->uiUdtCount = spParser->uiUdtCount;
    spExp->spRelRules = (rule_r*)vpVecPushn(spExp->vpVecRelRules, NULL, spExp->uiRuleCount);
    for(ui = 0; ui < spExp->uiRuleCount; ui++){
        spExp->spRelRules[ui].cpRuleName = spParser->spRules[ui].cpRuleName;
        spExp->spRelRules[ui].uiRuleIndex = spParser->spRules[ui].uiRuleIndex;
        spExp->spRelRules[ui].uiFirstPhrase = APG_UNDEFINED;
        spExp->spRelRules[ui].uiLastPhrase = APG_UNDEFINED;
        spExp->spRelRules[ui].bEnabled = APG_FALSE;
    }
    if(spExp->uiUdtCount){
        // initialize the UDT list
        spExp->spRelUdts = (udt_r*)vpVecPushn(spExp->vpVecRelUdts, NULL, spExp->uiUdtCount);
        for(ui = 0; ui < spExp->uiUdtCount; ui++){
            spExp->spRelUdts[ui].cpUdtName = spParser->spUdts[ui].cpUdtName;
            spExp->spRelUdts[ui].uiUdtIndex = spParser->spUdts[ui].uiUdtIndex;
            spExp->spRelUdts[ui].uiFirstPhrase = APG_UNDEFINED;
            spExp->spRelUdts[ui].uiLastPhrase = APG_UNDEFINED;
            spExp->spRelUdts[ui].bEnabled = APG_FALSE;
        }
    }
}
static void vInitCallbacks(apgex* spExp){
    if(!spExp->vpParser){
        XTHROW(spExp->spException, "attempting to initialize rules and UDTS but parser not defined");
    }

    // set all rule and UDT call backs, even if disabled
    // we want the AST to collect records for ALL rules and UDTs
    aint ui;
    for(ui = 0; ui < spExp->uiRuleCount; ui++){
        vAstSetRuleCallback(spExp->vpAst, ui, pfnRuleCallback);
    }
    if(spExp->uiUdtCount){
        // initialize the UDT list
        for(ui = 0; ui < spExp->uiUdtCount; ui++){
            vAstSetUdtCallback(spExp->vpAst, ui, pfnUdtCallback);
        }
    }
}

static void vResetCallbacks(apgex* spExp){
    if(!spExp->vpParser){
        XTHROW(spExp->spException, "attempting to initialize rules and UDTS but parser not defined");
    }

    // set all rule and UDT call backs, even if disabled
    // we want the AST to collect records for ALL rules and UDTs
    aint ui;
    for(ui = 0; ui < spExp->uiRuleCount; ui++){
        vAstSetRuleCallback(spExp->vpAst, ui, NULL);
    }
    if(spExp->uiUdtCount){
        // initialize the UDT list
        for(ui = 0; ui < spExp->uiUdtCount; ui++){
            vAstSetUdtCallback(spExp->vpAst, ui, NULL);
        }
    }
}

static void vMatchResult(apgex* spExp, parser_config* spConfig, parser_state* spState, apgex_result* spResult){
    // translate the AST to get the relative phrases
    vAstTranslate(spExp->vpAst, (void*)spExp);

    // // push 3 relative phrases for the result, left and right context
    phrase_r* spRelResult = (phrase_r*)vpVecPush(spExp->vpVecRelPhrases, NULL);
    phrase_r* spRelLeft = (phrase_r*)vpVecPush(spExp->vpVecRelPhrases, NULL);
    phrase_r* spRelRight = (phrase_r*)vpVecPush(spExp->vpVecRelPhrases, NULL);
    spRelResult->uiNext = APG_UNDEFINED;
    spRelLeft->uiNext = APG_UNDEFINED;
    spRelRight->uiNext = APG_UNDEFINED;
    vMakeRelPhrase(spExp, spConfig->uiSubStringBeg, spState->uiPhraseLength, spRelResult);
    vMakeRelPhrase(spExp, 0, spConfig->uiSubStringBeg, spRelLeft);
    aint uiOffset = spConfig->uiSubStringBeg + spState->uiPhraseLength;
    vMakeRelPhrase(spExp, uiOffset, (spConfig->uiInputLength - uiOffset), spRelRight);

    // absolute phrases can now be generated
    aint uiCount = uiVecLen(spExp->vpVecRelPhrases);
    apgex_phrase* spAbsPhrases = (apgex_phrase*)vpVecPushn(spExp->vpVecPhrases, NULL, uiCount);
    apgex_phrase* spAbsPhrase = spAbsPhrases;

    // make the result
    vMakeAbsPhrase(spExp, spRelResult, spAbsPhrase);
    spExp->spLastMatch = spAbsPhrase;
    spResult->spResult = spAbsPhrase++;

    // make the left context
    vMakeAbsPhrase(spExp, spRelLeft, spAbsPhrase);
    spExp->spLeftContext = spAbsPhrase;
    spResult->spLeftContext = spAbsPhrase++;

    // make the right context
    vMakeAbsPhrase(spExp, spRelRight, spAbsPhrase);
    spExp->spRightContext = spAbsPhrase;
    spResult->spRightContext = spAbsPhrase++;

    // some stats
    spResult->uiTreeDepth = spState->uiMaxTreeDepth;
    spResult->uiNodeHits = spState->uiHitCount;

    phrase_r sRelPhrase;
    phrase_r* spRelPhrases = (phrase_r*)vpVecFirst(spExp->vpVecRelPhrases);
    spResult->uiRuleCount = spExp->uiEnabledRuleCount + spExp->uiEnabledUdtCount;
    if(spResult->uiRuleCount){
        spResult->spRules = (apgex_rule*)vpVecPushn(spExp->vpVecRules, NULL, spResult->uiRuleCount);
    }
    if(spExp->uiEnabledRuleCount){
        // add the rules
        apgex_rule* spAbsRule = spResult->spRules;
        // for each enabled rule, add the absolute phrases sequentially
        rule_r* spRule = spExp->spRelRules;
        rule_r* spRuleEnd = spRule + spExp->uiRuleCount;
        phrase_r* spListPhrase;
        for(; spRule < spRuleEnd; spRule++){
            if(spRule->bEnabled){
                spAbsRule->cpRuleName = spRule->cpRuleName;
                spAbsRule->uiIndex = spRule->uiRuleIndex;
                spAbsRule->uiPhraseCount = spRule->uiPhraseCount;
                spAbsRule->spPhrases = spAbsPhrase;
                // follow the linked list of relative phrases, building a sequential list of absolute phrases
                aint uiNext = spRule->uiFirstPhrase;
                aint uiSanityCount = 0;
                while(uiNext != APG_UNDEFINED){
                    spListPhrase = spRelPhrases + uiNext;
                    vMakeRelPhrase(spExp, spListPhrase->uiSourceOffset, spListPhrase->uiLength, &sRelPhrase);
                    vMakeAbsPhrase(spExp, &sRelPhrase, spAbsPhrase);
                    uiNext = spListPhrase->uiNext;
                    spAbsPhrase++;
                    uiSanityCount++;
                }
                if(uiSanityCount != spAbsRule->uiPhraseCount){
                    XTHROW(spExp->spException, "number of phrases in linked list not the same as the phrase count");
                }
                spAbsRule++;
            }
        }
    }
    if(spExp->uiEnabledUdtCount){
        // add the UDTs
        apgex_rule* spAbsRule = &spResult->spRules[spExp->uiEnabledRuleCount];
        // for each enabled UDT, add the absolute phrases sequentially
        udt_r* spUdt = spExp->spRelUdts;
        udt_r* spUdtEnd = spUdt + spExp->uiUdtCount;
        phrase_r* spListPhrase;
        for(; spUdt < spUdtEnd; spUdt++){
            if(spUdt->bEnabled){
                spAbsRule->cpRuleName = spUdt->cpUdtName;
                spAbsRule->uiIndex = spUdt->uiUdtIndex;
                spAbsRule->uiPhraseCount = spUdt->uiPhraseCount;
                spAbsRule->spPhrases = spAbsPhrase;
                // follow the linked list of relative phrases, building a sequential list of absolute phrases
                aint uiNext = spUdt->uiFirstPhrase;
                aint uiSanityCount = 0;
                while(uiNext != APG_UNDEFINED){
                    spListPhrase = spRelPhrases + uiNext;
                    vMakeRelPhrase(spExp, spListPhrase->uiSourceOffset, spListPhrase->uiLength, &sRelPhrase);
                    vMakeAbsPhrase(spExp, &sRelPhrase, spAbsPhrase);
                    uiNext = spListPhrase->uiNext;
                    spAbsPhrase++;
                    uiSanityCount++;
                }
                if(uiSanityCount != spAbsRule->uiPhraseCount){
                    XTHROW(spExp->spException, "number of phrases in linked list not the same as the phrase count");
                }
                spAbsRule++;
            }
        }
    }

    // remove all AST call back functions in case user wants to use the AST
    aint ui;
    for(ui = 0; ui < spExp->uiRuleCount; ui++){
        vAstSetRuleCallback(spExp->vpAst, ui, NULL);
    }
    if(spExp->uiUdtCount){
        for(ui = 0; ui < spExp->uiUdtCount; ui++){
            vAstSetUdtCallback(spExp->vpAst, ui, NULL);
        }
    }
    ui = 0;
}
static void vMatchDefault(apgex* spExp, parser_config* spConfig, apgex_result* spResult){
    parser_state sState;
    memset(spResult, 0, sizeof(*spResult));
    while(spConfig->uiSubStringBeg < spConfig->uiInputLength){
        if(spExp->vpTrace){
            TRACE_APGEX_SEPARATOR(spExp);
        }
        vParserParse(spExp->vpParser, spConfig, &sState);
        if(sState.uiState == ID_MATCH){
            vMatchResult(spExp, spConfig, &sState, spResult);
            break;
        }
        spConfig->uiSubStringBeg++;
        spExp->uiLastIndex++;
    }
}

static void vMatchGlobal(apgex* spExp, parser_config* spConfig, apgex_result* spResult){
    parser_state sState;
    memset(spResult, 0, sizeof(*spResult));
    while(spConfig->uiSubStringBeg < spConfig->uiInputLength){
        if(spExp->vpTrace){
            TRACE_APGEX_SEPARATOR(spExp);
        }
        vParserParse(spExp->vpParser, spConfig, &sState);
        if(sState.uiState == ID_MATCH){
            vMatchResult(spExp, spConfig, &sState, spResult);
            if(sState.uiPhraseLength){
                spExp->uiLastIndex = spConfig->uiSubStringBeg + sState.uiPhraseLength;
            }else{
                spExp->uiLastIndex = spConfig->uiSubStringBeg + 1;
            }
            break;
        }
        spConfig->uiSubStringBeg++;
        spExp->uiLastIndex++;
    }
}

static void vMatchSticky(apgex* spExp, parser_config* spConfig, apgex_result* spResult){
    parser_state sState;
    memset(spResult, 0, sizeof(*spResult));
    if(spExp->vpTrace){
        TRACE_APGEX_SEPARATOR(spExp);
    }
    vParserParse(spExp->vpParser, spConfig, &sState);
    if(sState.uiState == ID_MATCH){
        vMatchResult(spExp, spConfig, &sState, spResult);
        if(sState.uiPhraseLength){
            spExp->uiLastIndex = spConfig->uiSubStringBeg + sState.uiPhraseLength;
        }else{
            spExp->uiLastIndex = spConfig->uiSubStringBeg + 1;
        }
    }
}

static abool bTestDefault(apgex* spExp, parser_config* spConfig){
    abool bReturn = APG_FALSE;
    parser_state sState;
    while(spConfig->uiSubStringBeg < spConfig->uiInputLength){
        if(spExp->vpTrace){
            TRACE_APGEX_SEPARATOR(spExp);
        }
        vParserParse(spExp->vpParser, spConfig, &sState);
        if(sState.uiState == ID_MATCH){
            bReturn = APG_TRUE;
            break;
        }
        spConfig->uiSubStringBeg++;
        spExp->uiLastIndex++;
    }
    return bReturn;
}

static abool bTestGlobal(apgex* spExp, parser_config* spConfig){
    abool bReturn = APG_FALSE;
    parser_state sState;
    while(spConfig->uiSubStringBeg < spConfig->uiInputLength){
        if(spExp->vpTrace){
            TRACE_APGEX_SEPARATOR(spExp);
        }
        vParserParse(spExp->vpParser, spConfig, &sState);
        if(sState.uiState == ID_MATCH){
            bReturn = APG_TRUE;
            if(sState.uiPhraseLength){
                spExp->uiLastIndex = spConfig->uiSubStringBeg + sState.uiPhraseLength;
            }else{
                spExp->uiLastIndex = spConfig->uiSubStringBeg + 1;
            }
            break;
        }
        spConfig->uiSubStringBeg++;
        spExp->uiLastIndex++;
    }
    return bReturn;
}

static abool bTestSticky(apgex* spExp, parser_config* spConfig){
    abool bReturn = APG_FALSE;
    parser_state sState;
    if(spExp->vpTrace){
        TRACE_APGEX_SEPARATOR(spExp);
    }
    vParserParse(spExp->vpParser, spConfig, &sState);
    if(sState.uiState == ID_MATCH){
        bReturn = APG_TRUE;
        if(sState.uiPhraseLength){
            spExp->uiLastIndex = spConfig->uiSubStringBeg + sState.uiPhraseLength;
        }else{
            spExp->uiLastIndex = spConfig->uiSubStringBeg + 1;
        }
    }
    return bReturn;
}

/** \brief Display a single phrase from the ApgExp pattern matching engine.
 *
 * The phrase will be printed as a null-terminated ASCII string if possible.
 * If the source phrase has non-printing characters, the format object will be used
 * to display the phrase as hex bytes.
 *
 * \param spPhrase Pointer to the phrase to display.
 * \param vpFmt Pointer to a format object. Returned from vpFmtCtor().
 */
static void vPrintPhrase(apgex* spExp, apgex_phrase* spPhrase, FILE* spOut){
    if(spPhrase->sPhrase.uiLength){
        if(bIsPhraseAscii(&spPhrase->sPhrase)){
            aint ui = 0;
            char cChar;
            fprintf(spOut, "offset: %"PRIuMAX" length: %"PRIuMAX": '",
                    (luint)spPhrase->uiPhraseOffset, (luint)spPhrase->sPhrase.uiLength);
            for(; ui < spPhrase->sPhrase.uiLength; ui++){
                cChar = (char)spPhrase->sPhrase.acpPhrase[ui];
                if(cChar == 9){
                    fprintf(spOut, "\\t");
                    continue;
                }
                if(cChar == 10){
                    fprintf(spOut, "\\n");
                    continue;
                }
                if(cChar == 13){
                    fprintf(spOut, "\\r");
                    continue;
                }
                fprintf(spOut, "%c", cChar);
            }
            fprintf(spOut, "'\n");
        }else{
            const char* cpLine;
            aint uiWordSize = (aint)sizeof(achar);
            uint8_t* ucpBytes = (uint8_t*)spPhrase->sPhrase.acpPhrase;
            long int iByteCount = (long int)(uiWordSize * spPhrase->sPhrase.uiLength);
            if(uiWordSize == 1){
                fprintf(spOut, "offset: %"PRIuMAX" length: %"PRIuMAX" bytes\n",
                        (luint)spPhrase->uiPhraseOffset, (luint)spPhrase->sPhrase.uiLength);
            }else{
                fprintf(spOut, "offset: %"PRIuMAX" length: %"PRIuMAX", %d-byte, %s-endian words\n",
                        (luint)spPhrase->uiPhraseOffset, (luint)spPhrase->sPhrase.uiLength, (int)uiWordSize, s_cpEndian);
            }
            cpLine = cpFmtFirstBytes(spExp->vpFmt, ucpBytes, iByteCount, FMT_CANONICAL, 0, 0);
            while(cpLine){
                fprintf(spOut, "%s", cpLine);
                cpLine = cpFmtNext(spExp->vpFmt);
            }
        }
    }else{
        fprintf(spOut, "''\n");
    }
}
static aint pfnRuleCallback(ast_data* spData){
    // Note: callback_data acpString, uiStringLength and uiParserOffset refer to the sub-string being parser.
    if(spData->uiState == ID_AST_PRE){
        apgex* spExp = (apgex*)spData->vpUserData;
        phrase_r* spLastPhrase;
        rule_r* spRule = &spExp->spRelRules[spData->uiIndex];
        if(spRule->bEnabled){
            // push this phrase on the list
            aint uiPhraseIndex = uiVecLen(spExp->vpVecRelPhrases);
            phrase_r* spPhrase = (phrase_r*)vpVecPush(spExp->vpVecRelPhrases, NULL);
            aint uiAbsoluteOffset = spData->uiPhraseOffset;
            vMakeRelPhrase(spExp, uiAbsoluteOffset, spData->uiPhraseLength, spPhrase);
            spPhrase->uiNext = APG_UNDEFINED;
            spRule->uiPhraseCount++;
            if(spRule->uiFirstPhrase == APG_UNDEFINED){
                // first phrase for this rule
                spRule->uiFirstPhrase = uiPhraseIndex;
            }else{
                // link it to the last phrase for this rule
                spLastPhrase = (phrase_r*)vpVecAt(spExp->vpVecRelPhrases, spRule->uiLastPhrase);
                if(!spLastPhrase){
                    XTHROW(spExp->spException, "rule phrases malfunction: last phrase is not in the list");
                }
                spLastPhrase->uiNext = uiPhraseIndex;
            }
            spRule->uiLastPhrase = uiPhraseIndex;
        }
    }
    return ID_AST_OK;
}

//static void pfnRuleCallback(callback_data* spData){
//    /*
//     * Note: callback_data acpString, uiStringLength and uiParserOffset refer to the sub-string being parser.
//     */
//    if(spData->uiParserState == ID_MATCH){
//        apgex* spExp = (apgex*)spData->vpUserData;
//        phrase_r* spLastPhrase;
//        rule_r* spRule = &spExp->spRelRules[spData->uiRuleIndex];
//        if(spRule->bEnabled){
//            // push this phrase on the list
//            aint uiPhraseIndex = uiVecLen(spExp->vpVecRelPhrases);
//            phrase_r* spPhrase = (phrase_r*)vpVecPush(spExp->vpVecRelPhrases, NULL);
//            aint uiAbsoluteOffset = spExp->uiLastIndex + spData->uiParserOffset;
//            vMakeRelPhrase(spExp, uiAbsoluteOffset, spData->uiParserPhraseLength, spPhrase);
//            spPhrase->uiNext = APG_UNDEFINED;
//            spRule->uiPhraseCount++;
//            if(spRule->uiFirstPhrase == APG_UNDEFINED){
//                // first phrase for this rule
//                spRule->uiFirstPhrase = uiPhraseIndex;
//            }else{
//                // link it to the last phrase for this rule
//                spLastPhrase = (phrase_r*)vpVecAt(spExp->vpVecRelPhrases, spRule->uiLastPhrase);
//                if(!spLastPhrase){
//                    XTHROW(spExp->spException, "rule phrases malfunction: last phrase is not in the list");
//                }
//                spLastPhrase->uiNext = uiPhraseIndex;
//            }
//            spRule->uiLastPhrase = uiPhraseIndex;
//        }
//    }
//}
//
static aint pfnUdtCallback(ast_data* spData){
    if(spData->uiState == ID_AST_PRE){
        apgex* spExp = (apgex*)spData->vpUserData;
        phrase_r* spLastPhrase;
        udt_r* spUdt = &spExp->spRelUdts[spData->uiIndex];
        if(spUdt->bEnabled){
            // push this phrase on the list
            aint uiPhraseIndex = uiVecLen(spExp->vpVecRelPhrases);
            phrase_r* spPhrase = (phrase_r*)vpVecPush(spExp->vpVecRelPhrases, NULL);
            aint uiAbsoluteOffset = spData->uiPhraseOffset;
            vMakeRelPhrase(spExp, uiAbsoluteOffset, spData->uiPhraseLength, spPhrase);
            spPhrase->uiNext = APG_UNDEFINED;
            spUdt->uiPhraseCount++;
            if(spUdt->uiFirstPhrase == APG_UNDEFINED){
                // first phrase for this rule
                spUdt->uiFirstPhrase = uiPhraseIndex;
            }else{
                // link it to the last phrase for this rule
                spLastPhrase = (phrase_r*)vpVecAt(spExp->vpVecRelPhrases, spUdt->uiLastPhrase);
                if(!spLastPhrase){
                    XTHROW(spExp->spException, "UDT phrases malfunction: last phrase is not in the list");
                }
                spLastPhrase->uiNext = uiPhraseIndex;
            }
            spUdt->uiLastPhrase = uiPhraseIndex;
        }
    }
    return ID_AST_OK;
}

//static void pfnUdtCallback(callback_data* spData){
//    if(spData->uiParserState == ID_ACTIVE){
//        apgex* spExp = (apgex*)spData->vpUserData;
//        phrase_r* spLastPhrase;
//        udt_r* spUdt = &spExp->spRelUdts[spData->uiUDTIndex];
//        spUdt->pfnUdt(spData);
//        if(spUdt->bEnabled && spData->uiCallbackState == ID_MATCH){
//            // push this phrase on the list
//            aint uiPhraseIndex = uiVecLen(spExp->vpVecRelPhrases);
//            phrase_r* spPhrase = (phrase_r*)vpVecPush(spExp->vpVecRelPhrases, NULL);
//            aint uiAbsoluteOffset = spExp->uiLastIndex + spData->uiParserOffset;
//            vMakeRelPhrase(spExp, uiAbsoluteOffset, spData->uiCallbackPhraseLength, spPhrase);
//            spPhrase->uiNext = APG_UNDEFINED;
//            spUdt->uiPhraseCount++;
//            if(spUdt->uiFirstPhrase == APG_UNDEFINED){
//                // first phrase for this rule
//                spUdt->uiFirstPhrase = uiPhraseIndex;
//            }else{
//                // link it to the last phrase for this rule
//                spLastPhrase = (phrase_r*)vpVecAt(spExp->vpVecRelPhrases, spUdt->uiLastPhrase);
//                if(!spLastPhrase){
//                    XTHROW(spExp->spException, "UDT phrases malfunction: last phrase is not in the list");
//                }
//                spLastPhrase->uiNext = uiPhraseIndex;
//            }
//            spUdt->uiLastPhrase = uiPhraseIndex;
//        }
//    }
//}

static void vClearForParse(apgex* spExp){
    spExp->uiNodeHits = 0;
    spExp->uiTreeDepth = 0;
    vVecClear(spExp->vpVecStrings);
    vVecClear(spExp->vpVecSource);
    vVecClear(spExp->vpVecOriginalSource);
    vVecClear(spExp->vpVecPhrases);
    vVecClear(spExp->vpVecRelPhrases);
    vVecClear(spExp->vpVecRules);
    vVecClear(spExp->vpVecUdts);
    vVecClear(spExp->vpVecSource);
    vVecClear(spExp->vpVecReplaceRaw);
    vVecClear(spExp->vpVecReplacement);
    vVecClear(spExp->vpVecSplitPhrases);
    spExp->bReplaceMode = APG_FALSE;
    spExp->spLastMatch = NULL;
    spExp->spLeftContext = NULL;
    spExp->spRightContext = NULL;
    aint ui;
    for(ui = 0; ui < spExp->uiRuleCount; ui++){
        spExp->spRelRules[ui].uiFirstPhrase = APG_UNDEFINED;
        spExp->spRelRules[ui].uiLastPhrase = APG_UNDEFINED;
        spExp->spRelRules[ui].uiPhraseCount = 0;
    }
    if(spExp->uiUdtCount){
        for(ui = 0; ui < spExp->uiUdtCount; ui++){
            spExp->spRelUdts[ui].uiFirstPhrase = APG_UNDEFINED;
            spExp->spRelUdts[ui].uiLastPhrase = APG_UNDEFINED;
            spExp->spRelUdts[ui].uiPhraseCount = 0;
        }
    }
}
static void vClearForPattern(apgex* spExp){
    vVecClear(spExp->vpVecStrings);
    vVecClear(spExp->vpVecSource);
    vVecClear(spExp->vpVecOriginalSource);
    vVecClear(spExp->vpVecPattern);
    vVecClear(spExp->vpVecFlags);
    vVecClear(spExp->vpVecRules);
    vVecClear(spExp->vpVecUdts);
    vVecClear(spExp->vpVecPhrases);
    vVecClear(spExp->vpVecRelPhrases);
    vVecClear(spExp->vpVecRelRules);
    vVecClear(spExp->vpVecRelUdts);
    vVecClear(spExp->vpVecReplaceRaw);
    vVecClear(spExp->vpVecReplacement);
    vVecClear(spExp->vpVecSplitPhrases);
    if(spExp->spDisplay && (spExp->spDisplay != stdout)){
        fclose(spExp->spDisplay);
        spExp->spDisplay = NULL;
    }
    if(spExp->vpExternalParser){
        // destroy the AST and trace objects, if any, but leave the user's external parser intact
        vAstDtor(spExp->vpAst);
        vTraceDtor(spExp->vpTrace);
    }else{
        // destroy the local parser which calls destructors for vpTrace & vpAst
        vParserDtor(spExp->vpParser);
    }
    spExp->vpParser = NULL;
    spExp->vpExternalParser = NULL;
    spExp->vpAst = NULL;
    spExp->vpTrace = NULL;
    spExp->spLastMatch = NULL;
    spExp->spLeftContext = NULL;
    spExp->spRightContext = NULL;
    spExp->spRelRules = NULL;
    spExp->spRelUdts = NULL;
    spExp->uiRuleCount = 0;
    spExp->uiUdtCount = 0;
    spExp->uiEnabledRuleCount = 0;
    spExp->uiEnabledUdtCount = 0;
    spExp->uiLastIndex = 0;
    spExp->uiNodeHits = 0;
    spExp->uiTreeDepth = 0;
    spExp->bDefaultMode = APG_TRUE;
    spExp->bTraceMode  = APG_FALSE;
    spExp->bTraceHtmlMode  = APG_FALSE;
    spExp->bGlobalMode = APG_FALSE;
    spExp->bPpptMode = APG_FALSE;
    spExp->bStickyMode = APG_FALSE;
    spExp->bReplaceMode = APG_FALSE;
}

static rule_r* spFindRule(apgex* spExp, const char* cpName){
    if(!spExp->vpParser || !spExp->uiRuleCount){
        XTHROW(spExp->spException, "no rules defined - vApgexPattern() or vApgexParser() must preceed this call");
    }
    if(!cpName || !cpName[0]){
        XTHROW(spExp->spException, "name cannot be NULL or empty");
    }
    // linear search for now
    aint ui;
    for(ui = 0; ui < spExp->uiRuleCount; ui++){
        if(iStriCmp(spExp->spRelRules[ui].cpRuleName, cpName) == 0){
            return &spExp->spRelRules[ui];
        }
    }
    return NULL;
}

static udt_r* spFindUdt(apgex* spExp, const char* cpName){
    if(!spExp->vpParser || !spExp->uiRuleCount){
        XTHROW(spExp->spException, "no rules defined - vApgexPattern() or vApgexParser() must preceed this call");
    }
    if(!cpName || !cpName[0]){
        XTHROW(spExp->spException, "name cannot be NULL or empty");
    }
    // linear search for now
    aint ui;
    for(ui = 0; ui < spExp->uiUdtCount; ui++){
        if(iStriCmp(spExp->spRelUdts[ui].cpUdtName, cpName) == 0){
            return &spExp->spRelUdts[ui];
        }
    }
    return NULL;
}

static abool bIsNameChar(char cChar){
    if(cChar >= 65 && cChar <= 90){ // upper case
        return APG_TRUE;
    }
    if(cChar >= 97 && cChar <= 122){ // lower case
        return APG_TRUE;
    }
    if(cChar == 45){ // hyphen "-"
        return APG_TRUE;
    }
    if(cChar == 95){ // underscore "_" (required in UDT names)
        return APG_TRUE;
    }
    return APG_FALSE;
}

