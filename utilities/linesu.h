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
#ifndef LINESU_H_SAVE_
#define LINESU_H_SAVE_

/** \file linesu.h
 * \brief Header file for the 32-bit integer version of the lines objects.
 */

/** \struct line_u
 * \brief Carries detailed information about the characters and line endings. One for each line in the input grammar file.
 */
typedef struct {
    aint uiLineIndex; /**< \brief  zero-based line number */
    aint uiCharIndex; /**< \brief  zero-based index of the first Unicode character of the line*/
    aint uiLineLength; /**< \brief  number of Unicode characters in the line, including line end characters*/
    aint uiTextLength; /**< \brief  number of Unicode text characters in the line, excluding line end characters*/
    uint32_t uiaLineEnd[3]; /**< \brief  the actual string of line ending character(s), if any */
} line_u;

void* vpLinesuCtor(exception* spEx, const uint32_t* uipInput, aint uiLength);
void vLinesuDtor(void* vpCtx);
line_u* spLinesuFirst(void* vpCtx);
line_u* spLinesuNext(void* vpCtx);
aint uiLinesuCount(void* vpCtx);
aint uiLinesuLength(void* vpCtx);
abool bLinesuFindLine(void* vpCtx, aint uiOffset, aint* uipLine, aint* uipRelOffset);

#endif /* LINESU_H_SAVE_ */
