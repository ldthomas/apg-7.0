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
#ifndef LIB_BACKREF_H_
#define LIB_BACKREF_H_
#ifdef APG_BKR

/** \file backref.h
 * \brief Private declarations common to both universal and parent modes.
 *
 * Used internally by the parser's operators. Should never be called directly by the application.
 */

/** \struct bkr_rule
 * \brief Back referencing information for each rule.
 */
typedef struct  {
    rule* spRule;
    aint uiHasBackRef; /**< \brief APG_TRUE if this rule refers to a referenced rule in it syntax tree, APG_FALSE otherwise */
    aint uiIsBackRef; /**< \brief APG_TRUE if this rule has been back referenced, APG_FALSE otherwise */
    aint uiBackRefIndex; /**< \brief If this rule is back referenced, this is the rule's index in the bkr map */
} bkr_rule;

/** \struct bkr_udt
 * \brief Back referencing information for each UDT.
 */
typedef struct {
    udt* spUdt;
    aint uiIsBackRef; /**< \brief APG_TRUE if this UDT has been back referenced in, APG_FALSE otherwise */
    aint uiBackRefIndex; /**< \brief If this UDT is back referenced, this is the UDT's index in the bkr map */
} bkr_udt;

/** \struct bkr_phrase
 * \brief Defines one frame on the back reference stack.
 *
 */
typedef struct {
    aint uiPhraseOffset; /**< \brief The offset to the matched phrase. */
    aint uiPhraseLength; /**< \brief The matched phrase length. */
} bkr_phrase;

/** \struct backref
 * \brief The back reference object's context.
 */
typedef struct {
    const void* vpValidate; /**< \brief "magic" number to validate that the context is valid */
    exception* spException; /**< \brief Pointer to the exception structure for reporting fatal errors. */
    parser* spParserCtx; /**< \brief Pointer to the parser context that created this object */
    bkr_rule* spRules; /**< \brief Pointer to the back reference rule information. */
    bkr_udt* spUdts; /**< \brief Pointer to the back reference UDT information. */
    void** vppPhraseStacks; /**< \brief An array of frame structs, vector context if rule/UDT index is universally back referenced.
                         NULL otherwise. */
    void* vpCheckPoints; /**< \brief A stack of check points (a check point is the current record count in each of the frame stacks). */
    void* vpOpenRules; /**< \brief A stack indicating if the top rule has a BKR in its syntax tree. */
    aint uiBkrCount; /**< \brief Number of back referenced rules/UDTS. */
    aint uiBkrRulesOpen; /**< \brief Counter for the number of open rules that have BKR nodes in the rule SEST.
    (Single-Expansion Syntax Tree) */
} backref;

#endif /* APG_BKR */
#endif /* LIB_BACKREF_H_ */
