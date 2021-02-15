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
/** \file backrefu.c
 * \brief The universal-mode back reference object.
 *
 * This component will maintain a list of frame stacks, one for each rule that is universal-mode back referenced.
 * It will have a map, that maps each rule index to the proper frame stack index.
 * It also needs a stack of check points. RNM, ALT, CAT and REP operators need to checkpoint the frame stacks going down the tree,
 * and restore the checkpoint coming up the tree if the node does not find a phrase match.
 *
 * Each rule's Single-Expansion Syntax Tree(SEST) will be scanned for any references to any of the back referenced rules.
 * If a rule does not reference any back referenced rules (which is anticipated to be most rules) it will set a "don't backref" flag
 * that will prevent any back reference code in the tree below the rule's RNM node, saving a lot of check point work.
 */
#include "./apg.h"
#ifdef APG_BKR
#include "./lib.h"

#include "./parserp.h"
#include "./backref.h"
#include "./backrefu.h"

//static const int iError = 1;
static const char* s_cpEmpty = "vector is empty";

// rule states
static const aint uiUndefined = APG_UNDEFINED;
static const aint uiNotFound = 0;
static const aint uiFound = 1;
static const aint uiOpen = 2;
//static const aint uiIsBkr = 3;
//static const aint uiRecursive = 4;

typedef struct {
    bkr_rule* spRule;
} bkru_frame;

typedef struct {
    const opcode* spOp;
    bkr_rule* spRule;
    jmp_buf aJumpBuf;
    void* vpVecStack;
} bkru_input;

static void vSESTWalk(backref* spCtx);
static void vOpWalk(backref* spCtx, bkru_input* spInput);
static void vRnmWalk(backref* spCtx, bkru_input* spInput);
static void vAltWalk(backref* spCtx, bkru_input* spInput);
static void vCatWalk(backref* spCtx, bkru_input* spInput);
static void vRepWalk(backref* spCtx, bkru_input* spInput);
static void vFreeAll(backref* spCtx);
static void vSetPhrase(backref* spCtx, aint uiIndex, aint uiOffset, aint uiLength);
static void vRestoreCheckPoints(backref* spCtx, aint* uipCheckPoints);
static void vSetCheckPoints(backref* spCtx, aint* uipCheckPoints);

// !!!! DEBUG
//#include <stdio.h>
//typedef struct print_tag {
//    bkr_rule* spRule;
//    aint uiTreeDepth;
//    aint uiDirection; // down = 1, up = 0, middle = 2;
//    aint uiState;
//} print;
//static char* cpState(aint uiState);
//static void vPrintRule(print* spPrint);
//
//// node direction
//static const aint uiDown = 0;
//static const aint uiInner = 1;
//static const aint uiUp = 2;
// !!!! DEBUG

void* vpBkruCtor(parser* spParserCtx) {
    // component mem allocations
    backref* spCtx = NULL;

    // temp allocations
    aint* uipCounters = NULL;

    void* vpReturn = NULL;
    aint ui = 0;
    aint uiCount = spParserCtx->uiRuleCount + spParserCtx->uiUdtCount;
    aint uiIndex;
    while (APG_TRUE) {
        // initialize the component context
        spCtx = (backref*) vpMemAlloc(spParserCtx->vpMem, (aint) sizeof(backref));
        memset((void*) spCtx, 0, (aint) sizeof(backref));
        spCtx->spParserCtx = spParserCtx;
        spCtx->spException = spMemException(spParserCtx->vpMem);

        // create space for temp array of counters
        uipCounters = (aint*) vpMemAlloc(spParserCtx->vpMem, ((aint) sizeof(aint) * uiCount));
        memset((void*) uipCounters, 0, ((aint) sizeof(aint) * uiCount));

        // initialize rules and UDTS
        spCtx->spRules = (bkr_rule*) vpMemAlloc(spParserCtx->vpMem,
                ((aint) sizeof(bkr_rule) * spParserCtx->uiRuleCount));
        memset((void*) spCtx->spRules, 0, ((aint) sizeof(bkr_rule) * spParserCtx->uiRuleCount));
        if (spParserCtx->uiUdtCount) {
            spCtx->spUdts = (bkr_udt*) vpMemAlloc(spParserCtx->vpMem,
                    ((aint) sizeof(bkr_udt) * spParserCtx->uiUdtCount));
        }
        for (ui = 0; ui < spParserCtx->uiRuleCount; ui += 1) {
            spCtx->spRules[ui].spRule = &spParserCtx->spRules[ui];
            spCtx->spRules[ui].uiIsBackRef = APG_FALSE;
            spCtx->spRules[ui].uiHasBackRef = uiUndefined;
            spCtx->spRules[ui].uiBackRefIndex = uiUndefined;
        }
        for (ui = 0; ui < spParserCtx->uiUdtCount; ui += 1) {
            spCtx->spUdts[ui].spUdt = &spParserCtx->spUdts[ui];
            spCtx->spUdts[ui].uiIsBackRef = APG_FALSE;
            spCtx->spUdts[ui].uiBackRefIndex = uiUndefined;
        }

        // count the number of back universal-mode referenced rules and UDTs
        spCtx->uiBkrCount = 0;
        for (ui = 0; ui < spParserCtx->uiOpcodeCount; ui += 1) {
            if (spParserCtx->spOpcodes[ui].sGen.uiId == ID_BKR) {
                if (spParserCtx->spOpcodes[ui].sBkr.uiMode == ID_BKR_MODE_U) {
                    uiIndex = spParserCtx->spOpcodes[ui].sBkr.uiRuleIndex;
                    if (uipCounters[uiIndex] == 0) {
                        if (uiIndex < spParserCtx->uiRuleCount) {
                            spCtx->spRules[uiIndex].uiIsBackRef = APG_TRUE;
                            spCtx->spRules[uiIndex].uiBackRefIndex = spCtx->uiBkrCount;
                        } else {
                            spCtx->spUdts[uiIndex - spParserCtx->uiRuleCount].uiIsBackRef = APG_TRUE;
                            spCtx->spUdts[uiIndex - spParserCtx->uiRuleCount].uiBackRefIndex = spCtx->uiBkrCount;
                        }
                        spCtx->uiBkrCount += 1;
                    }
                    uipCounters[uiIndex] += 1;
                }
            }
        }

        // !!!! DEBUG
//        {
//            aint uiCount = 0;
//            printf("RULES THAT ARE UNIVERSAL-MODE BACK REFERENCED:\n");
//            for (ui = 0; ui < spParserCtx->uiRuleCount; ui += 1) {
//                bkr_rule* spRule = &spCtx->spRules[ui];
//                if(spRule->uiIsBackRef){
//                    printf("rule: %s: uiIsBackRef: %"PRIuMAX": uiBackRefIndex: %"PRIuMAX"\n",
//                            spRule->spRule->cpRuleName, (luint) spRule->uiIsBackRef, (luint) spRule->uiBackRefIndex);
//                    uiCount++;
//                }
//            }
//            for (ui = 0; ui < spParserCtx->uiUdtCount; ui += 1) {
//                bkr_udt* spUdt = &spCtx->spUdts[ui];
//                if(spUdt->uiIsBackRef){
//                    printf(" udt: %s: uiIsBackRef: %"PRIuMAX": uiBackRefIndex: %"PRIuMAX"\n",
//                            spUdt->spUdt->cpUdtName, (luint) spUdt->uiIsBackRef, (luint) spUdt->uiBackRefIndex);
//                    uiCount++;
//                }
//            }
//            if(!uiCount){
//                printf("    *** NONE ***\n");
//            }
//            printf("\n");
//        }
        // !!!! DEBUG

        if (spCtx->uiBkrCount == 0) {
            // no universal-mode back referenced rules, nothing more to do, return a NULL context
            break;
        }

        // create an initialize the array of stack frames
        spCtx->vppPhraseStacks = (void**) vpMemAlloc(spParserCtx->vpMem, ((aint) sizeof(void*) * spCtx->uiBkrCount));
        memset(spCtx->vppPhraseStacks, 0, ((aint) sizeof(void*) * spCtx->uiBkrCount));
        aint uiIndex = 0;
        for (ui = 0; ui < uiCount; ui += 1) {
            if (uipCounters[ui]) {
                spCtx->vppPhraseStacks[uiIndex] = vpVecCtor(spParserCtx->vpMem, (aint) sizeof(bkr_phrase), 20);
                uiIndex++;
            }
        }

        // walk the SEST an mark all rules that reference universal-mode back referenced rules & UDTs.
        vSESTWalk(spCtx);

        // !!!! DEBUG
//        {
//            aint uiCount = 0;
//            printf("RULES CONTAINING AT LEAST ONE UNIVERSAL-MODE BACK REFERENCED RULE IN ITS (SEST) SYNTAX TREE:\n");
//            for (ui = 0; ui < spParserCtx->uiRuleCount; ui += 1) {
//                bkr_rule* spRule = &spCtx->spRules[ui];
//                if(spRule->uiHasBackRef){
//                    printf("rule: %s: uiHasBackRef: %"PRIuMAX"\n",
//                            spRule->spRule->cpRuleName, (luint) spRule->uiHasBackRef);
//                    uiCount++;
//                }
//            }
//            if(!uiCount){
//                printf("    *** NONE ***\n");
//            }
//            printf("\n");
//        }
        // !!!! DEBUG

        // stack of check points
        spCtx->vpCheckPoints = vpVecCtor(spParserCtx->vpMem, ((aint) sizeof(aint) * spCtx->uiBkrCount), 100);

        // create the stack of open rules
        spCtx->vpOpenRules = vpVecCtor(spParserCtx->vpMem, (aint) sizeof(aint), 100);

        spCtx->vpValidate = (void*) spCtx;
        vpReturn = (void*) spCtx;
        // success
        break;
    }
    if (!vpReturn) {
        // free allocated memory on error
        vFreeAll(spCtx);
    }

    // always free temporary storage, if any
    vMemFree(spParserCtx->vpMem, uipCounters);
    return vpReturn;
}

static void vFreeAll(backref* spCtx){
    // free all vectors
    aint ui;
    for(ui = 0; ui < spCtx->uiBkrCount; ui += 1){
        vVecDtor(spCtx->vppPhraseStacks[ui]);
    }
    vVecDtor(spCtx->vpCheckPoints);
    vVecDtor(spCtx->vpOpenRules);
    // free all memory allocations
    void* vpMem = spCtx->spParserCtx->vpMem;
    vMemFree(vpMem, spCtx->vppPhraseStacks);
    vMemFree(vpMem, (void*)spCtx->spRules);
    vMemFree(vpMem, (void*)spCtx->spUdts);
    memset((void*)spCtx, 0, sizeof(*spCtx));
    vMemFree(vpMem, (void*)spCtx);
}

static void vSetCheckPoints(backref* spCtx, aint* uipCheckPoints) {
    aint ui;
    for (ui = 0; ui < spCtx->uiBkrCount; ui += 1) {
        uipCheckPoints[ui] = uiVecLen(spCtx->vppPhraseStacks[ui]);
    }
}
static void vRestoreCheckPoints(backref* spCtx, aint* uipCheckPoints) {
    aint ui;
    for (ui = 0; ui < spCtx->uiBkrCount; ui += 1) {
        vpVecPopi(spCtx->vppPhraseStacks[ui], uipCheckPoints[ui]);
    }
}
static void vSetPhrase(backref* spCtx, aint uiIndex, aint uiOffset, aint uiLength) {
    bkr_phrase sPhrase = { uiOffset, uiLength };
    vpVecPush(spCtx->vppPhraseStacks[uiIndex], (void*) &sPhrase);
}
void vBkruRuleOpen(void* vpCtx, aint uiIndex) {
    backref* spCtx = (backref*) vpCtx;
    aint* uipCheckPoints;
    if (spCtx->spRules[uiIndex].uiHasBackRef || spCtx->spRules[uiIndex].uiIsBackRef) {
        // this rule is or has on its syntax tree a back referenced rule
        // save checkpoints on the found back reference stack - may need to restore if this rule fails
        uipCheckPoints = (aint*)vpVecPush(spCtx->vpCheckPoints, NULL);
        vSetCheckPoints(spCtx, uipCheckPoints);
    }
    // let the operators in this rule's syntax tree know whether or not they need to bother looking for back referenced rules
    vpVecPush(spCtx->vpOpenRules, (void*) &spCtx->spRules[uiIndex].uiHasBackRef);
}

void vBkruRuleClose(void* vpCtx, aint uiIndex, aint uiState, aint uiPhraseOffset, aint uiPhraseLength) {
    backref* spCtx = (backref*) vpCtx;
    aint* uipCheckPoints;
    if(uiState == ID_MATCH){
        if (spCtx->spRules[uiIndex].uiIsBackRef){
            // push the found back referenced phrase on MATCH
            vSetPhrase(spCtx, spCtx->spRules[uiIndex].uiBackRefIndex, uiPhraseOffset, uiPhraseLength);
        }
    }else{
        if (spCtx->spRules[uiIndex].uiHasBackRef || spCtx->spRules[uiIndex].uiIsBackRef) {
            // restore the check points on NOMATCH
            uipCheckPoints = (aint*)vpVecPop(spCtx->vpCheckPoints);
            if(!uipCheckPoints){
                XTHROW(spCtx->spException, s_cpEmpty);
            }
            vRestoreCheckPoints(spCtx, uipCheckPoints);
        }

    }
    // always pop the open rules stack - it was only of use to operators in syntax tree below this rule
    if (!vpVecPop(spCtx->vpOpenRules)) {
        XTHROW(spCtx->spException, s_cpEmpty);
    }
}

void vBkruUdtClose(void* vpCtx, aint uiIndex, aint uiState, aint uiPhraseOffset, aint uiPhraseLength) {
    backref* spCtx = (backref*) vpCtx;
    if (spCtx->spUdts[uiIndex].uiIsBackRef && (uiState == ID_MATCH)) {
        vSetPhrase(spCtx, spCtx->spUdts[uiIndex].uiBackRefIndex, uiPhraseOffset, uiPhraseLength);
    }
}

void vBkruOpOpen(void* vpCtx) {
    backref* spCtx = (backref*) vpCtx;
    aint* uipCheckPoints;
    aint* uipOpen = (aint*) vpVecLast(spCtx->vpOpenRules);
    if (!uipOpen) {
        XTHROW(spCtx->spException, s_cpEmpty);
    }
    if (*uipOpen) {
        uipCheckPoints = (aint*)vpVecPush(spCtx->vpCheckPoints, NULL);
        vSetCheckPoints(spCtx, uipCheckPoints);
    }
}

void vBkruOpClose(void* vpCtx, aint uiState) {
    backref* spCtx = (backref*) vpCtx;
    aint* uipCheckPoints;
    aint* uipOpen = (aint*) vpVecLast(spCtx->vpOpenRules);
    if (!uipOpen) {
        XTHROW(spCtx->spException, s_cpEmpty);
    }
    if (*uipOpen) {
        uipCheckPoints = (aint*)vpVecPop(spCtx->vpCheckPoints);
        if(!uipCheckPoints){
            XTHROW(spCtx->spException, s_cpEmpty);
        }
        if(uiState == ID_NOMATCH){
            vRestoreCheckPoints(spCtx, uipCheckPoints);
        }
    }
}

bkr_phrase sBkruFetch(void* vpCtx, aint uiIndex) {
    backref* spCtx = (backref*) vpCtx;
    bkr_phrase* spFrame;
    bkr_phrase sReturn;
    if (uiIndex < spCtx->spParserCtx->uiRuleCount) {

        // !!!! DEBUG
//        aint uiLen = uiVecLen(spCtx->vppPhraseStacks[spCtx->spRules[uiIndex].uiBackRefIndex]);
//        aint ui;
//        bkr_phrase* spPhrase = (bkr_phrase*)vpVecFirst(spCtx->vppPhraseStacks[spCtx->spRules[uiIndex].uiBackRefIndex]);
//        printf("sBkruFetch: phrase frames: %"PRIuMAX"\n", (luint)uiLen);
//        for(ui = 0; ui < uiLen; ui++, spPhrase++){
//            printf("    phrase offset: %"PRIuMAX": phrase length: %"PRIuMAX"\n", (luint)spPhrase->uiPhraseOffset, (luint)spPhrase->uiPhraseLength);
//        }
        // !!!! DEBUG

        spFrame = (bkr_phrase*) vpVecLast(spCtx->vppPhraseStacks[spCtx->spRules[uiIndex].uiBackRefIndex]);
        if (!spFrame) {
            XTHROW(spCtx->spException, "unexpected empty phrase stack vector");
        }
        sReturn.uiPhraseLength = spFrame->uiPhraseLength;
        sReturn.uiPhraseOffset = spFrame->uiPhraseOffset;
    } else {
        spFrame = (bkr_phrase*) vpVecLast(
                spCtx->vppPhraseStacks[spCtx->spUdts[uiIndex - spCtx->spParserCtx->uiRuleCount].uiBackRefIndex]);
        if (!spFrame) {
            XTHROW(spCtx->spException, "unexpected empty phrase stack vector");
        }
        sReturn.uiPhraseLength = spFrame->uiPhraseLength;
        sReturn.uiPhraseOffset = spFrame->uiPhraseOffset;
    }
    return sReturn;
}

static void vSetAllParents(backref* spCtx, bkru_input* spInput) {
    aint uiLen = uiVecLen(spInput->vpVecStack);
    if (uiLen) {
        aint ui = 0;
        bkru_frame* spBeg = (bkru_frame*) vpVecFirst(spInput->vpVecStack);
        if (!spBeg) {
            XTHROW(spCtx->spException, s_cpEmpty);
        }
        for (; ui < uiLen; ui += 1) {
            spBeg[ui].spRule->uiHasBackRef = uiFound;
        }
    }
}
static void vRnmWalk(backref* spCtx, bkru_input* spInput) {
    bkru_frame* spFrame;
//    print sPrint;
    bkr_rule* spRule = spInput->spRule;
//    sPrint.spRule = spInput->spRule;
    if (spRule->uiHasBackRef == uiOpen) {
        if (spRule->uiIsBackRef) {
            // set all rules on the open frames
            vSetAllParents(spCtx, spInput);
//            sPrint.uiDirection = uiInner;
//            sPrint.uiTreeDepth = uiVecLen(spInput->vpVecStack);
//            sPrint.uiState = uiIsBkr;
//            vPrintRule(&sPrint);
        }
//        sPrint.uiDirection = uiInner;
//        sPrint.uiTreeDepth = uiVecLen(spInput->vpVecStack);
//        sPrint.uiState = uiRecursive;
//        vPrintRule(&sPrint);
    } else if (spRule->uiHasBackRef == uiUndefined) {
        if (spRule->uiIsBackRef) {
            // set all rules on the open frames
            vSetAllParents(spCtx, spInput);
//            sPrint.uiDirection = uiInner;
//            sPrint.uiTreeDepth = uiVecLen(spInput->vpVecStack);
//            sPrint.uiState = uiIsBkr;
//            vPrintRule(&sPrint);
        }

//        sPrint.uiDirection = uiDown;
//        sPrint.uiTreeDepth = uiVecLen(spInput->vpVecStack);
//        sPrint.uiState = spRule->uiHasBackRef;
//        vPrintRule(&sPrint);

        spFrame = (bkru_frame*) vpVecPush(spInput->vpVecStack, NULL);
        spFrame->spRule = spRule;
        spInput->spOp = spRule->spRule->spOp;
        spRule->uiHasBackRef = uiOpen;
        vOpWalk(spCtx, spInput);
        if (spRule->uiHasBackRef == uiOpen) {
            spRule->uiHasBackRef = uiNotFound;
        }
        if (!vpVecPop(spInput->vpVecStack)) {
            longjmp(spInput->aJumpBuf, 1);
        }

//        sPrint.uiDirection = uiUp;
//        sPrint.uiTreeDepth = uiVecLen(spInput->vpVecStack);
//        sPrint.uiState = spRule->uiHasBackRef;
//        vPrintRule(&sPrint);
    } else {
        if (spRule->uiHasBackRef || spRule->uiIsBackRef) {
            // set all rules on the open frames
            vSetAllParents(spCtx, spInput);
//            sPrint.uiDirection = uiInner;
//            sPrint.uiTreeDepth = uiVecLen(spInput->vpVecStack);
//            sPrint.uiState = uiIsBkr;
//            vPrintRule(&sPrint);
        }
        // already know the state
//        sPrint.uiDirection = uiDown;
//        sPrint.uiTreeDepth = uiVecLen(spInput->vpVecStack);
//        sPrint.uiState = spRule->uiHasBackRef;
//        vPrintRule(&sPrint);
//        sPrint.uiDirection = uiUp;
//        vPrintRule(&sPrint);
    }
}
static void vAltWalk(backref* spCtx, bkru_input* spInput) {
    const aint* uipChildBeg = spInput->spOp->sAlt.uipChildList;
    const aint* uipChildEnd = uipChildBeg + spInput->spOp->sAlt.uiChildCount;
    for (; uipChildBeg < uipChildEnd; uipChildBeg++) {
        spInput->spOp = spCtx->spParserCtx->spOpcodes + *uipChildBeg;
        vOpWalk(spCtx, spInput);
    }
}
static void vCatWalk(backref* spCtx, bkru_input* spInput) {
    const aint* uipChildBeg = spInput->spOp->sCat.uipChildList;
    const aint* uipChildEnd = uipChildBeg + spInput->spOp->sCat.uiChildCount;
    for (; uipChildBeg < uipChildEnd; uipChildBeg++) {
        spInput->spOp = spCtx->spParserCtx->spOpcodes + *uipChildBeg;
        vOpWalk(spCtx, spInput);
    }
}
static void vRepWalk(backref* spCtx, bkru_input* spInput) {
    spInput->spOp = spInput->spOp + 1;
    vOpWalk(spCtx, spInput);
}
static void vOpWalk(backref* spCtx, bkru_input* spInput) {
    switch (spInput->spOp->sGen.uiId) {
    case ID_RNM:
        spInput->spRule = &spCtx->spRules[spInput->spOp->sRnm.spRule->uiRuleIndex];
        vRnmWalk(spCtx, spInput);
        break;
    case ID_UDT:
        if (spCtx->spUdts[spInput->spOp->sUdt.spUdt->uiUdtIndex].uiIsBackRef) {
            vSetAllParents(spCtx, spInput);
        }
        break;
    case ID_ALT:
        vAltWalk(spCtx, spInput);
        break;
    case ID_CAT:
        vCatWalk(spCtx, spInput);
        break;
    case ID_REP:
        vRepWalk(spCtx, spInput);
        break;
    default:
        // all other operators either terminals or empty strings
        break;
    }
}
static void vSESTWalk(backref* spCtx) {
    aint ui;
    bkru_input sInput;
    sInput.vpVecStack = vpVecCtor(spCtx->spParserCtx->vpMem, (aint) sizeof(bkru_frame), 100);
    if (setjmp(sInput.aJumpBuf) != 0) {
        vVecDtor(sInput.vpVecStack);
        XTHROW(spCtx->spException,"internal error - unexpected vpVecPop() error walking the tree");
    }

    // walk the SEST of each rule
    for (ui = 0; ui < spCtx->spParserCtx->uiRuleCount; ui += 1) {
        sInput.spRule = &spCtx->spRules[ui];
        if (sInput.spRule->uiHasBackRef == uiUndefined) {
//                printf("SEST WALK BEGIN: RULE: %s\n", sInput.spRule->spRule->cpRuleName);
            vRnmWalk(spCtx, &sInput);
//                printf("SEST WALK END: tree depth: %"PRIuMAX"\n", (luint) uiVecLen(sInput.vpVecStack));
//                printf("\n");
        }
    }
    vVecDtor(sInput.vpVecStack);
}

// !!!! DEBUG
//static char* cpState(aint uiState) {
//    if (uiState == uiUndefined) {
//        return "undefined";
//    }
//    if (uiState == uiNotFound) {
//        return "BKR not found";
//    }
//    if (uiState == uiFound) {
//        return "BKR found";
//    }
//    if (uiState == uiOpen) {
//        return "recursive rule open";
//    }
//    if (uiState == uiIsBkr) {
//        return "rule is back referenced";
//    }
//    return "unknown";
//}
//static void vPrintRule(print* spPrint) {
//    aint ui = 0;
//    bkr_rule* spRule = spPrint->spRule;
//
//    // indent
//    for (; ui < spPrint->uiTreeDepth; ui += 1) {
//        printf(" ");
//    }
//    if (spPrint->uiDirection == uiDown) {
//        printf("+");
//    } else if (spPrint->uiDirection == uiUp) {
//        printf("-");
//    } else if (spPrint->uiDirection == uiInner) {
//        printf("=>");
//    } else {
//        printf("???");
//    }
//    printf("%s: node state: %s: rule uiHasBackRef: %s\n", spRule->spRule->cpRuleName, cpState(spPrint->uiState),
//            cpState(spRule->uiHasBackRef));
//}
// !!!! DEBUG
#endif /* APG_BKR */
