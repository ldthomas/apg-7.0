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
/** \file linesu.c
 * \brief A 32-bit integer version of the lines objects.
 *
 * This object works similarly to the [`lines`](\ref lines.h) object except that the data is 32-bit words
 * rather that 8-bit ASCII characters. Line breaks are made on the [Unicode line ending characters.](\ref lines_endings)
 *
 * Other than the line breaks, the data is considered raw 32-bit data.
 * It is given no other Unicode considerations.
 */

#include "../library/lib.h"
#include "./linesu.h"


/** @name Unicode Line Ending Characters
 * Macro definitions of the set of [Unicode line ending characters](https://en.wikipedia.org/wiki/Newline#Unicode).
 * In addition to the below characters, the combined pair, `CRLF`, is also recognized as a line ending.
 *
 * \anchor lines_endings
 */
///@{
#define LF  0x0A ///< \brief Line Feed
#define VT  0x0B ///< \brief Vertical Tab
#define FF  0x0C ///< \brief Form Feed
#define CR  0x0D ///< \brief Carriage Return
#define NEL 0x85 ///< \brief Next Line
#define LS  0x2028 ///< \brief Line Separator
#define PS  0x2029 ///< \brief Paragraph Separator
///@}

static const void* s_vpMagicNumber = (void*)"lines_u";

/** \struct lines_u
 * \brief The `lines` object context.
 */
typedef struct{
    const void* vpValidate; ///< \brief A "magic number" to validate the context.
    exception* spException; ///< \brief Pointer to an exception structure to report fatal errors
                            /// back to the application's catch block.
    void* vpMem; ///< \brief Pointer to a memory object for allocating all memory associated with this object.
    uint32_t* uipInput; ///< \brief Pointer to the 32-bit integer array.
    aint uiLength; ///< \brief Number of integers in the array.
    void* vpVecLines; ///< \brief Pointer to a vector of parsed lines.
    line_u* spLines; ///< \brief Pointer to the first line.
    aint uiLineCount; ///< \brief Number of lines in the array.
    aint uiIterator; ///< \brief Used by the iterator.
} lines_u;

static void vInputLines(lines_u* spCtx);
static abool bFindLine(line_u* spLines, aint uiLineCount, aint uiCharIndex, aint* uipLine);

/** \brief The `linesu` object constructor.
 *
 * Reads the 32-bint integer input and
 * separates it into individual lines, generating a list of \ref line_u structures, one for each line.
 * \param spEx Pointer to a valid exception structure initialized with \ref XCTOR().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param uipInput Pointer to the array of integers
 * \param uiLength - the number of integers in the array
 * \return Pointer to the object context.
 * Exception thrown on memory allocation failure.
 */
void* vpLinesuCtor(exception* spEx, const uint32_t* uipInput, aint uiLength){
    if(bExValidate(spEx)){
        if(!uipInput || !uiLength){
            XTHROW(spEx, "input is NULL or empty");
        }
        // get the context
        void* vpMem = vpMemCtor(spEx);
        lines_u* spCtx = (lines_u*)vpMemAlloc(vpMem, sizeof(lines_u));
        memset((void*)spCtx, 0, sizeof(lines_u));
        spCtx->uipInput = (uint32_t*)vpMemAlloc(vpMem, ((sizeof(uint32_t) * uiLength)));
        memcpy((void*)spCtx->uipInput, (void*)uipInput, (sizeof(uint32_t) * uiLength));
        spCtx->uiLength = uiLength;
        spCtx->vpMem = vpMem;
        spCtx->spException = spEx;
        spCtx->vpVecLines = vpVecCtor(vpMem, sizeof(line_u), 512);

        // compute lines
        vInputLines(spCtx);
        spCtx->vpValidate = s_vpMagicNumber;
        return (void*)spCtx;
    }else{
        vExContext();
    }
    return NULL;
}

/** \brief The `linesu` object destructor.
 *
 * Releases all allocated memory and clears the context to prevent accidental reuse.
 \param vpCtx A pointer to a valid lines context previously return from vpLinesuCtor().
 Silently ignored if NULL.
 However, if non-NULL it must be a valid lines context pointer.
 The application will silently exit with \ref BAD_CONTEXT exit code if vpCtx is invalid.
 */
void vLinesuDtor(void* vpCtx){
    lines_u* spCtx = (lines_u*)vpCtx;
    if(vpCtx){
        if(spCtx->vpValidate == s_vpMagicNumber){
            // otherwise, just free what was allocated explicitly
            void* vpMem = spCtx->vpMem;
            memset(vpCtx, 0, sizeof(lines_u));
            vMemDtor(vpMem);
        }else{
            vExContext();
        }
    }
}

/** \brief Find the line that the given integer is in.
 * \param vpCtx Pointer to a valid `linesu` context, previously returned from vpLinesuCtor()
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param uiOffset The zero-based offset of the integer to find.
 * \param uipLine Pointer to an integer, set to the found line number on return.
 * \param uipRelOffset Pointer to an integer, set to the relative offset of the integer in the found line.
 * \return True if the line is found, false if the line could not be found
 * (i.e. uiOffset is beyond the last integer of data)
 */
abool bLinesuFindLine(void* vpCtx, aint uiOffset, aint* uipLine, aint* uipRelOffset){
    abool bReturn = APG_FAILURE;
    lines_u* spCtx = (lines_u*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if(!uipLine || !uipRelOffset){
            XTHROW(spCtx->spException, "line and relative offset pointers cannot be NULL");
        }
        if(bFindLine(spCtx->spLines, spCtx->uiLineCount, uiOffset, uipLine)){
            *uipRelOffset = uiOffset - spCtx->spLines[*uipLine].uiCharIndex;
            bReturn = APG_SUCCESS;
        }
    }else{
        vExContext();
    }
    return bReturn;
}

/** \brief Initialize an iterator over the lines.
 *
 * Sets up the iterator and returns the first line.
 * \param vpCtx Pointer to a valid `linesu` context, previously returned from vpLinesuCtor()
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Pointer to the first line.
 */
line_u* spLinesuFirst(void* vpCtx){
    lines_u* spCtx = (lines_u*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        spCtx->uiIterator = 1;
        return spCtx->spLines;
    }
    vExContext();
    return NULL;
}
/** \brief Returns the next line from the iterator.
 * \param vpCtx Pointer to a valid `linesu` context, previously returned from vpLinesuCtor()
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Pointer to the next line. NULL if no further lines are available.
 */
line_u* spLinesuNext(void* vpCtx){
    lines_u* spCtx = (lines_u*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if(spCtx->uiIterator < spCtx->uiLineCount){
            return &spCtx->spLines[spCtx->uiIterator++];
        }
        return NULL;
    }
    vExContext();
    return NULL;
}

/** \brief Returns the number of lines.
 * \param vpCtx Pointer to a valid `linesu` context, previously returned from vpLinesuCtor()
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return The number of lines.
 */
aint uiLinesuCount(void* vpCtx){
    lines_u* spCtx = (lines_u*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        return spCtx->uiLineCount;
    }
    vExContext();
    return 0;
}

/** \brief Returns the number of integers in the 32-bit integer array.
 *
 * Count includes the line ending integers.
 * \param vpCtx Pointer to a valid `linesu` context, previously returned from vpLinesuCtor()
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return The number of array integers.
 */
aint uiLinesuLength(void* vpCtx){
    lines_u* spCtx = (lines_u*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        return spCtx->uiLength;
    }
    vExContext();
    return 0;
}

static abool bFindLine(line_u* spLines, aint uiLineCount, aint uiCharIndex, aint* uipLine) {
    abool bReturn = APG_FAILURE;
    if (!spLines || !uiLineCount) {
        goto fail;
    }
    aint ui;
    line_u* spThis;
    if (uiLineCount < 5) {
        // linear search
        for (ui = 0; ui < uiLineCount; ui += 1) {
            spThis = &spLines[ui];
            if ((uiCharIndex >= spThis->uiCharIndex) && (uiCharIndex < (spThis->uiCharIndex + spThis->uiLineLength))) {
                *uipLine = ui;
                goto success;
            }
        }
        goto fail;
    } else {
        // binary search (https://en.wikipedia.org/wiki/Binary_search_algorithm)
        aint uiL = 0;
        aint uiR = uiLineCount - 1;
        aint uiM;
        while (uiL < uiR) {
            uiM = uiL + (uiR - uiL) / 2;
            spThis = &spLines[uiM];
            if (uiCharIndex >= (spThis->uiCharIndex + spThis->uiLineLength)) {
                uiL = uiM + 1;
                continue;
            }
            if (uiCharIndex < spThis->uiCharIndex) {
                uiR = uiM - 1;
                continue;
            }
            *uipLine = uiM;
            goto success;
        }
        if (uiL == uiR) {
            spThis = &spLines[uiL];
            if ((uiCharIndex >= spThis->uiCharIndex) && (uiCharIndex < (spThis->uiCharIndex + spThis->uiLineLength))) {
                *uipLine = uiL;
                goto success;
            }
        }
        goto fail;
    }
    success: bReturn = APG_SUCCESS;
    fail: ;
    return bReturn;
}

/** Create a vector of line info structs from the input integers.
 * \param spCtx - pointer to an API context
 * \return a vector context for the array of line info structs
 */
static void vInputLines(lines_u* spCtx) {
    aint uiLineIndex = 0;
    aint uiCharIndex = 0;
    aint uiTextLength = 0;
    uint32_t uiChar;
    line_u sLine;
    void* vpVec = spCtx->vpVecLines;
    uint32_t* uipInput = spCtx->uipInput;
    aint uiLen = spCtx->uiLength;
    vVecClear(vpVec);
    while (uiCharIndex < uiLen) {
        uiChar = uipInput[uiCharIndex];
        if (uiChar == LF || uiChar == VT || uiChar == FF || uiChar == NEL || uiChar == LS || uiChar == PS ) {
            // LF line ending
            sLine.uiLineIndex = uiLineIndex;
            sLine.uiCharIndex = uiCharIndex - uiTextLength;
            sLine.uiTextLength = uiTextLength;
            sLine.uiLineLength = uiTextLength + 1;
            uiCharIndex += 1;
            sLine.uiaLineEnd[0] = uiChar;
            sLine.uiaLineEnd[1] = 0;
            sLine.uiaLineEnd[2] = 0;
            vpVecPush(vpVec, (void*) &sLine);
            uiLineIndex += 1;
            uiTextLength = 0;
        } else if (uiChar == CR) {
            sLine.uiLineIndex = uiLineIndex;
            sLine.uiCharIndex = uiCharIndex - uiTextLength;
            sLine.uiTextLength = uiTextLength;
            if ((uiCharIndex < (uiLen - 1)) && (uipInput[uiCharIndex + 1] == LF)) {
                // CRLF line ending
                sLine.uiLineLength = uiTextLength + 2;
                sLine.uiaLineEnd[0] = CR;
                sLine.uiaLineEnd[1] = LF;
                sLine.uiaLineEnd[2] = 0;
                uiCharIndex += 2;
            } else {
                // CR line ending
                sLine.uiLineLength = uiTextLength + 1;
                sLine.uiaLineEnd[0] = CR;
                sLine.uiaLineEnd[1] = 0;
                sLine.uiaLineEnd[2] = 0;
                uiCharIndex += 1;
            }
            vpVecPush(vpVec, (void*) &sLine);
            uiLineIndex += 1;
            uiTextLength = 0;
        } else {
            uiTextLength += 1;
            uiCharIndex += 1;
        }
    }
    if (uiTextLength > 0) {
        // last line had no line ending
        sLine.uiLineIndex = uiLineIndex;
        sLine.uiCharIndex = uiCharIndex - uiTextLength;
        sLine.uiTextLength = uiTextLength;
        sLine.uiLineLength = uiTextLength;
        sLine.uiaLineEnd[0] = 0;
        sLine.uiaLineEnd[1] = 0;
        sLine.uiaLineEnd[2] = 0;
        vpVecPush(vpVec, (void*) &sLine);
    }
    spCtx->spLines = (line_u*) vpVecFirst(spCtx->vpVecLines);
    spCtx->uiLineCount = uiVecLen(spCtx->vpVecLines);
}
