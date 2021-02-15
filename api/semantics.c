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
/** \file semantics.c
 * \brief Processes the semantics phase. Parses the grammar and translates the AST to opcodes. The compiler, so to speak.
 */

#include "./api.h"
#include "./apip.h"
#include "./semantics.h"

//#define SEMANTICS_DEBUG 1
#ifdef SEMANTICS_DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "./parserp.h"
#include "./astp.h"
#include "../utilities/utilities.h"
static void vPrintSemanticOpcodes(api* spApi, semantic_data* spData);
#endif /* SEMANTICS_DEBUG */

#define BUF_SIZE 256

static void vGenerateUdtList(api* spApi, semantic_data* spData);
static void vValidateRnmOps(api* spApi, semantic_data* spData);
static void vValidateBkrOps(api* spApi, semantic_data* spData);
static void vReduceOpcodes(api* spApi, semantic_data* spData);
static void vStringTable(api* spApi, semantic_data* spData);
static abool bLCCompare(const char* cpL, aint uiLLen, const char* cpR, aint uiRLen);
static void vPpptSize(api* spApi);

/** \brief Parse the SABNF grammar and translate its AST into opcodes for all the rules.
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 */
void vApiOpcodes(void* vpCtx) {
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spApi = (api*) vpCtx;
    semantic_data sData = {};
    memset((void*) &sData, 0, sizeof(sData));
    vMsgsClear(spApi->vpLog);
    // validate that we are at the semantic stage
    if (!spApi->bInputValid) {
        XTHROW(spApi->spException,
                "attempted opcodes phase but input grammar not validated");
    }
    if (!spApi->bSyntaxValid) {
        XTHROW(spApi->spException,
                "attempted opcodes phase but syntax not validated");
    }
    if (spApi->bSemanticsValid) {
        XTHROW(spApi->spException,
                "attempted opcodes phase but opcodes have already been constructed and validated");
    }

    // initialize the callback data
    sData.vpMem = spApi->vpMem;
    sData.spApi = spApi;
    sData.vpVecAcharsTable = vpVecCtor(sData.vpMem, sizeof(luint), 1000);
    sData.vpVecChildIndexTable = vpVecCtor(sData.vpMem, sizeof(aint), 1000);
    sData.vpVecStringTable = vpVecCtor(sData.vpMem, sizeof(char), 1000);
    sData.vpVecRules = vpVecCtor(sData.vpMem, sizeof(semantic_rule), 1000);
    sData.vpVecUdts = vpVecCtor(sData.vpMem, sizeof(semantic_udt), 1000);

    // semantics - translate the AST generated in the syntax phase
    // NOTE: the achar table (for TLS/TBS character strings) is created during translation (semantic-callbacks.c)
    vAstTranslate(spApi->vpAst, &sData);

    // scan the opcodes and find all UDTs referenced
    vGenerateUdtList(spApi, &sData);

    // make sure all RNM and BKR operators refer to defined rules or UDTs
    vValidateRnmOps(spApi, &sData);
    vValidateBkrOps(spApi, &sData);

    // create the string table and initialize the rule and UDT lists
    vStringTable(spApi, &sData);

    // eliminate redundant opcodes (ALT & CAT with only one child, REP(1,1) or 1*1)
    // this also moves all required data from sData, to the permanent API context
#ifdef SEMANTICS_DEBUG
    vPrintSemanticOpcodes(spApi, &sData);
    vReduceOpcodes(spApi, &sData);
        break;
    }
    vPrintGeneratedOpcodes(spApi);
#else
    vReduceOpcodes(spApi, &sData);
#endif /* SEMANTICS_DEBUG */
    vPpptSize(spApi);

    // AST translation to opcodes success
    spApi->bSemanticsValid = APG_TRUE;

    // free the callback data
    vVecDtor(sData.vpVecAcharsTable);
    vVecDtor(sData.vpVecChildIndexTable);
    vVecDtor(sData.vpVecStringTable);
    aint ui = 0;
    aint uiRules = uiVecLen(sData.vpVecRules);
    semantic_rule* spRules = vpVecFirst(sData.vpVecRules);
    if(!spRules || !uiRules){
        XTHROW(spApi->spException, "no rule names found");
    }
    for(; ui < uiRules; ui++){
        vVecDtor(spRules[ui].vpVecAltStack);
        vVecDtor(spRules[ui].vpVecCatStack);
        semantic_op* spOp = (semantic_op*)vpVecFirst(spRules[ui].vpVecOps);
        semantic_op* spOpEnd = (semantic_op*)vpVecLast(spRules[ui].vpVecOps);
        if(spOp){
            for(; spOp <= spOpEnd; spOp++){
                vVecDtor(spOp->vpVecChildList);
            }
        }
        vVecDtor(spRules[ui].vpVecOps);
    }
    vVecDtor(sData.vpVecRules);
    vVecDtor(sData.vpVecUdts);
}

/** \brief Find the index of the named rule in the rule list.
 *
 * Does a simple linear search.
 * \param[in] spRules - the array of rules
 * \param[in] uiRuleCount - the number of rules
 * \param[in] cpName - pointer to the name to look for (not a null-terminated string)
 * \param[in] uiNameLength - the number of characters in the name
 * \return the index of the rule in the rule list if the name is found, APG_UNDEFINED otherwise
 */
aint uiFindRule(semantic_rule* spRules, aint uiRuleCount, const char* cpName, aint uiNameLength) {
    aint ui, uiFound;
    for(ui = 0; ui < uiRuleCount; ui++){
        uiFound = bLCCompare(spRules->cpName, spRules->uiNameLength, cpName, uiNameLength);
        if (uiFound) {
            return ui;
        }
        spRules++;
    }
    return APG_UNDEFINED;
}

aint uiFindUdt(semantic_udt* spUdts, aint uiUdtCount, const char* cpName, aint uiNameLength) {
    aint ui, uiFound;
    for(ui = 0; ui < uiUdtCount; ui++){
        uiFound = bLCCompare(spUdts->cpName, spUdts->uiNameLength, cpName, uiNameLength);
        if (uiFound) {
            return ui;
        }
        spUdts++;
    }
    return APG_UNDEFINED;
}

static abool bLCCompare(const char* cpL, aint uiLLen, const char* cpR, aint uiRLen) {
    if (uiLLen == uiRLen) {
        char l, r;
        while (uiLLen--) {
            l = *cpL;
            if (l >= 65 && l <= 90) {
                l += 32;
            }
            r = *cpR;
            if (r >= 65 && r <= 90) {
                r += 32;
            }
            if (l != r) {
                return APG_FALSE;
            }
            cpL++;
            cpR++;
        }
        return APG_TRUE;
    }
    return APG_FALSE;
}
/** \brief Allocates the string table and completes it.
 * Initializes the rule and UDT lists.
 * \param[in] spApi - pointer to the API context
 * \param[in] spData - pointer to the semantic translation data
 */
static void vStringTable(api* spApi, semantic_data* spData) {
    // compute the string table length
    semantic_rule* spSemRules = (semantic_rule*) vpVecFirst(spData->vpVecRules);
    semantic_udt* spSemUdts = (semantic_udt*) vpVecFirst(spData->vpVecUdts);
    aint uiSize = 0;
    char* cpNext;
    api_rule* spRule;
    api_udt* spUdt;
    aint ui;
    spApi->uiRuleCount = uiVecLen(spData->vpVecRules);
    spApi->uiUdtCount = uiVecLen(spData->vpVecUdts);
    for (ui = 0; ui < spApi->uiRuleCount; ui++) {
        uiSize += spSemRules[ui].uiNameLength + 1;
    }
    for (ui = 0; ui < spApi->uiUdtCount; ui++) {
        uiSize += spSemUdts[ui].uiNameLength + 1;
    }
    uiSize += strlen(APG_VERSION) + 1;
    uiSize += strlen(APG_COPYRIGHT) + 1;
    uiSize += strlen(APG_LICENSE) + 1;
    uiSize++;

    // get the string table
    spApi->cpStringTable = (char*) vpMemAlloc(spApi->vpMem, (aint) (sizeof(char) * uiSize));
    spApi->uiStringTableLength = uiSize;
    memset((void*) spApi->cpStringTable, 0, (sizeof(char) * uiSize));

    // get the achar table
    uiSize = uiVecLen(spData->vpVecAcharsTable);
    if(uiSize){
        spApi->luipAcharTable = (luint*) vpMemAlloc(spApi->vpMem, (aint) (sizeof(luint) * uiSize));
        spApi->uiAcharTableLength = uiSize;
        memcpy((void*) spApi->luipAcharTable, vpVecFirst(spData->vpVecAcharsTable), (sizeof(luint) * uiSize));
    }else{
        spApi->luipAcharTable = NULL;
        spApi->uiAcharTableLength = 0;
    }

    // allocate the rule list
    spApi->spRules = (api_rule*) vpMemAlloc(spApi->vpMem, (aint) (sizeof(api_rule) * spApi->uiRuleCount));
    memset((void*) spApi->spRules, 0, (sizeof(api_rule) * spApi->uiRuleCount));

    if (spApi->uiUdtCount) {
        // allocate the UDT list
        spApi->spUdts = (api_udt*) vpMemAlloc(spApi->vpMem, (aint) (sizeof(api_udt) * spApi->uiUdtCount));
        memset((void*) spApi->spUdts, 0, (sizeof(api_udt) * spApi->uiUdtCount));
    }

    // initialize the rule list and put rule names in string table
    cpNext = spApi->cpStringTable;
    spRule = spApi->spRules;
    for (ui = 0; ui < spApi->uiRuleCount; ui++, spRule++) {
        memcpy(cpNext, spSemRules[ui].cpName, spSemRules[ui].uiNameLength);
        spRule->cpName = cpNext;
        spRule->uiIndex = ui;
        cpNext += spSemRules[ui].uiNameLength + 1;
    }

    // initialize the UDT list, if any, and put rule names in string table
    spUdt = spApi->spUdts;
    for (ui = 0; ui < spApi->uiUdtCount; ui++, spUdt++) {
        memcpy(cpNext, spSemUdts[ui].cpName, spSemUdts[ui].uiNameLength);
        spUdt->cpName = cpNext;
        spUdt->uiIndex = ui;
        spUdt->uiEmpty = spSemUdts[ui].uiEmpty;
        cpNext += spSemUdts[ui].uiNameLength + 1;
    }

    // add the version info
    spApi->uiVersionOffset = cpNext - spApi->cpStringTable;
    strcpy(cpNext, APG_VERSION);
    cpNext += strlen(APG_VERSION) + 1;
    spApi->uiVersionLength = strlen(APG_VERSION) + 1;

    // add the license info
    spApi->uiLicenseOffset = cpNext - spApi->cpStringTable;
    strcpy(cpNext, APG_LICENSE);
    cpNext += strlen(APG_LICENSE) + 1;
    spApi->uiLicenseLength = strlen(APG_LICENSE) + 1;

    // add the copyright info
    spApi->uiCopyrightOffset = cpNext - spApi->cpStringTable;
    strcpy(cpNext, APG_COPYRIGHT);
    cpNext += strlen(APG_COPYRIGHT) + 1;
    spApi->uiCopyrightLength = strlen(APG_COPYRIGHT) + 1;
}

static void vGenerateUdtList(api* spApi, semantic_data* spData) {
    semantic_op* spOp;
    aint uiUdtCount;
    semantic_udt* spUdts;
    aint uiFound;
    semantic_op* spOps;
    aint uiRuleCount;
    aint ui, uii;
    aint uiOpCount;
    semantic_udt sUdt;
    semantic_rule* spRules;
    char caBuf[BUF_SIZE];
    spRules = (semantic_rule*) vpVecFirst(spData->vpVecRules);
    if (!spRules) {
        XTHROW(spData->spApi->spException, "grammar contains no rules");
    }
    uiRuleCount = uiVecLen(spData->vpVecRules);
    for (ui = 0; ui < uiRuleCount; ui++) {
        spOps = (semantic_op*) vpVecFirst(spRules[ui].vpVecOps);
        if (!spOps) {
            snprintf(caBuf, BUF_SIZE, "rule %"PRIuMAX" has no opcodes", (luint) ui);
            XTHROW(spData->spApi->spException, caBuf);
        }
        uiOpCount = uiVecLen(spRules[ui].vpVecOps);
        for (uii = 0; uii < uiOpCount; uii++) {
            spOp = &spOps[uii];
            if (spOp->uiId == ID_UDT) {
                sUdt.cpName = spOp->cpName;
                sUdt.uiNameLength = spOp->uiNameLength;
                sUdt.uiEmpty = spOp->uiEmpty;
                uiUdtCount = uiVecLen(spData->vpVecUdts);
                spUdts = (semantic_udt*) vpVecFirst(spData->vpVecUdts);
                uiFound = uiFindUdt(spUdts, uiUdtCount, sUdt.cpName, sUdt.uiNameLength);
                if (uiFound == APG_UNDEFINED) {
                    sUdt.uiIndex = spData->uiUdtIndex++;
                    spOp->uiBkrIndex = sUdt.uiIndex;
                    vpVecPush(spData->vpVecUdts, (void*) &sUdt);
                }else{
                    // add the UDT index to the UDT opcode
                    spOp->uiBkrIndex = uiFound;
                }
            }
        }
    }

    /*
    // !!!! SEMANTICS_DEBUG
    spUdts = (semantic_udt*) vpVecFirst(spData->vpVecUdts);
    uiUdtCount = uiVecLen(spData->vpVecUdts);
    printf("\nUDTS FOUND: %"PRIuMAX"\n", (luint) uiUdtCount);
    for (ui = 0; ui < uiUdtCount; ui++) {
        char* cpTF;
        char caBuf[128];
        memcpy((void*) &caBuf[0], (void*) spUdts[ui].cpName, (size_t) spUdts[ui].uiNameLength);
        caBuf[spUdts[ui].uiNameLength] = 0;
        printf("UDT: %s: empty: ", caBuf);
        cpTF = spUdts[ui].uiEmpty ? "TRUE" : "FALSE";
        printf("%s\n", cpTF);
    }
*/
}

static void vValidateRnmOps(api* spApi, semantic_data* spData) {
    semantic_op* spOp;
    aint uiFound;
    semantic_op* spOps;
    aint uiRuleCount;
    aint ui, uii;
    aint uiOpCount;
    semantic_rule* spRules;
    char caBuf[BUF_SIZE];
    spRules = (semantic_rule*) vpVecFirst(spData->vpVecRules);
    if (!spRules) {
        XTHROW(spData->spApi->spException, "grammar contains no rules");
    }
    uiRuleCount = uiVecLen(spData->vpVecRules);
    for (ui = 0; ui < uiRuleCount; ui++) {
        spOps = (semantic_op*) vpVecFirst(spRules[ui].vpVecOps);
        if (!spOps) {
            snprintf(caBuf, BUF_SIZE, "rule %"PRIuMAX" has no opcodes", (luint) ui);
            XTHROW(spData->spApi->spException, caBuf);
        }
        uiOpCount = uiVecLen(spRules[ui].vpVecOps);
        for (uii = 0; uii < uiOpCount; uii++) {
            spOp = &spOps[uii];
            if (spOp->uiId == ID_RNM) {
                uiFound = uiFindRule(spRules, uiRuleCount, spOp->cpName, spOp->uiNameLength);
                if (uiFound == APG_UNDEFINED) {
                    aint uiIndex = spOp->cpName - spApi->cpInput;
                    char caName[126];
                    char caMsg[256];
                    memcpy((void*) &caName[0], (void*) spOp->cpName, (size_t) spOp->uiNameLength);
                    caName[spOp->uiNameLength] = 0;
                    snprintf(caMsg, 256, "rule name \"%s\" not found", caName);
                    vLineError(spApi, uiIndex, "invalid RNM", caMsg);
                }else{
                    spOp->uiBkrIndex = uiFound;
                }
            }
        }
        if(uiMsgsCount(spApi->vpLog)){
            XTHROW(spData->spApi->spException, "some rule names not found - see the API error log");
        }
    }
}
static void vValidateBkrOps(api* spApi, semantic_data* spData) {
    semantic_op* spOp;
    aint uiFound;
    semantic_op* spOps;
    aint ui, uii;
    aint uiOpCount;
    semantic_rule* spRules;
    aint uiRuleCount;
    semantic_udt* spUdts;
    aint uiUdtCount;
    char caBuf[BUF_SIZE];
    spRules = (semantic_rule*) vpVecFirst(spData->vpVecRules);
    if (!spRules) {
        XTHROW(spData->spApi->spException, "grammar contains no rules");
    }
    uiRuleCount = uiVecLen(spData->vpVecRules);
    spUdts = (semantic_udt*) vpVecFirst(spData->vpVecUdts);
    uiUdtCount = uiVecLen(spData->vpVecUdts);
    for (ui = 0; ui < uiRuleCount; ui++) {
        spOps = (semantic_op*) vpVecFirst(spRules[ui].vpVecOps);
        if (!spOps) {
            snprintf(caBuf, BUF_SIZE, "rule %"PRIuMAX" has no opcodes", (luint) ui);
            XTHROW(spData->spApi->spException, caBuf);
        }
        uiOpCount = uiVecLen(spRules[ui].vpVecOps);
        for (uii = 0; uii < uiOpCount; uii++) {
            spOp = &spOps[uii];
            if (spOp->uiId == ID_BKR) {
                uiFound = uiFindRule(spRules, uiRuleCount, spOp->cpName, spOp->uiNameLength);
                if (uiFound == APG_UNDEFINED) {
                    uiFound = uiFindUdt(spUdts, uiUdtCount, spOp->cpName, spOp->uiNameLength);
                    if (uiFound == APG_UNDEFINED) {
                        aint uiIndex = spOp->cpName - spApi->cpInput;
                        char caName[128];
                        char caMsg[256];
                        memcpy((void*) &caName[0], (void*) spOp->cpName, (size_t) spOp->uiNameLength);
                        caName[spOp->uiNameLength] = 0;
                        snprintf(caMsg, 256, "back reference rule or UDT name, \"%s\", not found", caName);
                        vLineError(spApi, uiIndex, "invalid BKR", caMsg);
                    } else {
                        spOp->uiBkrIndex = uiRuleCount + uiFound;
                    }
                } else {
                    spOp->uiBkrIndex = uiFound;
                }
            }
        }
    }
    if(uiMsgsCount(spApi->vpLog)){
        XTHROW(spData->spApi->spException, "some rule names not found - see the API error log");
    }
}

static void vReduceOpcodes(api* spApi, semantic_data* spData) {
    aint uiSeq, uiLen, uiOpcodes, uiOffset;
    api_rule* spApiRule;
    semantic_rule* spRule;
    semantic_rule* spRuleBeg = (semantic_rule*) vpVecFirst(spData->vpVecRules);
    if(!spRuleBeg){
        XTHROW(spData->spApi->spException, "no rules found");
    }
    semantic_rule* spRuleEnd = (semantic_rule*) vpVecLast(spData->vpVecRules);
    semantic_op* spOp;
    semantic_op* spOpEnd;
    semantic_op* spOpChild;
    api_op* spApiOp;
    aint* uipIndex;
    aint* uipIndexEnd;
    aint* uipChildIndexTable;

    /*
    // !!!! SEMANTICS_DEBUG
    // set all sequence numbers and print the raw opcodes before reduction
    spRule = spRuleBeg;
    for (; spRule <= spRuleEnd; spRule++) {
        spOp = (semantic_op*) vpVecFirst(spRule->vpVecOps);
        spOpEnd = (semantic_op*) vpVecLast(spRule->vpVecOps);
        for (uiSeq = 0; spOp <= spOpEnd; spOp++) {
            spOp->uiSeq = uiSeq++;
        }
    }
    printf("All opcodes\n");
    vPrintSemanticOpcodes(spApi, spData);
    // !!!! SEMANTICS_DEBUG
*/

    // sequence essential opcodes - ALT & CAT with only one child and REP(1,1) are redundant and can be eliminated
    spRule = spRuleBeg;
    for (; spRule <= spRuleEnd; spRule++) {
        spOp = (semantic_op*) vpVecFirst(spRule->vpVecOps);
        spOpEnd = (semantic_op*) vpVecLast(spRule->vpVecOps);
        for (uiSeq = 0; spOp <= spOpEnd; spOp++) {
            if ((spOp->uiId == ID_ALT) || (spOp->uiId == ID_CAT)) {
                uiLen = uiVecLen(spOp->vpVecChildList);
                if (uiLen == 0) {
                    XTHROW(spData->spApi->spException, "ALT or CAT operator has no children");
                }
                if (uiVecLen(spOp->vpVecChildList) > 1) {
                    spOp->uiSeq = uiSeq++;
                } else {
                    spOp->uiSeq = APG_UNDEFINED;
                }
            } else if (spOp->uiId == ID_REP) {
                if (spOp->luiMin == 1 && spOp->luiMax == 1) {
                    spOp->uiSeq = APG_UNDEFINED;
                } else {
                    spOp->uiSeq = uiSeq++;
                }
            } else {
                spOp->uiSeq = uiSeq++;
            }
        }
    }

    // reset the ALT and CAT child lists
    spRule = spRuleBeg;
    for (; spRule <= spRuleEnd; spRule++) {
        spOp = (semantic_op*) vpVecFirst(spRule->vpVecOps);
        spOpEnd = (semantic_op*) vpVecLast(spRule->vpVecOps);
        for (; spOp <= spOpEnd; spOp++) {
            if ((spOp->uiId == ID_ALT) || (spOp->uiId == ID_CAT)) {
                if (spOp->uiSeq != APG_UNDEFINED) {
                    uiLen = uiVecLen(spOp->vpVecChildList);
                    if (uiLen <= 1) {
                        XTHROW(spData->spApi->spException, "ALT or CAT operator has no children");
                    }
                    uipIndex = (aint*) vpVecFirst(spOp->vpVecChildList);
                    uipIndexEnd = (aint*) vpVecLast(spOp->vpVecChildList);
                    for (; uipIndex <= uipIndexEnd; uipIndex++) {
                        spOpChild = (semantic_op*) vpVecAt(spRule->vpVecOps, *uipIndex);
                        while (spOpChild->uiSeq == APG_UNDEFINED) {
                            spOpChild++;
                        }
                        *uipIndex = spOpChild->uiSeq;
                    }
                }
            }
        }
    }
//    printf("Reduced opcodes\n");
//    vPrintSemanticOpcodes(spApi, spData);

    // get the size of child and opcode lists
    spRule = spRuleBeg;
    uiLen = 0;
    uiOpcodes = 0;
    for (; spRule <= spRuleEnd; spRule++) {
        spOp = (semantic_op*) vpVecFirst(spRule->vpVecOps);
        spOpEnd = (semantic_op*) vpVecLast(spRule->vpVecOps);
        for (; spOp <= spOpEnd; spOp++) {
            if (spOp->uiSeq != APG_UNDEFINED) {
                if ((spOp->uiId == ID_ALT) || (spOp->uiId == ID_CAT)) {
                    uiLen += uiVecLen(spOp->vpVecChildList);
                }
                uiOpcodes++;
            }
        }
    }

    // allocate the child list
    spApi->uipChildIndexTable = (aint*) vpMemAlloc(spApi->vpMem, (aint) (sizeof(aint) * uiLen));
    spApi->uiChildIndexTableLength = uiLen;

    // allocate the opcode list
    spApi->spOpcodes = (api_op*) vpMemAlloc(spApi->vpMem, (aint) (sizeof(api_op) * uiOpcodes));
    spApi->uiOpcodeCount = uiOpcodes;
    memset((void*)spApi->spOpcodes, 0, (sizeof(api_op) * uiOpcodes));

    // complete the API opcode and child list tables
    aint ui;
    aint* uipBeg;
    spRule = spRuleBeg;
    uiLen = 0;
    uiOffset = 0;
    uipChildIndexTable = spApi->uipChildIndexTable;
    spApiRule = spApi->spRules;
    spApiOp = spApi->spOpcodes;
    for (; spRule <= spRuleEnd; spRule++, spApiRule++) {
        uiOpcodes = 0;
        spOp = (semantic_op*) vpVecFirst(spRule->vpVecOps);
        spOpEnd = (semantic_op*) vpVecLast(spRule->vpVecOps);
        for (; spOp <= spOpEnd; spOp++) {
            if (spOp->uiSeq != APG_UNDEFINED) {
                spApiOp->uiId = spOp->uiId;
                switch (spOp->uiId) {
                case ID_ALT:
                case ID_CAT:
                    spApiOp->uiChildCount = uiVecLen(spOp->vpVecChildList);
                    spApiOp->uipChildIndex = uipChildIndexTable;
                    uipBeg = (aint*) vpVecFirst(spOp->vpVecChildList);
                    for(ui = 0; ui < spApiOp->uiChildCount; ui++){
                        *uipChildIndexTable++ = uiOffset + *uipBeg++;
                    }
                    break;
                case ID_REP:
                case ID_TRG:
                    spApiOp->luiMin = spOp->luiMin;
                    spApiOp->luiMax = spOp->luiMax;
                    break;
                case ID_RNM:
                    spApiOp->uiIndex = spOp->uiBkrIndex;
                    break;
                case ID_TBS:
                case ID_TLS:
                    spApiOp->luipAchar = &spApi->luipAcharTable[spOp->uiStringIndex];
                    spApiOp->uiAcharLength = spOp->uiStringLength;
                    break;
                case ID_UDT:
                    spApiOp->uiIndex = spOp->uiBkrIndex;
                    spApiOp->uiEmpty = spOp->uiEmpty;
                    break;
                case ID_BKR:
                    spApiOp->uiBkrIndex = spOp->uiBkrIndex;
                    spApiOp->uiCase = spOp->uiCase;
                    spApiOp->uiMode = spOp->uiMode;
                    break;
                case ID_AND:
                case ID_NOT:
                case ID_BKA:
                case ID_BKN:
                case ID_ABG:
                case ID_AEN:
                    break;
                default:
                    XTHROW(spData->spApi->spException, "unrecognized operator ID");
                    break;
                }
                uiOpcodes++;
                spApiOp++;
            }
        }
        spApiRule->uiOpCount = uiOpcodes;
        spApiRule->uiOpOffset = uiOffset;
        uiOffset += uiOpcodes;
    }
//    vPrintReducedOpcodes(spApi);
}

static void vPpptSize(api* spApi){
    aint ui;
//    char caBuf[128];
    api_op* spOp = NULL;

    // get achar min and max
    spApi->luiAcharMin = (luint)-1;
    spApi->luiAcharMax = 0;
    spOp = spApi->spOpcodes;
    for(ui = 0; ui < spApi->uiOpcodeCount; ui++, spOp++){
        if(spOp->uiId == ID_TRG) {
            if(spOp->luiMax > spApi->luiAcharMax){
                spApi->luiAcharMax = spOp->luiMax;
            }
            if(spOp->luiMin < spApi->luiAcharMin){
                spApi->luiAcharMin = spOp->luiMin;
            }
        }else if(spOp->uiId == ID_TBS){
            if(*spOp->luipAchar > spApi->luiAcharMax){
                spApi->luiAcharMax = *spOp->luipAchar;
            }
            if(*spOp->luipAchar < spApi->luiAcharMin){
                spApi->luiAcharMin = *spOp->luipAchar;
            }
        }else if(spOp->uiId == ID_TLS){
            if(spOp->uiAcharLength){
                if(*spOp->luipAchar > spApi->luiAcharMax){
                    spApi->luiAcharMax = *spOp->luipAchar;
                }
                if(*spOp->luipAchar >= 97 && *spOp->luipAchar <= 122){
                    if((*spOp->luipAchar - 32) < spApi->luiAcharMin){
                        spApi->luiAcharMin = (*spOp->luipAchar - 32);
                    }
                }else{
                    if(*spOp->luipAchar < spApi->luiAcharMin){
                        spApi->luiAcharMin = *spOp->luipAchar;
                    }
                }
            }
        }
    }

    // get map size per opcode in bytes
    if(spApi->luiAcharMin == (luint)-1){
        spApi->luiAcharMin = 0;
    }
    // NOTE: map size is the number of alphabet characters plus one for the (virtual) EOS character
    if(spApi->luiAcharMax == (luint)-1){
        spApi->luiAcharEos = 0;
        spApi->luiPpptMapSize = (luint)-1;
    }else{
        spApi->luiAcharEos = spApi->luiAcharMax + 1;
        spApi->luiPpptMapSize = spApi->luiAcharEos - spApi->luiAcharMin + 1;
    }

    // compute the number of opcode maps needed
    spApi->luiPpptMapCount = 0;
    for(ui = 0; ui < spApi->uiRuleCount; ui++){
        spApi->luiPpptMapCount++;
    }
    spOp = spApi->spOpcodes;
    for(ui = 0; ui < spApi->uiOpcodeCount; ui++, spOp++){
        switch (spOp->uiId) {
        case ID_ALT:
        case ID_CAT:
        case ID_REP:
        case ID_TRG:
        case ID_TLS:
        case ID_TBS:
        case ID_AND:
        case ID_NOT:
            spApi->luiPpptMapCount++;
            break;
            // rule indexes set consecutively later
        case ID_RNM:
            // these opcodes have no PPPT map
            // we can't predict what the user will do in a UDT
        case ID_UDT:
            // case insensitivity BKR it is possible for BKR to accept characters outside of AcharMin and AcharMax
        case ID_BKR:
            // look behind is iterative and impossible (or extremely difficult) to determine a PPPT map
        case ID_BKA:
        case ID_BKN:
            // anchor opcodes only examine the character position, not the character value
        case ID_ABG:
        case ID_AEN:
            break;
        default:
            XTHROW(spApi->spException, "unrecognized operator ID");
        }
    }

    // compute the PPPT table size
    spApi->luiPpptTableLength = (luint)APG_MAX_AINT;
    luint luiTemp, luiTest;
    while(APG_TRUE){
        if(!bMultiplyLong(spApi->luiPpptMapCount, spApi->luiPpptMapSize, &luiTemp)){
//            XTHROW(spApi->spException, "multiplication overflow computing PPPT table size");
            break;
        }
        if(!bMultiplyLong((luint)sizeof(uint8_t), luiTemp, &luiTest)){
//            XTHROW(spApi->spException, "multiplication overflow computing PPPT table size");
            break;
        }
        luiTemp = (luint)APG_MAX_AINT;
        if(spApi->luiPpptMapSize >= luiTemp){
//            snprintf(caBuf, 128, "PPPT map size too big: %"PRIuMAX"", spApi->luiPpptMapSize);
//            XTHROW(spApi->spException, caBuf);
            break;
        }
        if(spApi->luiPpptMapCount >= luiTemp){
//            snprintf(caBuf, 128, "PPPT number of maps too big: %"PRIuMAX"", spApi->luiPpptMapCount);
//            XTHROW(spApi->spException, caBuf);
            break;
        }
        if(luiTest >= luiTemp){
//            snprintf(caBuf, 128, "PPPT table length (bytes) too long: %"PRIuMAX"", spApi->luiPpptTableLength);
//            XTHROW(spApi->spException, caBuf);
            break;
        }

        // table size is valid (but may still be too big to be handled)
        spApi->luiPpptTableLength = luiTest;
        break;
    }
}

#ifdef SEMANTICS_DEBUG
static void vPrintSemanticOpcodes(api* spApi, semantic_data* spData) {
    char caName[RULENAME_MAX];
    aint uiRuleCount = uiVecLen(spData->vpVecRules);
    aint uiOpCount;
    aint ui, uj, uk;
    aint uiCount;
    aint* uipBeg;
    luint* luipBeg;
    semantic_rule* spRules = (semantic_rule*) vpVecFirst(spData->vpVecRules);
    semantic_rule* spRule;
    semantic_op* spOps;
    semantic_op* spOp;
    printf("RAW OPCODES:\n");
    for (ui = 0; ui < uiRuleCount; ui++) {
        spRule = &spRules[ui];
        memcpy(caName, spRule->cpName, spRule->uiNameLength);
        caName[spRule->uiNameLength] = 0;
        printf("rule: %"PRIuMAX": %s\n", (luint) ui, caName);
        uiOpCount = uiVecLen(spRule->vpVecOps);
        spOps = (semantic_op*) vpVecFirst(spRule->vpVecOps);
        for (uj = 0; uj < uiOpCount; uj++) {
            spOp = &spOps[uj];
            if (spOp->uiSeq == APG_UNDEFINED) {
                continue;
            }
            switch (spOp->uiId) {
            case ID_ALT:
                uiCount = uiVecLen(spOp->vpVecChildList);
                uipBeg = (aint*) vpVecFirst(spOp->vpVecChildList);
                printf("%"PRIuMAX": ", (luint) uj);
                printf("ALT: ");
                printf("children: %"PRIuMAX":", (luint) uiCount);
                for (uk = 0; uk < uiCount; uk++, uipBeg++) {
                    if (uk == 0) {
                        printf(" %"PRIuMAX"", (luint) *uipBeg);
                    } else {
                        printf(", %"PRIuMAX"", (luint) *uipBeg);

                    }
                }
                printf("\n");
                break;
            case ID_CAT:
                uiCount = uiVecLen(spOp->vpVecChildList);
                uipBeg = (aint*) vpVecFirst(spOp->vpVecChildList);
                printf("%"PRIuMAX": ", (luint) uj);
                printf("CAT: ");
                printf("children: %"PRIuMAX":", (luint) uiCount);
                for (uk = 0; uk < uiCount; uk++, uipBeg++) {
                    if (uk == 0) {
                        printf(" %"PRIuMAX"", (luint) *uipBeg);
                    } else {
                        printf(", %"PRIuMAX"", (luint) *uipBeg);

                    }
                }
                printf("\n");
                break;
            case ID_REP:
                printf("%"PRIuMAX": ", (luint) uj);
                printf("REP: ");
                printf("min: %"PRIuMAX": ", spOp->luiMin);
                if (spOp->luiMax == (luint) -1) {
                    printf("max: infinity");
                } else {
                    printf("max: %"PRIuMAX"", spOp->luiMax);
                }
                printf("\n");
                break;
            case ID_RNM:
                memcpy(caName, spOp->cpName, spOp->uiNameLength);
                caName[spOp->uiNameLength] = 0;
                printf("%"PRIuMAX": ", (luint) uj);
                printf("RNM: ");
                printf("%s", caName);
                printf("\n");
                break;
            case ID_TBS:
                printf("%"PRIuMAX": ", (luint) uj);
                printf("TBS: ");
                uiCount = spOp->uiStringLength;
                luipBeg = (luint*) vpVecFirst(spData->vpVecAcharsTable);
                luipBeg += spOp->uiStringIndex;
                for (uk = 0; uk < uiCount; uk++, luipBeg++) {
                    if (*luipBeg >= 32 && *luipBeg <= 126) {
                        printf("%c", (char) *luipBeg);
                    } else {
                        printf("0x%.2lX", *luipBeg);
                    }
                }
                printf("\n");
                break;
            case ID_TLS:
                printf("%"PRIuMAX": ", (luint) uj);
                printf("TLS: ");
                uiCount = spOp->uiStringLength;
                luipBeg = (luint*) vpVecFirst(spData->vpVecAcharsTable);
                luipBeg += spOp->uiStringIndex;
                for (uk = 0; uk < uiCount; uk++, luipBeg++) {
                    printf("%c", (char) *luipBeg);
                }
                printf("\n");
                break;
            case ID_TRG:
                printf("%"PRIuMAX": ", (luint) uj);
                printf("TRG: ");
                printf("min: %"PRIuMAX": ", spOp->luiMin);
                printf("max: %"PRIuMAX"", spOp->luiMax);
                printf("\n");
                break;
            case ID_UDT:
                printf("%"PRIuMAX": ", (luint) uj);
                printf("UDT: ");
                memcpy(caName, spOp->cpName, spOp->uiNameLength);
                caName[spOp->uiNameLength] = 0;
                printf("%s", caName);
                printf("\n");
                break;
            case ID_AND:
                printf("%"PRIuMAX": ", (luint) uj);
                printf("AND: ");
                printf("\n");
                break;
            case ID_NOT:
                printf("%"PRIuMAX": ", (luint) uj);
                printf("NOT: ");
                printf("\n");
                break;
            case ID_BKA:
                printf("%"PRIuMAX": ", (luint) uj);
                printf("BKA: ");
                printf("\n");
                break;
            case ID_BKN:
                printf("%"PRIuMAX": ", (luint) uj);
                printf("BKN: ");
                printf("\n");
                break;
            case ID_BKR:
                printf("%"PRIuMAX": ", (luint) uj);
                printf("BKR: ");
                if (spOp->uiCase == ID_BKR_CASE_I) {
                    printf("\\%%i");
                } else {
                    printf("\\%%s");
                }
                if (spOp->uiMode == ID_BKR_MODE_U) {
                    printf("%%u");
                } else {
                    printf("%%p");
                }
                memcpy(caName, spOp->cpName, spOp->uiNameLength);
                caName[spOp->uiNameLength] = 0;
                printf("%s", caName);
                printf("\n");
                break;
            case ID_ABG:
                printf("%"PRIuMAX": ", (luint) uj);
                printf("ABG: ");
                printf("\n");
                break;
            case ID_AEN:
                printf("%"PRIuMAX": ", (luint) uj);
                printf("AEN: ");
                printf("\n");
                break;
            }
        }
        printf("\n");
    }
}
#endif /* SEMANTICS_DEBUG */
