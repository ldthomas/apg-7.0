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
/// \file tracep.h
/// \brief Private header file for the trace functions.
///
/// Structures and function prototypes used only by the trace object.

#ifndef LIB_TRACEP_H_
#define LIB_TRACEP_H_

#ifdef APG_TRACE

#include <stdio.h>

// apgex handles display of header and footer explicitly
/** @name Trace Header Handling
 * The header may be handled by the trace object directly
 * or if apgex is tracing, apgex will handle the header.
 */
///@{
/** \def TRACE_HEADER_APGEX
 * \brief Identifies apgex as the header handler. */
#define TRACE_HEADER_APGEX         2

/** \def TRACE_HEADER_TRACE
 * \brief Identifies the trace object as the header handler. */
#define TRACE_HEADER_TRACE         3
///@}

/** \struct trace_record
 * \brief The information recorded & displayed by the trace object for each node visited.
 */
typedef struct {
    aint uiTreeDepth; ///< \brief The actual parse tree depth.
    int iTraceDepth;  ///< \brief The partial parse tree depth (a subset possibly restricted by the configuration.)
    aint uiThisRecord;  ///< \brief The record index of the current record.
    aint uiOffset;  ///< \brief The offset into the input string for the first character of the sub-phrase being matched.
    aint uiPhraseLength;  ///< \brief The phrase length of a successful match.
    aint uiState;  ///< \brief The parser's state for this node (\ref ID_ACTIVE, \ref ID_MATCH, etc.)
    const opcode* spOpcode;  ///< \brief Pointer to the opcode for the current node.
} trace_record;

/** \struct trace_config
 * \brief Configuration defining the subset of nodes to display information for.
 */
typedef struct{
    abool* bpRules;  ///< \brief An array of true/false indicators for each rule in the SABNF grammar.
    abool* bpUdts;  ///< \brief  An array of true/false indicators for each UDT in the SABNF grammar.
    abool* bpOps;  ///< \brief  An array of true/false indicators for each opcode in the SABNF grammar.
    aint uiOutputType;  ///< \brief Output type identifier (\ref TRACE_ASCII or \ref TRACE_HTML)
    aint uiHeaderType;  ///< \brief Indicates whether the trace is being done by apgex.
    aint uiFirstRecord;  ///< \brief Number of the first record to display.
    aint uiMaxRecords;  ///< \brief Maximun number of records to display.
    abool bAllRules;  ///< \brief If true, all rule nodes will be displayed.
    abool bAllOps;  ///< \brief If true, all UDT nodes will be displayed.
    abool bCountOnly;  ///< \brief If true, trace will only count the number of records that would have been displayed.
                        /// However, no records are actually displayed.
    abool bPppt;  ///< \brief If true records will use a special PPPT form for the displayed record.
} trace_config;

/** \struct trace
 * \brief The trace object context. Maintains the trace object's state.
 */
typedef struct{
    void* vpMem;  ///< \brief Pointer to the memory context to use for all memory allocation.1
    exception* spException;  ///< \brief Pointer to the exception to use to report fatal errors back to the parser's catch block.
    void* vpLog;  ///< \brief Pointer to a [message log](\ref msglog.c) object for reporting configuration errors.
    parser* spParserCtx;  ///< \brief Pointer back to the parent parser's context.
    char* cpFileName;  ///< \brief Name of the file to display the trace records to.
    void* vpVecFileName; ///< \brief Vector to allocate and re-allocate the memory for the file name.
    FILE* spOut;  ///< \brief Pointer to the open output file.
    FILE* spOpenFile;  ///< \brief Pointer to any other open file.
    void* vpLookaroundStack;  ///< \brief Pointer to a stack require to keep track of when tracing in look around mode.
    aint uiThisRecord;  ///< \brief Index of the current trace record.
    aint uiTreeDepth;  ///< \brief The current full parse tree depth.
    aint uiTreeDepthMax;  ///< \brief The maximum full parse tree depth achieved.
    int iTraceDepth;  ///< \brief The current (possibly partial) traced node depth.
    int iTraceDepthMax;  ///< \brief The maximum (possibly partial) traced node depth.
    int iTraceDepthMin;  ///< \brief The minimum (possibly partial) traced node depth.
                            /// May be greater than zero if configured to not display the first N records.
    char* cpBuf;  ///< \brief Pointer to a scratch buffer.
    aint uiBufSize;  ///< \brief Size of the scratch buffer.
    trace_config sConfig;  ///< \brief Pointer to the trace configuration.
} trace;

/** @name Special Tracing Functions
 * These tracing functions are designed especially and exclusively for apgex tracing.
 */
///@{
void vTraceApgexSeparator(void* vpCtx, aint uiLastIndex);
void vTraceApgexType(void* vpCtx, aint uiType);
void vTraceApgexHeader(void* vpCtx);
void vTraceApgexFooter(void* vpCtx);
///@}

/** @name Private Tracing Functions
 * These functions are only used by the trace and parser objects.
 *
 */
///@{
void vTraceBegin(void* vpCtx);
void vTraceEnd(void* vpCtx);
void vTraceDown(void* vpCtx, const opcode* spOp, aint uiOffset);
void vTraceUp(void* vpCtx, const opcode* spOp, aint uiState, aint uiOffset, aint uiPhraseLength);
void vDisplayRecord(trace* spCtx, trace_record* spRec, abool bIsMatchedPppt);
void vDisplayHeader(trace* spCtx);
void vDisplaySeparator(trace* spCtx, aint uiLastIndex);
void vDisplayFooter(trace* spCtx);
void vSetDefaultConfig(trace* spTrace);
///@}

#endif /* APG_TRACE */

#endif /* LIB_TRACEP_H_ */
