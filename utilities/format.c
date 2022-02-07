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
/** \file format.c
 * \brief A formatting object for displaying binary data in human-readable formats.
 *
 * Displaying bytes and Unicode code points which often do not have printing ASCII character counterparts
 * are a common problem with many applications. This object provides a common solution to this problem.
 * It is roughly patterned after the Linux [`hexdump`](https://www.man7.org/linux/man-pages/man1/hexdump.1.html) command.
 * Once the object has been created it can be used as an iterator to display fixed line length displays
 * of the data in several common formats.
 *  - FMT_HEX - 8-bit bytes are displayed as pairs of hexadecimal digits, 16 bytes to a line
 *  - FMT_HEX2 - data is displayed as 16-bit integers, 8 integers to a row.
 *              The byte order will depend on the "endianness" of the hardware.
 *  - FMT_ASCII - 8-bit bytes are displayed as characters, 16 characters to a line
 *    - the ASCII character if printable
 *    - \\t for tab, \\n for line feed, \\r for carriage return
 *    - a 3-digit decimal integer otherwise. e.g. 000, 001, 128, 255
 *  - FMT_CANONICAL - same as FMT_HEX but followed by attempted ASCII
 *    - that is, the ASCII character if printable, a period (.) if not
 *  - FMT_UNICODE - 32-bit data is displayed as 24-bit (6 hexadecimal digit) integers,
 *                  8 big-endian integers to a line
 *
 * Each line is preceded with the 8 hexadecimal digit offset to the first byte of the line.
 * The final line is empty with just the offset to the (empty) byte following the data.
 */

/** @name Private Macros &ndash; used internally by the formatting object */
///@{
#define MAX_INDENT  80
#define LINE_LEN    16
#define LINE_LEN4   4
#define LINE_LEN8   8
#define LINE_LEN12  12
#define FMT_BUFLEN  (128+MAX_INDENT)
#define FILE_END    ((uint64_t)-1)
///@}

static const void* s_vpMagicNumber = (void*)"format";

#include <stdio.h>
#include <limits.h>
#include "../library/lib.h"
#include "./objects.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/** \struct fmt_tag
 * \brief The context for the format object.
 *
 * For use by the formatting object only.
 */
typedef struct fmt_tag{
    const void* vpValidate; ///< \brief A "magic number" used to validate the context.
    exception* spException; ///< \brief Pointer to an exception structure to report fatal errors
                            /// back to the application's catch block.
    void* vpMem; ///< \brief Pointer to a memory object for allocating all memory associated with this object.
    FILE* spFile; ///< \brief Pointer to an open file, if the data is from a file byte stream. Otherwise NULL.
    uint8_t* ucpBuf8; ///< \brief Pointer to a byte buffer for formatting the next byte-stream line.
    uint32_t* uipBuf32; ///< \brief Pointer to a 32-bit buffer for formatting the next Unicode line.
    char* cpFmtBuf; ///< \brief Pointer to a temporary, working buffer for formatting the data in the data buffers.
    const uint8_t* ucpChars8; ///< \brief Pointer to the array of 8-bit bytes to be formatted, if any. Otherwise, NULL.
    const uint32_t* uipChars32; ///< \brief Pointer to the 32-bit Unicode code points to be formatted, if any.
                                /// Otherwise, NULL.
    int iStyle; ///< \brief The display style identifier. [One of these.](\ref format_identifiers).
    int iIndent; ///< \brief The number of spaces to indent before displaying data.
    uint64_t (*pfnFill)(struct fmt_tag*); ///< \brief Function pointer for filling the temp byte or code point buffer.
    void (*pfnFmt)(struct fmt_tag*, uint64_t); ///< \brief Function pointer for
                                                /// formatting the data in the byte or code point buffer.
    uint64_t uiFillLineLen; ///< \brief The max number of data in the buffer (16 for bytes, 8 for code points.)
    uint64_t uiDisplayOffset; ///< \brief The offset to the first byte or code point to display.
    uint64_t uiLastOffset; ///< \brief Keeps the offset to the previous line displayed.
    uint64_t uiNextOffset; ///< \brief Keeps the offset to the next byte or code point to display next.
    uint64_t uiDisplayEnd; ///< \brief The last byte or code point to display.
    abool bDone; ///< \brief True if there are no more lines to display. False otherwise.
} fmt;

static char* cpNext(fmt* spCtx);
static void vFmtBuf8Hex(fmt* spCtx, uint64_t uiChars);
static void vFmtBuf8Hex2(fmt* spCtx, uint64_t uiChars);
static void vFmtBuf8Ascii(fmt* spCtx, uint64_t uiChars);
static void vFmtBuf8Canonical(fmt* spCtx, uint64_t uiChars);
static void vFmtBufUnicode(fmt* spCtx, uint64_t uiChars);
static uint64_t uiFillBytes(fmt* spCtx);
static uint64_t uiFillFile(fmt* spCtx);
static uint64_t uiFillUnicode(fmt* spCtx);
static void vReset(fmt* spCtx);

/** \brief The object constructor.
 * \param spEx Pointer to a valid exception structure initialized with \ref XCTOR().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Pointer to the object context on success.
 */
void* vpFmtCtor(exception* spEx) {
    if(bExValidate(spEx)){
        fmt* spCtx;
        void* vpMem = vpMemCtor(spEx);
        spCtx = (fmt*) vpMemAlloc(vpMem, sizeof(fmt));
        memset((void*) spCtx, 0, sizeof(fmt));
        spCtx->ucpBuf8 = (uint8_t*) vpMemAlloc(vpMem, (LINE_LEN * sizeof(uint8_t)));
        spCtx->uipBuf32 = (uint32_t*) vpMemAlloc(vpMem, (LINE_LEN * sizeof(uint32_t)));
        spCtx->cpFmtBuf = (char*) vpMemAlloc(vpMem, FMT_BUFLEN);
        spCtx->vpMem = vpMem;
        spCtx->spException = spEx;
        spCtx->vpValidate = s_vpMagicNumber;
        return (void*) spCtx;
    }else{
        vExContext();
    }
    return NULL;
}

/** \brief The object destructor.
 *
 * Closes any open files, frees all memory associated with this object.
 * Clears the context to prevent accidental reuse attempts.
 * \param vpCtx A pointer to a valid format context previously return from vpFmtCtor().
 * Silently ignored if NULL.
 * However, if non-NULL it must be a valid format context pointer.
 * The application will silently exit with \ref BAD_CONTEXT exit code if vpCtx is invalid.
 */
void vFmtDtor(void* vpCtx) {
    if (vpCtx) {
        fmt* spCtx = (fmt*) vpCtx;
        if (spCtx->vpValidate == s_vpMagicNumber) {
            if (spCtx->spFile) {
                fclose(spCtx->spFile);
            }
            void* vpMem = spCtx->vpMem;
            memset(vpCtx, 0, sizeof(fmt));
            vMemDtor(vpMem);
        }else{
            vExContext();
        }
    }
}

/** \brief Validate a format context pointer.
 *
 * \param vpCtx A pointer to a possibly valid format context previously return from vpFmtCtor().
 * \return True if valid. False otherwise.
 */
abool bFmtValidate(void* vpCtx){
    fmt* spCtx = (fmt*) vpCtx;
    if (vpCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        return APG_TRUE;
    }
    return APG_FALSE;
}

/** \brief Set the an indentation for the display.
 * \param vpCtx A pointer to a valid format context previously return from vpFmtCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param iIndent - The number of spaces to indent before displaying each line.
 * The value is truncated to the range 0 <= iIndent <= MAX_INDENT.
 */
void vFmtIndent(void* vpCtx, int iIndent) {
    fmt* spCtx = (fmt*) vpCtx;
    if (vpCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        if (iIndent > 0) {
            if (iIndent > MAX_INDENT) {
                spCtx->iIndent = MAX_INDENT;
            } else {
                spCtx->iIndent = iIndent;
            }
        } else {
            spCtx->iIndent = 0;
        }
    }else{
        vExContext();
    }
}

/** \brief Initiate the iterator over an array of 8-bit byte data.
 *
 * \param vpCtx - Pointer to a valid object context, previously returned from \ref vpFmtCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param ucpBytes - Pointer to the array of bytes to display. Cannot be NULL.
 * \param uiLength - The number of bytes in the array. Cannot be 0.
 * \param iStyle - The display style identifier. Any of [these](\ref format_identifiers) except FMT_UNICODE.
 *  - default - invalid types default to FMT_HEX
 * \param uiOffset - If > 0, this is the offset to the first byte do display.
 * \param uiLimit - If > 0, this is the maximum number of bytes to display.
 * \return Pointer to a string representing the first display line.
 */
const char* cpFmtFirstBytes(void* vpCtx, const uint8_t* ucpBytes, uint64_t uiLength,
        int iStyle, uint64_t uiOffset, uint64_t uiLimit) {
    fmt* spCtx = (fmt*) vpCtx;
    if (vpCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        if(ucpBytes == NULL){
            XTHROW(spCtx->spException, "ucpBytes: input cannot be NULL");
        }
        if(uiLength <= 0){
            XTHROW(spCtx->spException, "uiLength: input length must be > 0");
        }
        vReset(spCtx);
        spCtx->iStyle = FMT_HEX;
        spCtx->pfnFmt = vFmtBuf8Hex;
        switch (iStyle) {
        case FMT_HEX2:
            spCtx->iStyle = FMT_HEX2;
            spCtx->pfnFmt = vFmtBuf8Hex2;
            break;
        case FMT_ASCII:
            spCtx->iStyle = FMT_ASCII;
            spCtx->pfnFmt = vFmtBuf8Ascii;
            break;
        case FMT_CANONICAL:
            spCtx->iStyle = FMT_CANONICAL;
            spCtx->pfnFmt = vFmtBuf8Canonical;
            break;
        }
        spCtx->pfnFill = uiFillBytes;
        spCtx->uiFillLineLen = LINE_LEN;
        spCtx->ucpChars8 = ucpBytes;
        if(uiOffset){
            spCtx->uiDisplayOffset = (uiOffset > uiLength) ? uiLength : uiOffset;
        }
        spCtx->uiNextOffset = spCtx->uiDisplayOffset;
        if (uiLimit == 0L) {
            spCtx->uiDisplayEnd = uiLength;
        } else {
            spCtx->uiDisplayEnd = spCtx->uiDisplayOffset + uiLimit;
            if (spCtx->uiDisplayEnd > uiLength) {
                spCtx->uiDisplayEnd = uiLength;
            }
        }
        return cpNext(spCtx);
    }else{
        vExContext();
    }
    return NULL;
}

/** \brief Initiate the iterator over file of 8-bit byte data.
 *
 * \param vpCtx - Pointer to a valid object context, previously returned from \ref vpFmtCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpFileName - Name of the file whose bytes to display.
 * \param iStyle - The display style identifier. Any of [these](\ref format_identifiers) except FMT_UNICODE.
 *  - default - invalid types default to FMT_HEX
 * \param uiOffset - If > 0, this is the offset to the first byte do display.
 * \param uiLimit - If > 0, this is the maximum number of bytes to display.
 * \return Pointer to a string representing the first display line.
 * Exception thrown on file open failure.
 */
const char* cpFmtFirstFile(void* vpCtx, const char* cpFileName, int iStyle, uint64_t uiOffset, uint64_t uiLimit) {
    char* cpReturn = NULL;
    fmt* spCtx = (fmt*) vpCtx;
    if (vpCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        if(cpFileName == NULL || (cpFileName[0] == 0)){
            XTHROW(spCtx->spException, "cpFileName: cannot be NULL or empty");
        }
        char caBuf[PATH_MAX + 128];
        size_t uiBufSize = PATH_MAX + 128;
        vReset(spCtx);
        spCtx->spFile = fopen(cpFileName, "rb");
        if (!spCtx->spFile) {
            snprintf(caBuf, uiBufSize, "can't open file: %s", cpFileName);
            XTHROW(spCtx->spException, caBuf);
        }
        spCtx->iStyle = FMT_HEX;
        spCtx->pfnFmt = vFmtBuf8Hex;
        switch (iStyle) {
        case FMT_HEX2:
            spCtx->iStyle = FMT_HEX2;
            spCtx->pfnFmt = vFmtBuf8Hex2;
            break;
        case FMT_ASCII:
            spCtx->iStyle = FMT_ASCII;
            spCtx->pfnFmt = vFmtBuf8Ascii;
            break;
        case FMT_CANONICAL:
            spCtx->iStyle = FMT_CANONICAL;
            spCtx->pfnFmt = vFmtBuf8Canonical;
            break;
        }
        spCtx->pfnFill = uiFillFile;
        spCtx->uiFillLineLen = LINE_LEN;
        spCtx->iStyle = iStyle;
        spCtx->uiNextOffset = 0L;
        if(uiOffset){
            int iChar;
            while(spCtx->uiNextOffset < uiOffset){
                iChar = fgetc(spCtx->spFile);
                if (iChar == EOF) {
                    break;
                }
                spCtx->uiNextOffset++;
            }
        }
        spCtx->uiDisplayOffset = spCtx->uiNextOffset;
        spCtx->uiDisplayEnd = FILE_END;
        if (uiLimit > 0L) {
            spCtx->uiDisplayEnd = spCtx->uiDisplayOffset + uiLimit;
        }
        return cpNext(spCtx);
    }else{
        vExContext();
    }
    return cpReturn;
}

/** \brief Initiate the iterator over an array of 32-bit Unicode code points.
 *
 * Will actually work for any 32-bit unsigned integers.
 * It will display values in the surrogate pair range. However, values outside
 * the range (> 0xFFFFFF) will result in a distorted display.
 * \param vpCtx - Pointer to a valid object context, previously returned from \ref vpFmtCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param uipChars - Pointer to the 32-bit data array.
 * \param uiLength - The number of code points in the arry.
 * \param uiOffset - If > 0, this is the offset to the first code point do display.
 * \param uiLimit - If > 0, this is the maximum number of code points to display.
 * \return Pointer to a string representing the first line of data.
 */
const char* cpFmtFirstUnicode(void* vpCtx, const uint32_t* uipChars, uint64_t uiLength, uint64_t uiOffset,
        uint64_t uiLimit) {
    char* cpReturn = NULL;
    fmt* spCtx = (fmt*) vpCtx;
    if (vpCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        while (1) {
            vReset(spCtx);
            if(uipChars == NULL){
                XTHROW(spCtx->spException, "uipChars: input cannot be NULL");
            }
            if(uiLength <= 0){
                XTHROW(spCtx->spException, "uiLength: input length must be > 0");
            }
            spCtx->pfnFmt = vFmtBufUnicode;
            spCtx->pfnFill = uiFillUnicode;
            spCtx->uiFillLineLen = LINE_LEN8;
            spCtx->iStyle = FMT_UNICODE;
            spCtx->uipChars32 = uipChars;
            if(uiOffset){
                spCtx->uiDisplayOffset = (uiOffset > uiLength) ? uiLength : uiOffset;
            }
            if (uiLimit == 0L) {
                spCtx->uiDisplayEnd = uiLength;
            } else {
                spCtx->uiDisplayEnd = spCtx->uiDisplayOffset + uiLimit;
                if (spCtx->uiDisplayEnd > uiLength) {
                    spCtx->uiDisplayEnd = uiLength;
                }
            }
            spCtx->uiNextOffset = spCtx->uiDisplayOffset;
            cpReturn = cpNext(spCtx);
            break;
        }
    }else{
        vExContext();
    }
    return cpReturn;
}

/** \brief Formats the next line of data.
 * \param vpCtx - Pointer to a valid object context, previously returned from \ref vpFmtCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Pointer to a string representing the next line of data.
 * NULL if the end of data has been reached.
 */
const char* cpFmtNext(void* vpCtx) {
    const char* cpReturn = NULL;
    fmt* spCtx = (fmt*) vpCtx;
    if (vpCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        if(spCtx->bDone){
            return NULL;
        }else{
            cpReturn = cpNext(spCtx);
        }
    }else{
        vExContext();
    }
    return cpReturn;
}

// static functions
static char* cpNext(fmt* spCtx) {
    uint64_t uiChars = spCtx->pfnFill(spCtx);
    spCtx->pfnFmt(spCtx, uiChars);
    if (uiChars == 0) {
        spCtx->bDone = APG_TRUE;
    }
    return spCtx->cpFmtBuf;
}

static uint64_t uiFillBytes(fmt* spCtx) {
    uint64_t uiDst = 0L;
    spCtx->uiLastOffset = spCtx->uiNextOffset;
    while ((uiDst < spCtx->uiFillLineLen) && (spCtx->uiNextOffset < spCtx->uiDisplayEnd)) {
        spCtx->ucpBuf8[uiDst++] = spCtx->ucpChars8[spCtx->uiNextOffset++];
    }
    return uiDst;
}

static uint64_t uiFillFile(fmt* spCtx) {
    int iChar;
    uint64_t uiDst = 0L;
    spCtx->uiLastOffset = spCtx->uiNextOffset;
    while ((uiDst < spCtx->uiFillLineLen) && (spCtx->uiNextOffset < spCtx->uiDisplayEnd)) {
        iChar = fgetc(spCtx->spFile);
        if (iChar == EOF) {
            break;
        }
        spCtx->uiNextOffset++;
        spCtx->ucpBuf8[uiDst++] = (uint8_t) iChar;
    }
    return uiDst;
}

static uint64_t uiFillUnicode(fmt* spCtx) {
    uint64_t uiDst = 0L;
    spCtx->uiLastOffset = spCtx->uiNextOffset;
    while ((uiDst < spCtx->uiFillLineLen) && (spCtx->uiNextOffset < spCtx->uiDisplayEnd)) {
        spCtx->uipBuf32[uiDst++] = spCtx->uipChars32[spCtx->uiNextOffset++];
    }
    return uiDst;
}

static void vFmtBufUnicode(fmt* spCtx, uint64_t uiChars) {
    uint64_t ui = 0;
    int n = 0;
    if (spCtx->iIndent > 0) {
        for (; n < spCtx->iIndent; n++) {
            spCtx->cpFmtBuf[n] = 32;
        }
    }
    n += sprintf(&spCtx->cpFmtBuf[n], "%08"PRIxMAX" ", spCtx->uiLastOffset);
    if(uiChars > 0){
        for (; ui < uiChars; ui++) {
            if (ui == LINE_LEN4) {
                n += sprintf(&spCtx->cpFmtBuf[n], " ");
            }
            n += sprintf(&spCtx->cpFmtBuf[n], " %06X", spCtx->uipBuf32[ui]);
        }
        if (uiChars < LINE_LEN8) {
            for (; ui < LINE_LEN8; ui++) {
                if (ui == LINE_LEN4) {
                    n += sprintf(&spCtx->cpFmtBuf[n], " ");
                }
                n += sprintf(&spCtx->cpFmtBuf[n], "       ");
            }
        }
        n += sprintf(&spCtx->cpFmtBuf[n], "  |");
        for (ui = 0; ui < uiChars; ui++) {
            if ((spCtx->uipBuf32[ui] >= 32) && (spCtx->uipBuf32[ui] <= 126)) {
                n += sprintf(&spCtx->cpFmtBuf[n], "%c", (char) spCtx->uipBuf32[ui]);
            } else {
                n += sprintf(&spCtx->cpFmtBuf[n], ".");
            }
        }
        n += sprintf(&spCtx->cpFmtBuf[n], "|");
    }
    spCtx->cpFmtBuf[n++] = 10;
    spCtx->cpFmtBuf[n] = 0;
}

static void vFmtBuf8Hex(fmt* spCtx, uint64_t uiChars) {
    uint64_t ui;
    int n = 0;
    if (spCtx->iIndent > 0) {
        for (; n < spCtx->iIndent; n++) {
            spCtx->cpFmtBuf[n] = 32;
        }
    }
    n += sprintf(&spCtx->cpFmtBuf[n], "%08"PRIxMAX" ", spCtx->uiLastOffset);
    if(uiChars > 0){
        for (ui = 0; ui < uiChars; ui++) {
            if (ui == LINE_LEN8) {
                n += sprintf(&spCtx->cpFmtBuf[n], " ");
            }
            n += sprintf(&spCtx->cpFmtBuf[n], " %02x", spCtx->ucpBuf8[ui]);
        }
    }
    spCtx->cpFmtBuf[n++] = 10;
    spCtx->cpFmtBuf[n] = 0;
}

static void vFmtBuf8Hex2(fmt* spCtx, uint64_t uiChars) {
    uint64_t ui;
    uint64_t uiCount;
    int n = 0;
    int j = 0;
    uint16_t ui16 = 0;
    uint16_t* uip16 = (uint16_t*)spCtx->ucpBuf8;
    if (spCtx->iIndent > 0) {
        for (; n < spCtx->iIndent; n++) {
            spCtx->cpFmtBuf[n] = 32;
        }
    }
    n += sprintf(&spCtx->cpFmtBuf[n], "%08"PRIxMAX"", spCtx->uiLastOffset);
    if(uiChars > 0){
        uiCount = uiChars;
        if(uiChars % 2){
            ui16 = (uint16_t)spCtx->ucpBuf8[uiChars - 1];
            uiCount--;
        }
        for (ui = 0; ui < uiCount; ui += 2) {
            n += sprintf(&spCtx->cpFmtBuf[n], " %04x", uip16[j++]);
        }
        if(uiCount < uiChars){
            n += sprintf(&spCtx->cpFmtBuf[n], " %04x", ui16);
        }
    }
    spCtx->cpFmtBuf[n++] = 10;
    spCtx->cpFmtBuf[n] = 0;
}

static void vFmtBuf8Ascii(fmt* spCtx, uint64_t uiChars) {
    uint64_t ui = 0;
    int n = 0;
    int iChar;
    if (spCtx->iIndent > 0) {
        for (; n < spCtx->iIndent; n++) {
            spCtx->cpFmtBuf[n] = 32;
        }
    }
    n += sprintf(&spCtx->cpFmtBuf[n], "%08"PRIxMAX" ", spCtx->uiLastOffset);
    if(uiChars > 0){
        for (; ui < uiChars; ui++) {
            iChar = (int)spCtx->ucpBuf8[ui];
            if(iChar == 9){
                n += sprintf(&spCtx->cpFmtBuf[n], "  \\t");
            }else if(iChar == 10){
                n += sprintf(&spCtx->cpFmtBuf[n], "  \\n");
            }else if(iChar == 13){
                n += sprintf(&spCtx->cpFmtBuf[n], "  \\r");
            }else if(iChar >= 32 && iChar <= 126){
                n += sprintf(&spCtx->cpFmtBuf[n], "   %c", (char)iChar);
            }else{
                n += sprintf(&spCtx->cpFmtBuf[n], " %03d", iChar);
            }
        }
    }
    spCtx->cpFmtBuf[n++] = 10;
    spCtx->cpFmtBuf[n] = 0;
}

static void vFmtBuf8Canonical(fmt* spCtx, uint64_t uiChars) {
    uint64_t ui = 0;
    int n = 0;
    if (spCtx->iIndent > 0) {
        for (; n < spCtx->iIndent; n++) {
            spCtx->cpFmtBuf[n] = 32;
        }
    }
    n += sprintf(&spCtx->cpFmtBuf[n], "%08"PRIxMAX" ", spCtx->uiLastOffset);
    if(uiChars > 0){
        for (; ui < uiChars; ui++) {
            if (ui == LINE_LEN8) {
                n += sprintf(&spCtx->cpFmtBuf[n], " ");
            }
            n += sprintf(&spCtx->cpFmtBuf[n], " %02x", spCtx->ucpBuf8[ui]);
        }
        if (uiChars < LINE_LEN) {
            for (; ui < LINE_LEN; ui++) {
                if (ui == LINE_LEN8) {
                    n += sprintf(&spCtx->cpFmtBuf[n], " ");
                }
                n += sprintf(&spCtx->cpFmtBuf[n], "   ");
            }
        }
        n += sprintf(&spCtx->cpFmtBuf[n], "  |");
        for (ui = 0; ui < uiChars; ui++) {
            if ((spCtx->ucpBuf8[ui] >= 32) && (spCtx->ucpBuf8[ui] <= 126)) {
                n += sprintf(&spCtx->cpFmtBuf[n], "%c", (char) spCtx->ucpBuf8[ui]);
            } else {
                n += sprintf(&spCtx->cpFmtBuf[n], ".");
            }
        }
        n += sprintf(&spCtx->cpFmtBuf[n], "|");
    }
    spCtx->cpFmtBuf[n++] = 10;
    spCtx->cpFmtBuf[n] = 0;
}

static void vReset(fmt* spCtx) {
    if (spCtx->spFile) {
        fclose(spCtx->spFile);
        spCtx->spFile = NULL;
    }
    spCtx->ucpChars8 = NULL;
    spCtx->uipChars32 = NULL;
    spCtx->uiDisplayOffset = 0;
    spCtx->uiDisplayEnd = 0;
    spCtx->uiNextOffset = 0;
    spCtx->bDone = APG_FALSE;
}

