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
/** \file ast.c
 * \brief The functions for generating and translating the Abstract Syntax Tree (AST).
 */

#include "./apg.h"
#ifdef APG_AST
#include "./lib.h"
#include "./parserp.h"
#include "./astp.h"

static const void* s_vpMagicNumber = (void*)"ast";

/** \brief The AST object constructor.
 *
 * This object is a "sub-object" of the parser. The parser will keep a pointer to and use
 * this object to generate the AST records. Note that there is no associated destructor.
 * This object is destroyed by its parent parser object's destructor, vParserDtor().
 * \param vpParserCtx Pointer to a valid parser context returned from vpParserCtor();
 * If invalid, the application will silently exit with a \ref BAD_CONTEXT exit code;
 * \return Returns a pointer to the object context. Throws an exception on any errors.
 */
void* vpAstCtor(void* vpParserCtx){
    if(!bParserValidate(vpParserCtx)){
        vExContext();
        return NULL; // should never return
    }
    parser* spParser = (parser*)vpParserCtx;
    if(spParser->vpAst){
        vAstDtor(spParser->vpAst);
        spParser->vpAst = NULL;
    }
    ast* spCtx = NULL;
    spCtx = (ast*)vpMemAlloc(spParser->vpMem, sizeof(ast));
    memset((void*)spCtx, 0, sizeof(ast));
    spCtx->spException = spMemException(spParser->vpMem);
    spCtx->pfnRuleCallbacks = (ast_callback*)vpMemAlloc(spParser->vpMem, (sizeof(ast_callback) * spParser->uiRuleCount));
    memset((void*)spCtx->pfnRuleCallbacks, 0, (sizeof(ast_callback) * spParser->uiRuleCount));
    if(spParser->uiUdtCount){
        spCtx->pfnUdtCallbacks = (ast_callback*)vpMemAlloc(spParser->vpMem, (sizeof(ast_callback) * spParser->uiUdtCount));
        memset((void*)spCtx->pfnUdtCallbacks, 0, (sizeof(ast_callback) * spParser->uiUdtCount));
    }
    spCtx->vpVecThatStack = vpVecCtor(spParser->vpMem, sizeof(aint), 1000);
    spCtx->vpVecOpenStack = vpVecCtor(spParser->vpMem, sizeof(aint), 100);
    spCtx->vpVecRecords = vpVecCtor(spParser->vpMem, sizeof(ast_record), 1000);
    // success
    spCtx->spParser = spParser;
    spParser->vpAst = (void*)spCtx;
    spCtx->vpValidate = s_vpMagicNumber;
    return (void*)spCtx;
}

void vAstDtor(void* vpCtx){
    ast* spCtx = (ast*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        void* vpMem = spCtx->spParser->vpMem;
        vMemFree(vpMem, spCtx->pfnRuleCallbacks);
        if(spCtx->spParser->uiUdtCount){
            vMemFree(vpMem, spCtx->pfnUdtCallbacks);
        }
        vVecDtor(spCtx->vpVecThatStack);
        vVecDtor(spCtx->vpVecOpenStack);
        vVecDtor(spCtx->vpVecRecords);
        spCtx->spParser->vpAst = NULL;
        memset((void*)spCtx, 0, sizeof(ast));
        vMemFree(vpMem, spCtx);
    }else{
        vExContext();
    }
}

/** \brief Validate an AST context pointer.
 * \param vpCtx Pointer to a possible AST context returned by vpAstCtor().
 * \return True if the pointer is valid, false otherwise.
 */
abool bAstValidate(void* vpCtx){
    ast* spCtx = (ast*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        return APG_TRUE;
    }
    return APG_FALSE;
}

/** \brief Clear the AST records for reuse of the AST object.
 * \param vpCtx Pointer to an AST context returned from \ref vpAstCtor();
 * Silently ignored if NULL.
 * However, if non-NULL it must be a valid AST context pointer or the application
 * will exit with \ref BAD_CONTEXT.
 */
void vAstClear(void* vpCtx){
    ast* spCtx = (ast*)vpCtx;
    if(vpCtx){
        if(spCtx->vpValidate == s_vpMagicNumber){
            vVecClear(spCtx->vpVecThatStack);
            vVecClear(spCtx->vpVecOpenStack);
            vVecClear(spCtx->vpVecRecords);
        }else{
            vExContext();
        }
    }
}

/** \brief Retrieve basic information about the AST object.
 * \param vpCtx Pointer to a valid AST context returned by vpAstCtor().
 * If invalid, the application will silently exit with a \ref BAD_CONTEXT exit code;
 * \param spInfo Pointer to the user's info struct to receive the information.
 *
 */
void vAstInfo(void* vpCtx, ast_info* spInfo){
    ast* spCtx = (ast*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if(!spInfo){
            XTHROW(spCtx->spException, "spInfo cannot be NULL");
        }
        spInfo->acpString = spCtx->spParser->acpInputString;
        spInfo->uiStringLength = spCtx->spParser->uiInputStringLength;
        spInfo->uiRuleCount = spCtx->spParser->uiRuleCount;
        spInfo->uiUdtCount = spCtx->spParser->uiUdtCount;
        spInfo->uiRecordCount = uiVecLen(spCtx->vpVecRecords);
        spInfo->spRecords = (ast_record*)vpVecFirst(spCtx->vpVecRecords);
    }else{
        vExContext();
    }
}

/** \brief Do a depth-first traversal of the AST with user-defined callback functions to translate the AST records.
 *
 * NOTE: There is an important difference between the role of the call back functions
 * during parsing and translation. See this [note](\ref astcallback) for a discussion of this important distinction.
 *
 * \param vpCtx Pointer to a valid AST context returned by vpAstCtor().
 * If invalid, the application will silently exit with a \ref BAD_CONTEXT exit code;
 * \param vpUserData Pointer to optional user data. This pointer is passed to the user's callback functions
 * for any purpose the application requires for the translation. May be NULL.
 *
 */
void vAstTranslate(void* vpCtx, void* vpUserData){
    ast* spCtx = (ast*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        ast_record* spRecords = (ast_record*)vpVecFirst(spCtx->vpVecRecords);
        if(spRecords){
            aint uiRecords = uiVecLen(spCtx->vpVecRecords);
            aint ui = 0;
            ast_record* spRecord;
            ast_data sData;
            aint uiReturn;
            sData.acpString = spCtx->spParser->acpInputString;
            sData.uiStringLength = spCtx->spParser->uiInputStringLength;
            sData.vpUserData = vpUserData;
            sData.spException = spCtx->spException;
            while( ui < uiRecords){
                spRecord = &spRecords[ui];
                sData.uiPhraseLength = spRecord->uiPhraseLength;
                sData.uiPhraseOffset = spRecord->uiPhraseOffset;
                sData.uiState = spRecord->uiState;
                sData.cpName = spRecord->cpName;
                sData.uiIndex = spRecord->uiIndex;
                sData.bIsUdt = spRecord->bIsUdt;
                uiReturn = ID_AST_OK;
                if(spRecord->bIsUdt){
                    if(spCtx->pfnUdtCallbacks[spRecord->uiIndex]){
                        uiReturn = spCtx->pfnUdtCallbacks[spRecord->uiIndex](&sData);
                    }
                }else{
                    if(spCtx->pfnRuleCallbacks[spRecord->uiIndex]){
                        uiReturn = spCtx->pfnRuleCallbacks[spRecord->uiIndex](&sData);
                    }
                }
                if(sData.uiState == ID_AST_PRE && uiReturn == ID_AST_SKIP){
                    ui = spRecord->uiThatRecord;
                }else{
                    ui++;
                }
            }
        }
    }else{
        vExContext();
    }
}

/**
 * \page astcallback AST Call Back Functions
 *
 * During the parsing of the input string the AST object will collect records for
 * all rule and UDT nodes with non-NULL call back function pointers. The call back function itself
 * is never called during the parsing stage. Therefore, for parsing purposes, any non-NULL value will suffice
 * to collect a record for the rule or UDT.
 *
 * During translation, \ref vAstTranslate(),
 * rule and UDT call back functions are called only if
 *  - a record was collected for the node during parsing
 *  - a non-NULL pointer to the function has been defined
 *
 *  Therefore, it is possible to redefine the call back functions between the parsing and translation operations.
 *   - Prior to parsing, use the functions vAstSetRuleCallback() and vAstSetUdtCallback()
 *  to set any non-NULL values for the rules and UDTs of interest.
 *   - Prior to translation, use those same functions to set valid call back function pointers
 *  for the nodes of translation interest and NULL for those to ignore.
 *
 *  Note that this means, also, that multiple translations of an AST are possible.
 *  Simply redefine the call back functions with vAstSetRuleCallback() and vAstSetUdtCallback()
 *  and rerun vAstTranslate() as many times a needed. The AST data records remain unchanged
 *  until a new input string is parsed or the object is destroyed.
 */
/** \brief Define a callback function for a single rule on the AST.
 *
 * NOTE: There is an important difference between the role of the call back functions
 * during parsing and translation. See this [note](\ref astcallback) for a discussion of this important distinction.
 *
 * \param vpCtx Pointer to a valid AST context returned by vpAstCtor().
 * If invalid, the application will silently exit with a \ref BAD_CONTEXT exit code;
 * \param uiRuleIndex The index of the rule to attach this callback function to.
 * \param pfnCallback Pointer to the callback function. Must match the ast_callback prototype.
 */
void vAstSetRuleCallback(void* vpCtx, aint uiRuleIndex, ast_callback pfnCallback){
    ast* spCtx = (ast*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if(uiRuleIndex < spCtx->spParser->uiRuleCount){
            spCtx->pfnRuleCallbacks[uiRuleIndex] = pfnCallback;
        }else{
            XTHROW(spCtx->spException, "rule index out of range");
        }
    }else{
        vExContext();
    }
}

/** \brief Define a callback function for a single UDT on the AST.

 * NOTE: There is an important difference between the role of the call back functions
 * during parsing and translation. See this [note](\ref astcallback) for a discussion of this important distinction.
 *
 * \param vpCtx Pointer to a valid AST context returned by vpAstCtor().
 * If invalid, the application will silently exit with a \ref BAD_CONTEXT exit code;
 * \param uiUdtIndex The index of the UDT to attach this callback function to.
 * \param pfnCallback Pointer to the callback function. Must match the ast_callback prototype.
 */
void vAstSetUdtCallback(void* vpCtx, aint uiUdtIndex, ast_callback pfnCallback){
    ast* spCtx = (ast*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if(uiUdtIndex < spCtx->spParser->uiUdtCount){
            spCtx->pfnUdtCallbacks[uiUdtIndex] = pfnCallback;
        }else{
            XTHROW(spCtx->spException, "UDT index out of range");
        }
    }else{
        vExContext();
    }
}

/** \brief Called by parser's RNM operator before downward traversal.
 * \param vpCtx Pointer to a valid AST context returned by vpAstCtor().
 * No validation is done here as this function is always called by a trusted parser operator function.
 * \param uiRuleIndex The index of the RNM rule.
 * If uiRuleIndex > uiRuleCount then it represents a UDT whose index is uiRuleIndex - uiRuleCount.
 * \param uiPhraseOffset Offset into the input string of the offest of the matched phrase.
 */
void vAstRuleOpen(void* vpCtx, aint uiRuleIndex, aint uiPhraseOffset){
    ast* spCtx = (ast*)vpCtx;
    aint uiRecordCount = uiVecLen(spCtx->vpVecRecords);
    vpVecPush(spCtx->vpVecOpenStack, (void*)&uiRecordCount);
    aint uiIndex = uiRuleIndex;
    abool bIsUdt = APG_FALSE;
    if(uiRuleIndex >= spCtx->spParser->uiRuleCount){
        uiIndex = uiRuleIndex - spCtx->spParser->uiRuleCount;
        bIsUdt = APG_TRUE;
        if(spCtx->pfnUdtCallbacks[uiIndex] == NULL){
            return;
        }
    }else{
        if(spCtx->pfnRuleCallbacks[uiIndex] == NULL){
            return;
        }
    }
    ast_record sRecord;
    if(bIsUdt){
        sRecord.cpName = spCtx->spParser->spUdts[uiIndex].cpUdtName;
    }else{
        sRecord.cpName = spCtx->spParser->spRules[uiIndex].cpRuleName;
    }
    sRecord.bIsUdt = bIsUdt;
    sRecord.uiIndex = uiIndex;
    sRecord.uiPhraseLength = APG_UNDEFINED;
    sRecord.uiPhraseOffset = uiPhraseOffset;
    sRecord.uiState = ID_AST_PRE;
    sRecord.uiThatRecord = APG_UNDEFINED;
    sRecord.uiThisRecord = uiVecLen(spCtx->vpVecRecords);
    vpVecPush(spCtx->vpVecThatStack, (void*)&sRecord.uiThisRecord);
    vpVecPush(spCtx->vpVecRecords, (void*)&sRecord);
}

/** \brief Called by parser's RNM operator after upward traversal.
 * \param vpCtx - AST context handle returned from \see vpAstCtor.
 * No validation is done here as this function is always called by a trusted parser operator function.
 * \param uiRuleIndex The index of the RNM rule.
 * If uiRuleIndex > uiRuleCount then it represents a UDT whose index is uiRuleIndex - uiRuleCount.
 * \param uiState ID_MATCH or ID_NOMATCH, the result of the parse for this rule or UDT.
 * \param uiPhraseOffset Offset into the input string of the offest of the matched phrase.
 * \param uiPhraseLength The number of match characters in the phrase.
 */
void vAstRuleClose(void* vpCtx, aint uiRuleIndex, aint uiState, aint uiPhraseOffset, aint uiPhraseLength){
    ast* spCtx = (ast*)vpCtx;
    aint* uipRecordCount = (aint*)vpVecPop(spCtx->vpVecOpenStack);
    if(!uipRecordCount){
        XTHROW(spCtx->spException, "AST open record stack should not be empty empty");
    }
    aint uiRecordCount = *uipRecordCount;
    if(uiState == ID_MATCH){
        aint uiIndex = uiRuleIndex;
        abool bIsUdt = APG_FALSE;
        if(uiRuleIndex >= spCtx->spParser->uiRuleCount){
            uiIndex = uiRuleIndex - spCtx->spParser->uiRuleCount;
            bIsUdt = APG_TRUE;
            if(spCtx->pfnUdtCallbacks[uiIndex] == NULL){
                return;
            }
        }else{
            if(spCtx->pfnRuleCallbacks[uiIndex] == NULL){
                return;
            }
        }
        ast_record sRecord;
        ast_record* spThatRecord;
        aint* uipThatRecordIndex = (aint*)vpVecPop(spCtx->vpVecThatStack);
        if(!uipThatRecordIndex){
            XTHROW(spCtx->spException, "AST that stack is empty");
        }
        spThatRecord = (ast_record*)vpVecAt(spCtx->vpVecRecords, *uipThatRecordIndex);
        if(!spThatRecord){
            XTHROW(spCtx->spException, "requested AST record out of range");
        }
        sRecord.bIsUdt = bIsUdt;
        sRecord.cpName = spThatRecord->cpName;
        sRecord.uiIndex = uiIndex;
        sRecord.uiPhraseLength = uiPhraseLength;
        sRecord.uiPhraseOffset = uiPhraseOffset;
        sRecord.uiState = ID_AST_POST;
        sRecord.uiThatRecord = spThatRecord->uiThisRecord;
        sRecord.uiThisRecord = uiVecLen(spCtx->vpVecRecords);
        vpVecPush(spCtx->vpVecRecords, (void*)&sRecord);
        // refresh the pointer after the push (it is stale if the vector grows)
        spThatRecord = (ast_record*)vpVecAt(spCtx->vpVecRecords, *uipThatRecordIndex);
        if(!spThatRecord){
            XTHROW(spCtx->spException, "requested AST record out of range");
        }
        spThatRecord->uiPhraseLength = uiPhraseLength;
        spThatRecord->uiThatRecord = sRecord.uiThisRecord;
    }else{
        if(uiRecordCount < uiVecLen(spCtx->vpVecRecords)){
            if(!vpVecPopi(spCtx->vpVecRecords, uiRecordCount)){
                XTHROW(spCtx->spException, "AST record stack should not be empty");
            }
            if(!vpVecPop(spCtx->vpVecThatStack)){
                XTHROW(spCtx->spException, "AST \"that\" record stack should not be empty");
            }
        }
    }
}

/** \brief Called in preparation for a downward traversal of an RNM or UDT node.
 * \param vpCtx - AST context handle returned from \see vpAstCtor.
 * No validation is done here as this function is always called by a trusted parser operator function.
 */
void vAstOpOpen(void* vpCtx){
    ast* spCtx = (ast*)vpCtx;
    aint uiRecordCount = uiVecLen(spCtx->vpVecRecords);
    vpVecPush(spCtx->vpVecOpenStack, (void*)&uiRecordCount);
}

/** \brief Called to finish up after an upward traversal of an RNM or UDT node.
 * \param vpCtx - AST context handle returned from \see vpAstCtor.
 * No validation is done here as this function is always called by a trusted parser operator function.
 * \param uiState ID_MATCH or ID_NOMATCH, the result of the parse.
 */
void vAstOpClose(void* vpCtx, aint uiState){
    ast* spCtx = (ast*)vpCtx;
    aint* uipRecordCount = (aint*)vpVecPop(spCtx->vpVecOpenStack);
    if(!uipRecordCount){
        XTHROW(spCtx->spException, "AST open stack empty");
    }
    aint uiRecordCount = *uipRecordCount;

    if(uiState == ID_NOMATCH){
        if(uiRecordCount < uiVecLen(spCtx->vpVecRecords)){
            if(!vpVecPopi(spCtx->vpVecRecords, uiRecordCount)){
                XTHROW(spCtx->spException, "AST record stack empty");
            }
        }
    }
}
#endif /* APG_AST */
