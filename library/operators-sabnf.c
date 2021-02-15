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
/** \file library/operators-sabnf.c
 * \brief The additional superset SABNF operators.
 *
 * These functions are for internal, parser use only. They are never to be called directly by the application.
 *
*/

#include "./apg.h"
#ifndef APG_STRICT_ABNF
#include "./lib.h"
#include "./parserp.h"
#include "./astp.h"
#include "./tracep.h"
#include "./statsp.h"
#include "./operators.h"
#include "./backref.h"
#include "./backrefu.h"
#include "./backrefp.h"

void vUdt(parser* spCtx, const opcode* spOp) {
    udt* spUdt = spOp->sUdt.spUdt;
    parser_callback pfnCallback = spUdt->pfnCallback;
    aint uiOffset = spCtx->uiOffset;
    aint uiState;
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    AST_RULE_OPEN(spCtx->vpAst, spCtx->uiInLookaround, (spCtx->uiRuleCount + spUdt->uiUdtIndex), spCtx->uiOffset);

    // call the callback
    spCtx->sCBData.uiCallbackState = ID_ACTIVE;
    spCtx->sCBData.uiCallbackPhraseLength = 0;
    spCtx->sCBData.uiParserOffset = uiOffset - spCtx->uiSubStringBeg;
    spCtx->sCBData.uiParserState = ID_ACTIVE;
    spCtx->sCBData.uiParserPhraseLength = 0;
    spCtx->sCBData.uiRuleIndex = APG_UNDEFINED;
    spCtx->sCBData.uiUDTIndex = spUdt->uiUdtIndex;
    pfnCallback(&spCtx->sCBData);

    // validate the results
    uiState = spCtx->sCBData.uiCallbackState;
    if (uiState == ID_ACTIVE) {
        XTHROW(spCtx->spException,
                "user UDT callback function: returned invalid ID_ACTIVE state");
    }
    if (uiState == ID_EMPTY) {
        // caller should not return ID_EMPTY but give her a break
        spCtx->sCBData.uiCallbackState = ID_MATCH;
        spCtx->sCBData.uiCallbackPhraseLength = 0;
    }
    // validate the phrase length & state
    if ((uiOffset + spCtx->sCBData.uiCallbackPhraseLength) > spCtx->uiSubStringEnd) {
        XTHROW(spCtx->spException,
                "user UDT callback function: returned phrase length too long - beyond end of input string");
    }
    if ((spUdt->uiEmpty == APG_FALSE) && (spCtx->sCBData.uiCallbackState == ID_MATCH)
            && (spCtx->sCBData.uiCallbackPhraseLength == 0)) {
        XTHROW(spCtx->spException,
                "user UDT callback function: returned empty phrase for non-empty UDT");
    }

    // accept the results
    spCtx->uiOpState = spCtx->sCBData.uiCallbackState;
    spCtx->uiOffset = uiOffset + spCtx->sCBData.uiCallbackPhraseLength;
    spCtx->uiPhraseLength = spCtx->sCBData.uiCallbackPhraseLength;
    BKRU_UDT_CLOSE(spCtx->vpBkru, spUdt->uiUdtIndex, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    BKRP_UDT_CLOSE(spCtx->vpBkrp, spUdt->uiUdtIndex, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    AST_RULE_CLOSE(spCtx->vpAst, spCtx->uiInLookaround, (spCtx->uiRuleCount + spUdt->uiUdtIndex), spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

void vAnd(parser* spCtx, const opcode* spOp) {
    aint uiOffset = spCtx->uiOffset;
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    AST_OP_OPEN(spCtx->vpAst, spCtx->uiInLookaround);
    PPPT_OPEN(spCtx, spOp, spCtx->uiOffset);
    spCtx->uiInLookaround++;
    spCtx->pfnOpFunc[(spOp + 1)->sGen.uiId](spCtx, (spOp + 1));

    // if child returns ID_MATCH or ID_NOMATCH, AND returns ID_MATCH or ID_NOMATCH, respectively
    spCtx->uiOffset = uiOffset;
    spCtx->uiPhraseLength = 0;
    spCtx->uiInLookaround--;
    PPPT_CLOSE;
    AST_OP_CLOSE(spCtx->vpAst, spCtx->uiInLookaround, spCtx->uiOpState);
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

void vNot(parser* spCtx, const opcode* spOp) {
    aint uiOffset = spCtx->uiOffset;
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    AST_OP_OPEN(spCtx->vpAst, spCtx->uiInLookaround);
    PPPT_OPEN(spCtx, spOp, spCtx->uiOffset);
    spCtx->uiInLookaround++;
    spCtx->pfnOpFunc[(spOp + 1)->sGen.uiId](spCtx, (spOp + 1));

    // if child returns ID_MATCH or ID_NOMATCH, NOT returns ID_NOMATCH or ID_MATCH, respectively
    spCtx->uiOpState = spCtx->uiOpState == ID_MATCH ? ID_NOMATCH : ID_MATCH;
    spCtx->uiOffset = uiOffset;
    spCtx->uiPhraseLength = 0;
    spCtx->uiInLookaround--;
    PPPT_CLOSE;
    AST_OP_CLOSE(spCtx->vpAst, spCtx->uiInLookaround, spCtx->uiOpState);
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

static void vLookBack(parser* spCtx, const opcode* spOp) {
    aint uiOffset = spCtx->uiOffset;
    aint uiSubStringBeg = spCtx->uiSubStringBeg;
    aint uiSubStringEnd = spCtx->uiSubStringEnd;
    aint uiLen = uiOffset < spCtx->uiLookBehindLength ? uiOffset : spCtx->uiLookBehindLength;
    aint ui = 0;
    spCtx->uiSubStringBeg = uiOffset;
    spCtx->uiSubStringEnd = uiOffset;
    for (; ui <= uiLen; ui += 1) {
        spCtx->uiOffset = uiOffset - ui;
        spCtx->pfnOpFunc[spOp->sGen.uiId](spCtx, spOp);
        if (spCtx->uiOpState == ID_MATCH) {
            if(spCtx->uiPhraseLength != ui){
                spCtx->uiOpState = ID_NOMATCH;
            }
            break;
        }
    }
    spCtx->uiOffset = uiOffset;
    spCtx->uiPhraseLength = 0;
    spCtx->uiSubStringBeg = uiSubStringBeg;
    spCtx->uiSubStringEnd = uiSubStringEnd;
    spCtx->uiTreeDepth--;
}

/** The positive look-behind operator.
 * Looks for a pattern match by iteratively working back from the current parser offset to the beginning of the input string
 * (even if a sub-string is being parsed that does not begin and the beginning of the string)
 * or the maximum look-behind length specified in the parser configuration.
 * 1. BKA exists mainly to support a pattern-matching engine.
 * 2. This is not an efficient procedure. BKA should be avoided if parsing speed is imperative.
 * 3. Look behind stops at the first (and shortest) phrase matched.
 * 4. If the look-behind phrase can accept an empty string BKA will *always* succeed.
 */
void vBka(parser* spCtx, const opcode* spOp) {
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    AST_OP_OPEN(spCtx->vpAst, spCtx->uiInLookaround);
    spCtx->uiInLookaround++;
    vLookBack(spCtx, (spOp + 1));
    spCtx->uiInLookaround--;
    AST_OP_CLOSE(spCtx->vpAst, spCtx->uiInLookaround, spCtx->uiOpState);
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

/** The negative look-behind operator.
 * Looks for a pattern match by iteratively working back from the current parser offset to the beginning of the input string
 * (even if a sub-string is being parsed that does not begin and the beginning of the string)
 * or the maximum look-behind length specified in the parser configuration.
 * 1. BKN exists mainly to support a pattern-matching engine.
 * 2. This is not an efficient procedure. BKN should be avoided if parsing speed is imperative.
 * 3. Look behind stops at the first (and shortest) phrase matched.
 * 4. If the look-behind phrase can accept an empty string BKN will *always* fail.
 */
void vBkn(parser* spCtx, const opcode* spOp) {
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);
    AST_OP_OPEN(spCtx->vpAst, spCtx->uiInLookaround);
    spCtx->uiInLookaround++;
    vLookBack(spCtx, (spOp + 1));
    spCtx->uiOpState = (spCtx->uiOpState == ID_MATCH) ? ID_NOMATCH : ID_MATCH;
    spCtx->uiInLookaround--;
    AST_OP_CLOSE(spCtx->vpAst, spCtx->uiInLookaround, spCtx->uiOpState);
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

void vAbg(parser* spCtx, const opcode* spOp) {
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);

    // offset must match the beginning of the full input string (not just the sub-string)
    spCtx->uiOpState = (spCtx->uiOffset == 0) ? ID_MATCH : ID_NOMATCH;
    spCtx->uiPhraseLength = 0;
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

void vAen(parser* spCtx, const opcode* spOp) {
    spCtx->sState.uiHitCount++;
    spCtx->uiTreeDepth++;
    if(spCtx->uiTreeDepth > spCtx->sState.uiMaxTreeDepth){
        spCtx->sState.uiMaxTreeDepth = spCtx->uiTreeDepth;
    }
    TRACE_DOWN(spCtx->vpTrace, spOp, spCtx->uiOffset);

    // offset must match the end of the full input string (not just the sub-string)
    spCtx->uiOpState = (spCtx->uiOffset == spCtx->uiInputStringLength) ? ID_MATCH : ID_NOMATCH;
    spCtx->uiPhraseLength = 0;
    TRACE_UP(spCtx->vpTrace, spOp, spCtx->uiOpState, (spCtx->uiOffset - spCtx->uiPhraseLength), spCtx->uiPhraseLength);
    STATS_HIT(spCtx->vpStats, spOp, spCtx->uiOpState);
    spCtx->uiTreeDepth--;
}

#endif /* APG_STRICT_ABNF */
