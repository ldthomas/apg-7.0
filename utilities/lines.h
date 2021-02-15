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
#ifndef LINES_H_
#define LINES_H_

/** \file lines.h
 * \brief Header file for the `lines` object.
 */

/** \struct line
 * \brief Defines the characteristics of a single line.
 */
typedef struct {
    aint uiLineIndex; /**< \brief  The zero-based line index. */
    aint uiCharIndex; /**< \brief  The zero-based index of the first character of the line.*/
    aint uiLineLength; /**< \brief  The number of characters in the line, including the line end characters.*/
    aint uiTextLength; /**< \brief  The number of characters in the line, excluding the line end characters.*/
    char caLineEnd[3]; /**< \brief  The actual, null-terminated string of line ending character(s), if any. */
} line;

void* vpLinesCtor(exception* spEx, const char* cpInput, aint uiLength);
void vLinesDtor(void* vpCtx);
line* spLinesFirst(void* vpCtx);
line* spLinesNext(void* vpCtx);
aint uiLinesCount(void* vpCtx);
aint uiLinesLength(void* vpCtx);
abool bLinesFindLine(void* vpCtx, aint uiOffset, aint* uipLine, aint* uipRelOffset);

#endif /* LINES_H_ */
