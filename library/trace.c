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
/** \file trace.c
 * \brief The public trace object functions.
 *
 */


#include "./apg.h"
#ifdef APG_TRACE
#include "../utilities/utilities.h"
#include "./parserp.h"
#include "./tracep.h"

static void vUp(trace* spCtx, const opcode* spOp, aint uiState, aint uiOffset, aint uiPhraseLength);
static void vDown(trace* spCtx, const opcode* spOp, aint uiOffset);
static abool bTraceConfigCheck(trace* spCtx, const opcode* spOp);
#ifndef APG_NO_PPPT
static void vDownPppt(trace* spCtx, const opcode* spOp, aint uiOffset){
    aint uiState = uiPpptState(spCtx->spParserCtx, spOp, uiOffset);
    if(uiState == ID_ACTIVE){
        vDown(spCtx, spOp, uiOffset);
    }else{
        if (bTraceConfigCheck(spCtx, spOp)) {
            spCtx->iTraceDepth++;
            trace_record sRecord = {};
            if (spCtx->iTraceDepth < spCtx->iTraceDepthMin) {
                spCtx->iTraceDepthMin = spCtx->iTraceDepth;
            }
            sRecord.uiTreeDepth = spCtx->uiTreeDepth;
            sRecord.iTraceDepth = spCtx->iTraceDepth;
            sRecord.uiThisRecord = spCtx->uiThisRecord;
            sRecord.uiState = uiState;
            sRecord.uiOffset = uiOffset;
            sRecord.uiPhraseLength = (uiState == ID_MATCH) ? 1 : 0;
            sRecord.spOpcode = spOp;
            vDisplayRecord(spCtx, &sRecord, APG_TRUE);
            spCtx->uiThisRecord++;
        }
        spCtx->uiTreeDepth++;
        if (spCtx->uiTreeDepth > spCtx->uiTreeDepthMax) {
            spCtx->uiTreeDepthMax = spCtx->uiTreeDepth;
        }
    }
}
static void vUpPppt(trace* spCtx, const opcode* spOp, aint uiState, aint uiOffset, aint uiPhraseLength){
    aint uiPrevState = uiPpptState(spCtx->spParserCtx, spOp, uiOffset);
    if(uiPrevState == ID_ACTIVE){
        vUp(spCtx, spOp, uiState, uiOffset, uiPhraseLength);
    }else{
        spCtx->uiTreeDepth--;
        if (bTraceConfigCheck(spCtx, spOp)) {
            spCtx->iTraceDepth--;
        }
    }
}
#endif


/** \brief The trace object constructor.
 * \param vpCtx Pointer to a valid parser context previously returned from \ref vpParserCtor().
 * If invalid, application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Pointer to the trace object context.
 */
void* vpTraceCtor(void* vpCtx) {
    if(!bParserValidate(vpCtx)){
        vExContext();
    }
    parser* spParser = (parser*) vpCtx;
    if(spParser->vpTrace){
        vTraceDtor(spParser->vpTrace);
        spParser->vpTrace = NULL;
    }
    void* vpMem = spParser->vpMem;
    trace* spTrace = (trace*) vpMemAlloc(vpMem, (aint) sizeof(trace));
    memset((void*) spTrace, 0, sizeof(trace));
    spTrace->vpMem = vpMem;
    spTrace->spException = spMemException(vpMem);
    spTrace->spParserCtx = spParser;

    // open the display file
    spTrace->spOut = stdout;
    spTrace->cpFileName = NULL;
    spTrace->vpVecFileName = vpVecCtor(vpMem, sizeof(char), 1024);

    // lookaround stack
    spTrace->vpLookaroundStack = vpVecCtor(spTrace->vpMem, (aint) sizeof(aint), 500);

    // get space to configure individual rules
    spTrace->sConfig.bpRules = (abool*) vpMemAlloc(spTrace->vpMem,
            (aint) (sizeof(abool) * spTrace->spParserCtx->uiRuleCount));
    memset((void*) spTrace->sConfig.bpRules, 0, sizeof(abool) * spTrace->spParserCtx->uiRuleCount);
    if (spTrace->spParserCtx->uiUdtCount) {
        // get space to configure individual UDTS
        spTrace->sConfig.bpUdts = (abool*) vpMemAlloc(spTrace->vpMem,
                (aint) (sizeof(abool) * spTrace->spParserCtx->uiUdtCount));
        memset((void*) spTrace->sConfig.bpUdts, 0, sizeof(abool) * spTrace->spParserCtx->uiUdtCount);
    }
    // get space to configure individual operators (ALT, CAT, etc., NOTE: ID_GEN must be > all other ids)
    spTrace->sConfig.bpOps = (abool*) vpMemAlloc(spTrace->vpMem, (aint) (sizeof(abool) * ID_GEN));
    memset((void*) spTrace->sConfig.bpOps, 0, sizeof(abool) * ID_GEN);

    // get buffer for constructing display strings
    spTrace->uiBufSize = 4096;
    spTrace->cpBuf = (char*) vpMemAlloc(spTrace->vpMem, (aint) (sizeof(char) * spTrace->uiBufSize));
    memset((void*) spTrace->cpBuf, 0, (sizeof(char) * spTrace->uiBufSize));

    // set default config to all rules, UDTs, ops and records
    vSetDefaultConfig(spTrace);

    // success
    spParser->vpTrace = (void*) spTrace;
    return (void*)spTrace;
}

/** \brief Trace destructor.
 *
 * Note: Distruction of the trace object is optional.
 * The parent parser's destructor, vParserDtor(), will call this function
 * via the macro \ref TRACE_DTOR().
 * \param vpCtx Pointer to a trace context, returned from a previous call to \ref vpTraceCtor(),
 * NULL will be silently ignored. However, non-NULL values must be valid or
 * the application will exit with a \ref BAD_CONTEXT exit code.
 */
void vTraceDtor(void* vpCtx){
    trace* spTrace = (trace*) vpCtx;
    if (spTrace) {
        if(!bParserValidate(spTrace->spParserCtx)){
            vExContext();
        }
        if(spTrace->spOut && (spTrace->spOut != stdout)){
            fclose(spTrace->spOut);
            spTrace->spOut = NULL;
        }
        if(spTrace->spOpenFile && (spTrace->spOpenFile != stdout)){
            fclose(spTrace->spOpenFile);
            spTrace->spOpenFile = NULL;
        }
        vVecDtor(spTrace->vpVecFileName);
        vMsgsDtor(spTrace->vpLog);
        spTrace->spParserCtx->vpTrace = NULL;
    }
}

void vTraceSetOutput(void* vpCtx, const char* cpFileName){
    trace* spTrace = (trace*) vpCtx;
    if(!bParserValidate(spTrace->spParserCtx)){
        vExContext();
    }
    vVecClear(spTrace->vpVecFileName);
    if(spTrace->spOut && spTrace->spOut != stdout){
        fclose(spTrace->spOut);
    }
    spTrace->spOut = stdout;
    spTrace->cpFileName = NULL;
    if(cpFileName){
        spTrace->cpFileName = (char*)vpVecPushn(spTrace->vpVecFileName, NULL, (strlen(cpFileName) + 1));
        strcpy(spTrace->cpFileName, cpFileName);
        spTrace->spOut = fopen(spTrace->cpFileName, "wb");
        if(!spTrace->spOut){
            char caBuf[1024];
            snprintf(caBuf, 1024, "can't open file %s for trace output", cpFileName);
            XTHROW(spTrace->spException, caBuf);
        }
    }
}

/** \brief Only called by apgex.
 *
 * Displays a special form of header for apgex applications.
 */
void vTraceApgexHeader(void* vpCtx){
    trace* spCtx = (trace*) vpCtx;
    if(!bParserValidate(spCtx->spParserCtx)){
        vExContext();
    }
    spCtx->uiThisRecord = 0;
    vDisplayHeader(spCtx);
}
/** \brief Only called by apgex
 *
 * Displays a special form of footer for apgex applications.
 */
void vTraceApgexFooter(void* vpCtx){
    trace* spCtx = (trace*) vpCtx;
    if(!bParserValidate(spCtx->spParserCtx)){
        vExContext();
    }
    vDisplayFooter(spCtx);
    fflush(spCtx->spOut);
}

/** \brief Only called by apgex
 *
 * apgex may parse repeatedly on different sub-strings until it finds a match or fails completely.
 * This is a special separator that is used to distinguish the individual traces for each attempt.
 */
void vTraceApgexSeparator(void* vpCtx, aint uiLastIndex){
    trace* spCtx = (trace*) vpCtx;
    if(!bParserValidate(spCtx->spParserCtx)){
        vExContext();
    }
    vDisplaySeparator(spCtx, uiLastIndex);
}

/** \brief Called by the parser to start the trace.
 *
 * Called via the macro \ref TRACE_BEGIN only trace is implemented.
 */
void vTraceBegin(void* vpCtx){
    trace* spCtx = (trace*) vpCtx;
    if(!bParserValidate(spCtx->spParserCtx)){
        vExContext();
    }
    spCtx->uiThisRecord = 0;
    if(spCtx->sConfig.uiHeaderType == TRACE_HEADER_TRACE){
        vDisplayHeader(spCtx);
    }
}
/** \brief Called by the parser to end the trace.
 *
 * Called via the macro \ref TRACE_END only trace is implemented.
 */
void vTraceEnd(void* vpCtx){
    trace* spCtx = (trace*) vpCtx;
    if(!bParserValidate(spCtx->spParserCtx)){
        vExContext();
    }
    if(spCtx->sConfig.uiHeaderType == TRACE_HEADER_TRACE){
        vDisplayFooter(spCtx);
        fflush(spCtx->spOut);
    }
}
/** \brief Called by the parser prior to downward traversal of a parse tree node.
 *
 * Called via the macro \ref TRACE_DOWN only trace is implemented.
 */
#ifdef APG_NO_PPPT
void vTraceDown(void* vpCtx, const opcode* spOp, aint uiOffset){
    trace* spCtx = (trace*) vpCtx;
    vDown(spCtx, spOp, uiOffset);
}
#else
void vTraceDown(void* vpCtx, const opcode* spOp, aint uiOffset){
    trace* spCtx = (trace*) vpCtx;
    if(spCtx->sConfig.bPppt){
        vDownPppt(spCtx, spOp, uiOffset);
    }else{
        vDown(spCtx, spOp, uiOffset);
    }
}
#endif
/** \brief Called by the parser following upward traversal of a parse tree node.
 *
 * Called via the macro \ref TRACE_UP only trace is implemented.
 */
#ifdef APG_NO_PPPT
void vTraceUp(void* vpCtx, const opcode* spOp, aint uiState, aint uiOffset, aint uiPhraseLength){
    trace* spCtx = (trace*) vpCtx;
    vUp(spCtx, spOp, uiState, uiOffset, uiPhraseLength);
}
#else
void vTraceUp(void* vpCtx, const opcode* spOp, aint uiState, aint uiOffset, aint uiPhraseLength){
    trace* spCtx = (trace*) vpCtx;
    if(spCtx->sConfig.bPppt){
        vUpPppt(spCtx, spOp, uiState, uiOffset, uiPhraseLength);
    }else{
        vUp(spCtx, spOp, uiState, uiOffset, uiPhraseLength);
    }
}
#endif

/** \brief Get the number of traced records, displayed or not.
 *
 * The trace object may be configured to simply count the number of records that it would have displayed,
 * but not to actually display them. With this function the application can retrieve the record count
 * whether or not they were actually displayed.
 * \param vpCtx Pointer to a valid trace object context returned from vpTraceCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 */
aint uiTraceGetRecordCount(void* vpCtx){
    trace* spCtx = (trace*) vpCtx;
    if(!bParserValidate(spCtx->spParserCtx)){
        vExContext();
    }
    return spCtx->uiThisRecord;
}

static void vDown(trace* spCtx, const opcode* spOp, aint uiOffset){
    trace_record sRecord = {};
    aint uiId;
    if (bTraceConfigCheck(spCtx, spOp)) {
        if(spOp->sGen.uiId == ID_NOT || spOp->sGen.uiId == ID_AND){
            uiId = ID_LOOKAROUND_AHEAD;
            vpVecPush(spCtx->vpLookaroundStack, (void*) &uiId);
        }else if(spOp->sGen.uiId == ID_BKA || spOp->sGen.uiId == ID_BKN){
            uiId = ID_LOOKAROUND_BEHIND;
            vpVecPush(spCtx->vpLookaroundStack, (void*) &uiId);
        }
        sRecord.uiTreeDepth = spCtx->uiTreeDepth;
        sRecord.iTraceDepth = spCtx->iTraceDepth;
        sRecord.uiThisRecord = spCtx->uiThisRecord;
        sRecord.spOpcode = spOp;
        sRecord.uiOffset = uiOffset;
        sRecord.uiPhraseLength = 0;
        sRecord.uiState = (aint) ID_ACTIVE;
        vDisplayRecord(spCtx, &sRecord, APG_FALSE);
        spCtx->uiThisRecord++;
        spCtx->iTraceDepth++;
        if (spCtx->iTraceDepth > spCtx->iTraceDepthMax) {
            spCtx->iTraceDepthMax = spCtx->iTraceDepth;
        }
    }
    spCtx->uiTreeDepth++;
    if (spCtx->uiTreeDepth > spCtx->uiTreeDepthMax) {
        spCtx->uiTreeDepthMax = spCtx->uiTreeDepth;
    }
}
static void vUp(trace* spCtx, const opcode* spOp, aint uiState, aint uiOffset, aint uiPhraseLength) {
    trace_record sRecord = {};
    spCtx->uiTreeDepth--;
    if (bTraceConfigCheck(spCtx, spOp)) {
        spCtx->iTraceDepth--;
        if (spCtx->iTraceDepth < spCtx->iTraceDepthMin) {
            spCtx->iTraceDepthMin = spCtx->iTraceDepth;
        }
        if(spOp->sGen.uiId == ID_NOT || spOp->sGen.uiId == ID_AND ||
                spOp->sGen.uiId == ID_BKA || spOp->sGen.uiId == ID_BKN){
            vpVecPop(spCtx->vpLookaroundStack);
        }
        sRecord.uiTreeDepth = spCtx->uiTreeDepth;
        sRecord.iTraceDepth = spCtx->iTraceDepth;
        sRecord.uiThisRecord = spCtx->uiThisRecord;
        sRecord.spOpcode = spOp;
        sRecord.uiOffset = uiOffset;
        sRecord.uiPhraseLength = uiPhraseLength;
        sRecord.uiState = (aint) uiState;
        vDisplayRecord(spCtx, &sRecord, APG_FALSE);
        spCtx->uiThisRecord++;
    }
}
static abool bTraceConfigCheck(trace* spCtx, const opcode* spOp) {
    abool bReturn = APG_FALSE;
    trace_config* spConfig = &spCtx->sConfig;
    aint uiId = spOp->sGen.uiId;
    switch(uiId){
    case ID_ALT:
    case ID_CAT:
    case ID_REP:
    case ID_TRG:
    case ID_TLS:
    case ID_TBS:
    case ID_AND:
    case ID_NOT:
    case ID_BKR:
    case ID_BKA:
    case ID_BKN:
    case ID_ABG:
    case ID_AEN:
        bReturn = spConfig->bpOps[uiId];
        break;
    case ID_RNM:
        bReturn = spConfig->bpRules[spOp->sRnm.spRule->uiRuleIndex];
        break;
    case ID_UDT:
        bReturn = spConfig->bpUdts[spOp->sUdt.spUdt->uiUdtIndex];
        break;
    }
    if(spConfig->bCountOnly){
        // count the record but don't print it (inhibits vDown and vTraceUP)
        if(bReturn){
            spCtx->uiThisRecord++;
            bReturn = APG_FALSE;
        }
    }else{
        if(bReturn){
            if(spCtx->uiThisRecord < spConfig->uiFirstRecord){
                // haven't reached the first record to print yet
                spCtx->uiThisRecord++;
                bReturn = APG_FALSE;
            }else{
                if(spConfig->uiMaxRecords < (spCtx->uiThisRecord - spConfig->uiFirstRecord)){
                    // have already printed the maximum number of records allowed
                    spCtx->uiThisRecord++;
                    bReturn = APG_FALSE;
                }
            }
        }
    }
    return bReturn;
}

#endif /* APG_TRACE */
