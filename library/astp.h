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
#ifndef LIB_ASTP_H_
#define LIB_ASTP_H_

#ifdef APG_AST

/** \file astp.h
 * \brief Private header file for the AST functions.
 *
 * Applications should not need to include this header directly.
 */

/** struct ast
 * \brief The AST object context. Holds the object's state.
 *
 * For internal AST object use only. Should never be accessed by an application directly.
 */
typedef struct{
    const void* vpValidate; ///< \brief A "magic number" indicating a valid, initialized AST object.
    exception* spException; ///< \brief Pointer to an exception structure for reporting
                            /// fatal errors back to the parser's catch block scope.
    parser* spParser; ///< \brief Pointer to the parent parser.
    void* vpVecRecords; ///< \brief Pointer to the vector holding the AST records (two for each saved node).
    void* vpVecThatStack; ///< \brief Pointer to the vector holding a LIFO stack to match up and down node records.
    void* vpVecOpenStack; ///< \brief Pointer to a vector LIFO stack of open rule nodes.
    ast_callback* pfnRuleCallbacks; ///< \brief An array of rule name call back functions.
    ast_callback* pfnUdtCallbacks; ///< \brief An array of UDT call back functions.
//    achar* acpInput; ///< \brief Pointer to the input string.
//    aint uiLength; ///< \brief Number of alphabet characters in the input string.
} ast;

/** @name Private AST Functions
 *
 * These functions are primarily for the parser to call (via macros, e.g. \ref AST_CLEAR, etc.)
 */
///@{
void vAstClear(void* vpCtx);
void vAstRuleOpen(void* vpCtx, aint uiRuleIndex, aint uiPhraseOffset);
void vAstRuleClose(void* vpCtx, aint uiRuleIndex, aint uiState, aint uiPhraseOffset, aint uiPhraseLength);
void vAstOpOpen(void* vpCtx);
void vAstOpClose(void* vpCtx, aint uiState);
///@}

#endif /* APG_AST */
#endif /* LIB_ASTP_H_ */
