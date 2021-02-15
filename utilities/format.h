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
/** \file format.h
 * \brief Header file for the formatting object.
 */

#ifndef FMT_H_
#define FMT_H_

/** \anchor format_identifiers */
/** @name Format Style Identifiers */
///@{
/** \def FMT_HEX
 * \brief Display lines of single 8-bit hexadecimal bytes. */
#define FMT_HEX         0
/** \def FMT_HEX2
 * \brief Display lines of 16-bit hexadecimal integers. */
#define FMT_HEX2        1
/** \def FMT_ASCII
 * \brief Display lines of single 8-bit characters.
 *
 * ASCII characters are displayed when possible. Otherwise the 3-digit decimal value is displayed.
 *  */
#define FMT_ASCII       2
/** \def FMT_CANONICAL
 * \brief Display lines with both FMT_HEX and FMT_ASCII formats.
 *  */
#define FMT_CANONICAL   3
/** \def FMT_UNICODE
 * \brief Display lines of 24-bit integers.
 *  */
#define FMT_UNICODE     4
///@}

void* vpFmtCtor(exception* spEx);
void vFmtDtor(void* vpCtx);
abool bFmtValidate(void* vpCtx);
void vFmtIndent(void* vpCtx, int iIndent);
const char* cpFmtFirstBytes(void* vpCtx, const uint8_t* ucpChars, uint64_t uiLength, int iStyle, uint64_t uiOffset, uint64_t uiLimit);
const char* cpFmtFirstFile(void* vpCtx, const char* cpFileName, int iStyle, uint64_t uiOffset, uint64_t uiLimit);
const char* cpFmtFirstUnicode(void* vpCtx, const uint32_t* uipChars, uint64_t uiLength, uint64_t uiOffset, uint64_t uiLimit);
const char* cpFmtNext(void* vpCtx);

#endif /* FMT_H_ */
