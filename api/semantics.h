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
/// \file semantics.h
/// \brief Header file for the semantic translation functions.

#ifndef SEMANTICS_H_
#define SEMANTICS_H_

/**< Rule and UDT names have a maximum size of 255 characters (plus a null term.) */
#define RULENAME_MAX 256

/** \struct semantic_udt
 * \brief Generalized UDT for first-pass semantic processing */
typedef struct{
    char* cpName; /**< Pointer to the UDT name in input grammar (not null-termed). */
    aint uiNameLength; /**< Number of characters in the name, not including the null term */
    aint uiIndex; /**< index of this UDT in the UDT list */
    aint uiEmpty; /**< APG_TRUE if the UDT can be empty, APG_FALSE otherwise. */
} semantic_udt;

/** \struct semantic_op
 * \brief Generalized opcode for first-pass semantic processing */
typedef struct{
    aint uiId; /**< Opcode id, ID_ALT, etc. */
    void* vpVecChildList; /**< A vector of opcode indexes for children of ALT and CAT operators. */
    luint luiMin; /**< Minimum count for REP/TRG operator. */
    luint luiMax; /**< Maximum count for REP/TRG operator. */
    aint uiEmpty; /**< APG_TRUE if UDT can be empty, APG_FALSE otherwise. */
    aint uiCase; /**< ID_BKR_CASE_S or ID_BKR_CASE_I for BKR. */
    aint uiMode; /**< ID_BKR_MODE_U of ID_BKR_MODE_P for BKR. */
    int uiStringIndex; /**< Offset into the achar table for the string of a TLS & TBS operators. */
    aint uiStringLength; /**< The string length. */
    char* cpName; /**< Pointer to rule/UDT/BKR name in input grammar (not null-termed). */
    aint uiNameLength; /**< Number of characters in the name, not including the null term */
    aint uiBkrIndex; /**< the index to the rule, if < rule Count, or index to UDT if >= rule count */
    aint uiSeq; /**< used to sequence the remaining opcodes after removal of one-child ALT & CAT and REP(1,1) operators */
} semantic_op;

/** \struct semantic_rule
 * \brief Generalized rule for first-pass semantic processing */
typedef struct{
    char* cpName; /**< Pointer to rule name in input grammar (not null-termed). */
//    aint uiNameOffset; /**< Offset to the rule name in the string table. Points to a null-term string. */
    aint uiNameLength; /**< Number of characters in the name, not including the null term */
    aint uiIndex; /**< index of this rule in the rule list */
    aint uiCurrentAlt; /** Index to the current ALT operator. */
    aint uiCurrentCat; /**  Index to the current CAT operator. */
    semantic_op* spCurrentOp; /**< The current opcode. Points to the top (last) of the rule opcode vector. */
    void* vpVecAltStack; /** Stack of indexes pointing to the ALT operators in the opcode vector (vpVecOps).  */
    void* vpVecCatStack; /** Stack of indexes pointing to the CAT operators in the opcode vector (vpVecOps).  */
    void* vpVecOps; /**< vector of semantic_op structures */
} semantic_rule;

/** \struct semantic_data
 * \brief User data passed to the AST translator for use by the AST callback functions. */
typedef struct {
    api* spApi; /** Pointer to the API context.  */
    void* vpMem; /** Special memory context used only for the vectors created during AST translation.
    vMemDtor(vpMem) gives easy cleanup. */
    aint uiIncAlt; /** Used to report to rule name whether this is a new rule or continuation of a previous rule.  */
    aint uiErrorsFound; /** Incremented for each error reported  */
    luint luiNum; /**< used by lower-level rules, dnum, xnum, bnum, to save data for higher rules, dmax, etc. */
    semantic_rule* spCurrentRule; /**< always points to the current rule being processed */
    char* cpName; /**< Pointer to rule name in input grammar (not null-termed). */
    aint uiNameOffset; /**< offset to the first character of the rule name in the grammar */
    aint uiNameLength; /**< Number of characters in the name, not including the null term */
    aint uiRuleIndex; /**< next index of the rule in the rule list */
    aint uiUdtIndex; /**< next index of the Udt in the Udt list */
    void* vpVecAcharsTable; /** Vector of achar characters needed by TLS and TBS operators.  */
    void* vpVecChildIndexTable; /** Vector of child index lists used by ALT and CAT operators.  */
    void* vpVecStringTable; /** Table of char (ASCII) characters for rule names and UDT names. */
    void* vpVecRules; /**< vector of semantic_rule structures */
    void* vpVecUdts; /**< vector of semantic_Udt structures */
} semantic_data;

// prototypes
void vSabnfGrammarAstCallbacks(void* vpParserCtx);
//void vSabnfGrammarAstCallbacks2(void* vpParserCtx);
aint uiFindRule(semantic_rule* spRules, aint uiRuleCount, const char* cpName, aint uiNameLength);
aint uiFindUdt(semantic_udt* spUdts, aint uiUdtCount, const char* cpName, aint uiNameLength);

#endif /* SEMANTICS_H_ */
