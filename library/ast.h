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
#ifndef LIB_AST_H_
#define LIB_AST_H_
/// \file ast.h
/// \brief Public header file for the AST functions.

#ifdef APG_AST

/** \struct ast_record
 * \brief Format of an AST record.
 *
 * Available if user wants to write a custom AST translator.
 */
typedef struct{
    const char* cpName; ///< \brief Name of the rule or UDT of this record.
    aint uiIndex; ///< \brief Index of the rule or UDT of this record.
    aint uiThisRecord; ///< \brief The record number.
    aint uiThatRecord; /**< \brief The matching record number. That is, if uiThisRecord the number of the record that
    opens the rule, uiThatRecord is the number of the record that closes the rule. And vice versa. */
    aint uiPhraseOffset; ///< \brief The offset into the input string to the first character of the matched phrase.
    aint uiPhraseLength; ///< \brief The number of characters in the matched phrase.
    aint uiState; ///< \brief ID_AST_PRE if the current record opens the rule, ID_AST_POST if the current record closes the rule.
    abool bIsUdt; ///< \brief True if this record is for a UDT.
} ast_record;

/** \struct ast_info
 * \brief All the information a user needs to write a custom AST translator.
 */
typedef struct{
    const achar* acpString; ///< \brief The parsed input string.
    ast_record* spRecords; ///< \brief The list of records in the order of a depth-first traversal of the AST.
    aint uiRuleCount; ///< \brief The number of rules.
    aint uiUdtCount; ///< \brief The number of UDTs.
    aint uiStringLength; ///< \brief The number of characters in the input string.
    aint uiRecordCount; ///< \brief The number of records (two for each node of the AST, one down traversal, one up.)
} ast_info;

/** \struct ast_data
 * \brief Input data to the AST callback functions.
 */
typedef struct{
    const achar* acpString; /**< pointer to the input string */
    aint uiStringLength; /**< input string length */
    aint uiPhraseOffset; /**< offset to the first character of the matched phrase */
    aint uiPhraseLength; /**< matched phrase length */
    aint uiState; /**< ID_AST_PRE on pre-node traversal, ID_AST_POST on post-node traversal */
    const char* cpName; ///< \brief Name of the rule or UDT.
    aint uiIndex; ///< \brief Index of the rule or UDT.
    abool bIsUdt; ///< \brief True if this record is for a UDT. False if it is for a rule.
    exception* spException; /**< \brief Use but don't alter.
    Use to throw exceptions to the AST catch block. */
    void* vpUserData; /**< user-supplied data, if any. Not used by AST */
} ast_data;

/** \typedef ast_callback
 * \brief The prototype for AST translation callback functions.
 * \param spData Pointer to the callback data passed to the callback function.
 * \return One of:
 *  - ID_AST_OK normal return
 *  - ID_AST_SKIP skip traversal of the remaining branch below this node
 *
 */
typedef aint (*ast_callback)(ast_data* spData);

void* vpAstCtor(void* vpParserCtx);
void vAstDtor(void* vpCtx);
void vAstTranslate(void* vpCtx, void* vpUserData);
void vAstInfo(void* vpCtx, ast_info* spInfo);
void vAstSetRuleCallback(void* vpCtx, aint uiRuleIndex, ast_callback pfnCallback);
void vAstSetUdtCallback(void* vpCtx, aint uiUdtIndex, ast_callback pfnCallback);
void vAstClear(void* vpCtx);
abool bAstValidate(void* vpCtx);

#endif /* APG_AST */
#endif /* LIB_AST_H_ */
