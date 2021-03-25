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
/** \file apip.h
 * \brief Private header file for the APG API suite of functions.
 */

#ifndef APIP_H_
#define APIP_H_

/** \struct alt_data
 * \brief Used by syntax.c but needed here for constructor/destructor.
 */
typedef struct {
    aint uiGroupOpen;
    aint uiGroupError;
    aint uiOptionOpen;
    aint uiOptionError;
    aint uiTlsOpen;
    aint uiClsOpen;
    aint uiProseValOpen;
    aint uiStringTab;
    aint uiBasicError;
} alt_data;

/** \struct api_rule
 * \brief API information about each rule.
 */
typedef struct {
    char *cpName; /**< \brief  pointer to null-terminated string in the string table */
    aint uiIndex; /**< \brief  index of this rule in the rule list */
    aint uiOpOffset; /**< \brief  offset into the opcode table to the first opcode of this rule */
    aint uiOpCount; /**< \brief  the number of opcodes in this rule*/
    abool bIsOpen; /**< \brief  used for walking the SEST, set to true at the root it will tell when a recursive instance of the rule is found */
    abool bIsComplete; /**< \brief  used when processing rules recursively. If the rule is already complete it need not be recursed again. */
    aint uiPpptIndex; /**< \brief  Index to the PPPT map for this opcode */
    abool bProtected; /**< \brief  if true, this rule will be protected from being hidden under a fully-predictive node in the parse tree. */
} api_rule;

/** \struct api_udt
 * \brief API information about each UDT.
 */
typedef struct {
    char *cpName; /**< \brief  pointer to null-terminated string in the string table */
    aint uiIndex; /**< \brief  index of this UDT in the UDT list */
    aint uiEmpty; /**< \brief  APG_TRUE if this UDT can be empty, APG_FALSE otherwise  */
} api_udt;

/** \struct api_op
 * \brief API information about each opcode.
 */
typedef struct {
    aint uiId; /**< \brief  type of opcode, ID_ALT, etc.  */
    aint uiIndex; /**< \brief  index of this referenced rule or UDT  */
    aint uiEmpty; /**< \brief  APG_TRUE if this UDT can be empty, APG_FALSE otherwise  */
    aint *uipChildIndex; /**< \brief  pointer to the first child index of this ALT or CAT operator */
    aint uiChildCount; /**< \brief  number of children for this ALT or CAT operator */
    luint luiMin; /**< \brief  minimum value for REP and TRG opcodes */
    luint luiMax; /**< \brief  maximum value for REP and TRG opcodes */
    luint *luipAchar; /**< \brief  pointer to the first character in the achar table for this TLS/TBS operator */
    aint uiAcharLength; /**< \brief  number of characters in TLS/TBS strings */
    aint uiCase; /**< \brief  ID_BKR_CASE_S or ID_BKR_CASE_I for BKR. */
    aint uiMode; /**< \brief  ID_BKR_MODE_U of ID_BKR_MODE_P for BKR. */
    aint uiBkrIndex; /**< \brief  if BKR, this is the index to the rule or UDT that is being back referenced */
    aint uiPpptIndex; /**< \brief  Index to the PPPT map for this opcode */
} api_op;

/** \struct api_attr_w
 * \brief Working attribute information about a each rule.
 *
 * Attribute construction is multi-step and the working information retains attribute data between steps.
 */
typedef struct {
    abool bLeft; /**< \brief  APG_TRUE if the rule is left recursive */
    abool bNested; /**< \brief  APG_TRUE if the rule is nested recursive */
    abool bRight; /**< \brief  APG_TRUE if the rule is right recursive */
    abool bCyclic; /**< \brief  APG_TRUE if the rule is cyclic */
    abool bFinite; /**< \brief  APG_TRUE if the rule is finite */
    abool bEmpty; /**< \brief  APG_TRUE if the rule can be empty */
    abool bLeaf; /**< \brief APG_TRUE if this is a leaf rule (appears for a second time on a branch) */
    char *cpRuleName; /**< \brief  the rule name for these attributes */
    aint uiRuleIndex; /**< \brief  the index of the rule for these attributes */
    aint uiRecursiveType; /**< \brief  ID_ATTR_N, ID_ATTR_R, ID_ATTR_MR, ID_ATTR_NMR, or ID_ATTR_RMR */
    aint uiMRGroup; /**< \brief  the group number, if this is a member of a mutually-recursive group (there may be multiple groups) */
    abool *bpRefersToUdt; /**< \brief  a list of all the UDTs that this rule refers to */
    abool *bpRefersTo; /**< \brief  a list of all the rules that this rule refers to */
    abool *bpIsReferencedBy; /**< \brief  a list of all the rules that refer to this rule*/

    // admin during discovery
    abool bIsOpen; /**< \brief  admin */
    abool bIsComplete; /**< \brief  admin */
} api_attr_w;

/**< \struct api
 * \brief The API context.
 */
typedef struct {
    const void *vpValidate; /**< \brief  the "magic number" to indicate that this is a valid context */
    exception* spException;
    void *vpMem; /**< \brief  Pointer to the memory context used for all memory allocations and exceptions thrown. */
    void *vpParser; /**< \brief  context handle to the SABNF grammar parser object */
    void *vpAltStack; /**< \brief  A temporary vector for the AST translator. */
    void *vpAst; /**< \brief  context handle to the AST object */
    void *vpAttrsCtx; /**< \brief  context handle to the attributes object */
    void* vpOutputAcharTable; ///< \brief Storage for variable character width output parser achar table.
    void* vpOutputParserInit; ///< \brief Storage for variable integer width output parser init data.
    luint* luipInit; ///< \brief Storage variable for intermediate parser initialization data.
    char* cpLineBuffer; ///< \brief Storage variable for intermediate parser line data.

    // input
    void *vpVecInput; /**< \brief  The (ASCII) input grammar files and/or strings accumulate here. Always a NULL-terminated string. */
    void* vpVecGrammar; ///< \brief The (achar) input grammar, if sizeof(achar) > sizeof(char).
    char *cpInput; /** Pointer to the input in the above vpVecInput vector, if any. NULL otherwise. */
    aint uiInputLength; /**< \brief  The number of input characters. */
    void* vpLines; ///< \brief Context pointer to a `lines` object.
    void* vpVecTempChars; ///< \brief Temporary vector of characters. Here for clean up on unusual exit.

    // rules & opcodes
    api_rule *spRules; ///< \brief Points to an array of rule structures.
    aint uiRuleCount; ///< \brief The number of rules in the SABNF grammar and in the array.
    api_udt *spUdts; ///< \brief  Points to an array of UDT structures, if one or more UDTs are referenced in the SABNF grammar.
    aint uiUdtCount; ///< \brief  The number of UDTs referenced in the SABNF grammar.
    char *cpStringTable; ///< \brief Pointer to a list of null-terminated ASCII strings representing the rule and UDT names.
    aint uiStringTableLength; ///< \brief The number of characters in the string table.
    aint uiVersionOffset; ///< \brief Offset into the string table for the Version Number string.
    aint uiVersionLength; ///< \brief Length of the Version Number string.
    aint uiLicenseOffset; ///< \brief Offset into the string table for the License string.
    aint uiLicenseLength; ///< \brief Length of the License string.
    aint uiCopyrightOffset; ///< \brief Offset into the string table for the Copyright string.
    aint uiCopyrightLength; ///< \brief Length of the copyright string.
    luint *luipAcharTable; ///< \brief Pointer to the Achar Table - a table of all of the alphabet characters referenced by the terminal nodes, TLS, TBL & TRG.
    aint uiAcharTableLength; ///< \brief Number of alphabet characters in the Achar Table.
    aint *uipChildIndexTable; /**< \brief Pointer to a list of child indexes. ALT & CAT operators have two or more children operators.
    Each has a list of its children operators. This table has that list for each of the ALT and CAT operators in the SABNF grammar. */
    aint uiChildIndexTableLength; ///< \brief The number of indexes (integers) in the child index table.
    api_op *spOpcodes; ///< \brief Pointer to the array of opcodes for the SANF grammar.
    aint uiOpcodeCount; ///< \brief Number of opcodes.
//    char *cpInitName; ///< \brief

    // PPPT table
    abool bUsePppt; ///< \brief True of PPPT are being used.
    uint8_t *ucpPpptUndecidedMap; ///< \brief Common PPPT character map for an operator that is indeterminate on the next alphabet character.
    uint8_t *ucpPpptEmptyMap; ///< \brief Common PPPT character map for an operator that is an empty match on the next alphabet character.
    uint8_t *ucpPpptTable; ///< \brief Pointer to the PPPT table of operator maps.
    luint luiPpptTableLength; ///< \brief The PPPT length.
    luint luiPpptMapCount; ///< \brief The number of operator maps in the table.
    luint luiPpptMapSize; ///< \brief The size, in bytes, of a single operator map.
    luint luiAcharMin; ///< \brief The minimum alphabet character referenced by the terminal nodes, TLS, TBL & TRG.
    luint luiAcharMax; ///< \brief The maximum alphabet character referenced by the terminal nodes, TLS, TBL & TRG.
    luint luiAcharEos; ///< \brief The special End-Of-String character. In practice, luiAcharMax + 1.

    // the error hierarchy
    void *vpLog; ///< \brief A msglog context for error reporting.

    // the grammar stage indicators.
    abool bInputValid; /**< \brief  APG_TRUE if theer is input and it has been validated, APG_FALSE otherwise */
    abool bSyntaxValid; /**< \brief  APG_TRUE if the input syntax is valid, APG_FALSE otherwise */
    abool bSemanticsValid; /**< \brief  APG_TRUE if the the input semantics are valid.
         That is, the opcodes for the parser have been generated. APG_FALSE otherwise */
    abool bAttributesValid; /**< \brief  APG_TRUE if there the rule attributes have
        been computed and have no fatal errors, APG_FALSE otherwise. */
    abool bAttributesComputed; /**< \brief  APG_TRUE if attributes have been computed
        (even is there are attribute errors), APG_FALSE otherwise. */
} api;

void vLineError(api *spCtx, aint uiCharIndex, const char *cpSrc, const char *cpMsg);
void vHtmlHeader(FILE *spFile, const char *cpTitle);
void vHtmlFooter(FILE *spFile);

#endif /* APIP_H_ */
