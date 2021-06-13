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
/** \file library/operators-abnf.c
 * \brief The original seven ABNF operators defined by [RFC5234](https://tools.ietf.org/html/rfc5234).
 *
 * Note that each operation uses a set of pre-parsing macros (before proceeding down the tree)
 * and a set of post-parsing macros (up the tree.) Many of the parsing features, such as tracing and PPPT use
 * are configurable. Macros are used so that when a feature is configured to *not* be present,
 * the macro is empty - it defines no code at all. Depending on the operator these would be:<br>
 * pre-parsing
 *  - TRACE_DOWN()
    - AST_OP_OPEN()
    - BKRU_OP_OPEN()
    - BKRP_OP_OPEN()
    - PPPT_OPEN()

    post-parsing
     - \ref PPPT_CLOSE;
     - BKRU_OP_CLOSE()
     - BKRP_OP_CLOSE()
     - AST_OP_CLOSE()
     - TRACE_UP()
     - STATS_HIT()
 *
 *
 * These functions are for internal, parser use only. They are never to be called directly by the application.
*/

#include "./lib.h"
#include "./parserp.h"
#include "./astp.h"
#include "./tracep.h"
#include "./statsp.h"
#include "./operators.h"
#include "./backref.h"
#include "./backrefu.h"
#include "./backrefp.h"

void vAlt(parser* spCtx, const opcode* spOp) {
    const aint* uipChildBeg;
    const aint* uipChildEnd;
    const opcode* spChildOp;
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    // note: don't need AST_OP_OPEN & AST_OP_CLOSE because all of the
    //       ALT children do this on the syntax tree node immediately below
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    PPPT_OPEN(spCtx, spOp, spCtx->uiOffset);
    uipChildBeg = spOp->sAlt.uipChildList;
    uipChildEnd = uipChildBeg + spOp->sAlt.uiChildCount;
    for (; uipChildBeg < uipChildEnd; uipChildBeg++) {

        // execute the child opcode
        spCtx->uiOpState = ID_ACTIVE;
        spChildOp = spCtx->spOpcodes + *uipChildBeg;
        spCtx->pfnOpFunc[spChildOp->sGen.uiId](spCtx, spChildOp);
        if (spCtx->uiOpState == ID_MATCH) {
            // found a match, stop here, spCtx will have correct phrase length from matched child
            break;
        }
    }
    PPPT_CLOSE;
    // NOTE: if no match is found, spCtx will have NOMATCH and zero phrase length from last child
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

void vCat(parser* spCtx, const opcode* spOp) {
    aint uiOffset = spCtx->uiOffset;
    aint uiPhraseLength = 0;
    aint uiMatched = APG_TRUE;
    const aint* uipChildBeg;
    const aint* uipChildEnd;
    const opcode* spChildOp;
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    AST_OP_OPEN(spCtx->vpAst, spCtx->uiInLookaround);
    BKRU_OP_OPEN(spCtx->vpBkru);
    BKRP_OP_OPEN(spCtx->vpBkrp);
    PPPT_OPEN(spCtx, spOp, spCtx->uiOffset);
    uipChildBeg = spOp->sCat.uipChildList;
    uipChildEnd = uipChildBeg + spOp->sCat.uiChildCount;
    while (uipChildBeg < uipChildEnd) {
        // execute the child opcode
        spCtx->uiOpState = ID_ACTIVE;
        spChildOp = spCtx->spOpcodes + *uipChildBeg;
        spCtx->pfnOpFunc[spChildOp->sGen.uiId](spCtx, spChildOp);

        if (spCtx->uiOpState == ID_NOMATCH) {
            // if any child doesn't match, CAT doesn't match
            spCtx->uiOffset = uiOffset; // reset the offset on no match
            uiMatched = APG_FALSE;
            break;
        }
        uiPhraseLength += spCtx->uiPhraseLength;
        uipChildBeg++;
    }
    if (uiMatched) {
        spCtx->uiPhraseLength = uiPhraseLength;
    } else {
        spCtx->uiOpState = ID_NOMATCH;
        spCtx->uiOffset = uiOffset;
    }
    PPPT_CLOSE;
    BKRU_OP_CLOSE(spCtx->vpBkru, spCtx->uiOpState);
    BKRP_OP_CLOSE(spCtx->vpBkrp, spCtx->uiOpState);
    AST_OP_CLOSE(spCtx->vpAst, spCtx->uiInLookaround, spCtx->uiOpState);
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

void vRep(parser* spCtx, const opcode* spOp) {
    aint uiMatchCount = 0;
    aint uiPhraseLength = 0;
    aint uiOffset = spCtx->uiOffset;
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    AST_OP_OPEN(spCtx->vpAst, spCtx->uiInLookaround);
    BKRU_OP_OPEN(spCtx->vpBkru);
    BKRP_OP_OPEN(spCtx->vpBkrp);
    PPPT_OPEN(spCtx, spOp, spCtx->uiOffset);
    spCtx->uiOpState = ID_ACTIVE;
    while (APG_TRUE) {
        // setup
        AST_OP_OPEN(spCtx->vpAst, spCtx->uiInLookaround);
        BKRU_OP_OPEN(spCtx->vpBkru);
        BKRP_OP_OPEN(spCtx->vpBkrp);

        // execute the child opcode
        spCtx->pfnOpFunc[(spOp + 1)->sGen.uiId](spCtx, (spOp + 1));

        BKRU_OP_CLOSE(spCtx->vpBkru, spCtx->uiOpState);
        BKRP_OP_CLOSE(spCtx->vpBkrp, spCtx->uiOpState);
        AST_OP_CLOSE(spCtx->vpAst, spCtx->uiInLookaround, spCtx->uiOpState);
        if ((spCtx->uiOpState == ID_MATCH) && (spCtx->uiPhraseLength == 0)) {
            // REP succeeds on empty, regardless of min/max
            spCtx->uiOpState = ID_MATCH;
            spCtx->uiOffset = uiOffset + uiPhraseLength;
            spCtx->uiPhraseLength = uiPhraseLength;
            break;
        }
        if (spCtx->uiOpState == ID_NOMATCH) {
            // no match, see if match count is in range
            if (uiMatchCount >= spOp->sRep.uiMin && uiMatchCount <= spOp->sRep.uiMax) {
                // in range
                spCtx->uiOpState = ID_MATCH;
                spCtx->uiOffset = uiOffset + uiPhraseLength;
                spCtx->uiPhraseLength = uiPhraseLength;
            } else {
                // out of range - REP fails
                spCtx->uiOpState = ID_NOMATCH;
                spCtx->uiOffset = uiOffset;
                spCtx->uiPhraseLength = 0;
            }
            // all done with repetitions
            break;
        } else {
            // matched phrase, increment the match count and check if done
            uiMatchCount += 1;
            uiPhraseLength += spCtx->uiPhraseLength;
            if (uiMatchCount >= spOp->sRep.uiMax) {
                // reached max allowed matches, all done
                spCtx->uiOpState = ID_MATCH;
                spCtx->uiOffset = uiOffset + uiPhraseLength;
                spCtx->uiPhraseLength = uiPhraseLength;
                break;
            } // else repeat
        }
    }
    PPPT_CLOSE;
    BKRU_OP_CLOSE(spCtx->vpBkru, spCtx->uiOpState);
    BKRP_OP_CLOSE(spCtx->vpBkrp, spCtx->uiOpState);
    AST_OP_CLOSE(spCtx->vpAst, spCtx->uiInLookaround, spCtx->uiOpState);
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

static void vRnmValidateCallback(parser* spCtx, rule* spRule, aint uiOffset, const char* cpFile, const char* cpFunc,
        uint32_t uiLine) {
    aint uiState = spCtx->sCBData.uiCallbackState;
    if (!(uiState == ID_ACTIVE || uiState == ID_MATCH || uiState == ID_NOMATCH)) {
        XTHROW(spCtx->spException,
                "user rule name callback function: returned invalid state");
    }
    if (uiState != ID_ACTIVE) {
        // validate the phrase length & state
        if ((uiOffset + spCtx->sCBData.uiCallbackPhraseLength) > spCtx->uiSubStringEnd) {
            XTHROW(spCtx->spException,
                    "user rule name callback function: returned phrase length too long - beyond end of input string");
        }
        if ((spRule->uiEmpty == APG_FALSE) && (spCtx->sCBData.uiCallbackState == ID_MATCH)
                && (spCtx->sCBData.uiCallbackPhraseLength == 0)) {
            XTHROW(spCtx->spException,
                    "user rule name callback function: returned empty phrase for non-empty rule");
        }
        // enforce 0 phrase length on NOMATCH
        if(uiState == ID_NOMATCH){
            spCtx->sCBData.uiCallbackPhraseLength = 0;
        }
    }
}
//#define PPPT_FAILED bPpptFailed
//#define PPPT_FAILED_DECL abool bPpptFailed = APG_TRUE
void vRnm(parser* spCtx, const opcode* spOp) {
    rule* spRule = spOp->sRnm.spRule;
    parser_callback pfnCallback = spRule->pfnCallback;
    aint uiOffset = spCtx->uiOffset;
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    AST_RULE_OPEN(spCtx->vpAst, spCtx->uiInLookaround, spRule->uiRuleIndex, spCtx->uiOffset);
    BKRU_RULE_OPEN(spCtx->vpBkru, spRule->uiRuleIndex);
    BKRP_RULE_OPEN(spCtx->vpBkrp, spRule->uiRuleIndex);
    while (APG_TRUE) {
        if (pfnCallback) {
            // have callback for this rule, call it going down the tree
            spCtx->sCBData.uiCallbackState = ID_ACTIVE;
            spCtx->sCBData.uiCallbackPhraseLength = 0;
            spCtx->sCBData.uiParserOffset = uiOffset - spCtx->uiSubStringBeg;
            spCtx->sCBData.uiParserState = ID_ACTIVE;
            spCtx->sCBData.uiParserPhraseLength = 0;
            spCtx->sCBData.uiRuleIndex = spRule->uiRuleIndex;
            spCtx->sCBData.uiUDTIndex = APG_UNDEFINED;
            pfnCallback(&spCtx->sCBData);
            vRnmValidateCallback(spCtx, spRule, uiOffset, __FILE__, __func__, __LINE__);
            if (spCtx->sCBData.uiCallbackState != ID_ACTIVE) {
                // accept the callback phrase and quit parsing this node
                if (spCtx->sCBData.uiCallbackState == ID_EMPTY) {
                    // caller should not return ID_EMPTY but give her a break
                    spCtx->sCBData.uiCallbackState = ID_MATCH;
                    spCtx->sCBData.uiCallbackPhraseLength = 0;
                }
                spCtx->uiOpState = spCtx->sCBData.uiCallbackState;
                spCtx->uiOffset = uiOffset + spCtx->sCBData.uiCallbackPhraseLength;
                spCtx->uiPhraseLength = spCtx->sCBData.uiCallbackPhraseLength;
                break;
            }
        }
        // call the child
        PPPT_OPEN(spCtx, spOp, spCtx->uiOffset);
        spCtx->uiOpState = ID_ACTIVE;
        spCtx->pfnOpFunc[spOp->sRnm.spRule->spOp->sGen.uiId](spCtx, spOp->sRnm.spRule->spOp);
        PPPT_CLOSE;

        if (pfnCallback) {
            // have callback for this rule, call it going up the tree
            spCtx->sCBData.uiCallbackState = ID_ACTIVE;
            spCtx->sCBData.uiCallbackPhraseLength = 0;
            spCtx->sCBData.uiParserOffset = uiOffset - spCtx->uiSubStringBeg;
            spCtx->sCBData.uiParserState = spCtx->uiOpState;
            spCtx->sCBData.uiParserPhraseLength = spCtx->uiPhraseLength;
            spCtx->sCBData.uiRuleIndex = spRule->uiRuleIndex;
            spCtx->sCBData.uiUDTIndex = APG_UNDEFINED;
            pfnCallback(&spCtx->sCBData);
            vRnmValidateCallback(spCtx, spRule, uiOffset, __FILE__, __func__, __LINE__);
            if (spCtx->sCBData.uiCallbackState != ID_ACTIVE) {
                // accept the callback phrase and quit parsing this node
                if (spCtx->sCBData.uiCallbackState == ID_EMPTY) {
                    // caller should not return ID_EMPTY but give her a break
                    spCtx->sCBData.uiCallbackState = ID_MATCH;
                    spCtx->sCBData.uiCallbackPhraseLength = 0;
                }
                spCtx->uiOpState = spCtx->sCBData.uiCallbackState;
                spCtx->uiOffset = uiOffset + spCtx->sCBData.uiCallbackPhraseLength;
                spCtx->uiPhraseLength = spCtx->sCBData.uiCallbackPhraseLength;
                break;
            }
        }
        break;
    }
    BKRU_RULE_CLOSE(spCtx->vpBkru, spRule->uiRuleIndex, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    BKRP_RULE_CLOSE(spCtx->vpBkrp, spRule->uiRuleIndex, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    AST_RULE_CLOSE(spCtx->vpAst, spCtx->uiInLookaround, spRule->uiRuleIndex, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

void vTrg(parser* spCtx, const opcode* spOp) {
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    PPPT_OPEN(spCtx, spOp, spCtx->uiOffset);
    spCtx->uiOpState = ID_NOMATCH;
    spCtx->uiPhraseLength = 0;
    if (spCtx->uiOffset < spCtx->uiSubStringEnd) {
        achar aChar = spCtx->acpInputString[spCtx->uiOffset];
        if (aChar >= spOp->sTrg.acMin && aChar <= spOp->sTrg.acMax) {
            spCtx->uiOpState = ID_MATCH;
            spCtx->uiOffset += 1;
            spCtx->uiPhraseLength = 1;
        }
    }
    PPPT_CLOSE;
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

void vTls(parser* spCtx, const opcode* spOp) {
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    PPPT_OPEN(spCtx, spOp, spCtx->uiOffset);
    if (spCtx->uiOffset + spOp->sTls.uiStrLen > spCtx->uiSubStringEnd) {
        spCtx->uiOpState = ID_NOMATCH;
        spCtx->uiPhraseLength = 0;
    } else {
        const achar* acpInputBeg = &spCtx->acpInputString[spCtx->uiOffset];
        const achar* acpStrBeg = spOp->sTls.acpStrTbl;
        const achar* acpStrEnd = spOp->sTls.acpStrTbl + spOp->sTls.uiStrLen;
        spCtx->uiOpState = ID_MATCH;
        for (; acpStrBeg < acpStrEnd; acpStrBeg++, acpInputBeg++) {
            // compare lower case, character by character, TLS string already converted to lower case
            achar acChar = *acpInputBeg;
            if (acChar >= (achar) 65 && acChar <= (achar) 90) {
                acChar += (achar) 32;
            }
            if (acChar != *acpStrBeg) {
                spCtx->uiOpState = ID_NOMATCH;
                spCtx->uiPhraseLength = 0;
                break;
            }
        }
        if (spCtx->uiOpState == ID_MATCH) {
            spCtx->uiOffset += spOp->sTls.uiStrLen;
            spCtx->uiPhraseLength = spOp->sTls.uiStrLen;
        }
    }
    PPPT_CLOSE;
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

void vTbs(parser* spCtx, const opcode* spOp) {
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    PPPT_OPEN(spCtx, spOp, spCtx->uiOffset);
    if (spCtx->uiOffset + spOp->sTbs.uiStrLen > spCtx->uiSubStringEnd) {
        spCtx->uiOpState = ID_NOMATCH;
        spCtx->uiPhraseLength = 0;
    } else {
        const achar* acpInputBeg = &spCtx->acpInputString[spCtx->uiOffset];
        const achar* acpStrBeg = spOp->sTbs.acpStrTbl;
        const achar* acpStrEnd = spOp->sTbs.acpStrTbl + spOp->sTbs.uiStrLen;
        spCtx->uiOpState = ID_MATCH;
        for (; acpStrBeg < acpStrEnd; acpStrBeg++, acpInputBeg++) {
            // compare character by character
            if (*acpInputBeg != *acpStrBeg) {
                spCtx->uiOpState = ID_NOMATCH;
                spCtx->uiPhraseLength = 0;
                break;
            }
        }
        if (spCtx->uiOpState == ID_MATCH) {
            spCtx->uiOffset += spOp->sTbs.uiStrLen;
            spCtx->uiPhraseLength = spOp->sTbs.uiStrLen;
        }
    }
    PPPT_CLOSE;
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

