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
#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

/** \dir utilities
 * \brief A tool chest of APG utility functions and objects..
 */


/** \file
 *\brief Miscellaneous utility functions
 */

#include <stdio.h>
#include "../library/lib.h"
#include "./objects.h"

/** @name System Information */
///@{
void vUtilApgInfo(void);
void vUtilSizes(void);
void vUtilCurrentWorkingDirectory(void);
///@}

/** @name File Utilities */
///@{
void vUtilFileWrite(void* vpMem, const char* cpFileName, uint8_t* ucpData, aint uiLen);
void vUtilFileRead(void* vpMem, const char* cpFileName, uint8_t* ucpData, aint* uipLen);
abool bUtilCompareFiles(const char* cpFileL, const char* cpFileR);
abool bUtilCompareFileLines(void* vpMem, const char* cpFileL, const char* cpFileR);
///@}

/** @name Pretty Printing Utilities */
///@{
void vUtilPrintException(exception* spEx);
void vUtilPrintMemStats(const mem_stats* spStats);
void vUtilPrintVecStats(const vec_stats* spStats);
void vUtilPrintLine(line* spLine);
void vUtilPrintLineu(line_u* spLine);
char* cpUtilPrintChar(char cChar, char* cpBuf);
char* cpUtilPrintUChar(uint32_t uiChar, char* cpBuf);
const char* cpUtilUtfTypeName(aint uiType);
const char* cpUtilTrueFalse(luint luiTrue);
const char* cpUtilOpName(aint uiId);
void vUtilPrintParserState(parser_state* spState);
void vUtilPrintMsgs(void* vpMsgs);
///@}

/** @name apg_phrase Utilities */
///@{
achar* acpUtilStrToAchar(void* vpMem, const char* cpStr, aint* uipLen);
const char* cpUtilAcharToStr(void* vpMem, achar* acpAchar, aint uiLen);
apg_phrase* spUtilStrToPhrase(void* vpMem, const char* cpStr);
const char* cpUtilPhraseToStr(void* vpMem, apg_phrase* spPhrase);
///@}

/** @name Unicode u32_phrase Utilities */
///@{
uint32_t* uipUtilStrToUint32(void* vpMem, const char* cpStr, aint* uipLen);
const char* cpUtilUint32ToStr(void* vpMem, const uint32_t* uipUint, aint uiLen);
u32_phrase* spUtilStrToPhrase32(void* vpMem, const char* cpStr);
const char* cpUtilPhrase32ToStr(void* vpMem, u32_phrase* spPhrase);
///@}

/** @name Odd Jobs */
///@{
void vUtilIndent(FILE* spFile, aint uiIndent);
void vUtilCharsToAscii(FILE* spFile, const achar* acpChars, aint uiLength);
abool bUtilAstToXml(void* vpCtx, char* cpType, const char* cpFileName);
void vUtilConvertLineEnds(exception* spEx, const char *cpString, const char *cpEnd, const char *cpFileName);
///@}

/**
 * \page utils Utilities
 * The APG [library](\ref library)
 * is designed for minimum footprint with no I/O and the ability to exclude unneeded pieces of code
 * through configuration in \ref apg.h.
 * However, for a desktop application visuals are usually required and many of the utilities here are simply
 * pretty printers of APG structures and results.
 *
 * For a full list and descriptions of utility functions see utilities.h & utilities.c.
 *
 * Additionally, there are a number of more complex facilities that are used in two or more places in APG
 * and may be of use in user applications as well.
 * These are all implemented as classes, each with its own constructor and destructor.
 *
 * ### Data Conversions - Encoding and Decoding
 * conv.h & conv.c <br>
 * Many applications require data format conversions on the fly.
 * This data conversion object provides conversions to and from Latin1 (ISO-8859-1), UTF-8, UTF-16 and UTF-32.
 * Base64 decoding and encoding can be added as a prefix or postfix operation for any of these formats.
 *
 * ### Display Data in hexdump Style Format
 * format.h & format.c <br>
 * APG phrases are often not printable as ASCII characters. This utility object
 * will display arbitrary 8-bit bytes and 32-bit Unicode characters in hexdump-style formats.
 * For 8-bit byte data, this is similar to the Linux command
 * [`hexdump`](https://www.man7.org/linux/man-pages/man1/hexdump.1.html).
 * For 32-bit word data, similar formatting is provided.

 * ### Line Handling for ASCII Data
 * lines.h & lines.c <br>
 * Often it is convenient to parse ASCII data into lines and to be able to
 * easily iterate through the lines and/or normalize the line endings.
 * The `lines` object provides such a parser.

 * ### Line Handling for Unicode Data
 * linesu.h & linesu.c <br>
 * Often it is convenient to parse Unicode data into lines and to be able to
 * easily iterate through the lines and/or normalize the line endings.
 * The `linesu` object provides such a parser.

 * ### Message Log
 * msglog.h & msglog.c <br>
 * Often an application will need to collect error or warning messages for display at the end,
 * rather that stopping on each error or warning. The `msglog` object provides a consistent
 * and reusable means of doing this throughout APG and any other application that has a need for it.
 *
 */
#endif /* UTILS_UTILS_H_ */
