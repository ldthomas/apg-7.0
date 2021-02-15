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
/// \file trace.h
/// \brief Public header file for the trace functions.

#ifndef LIB_TRACE_H_
#define LIB_TRACE_H_

#ifdef APG_TRACE

/** \def TRACE_ASCII
 * \brief Identifier for plain ASCII trace record format.
 */
#define TRACE_ASCII         0
/** \def TRACE_HTML
 * \brief Identifier for HTML trace record format.
 */
#define TRACE_HTML          1

/** @name Public Tracing Functions
 * These functions are used by the application to construct and configure the trace object.
 */
///@{
void* vpTraceCtor(void* vpParserCtx);
void vTraceDtor(void* vpCtx);
void vTraceOutputType(void* vpCtx, aint uiType);
void vTraceSetOutput(void* vpCtx, const char* cpFileName);
void vTraceConfig(void* vpCtx, const char* cpFileName);
void vTraceConfigDisplay(void* vpCtx, const char* cpFileName);
void vTraceConfigGen(void* vpCtx, const char* cpFileName);
aint uiTraceGetRecordCount(void* vpCtx);
///@}

#endif /* LIB_TRACE_H_ */
#endif /* APG_TRACE */
