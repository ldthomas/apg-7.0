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
/** \file library/operators-bkr.c
 *  \brief The operator function for handling back references.

  * This function is for internal, parser use only, never to be called directly by the application.
 */

#include "./apg.h"
#ifdef APG_BKR
#include "./lib.h"
#include "./parserp.h"
#include "./astp.h"
#include "./tracep.h"
#include "./statsp.h"
#include "./operators.h"
#include "./backref.h"
#include "./backrefu.h"
#include "./backrefp.h"

static aint uiIMatch(parser* spCtx, aint uiOffset, aint uiPhraseOffset, aint uiPhraseLength);
static aint uiSMatch(parser* spCtx, aint uiOffset, aint uiPhraseOffset, aint uiPhraseLength);

void vBkr(parser* spCtx, const opcode* spOp) {
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    aint uiState = ID_NOMATCH;
    aint uiPhraseLength = 0;
    bkr_phrase sPhrase = {};
    if (spOp->sBkr.uiMode == ID_BKR_MODE_U) {
        sPhrase = sBkruFetch(spCtx->vpBkru, spOp->sBkr.uiRuleIndex);
    } else if (spOp->sBkr.uiMode == ID_BKR_MODE_P) {
        sPhrase = sBkrpFetch(spCtx->vpBkrp, spOp->sBkr.uiRuleIndex);
    } else {
        XTHROW(spCtx->spException,
                "back reference mode must be ID_BKR_MODE_U or ID_BKR_MODE_P");
    }
    if (spOp->sBkr.uiCase == ID_BKR_CASE_I) {
        uiState = uiIMatch(spCtx, spCtx->uiOffset, sPhrase.uiPhraseOffset, sPhrase.uiPhraseLength);
    } else if (spOp->sBkr.uiCase == ID_BKR_CASE_S) {
        uiState = uiSMatch(spCtx, spCtx->uiOffset, sPhrase.uiPhraseOffset, sPhrase.uiPhraseLength);
    } else {
        XTHROW(spCtx->spException,
                "back reference case must be ID_BKR_CASE_I or ID_BKR_CASE_S");
    }
    uiPhraseLength = uiState == ID_MATCH ? sPhrase.uiPhraseLength : 0;
    spCtx->uiOpState = uiState;
    spCtx->uiPhraseLength = uiPhraseLength;
    spCtx->uiOffset += uiPhraseLength;
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

static aint uiIMatch(parser* spCtx, aint uiOffset, aint uiPhraseOffset, aint uiPhraseLength) {
    if (uiOffset + uiPhraseLength > spCtx->uiSubStringEnd) {
        return ID_NOMATCH;
    }
    const achar* acpInputBeg = &spCtx->acpInputString[uiOffset];
    const achar* acpPhraseBeg = &spCtx->acpInputString[uiPhraseOffset];
    const achar* acpPhraseEnd = acpPhraseBeg + uiPhraseLength;
    for (; acpPhraseBeg < acpPhraseEnd; acpPhraseBeg++, acpInputBeg++) {
        // compare lower case, character by character
        achar acInput = *acpInputBeg;
        achar acPhrase = *acpPhraseBeg;
        if (acInput >= (achar) 65 && acInput <= (achar) 90) {
            acInput += (achar) 32;
        }
        if (acPhrase >= (achar) 65 && acPhrase <= (achar) 90) {
            acPhrase += (achar) 32;
        }
        if (acInput != acPhrase) {
            return ID_NOMATCH;
        }
    }
    return ID_MATCH;
}
static aint uiSMatch(parser* spCtx, aint uiOffset, aint uiPhraseOffset, aint uiPhraseLength) {
    if (uiOffset + uiPhraseLength > spCtx->uiSubStringEnd) {
        return ID_NOMATCH;
    }
    const achar* acpInputBeg = &spCtx->acpInputString[uiOffset];
    const achar* acpPhraseBeg = &spCtx->acpInputString[uiPhraseOffset];
    const achar* acpPhraseEnd = acpPhraseBeg + uiPhraseLength;
    for (; acpPhraseBeg < acpPhraseEnd; acpPhraseBeg++, acpInputBeg++) {
        if (*acpInputBeg != *acpPhraseBeg) {
            return ID_NOMATCH;
        }
    }
    return ID_MATCH;
}
#endif /*APG_BKR*/
