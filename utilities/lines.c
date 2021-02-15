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
/** \file lines.c
 * \brief  A utility object for parsing a text file into its component lines.
 *
 * SABNF grammars and many other files used by parser applications are defined as "lines" of text or characters.
 * Lines are typically ended with one or more of the ASCII control characters
 *  - line feed, 0x0A, \\n or LF
 *  - carriage return, 0x0D, \\r or CR
 *  - 0x0D0A, \\r\\n or CRLF
 *
 * This object provides facilities for iterating and finding lines.
 * See \ref vUtilConvertLineEnds() for an example of using this object to convert and normalize
 * a file's line ends.
 */

#include "../library/lib.h"
#include "./lines.h"

/** @name Private Macros &ndash; for internal object use only */
///@{
#define LF 10
#define CR 13
///@}

static const void* s_vpMagicNumber = (void*)"lines";

/** \struct lines
 * \brief The `lines` object context.
 */
typedef struct{
    const void* vpValidate; ///< \brief A "magic number" to validate the context.
    exception* spException; ///< \brief Pointer to an exception structure to report fatal errors
                            /// back to the application's catch block.
    void* vpMem; ///< \brief Pointer to a memory object for allocating all memory associated with this object.
    char* cpInput; ///< \brief Pointer to the character array.
    aint uiLength; ///< \brief Number of characters in the array.
    void* vpVecLines; ///< \brief Pointer to a vector of parsed lines.
    line* spLines; ///< \brief Pointer to the first line.
    aint uiLineCount; ///< \brief Number of lines in the array.
    aint uiIterator; ///< \brief Used by the iterator.
} lines;

static void vInputLines(lines* spCtx);
static abool bFindLine(line* spLines, aint uiLineCount, aint uiCharIndex, aint* uipLine);

/** \brief The `lines` object constructor.
 *
 * Reads the character input and
 * separates it into individual lines, generating a list of \ref line structures, one for each line of text.
 * \param spEx Pointer to a valid exception structure initialized with \ref XCTOR().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpInput - pointer to the array of characters
 * \param uiLength - the number of characters in the array
 * \return Pointer to the object context.
 * Exception thrown on memory allocation failure.
 */
void* vpLinesCtor(exception* spEx, const char* cpInput, aint uiLength){
    if(bExValidate(spEx)){
        if(!cpInput || !uiLength){
            XTHROW(spEx, "input is NULL or empty");
        }
        // get the context
        void* vpMem = vpMemCtor(spEx);
        lines* spCtx = (lines*)vpMemAlloc(vpMem, sizeof(lines));
        memset((void*)spCtx, 0, sizeof(lines));

        // keep a local copy of the characters
        spCtx->cpInput = (char*)vpMemAlloc(vpMem, ((sizeof(char) * uiLength)));
        memcpy((void*)spCtx->cpInput, (void*)cpInput, (sizeof(char) * uiLength));
        spCtx->uiLength = uiLength;

        // set up the context
        spCtx->vpMem = vpMem;
        spCtx->spException = spEx;
        spCtx->vpVecLines = vpVecCtor(vpMem, sizeof(line), 512);

        // compute lines
        vInputLines(spCtx);
        spCtx->vpValidate = s_vpMagicNumber;
        return (void*)spCtx;
    }else{
        vExContext();
    }
    return NULL;
}

/** \brief The `lines` object destructor.
 *
 * Releases all allocated memory and clears the context to prevent accidental reuse.
 \param vpCtx A pointer to a valid lines context previously return from vpLinesCtor().
 Silently ignored if NULL.
 However, if non-NULL it must be a valid lines context pointer.
 The application will silently exit with \ref BAD_CONTEXT exit code if vpCtx is invalid.
 */
void vLinesDtor(void* vpCtx){
    lines* spCtx = (lines*)vpCtx;
    if(vpCtx){
        if(spCtx->vpValidate == s_vpMagicNumber){
            void* vpMem = spCtx->vpMem;
            memset(vpCtx, 0, sizeof(lines));
            vMemDtor(vpMem);
        }else{
            vExContext();
        }
    }
}

/** \brief Find the line that the given character is in.
 * \param vpCtx Pointer to a `lines` context, previously returned from vpLinesCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param uiOffset The zero-based offset of the character to find.
 * \param uipLine Pointer to an integer, set to the line number that the character is in on return.
 * \param uipRelOffset Pointer to an integer, on return,
 *          set to the offset of the character relative to the beginning of the line it is in.
 * \return True if the line is found,
 *          false if the line could not be found (i.e. uiOffset is beyond the last character of data).
 */
abool bLinesFindLine(void* vpCtx, aint uiOffset, aint* uipLine, aint* uipRelOffset){
    abool bReturn = APG_FAILURE;
    lines* spCtx = (lines*)vpCtx;
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
 * \param vpCtx Pointer to a `lines` context, previously returned from vpLinesCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Pointer to the first line of text.
 */
line* spLinesFirst(void* vpCtx){
    lines* spCtx = (lines*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        spCtx->uiIterator = 1;
        return spCtx->spLines;
    }
    vExContext();
    return NULL;
}
/** \brief Returns the next line of text from the iterator.
 *
 * \param vpCtx Pointer to a `lines` context, previously returned from vpLinesCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return pointer to the next line of text on success, NULL if no further lines are available or on failure
 */
line* spLinesNext(void* vpCtx){
    lines* spCtx = (lines*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if(spCtx->uiIterator < spCtx->uiLineCount){
            return &spCtx->spLines[spCtx->uiIterator++];
        }
        return NULL;
    }
    vExContext();
    return NULL;
}

/** \brief Returns the number of lines of text.
 * \param vpCtx Pointer to a `lines` context, previously returned from vpLinesCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return The number of lines of text.
 */
aint uiLinesCount(void* vpCtx){
    lines* spCtx = (lines*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        return spCtx->uiLineCount;
    }
    vExContext();
    return 0;
}

/** \brief Returns the number of text characters.
 *
 * The number of all characters, including the line ending characters.
 * \param vpCtx Pointer to a `lines` context, previously returned from vpLinesCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return The number of characters of text.
 */
aint uiLinesLength(void* vpCtx){
    lines* spCtx = (lines*)vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        return spCtx->uiLength;
    }
    vExContext();
    return 0;
}

static abool bFindLine(line* spLines, aint uiLineCount, aint uiCharIndex, aint* uipLine) {
    abool bReturn = APG_FAILURE;
    if (!spLines || !uiLineCount) {
        goto fail;
    }
    aint ui;
    line* spThis;
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

/** Create a vector of line info structs from the input characters.
 * \param spCtx - pointer to an API context
 * \return On success, the vector os line structs is complete. Exception thrown otherwise.
 */
static void vInputLines(lines* spCtx) {
    aint uiLineIndex = 0;
    aint uiCharIndex = 0;
    aint uiTextLength = 0;
    char cChar;
    line sLine;
    void* vpVec = spCtx->vpVecLines;
    const char* cpInput = spCtx->cpInput;
    aint uiLen = spCtx->uiLength;
    vVecClear(vpVec);
    while (uiCharIndex < uiLen) {
        cChar = cpInput[uiCharIndex];
        if (cChar == LF) {
            // LF line ending
            sLine.uiLineIndex = uiLineIndex;
            sLine.uiCharIndex = uiCharIndex - uiTextLength;
            sLine.uiTextLength = uiTextLength;
            sLine.uiLineLength = uiTextLength + 1;
            uiCharIndex += 1;
            sLine.caLineEnd[0] = LF;
            sLine.caLineEnd[1] = 0;
            sLine.caLineEnd[2] = 0;
            vpVecPush(vpVec, (void*) &sLine);
            uiLineIndex += 1;
            uiTextLength = 0;
        } else if (cChar == CR) {
            sLine.uiLineIndex = uiLineIndex;
            sLine.uiCharIndex = uiCharIndex - uiTextLength;
            sLine.uiTextLength = uiTextLength;
            if ((uiCharIndex < (uiLen - 1)) && (cpInput[uiCharIndex + 1] == LF)) {
                // CRLF line ending
                sLine.uiLineLength = uiTextLength + 2;
                sLine.caLineEnd[0] = CR;
                sLine.caLineEnd[1] = LF;
                sLine.caLineEnd[2] = 0;
                uiCharIndex += 2;
            } else {
                // CR line ending
                sLine.uiLineLength = uiTextLength + 1;
                sLine.caLineEnd[0] = CR;
                sLine.caLineEnd[1] = 0;
                sLine.caLineEnd[2] = 0;
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
        sLine.caLineEnd[0] = 0;
        sLine.caLineEnd[1] = 0;
        sLine.caLineEnd[2] = 0;
        vpVecPush(vpVec, (void*) &sLine);
    }
    spCtx->spLines = (line*) vpVecFirst(spCtx->vpVecLines);
    spCtx->uiLineCount = uiVecLen(spCtx->vpVecLines);
}
