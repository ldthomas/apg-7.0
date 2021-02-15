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
/// \file syntax.h
/// \brief Header file for the syntax phase functions.

#ifndef SYNTAX_H_
#define SYNTAX_H_

/** \typedef pfn_findline
 * \brief Pointer to a function that finds the line that a given character is in.
 */
typedef aint (*pfn_findline)(line*, aint, aint, aint*);

/** struct syntax_data
 * \brief The syntax data that gets passed to the syntax parser's callback functions.
 */
typedef struct {
    api* spApi; ///< \brief Pointer to the parent API object context.
    abool bStrict; ///< \brief True if the grammar is to be treated as strict RFC5234 ABNF. No superset operators allowed.
    void* vpAltStack; ///< \brief  A stack vector to manage open ALT operators.
    alt_data* spTopAlt; ///< \brief Pointer to the top of the ALT stack.
    aint uiRuleError; ///< \brief True if an error has been found in the rule definition/
    aint uiRulesFound; ///< \brief True if rules have been found in the grammar.
    aint uiErrorsFound; ///< \brief True if any errors in the grammar have been found.
} syntax_data;

// prototypes
void vSabnfGrammarRuleCallbacks(void* vpParserCtx);

#endif /* SYNTAX_H_ */
