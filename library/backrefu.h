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
#ifndef LIB_BACKREFU_H_
#define LIB_BACKREFU_H_
#ifdef APG_BKR

/** \file backrefu.h
 * \brief The universal-mode back reference object.
 *
 * For internal parser use only. Should never be called by the application.
 *
   */

void* vpBkruCtor(parser* spParserCtx);
void vBkruRuleOpen(void* vpCtx, aint uiIndex);
void vBkruRuleClose(void* vpCtx, aint uiIndex, aint uiState, aint uiPhraseOffset, aint uiPhraseLength);
void vBkruUdtClose(void* vpCtx, aint uiIndex, aint uiState, aint uiPhraseOffset, aint uiPhraseLength);
void vBkruOpOpen(void* vpCtx);
void vBkruOpClose(void* vpCtx, aint uiState);
bkr_phrase sBkruFetch(void* vpCtx, aint uiIndex);

#endif /* APG_BKR */
#endif /* LIB_BACKREFU_H_ */
