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
/** \file utilities.c
 * \brief Utility functions code.
 */

#include <unistd.h>
#include <limits.h>

#include "./utilities.h"

static const size_t s_uiBufSize = (PATH_MAX + 128);
static char s_cPeriod = 46;
static char* s_cpBinaryVal[16] = {
        "00 00", "00 01", "00 10", "00 11",
        "01 00", "01 01", "01 10", "01 11",
        "10 00", "10 01", "10 10", "10 11",
        "11 00", "11 01", "11 10", "11 11",
};
static char* s_cpDecimalVal[16] = {
        "0 0", "0 1", "0 2", "0 3",
        "1 0", "1 1", "1 2", "1 3",
        "2 0", "2 1", "2 2", "2 3",
        "3 0", "3 1", "3 2", "3 3",
};

static void vDisplayBinary(uint8_t ucChar);
static void vDisplayDecimal(uint8_t ucChar);

/** \brief Display the current state of apg.h.
 *
 */
void vUtilApgInfo(void){
    static char* cpDefined = "defined";
    static char* cpUndefined = "undefined";
    char* cpDef;
    cpDef = cpDefined;   // prevents "defined but not used" error warnings
    cpDef = cpUndefined; // prevents "defined but not used" error warnings
    printf("TYPES & SIZES\n");
    printf("sizeof(achar)   : %"PRIuMAX" : the APG alphabet character\n", (luint)sizeof(achar));
    printf("sizeof(aint)    : %"PRIuMAX" : the APG unsigned int\n", (luint)sizeof(aint));
    printf("sizeof(abool)   : %"PRIuMAX" : the APG true/false boolean\n", (luint)sizeof(abool));
    printf("sizeof(luint)   : %"PRIuMAX" : longest unsigned int,\n", (luint)sizeof(luint));
    printf("                      used primarily for printing integers of unknown length(e.g. printf(\"%%\"PRIuMAX\"\", (luint)var)\n");
    printf("\n");
    printf("MACROS\n");
    printf("APG_VERSION     : %s\n", APG_VERSION);
    printf("APG_COPYRIGHT   : %s\n", APG_COPYRIGHT);
    printf("APG_LICENSE     : %s\n", APG_LICENSE);
#ifdef APG_ACHAR
    printf("APG_ACHAR       : %u : controls the size of the parser's alphabet character(achar)\n", (unsigned)APG_ACHAR);
#else
    printf("APG_ACHAR       : %s : controls the size of the parser's alphabet character(achar)\n", cpUndefined);
#endif
#ifdef APG_AINT
    printf("APG_AINT        : %u : controls the size of the parser's unsigned integer(aint)\n", (unsigned)APG_AINT);
#else
    printf("APG_AINT        : %s : controls the size of the parser's unsigned integer(aint)\n", cpUndefined);
#endif
    printf("APG_TRUE        : %u : the APG \"true\" value\n", (unsigned)APG_TRUE);
    printf("APG_FALSE       : %u : the APG \"false\" value\n", (unsigned)APG_FALSE);
    printf("APG_SUCCESS     : %u : function return value indicating success\n", (unsigned)APG_SUCCESS);
    printf("APG_FAILURE     : %u : function return value indicating failure\n", (unsigned)APG_FAILURE);
    printf("APG_UNDEFINED   : %"PRIuMAX" : used to indicate an undefined unsigned integer\n", (luint)APG_UNDEFINED);
    printf("APG_INFINITE    : %"PRIuMAX" : used to indicate an infinite unsigned integer\n", (luint)APG_INFINITE);
    printf("APG_MAX_AINT    : %"PRIuMAX" : maximum allowed unsigned integer value \n", (luint)APG_MAX_AINT);
    printf("APG_MAX_ACHAR   : %"PRIuMAX" : maximum allowed alphabet character\n", (luint)APG_MAX_ACHAR);

    cpDef = cpUndefined;
#ifdef APG_DEBUG
    cpDef = cpDefined;
#endif
    printf("APG_DEBUG       : %9s : if defined, defines APG_TRACE, APG_STATS, APG_MEM_STATS, APG_VEC_STATS, APG_AST & APG_BKR\n", cpDef);

    cpDef = cpUndefined;
#ifdef APG_TRACE
    cpDef = cpDefined;
#endif
    printf("APG_TRACE       : %9s : if defined, allow parser tracing (includes stdio.h)\n", cpDef);

    cpDef = cpUndefined;
#ifdef APG_STATS
    cpDef = cpDefined;
#endif
    printf("APG_STATS       : %9s : if defined, allows parser to collect parsing statistics (includes stdio.h)\n", cpDef);

    cpDef = cpUndefined;
#ifdef APG_MEM_STATS
    cpDef = cpDefined;
#endif
    printf("APG_MEM_STATS   : %9s : if defined, collect all memory object statistics\n", cpDef);

    cpDef = cpUndefined;
#ifdef APG_VEC_STATS
    cpDef = cpDefined;
#endif
    printf("APG_VEC_STATS   : %9s : if defined, collect all vector object statistics\n", cpDef);

    cpDef = cpUndefined;
#ifdef APG_AST
    cpDef = cpDefined;
#endif
    printf("APG_AST         : %9s : if defined, allow creation of the Absract Syntax Tree (AST)\n", cpDef);

    cpDef = cpUndefined;
#ifdef APG_BKR
    cpDef = cpDefined;
#endif
    printf("APG_BKR         : %9s : if defined, allow back reference operators, e.g. %%urulename\n", cpDef);

    cpDef = cpUndefined;
#ifdef APG_STRICT_ABNF
    cpDef = cpDefined;
#endif
    printf("APG_STRICT_ABNF : %9s : if defined, allow only grammars with ABNF as defined in RFCs 5234 & 7405\n", cpDef);

    cpDef = cpUndefined;
#ifdef APG_NO_PPPT
    cpDef = cpDefined;
#endif
    printf("APG_NO_PPPT     : %9s : if defined, no Partially-Predictive Parsing Tables (PPPT) will be generated\n", cpDef);
}

/** \brief Display the APG type sizes, the compiler's C-language type sizes and a few max values.
 */
void vUtilSizes(void) {
    printf("APG TYPES & SIZES\n");
    printf("sizeof(achar)                  %"PRIuMAX" : the APG alphabet character\n", (luint)sizeof(achar));
    printf("sizeof(aint)                   %"PRIuMAX" : the APG unsigned int\n", (luint)sizeof(aint));
    printf("sizeof(abool)                  %"PRIuMAX" : the APG true/false boolean\n", (luint)sizeof(abool));
    printf("sizeof(luint)                  %"PRIuMAX" : for printing ints of unknown length (e.g. printf(\"%%\"PRIuMAX\"\", (luint)uiVar)\n", (luint)sizeof(luint));
    printf("\nAPG MAXIMUM VALUES\n");
    printf("achar                          %"PRIuMAX"\n", (luint)((achar)-1));
    printf("aint                           %"PRIuMAX"\n", (luint)((aint)-1));
    printf("abool                          %"PRIuMAX"\n", (luint)((abool)-1));
    printf("luint                          %"PRIuMAX"\n", (luint)-1);
    printf("\nSYSTEM TYPES & SIZES\n");
    printf("sizeof(unsigned char)          %"PRIuMAX"\n", (luint)sizeof(unsigned char));
    printf("sizeof(unsigned short int)     %"PRIuMAX"\n", (luint)sizeof(unsigned short int));
    printf("sizeof(unsigned int)           %"PRIuMAX"\n", (luint)sizeof(unsigned int));
    printf("sizeof(unsigned long int)      %"PRIuMAX"\n", (luint)sizeof(unsigned long int));
    printf("sizeof(unsigned long long int) %"PRIuMAX"\n", (luint)sizeof(unsigned long long int));
    printf("sizeof(uintmax_t)              %"PRIuMAX"\n", (luint)sizeof(uintmax_t));
    printf("sizeof(uint8_t)                %"PRIuMAX"\n", (luint)sizeof(uint8_t));
    printf("sizeof(uint16_t)               %"PRIuMAX"\n", (luint)sizeof(uint16_t));
    printf("sizeof(uint32_t)               %"PRIuMAX"\n", (luint)sizeof(uint32_t));
    printf("sizeof(uint64_t)               %"PRIuMAX"\n", (luint)sizeof(uint64_t));
    printf("sizeof(uint_least8_t)          %"PRIuMAX"\n", (luint)sizeof(uint_least8_t));
    printf("sizeof(uint_least16_t)         %"PRIuMAX"\n", (luint)sizeof(uint_least16_t));
    printf("sizeof(uint_least32_t)         %"PRIuMAX"\n", (luint)sizeof(uint_least32_t));
    printf("sizeof(uint_least64_t)         %"PRIuMAX"\n", (luint)sizeof(uint_least64_t));
    printf("sizeof(uint_fast8_t)           %"PRIuMAX"\n", (luint)sizeof(uint_fast8_t));
    printf("sizeof(uint_fast16_t)          %"PRIuMAX"\n", (luint)sizeof(uint_fast16_t));
    printf("sizeof(uint_fast32_t)          %"PRIuMAX"\n", (luint)sizeof(uint_fast32_t));
    printf("sizeof(uint_fast64_t)          %"PRIuMAX"\n", (luint)sizeof(uint_fast64_t));
    printf("\nSYSTEM MAXIMUM VALUES\n");
    printf("uint8_t                        %"PRIuMAX"\n", (luint)((uint8_t)-1));
    printf("uint16_t                       %"PRIuMAX"\n", (luint)((uint16_t)-1));
    printf("uint32_t                       %"PRIuMAX"\n", (luint)((uint32_t)-1));
    printf("uint64_t                       %"PRIuMAX"\n", (luint)((uint64_t)-1));
}

/** \brief Display the current working directory. */
void vUtilCurrentWorkingDirectory(void){
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        printf("getcwd() error\n");
    }
}

/** \brief Write from the caller's data area to the given file name.
 *
 * \param vpMem Pointer to a valid memory context previously returned from vMemCtor().
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpFileName Relative or absolute path name of the file to write.
 * \param ucpData Pointer to the caller's 8-bit byte data array.
 * \param uiLen Length, or number of bytes in the data array.
 * \return Throws exceptions on file open and write errors.
 */
void vUtilFileWrite(void* vpMem, const char* cpFileName, uint8_t* ucpData, aint uiLen){
    if(!bMemValidate(vpMem)){
        vExContext();
    }
    char caBuf[s_uiBufSize];
    exception* spEx = spMemException(vpMem);
    if(!cpFileName || cpFileName[0] == 0){
        XTHROW(spEx, "file name cannot be NULL or empty");
    }
    if(!ucpData || !uiLen){
        XTHROW(spEx, "data cannot be NULL or empty");
    }
    // note: the "b" in "wb" is required on some systems (I'm looking at you Windows.)
    FILE* spFile = fopen(cpFileName, "wb");
    if(!spFile){
        snprintf(caBuf, s_uiBufSize, "can't open file \"%s\" for write", cpFileName);
        XTHROW(spEx, caBuf);
    }
    size_t uiWrite = fwrite((void*)ucpData, sizeof(uint8_t), uiLen, spFile);
    fclose(spFile);
    if((aint)uiWrite != uiLen){
        snprintf(caBuf, s_uiBufSize, "file write error: file name: %s: bytes to write: %"PRIuMAX": bytes written: %"PRIuMAX"",
                cpFileName, (luint)uiLen, (luint)uiWrite);
        XTHROW(spEx, caBuf);
    }
}

/** \brief Read a file into the caller's data area
 *
 * \param vpMem Pointer to a valid memory context previously returned from vMemCtor().
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpFileName Relative or absolute path name of the file to read into.
 * \param ucpData Pointer to the caller's 8-bit byte data buffer.
 * If NULL, no data is read.
 * \param[in, out] uipLen Pointer to the integer to receive the number of bytes in the file.
 * On input, this is the length of the caller's data buffer.
 * Data, up to this length, will be read into the buffer.
 * Regardless of its value on input, it will be set to the actual number of bytes in the file on output.
 * On return, if *uipLen is <= its original value then the caller can be confident that all data is in the
 * buffer provided. If *uipLen > its original value, then the caller must repeat the call with a larger data buffer.
 * \return Throws exception on file open or read error.
 */
void vUtilFileRead(void* vpMem, const char* cpFileName, uint8_t* ucpData, aint* uipLen){
    if(!bMemValidate(vpMem)){
        vExContext();
    }
    char caBuf[s_uiBufSize];
    exception* spEx = spMemException(vpMem);
    if(!cpFileName || cpFileName[0] == 0){
        XTHROW(spEx, "file name cannot be NULL or empty");
    }
    if(!uipLen){
        XTHROW(spEx, "data length pointer cannot be NULL");
    }
    // note: the "b" in "rb" is required on some systems (I'm looking at you Windows.)
    FILE* spFile = fopen(cpFileName, "rb");
    if(!spFile){
        snprintf(caBuf, s_uiBufSize, "can't open file \"%s\" for read", cpFileName);
        XTHROW(spEx, caBuf);
    }
    aint uiBufferLen;
    if(!ucpData){
        uiBufferLen = 0;
    }else{
        uiBufferLen = *uipLen;
    }
    size_t uiBytesRead = 0;
    size_t uiBytesCopied = 0;
    size_t uiBytesToCopy = uiBufferLen;
    uint8_t caReadBuf[4096];
    size_t uiBlockBytes = fread(caReadBuf, 1, sizeof(caReadBuf), spFile);
    while(uiBlockBytes != 0){
        uiBytesRead += uiBlockBytes;
        if(uiBytesCopied < uiBufferLen){
            uiBytesToCopy = uiBufferLen - uiBytesCopied;
            if(uiBytesToCopy > uiBlockBytes){
                uiBytesToCopy = uiBlockBytes;
            }
            memcpy((void*)&ucpData[uiBytesCopied], (void*)&caReadBuf[0], uiBytesToCopy);
            uiBytesCopied += uiBytesToCopy;
        }
        uiBlockBytes = fread(caReadBuf, 1, sizeof(caReadBuf), spFile);
    }
    fclose(spFile);
    if(uiBytesRead > APG_MAX_AINT){
        XTHROW(spEx, "sizeof(aint) too small");
    }
    *uipLen = (aint)(uiBytesRead);
}

/** \brief Compare two files, byte for byte.
 * \param cpFileL Pointer to left file name.
 * \param cpFileR Pointer to right file name.
 * \return True if both files are byte-for-byte equal.
 * False if not equal or if either file does not exist or otherwise cannot be opened.
 */
abool bUtilCompareFiles(const char* cpFileL, const char* cpFileR){
    FILE* spL = NULL, *spR = NULL;
    int iL, iR;
    abool bReturn = APG_FALSE;
    spL = fopen(cpFileL, "rb");
    if(!spL){
        goto notequal;
    }
    spR = fopen(cpFileR, "rb");
    if(!spR){
        goto notequal;
    }
    while(APG_TRUE){
        iL = fgetc(spL);
        iR = fgetc(spR);
        if(iL != iR){
            goto notequal;
        }
        if(iL == EOF){
            break;
        }
    }
    bReturn = APG_TRUE;
    notequal:;
    if(spL){
        fclose(spL);
    }
    if(spR){
        fclose(spR);
    }
    return bReturn;
}

/** \brief Compare two text files, line by line, without regard for the line ending characters.
 *
 * Developed for the problem of comparing files which are identical except for the line end characters.
 * Necessitated by the different line ending conventions of different OSs.
 *
 * \param vpMem Pointer to a valid memory context previously returned from vMemCtor().
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpFileL Left file name.
 * \param cpFileR Right file name.
 * \return True if all lines are identical. False otherwise.
 */
abool bUtilCompareFileLines(void* vpMem, const char* cpFileL, const char* cpFileR){
    if(!bMemValidate(vpMem)){
        vExContext();
    }
    exception* spEx = spMemException(vpMem);
    if(!cpFileL || !cpFileR){
        XTHROW(spEx, "file names cannot be NULL");
    }
    abool bReturn = APG_FALSE;
    aint ui;
    line* spL;
    line* spR;
    void* vpLinesL = NULL;
    void* vpLinesR = NULL;
    uint8_t* ucpLeft = NULL;
    uint8_t* ucpRight = NULL;
    aint uiSize = 2048;
    aint uiSizeL = uiSize;
    aint uiSizeR = uiSize;
    ucpLeft = (uint8_t*)vpMemAlloc(vpMem, uiSizeL);
    vUtilFileRead(vpMem, cpFileL, ucpLeft, &uiSizeL);
    if(uiSizeL > uiSize){
        vMemFree(vpMem, ucpLeft);
        ucpLeft = (uint8_t*)vpMemAlloc(vpMem, uiSizeL);
        vUtilFileRead(vpMem, cpFileL, ucpLeft, &uiSizeL);
    }
    ucpRight = (uint8_t*)vpMemAlloc(vpMem, uiSizeR);
    vUtilFileRead(vpMem, cpFileR, ucpRight, &uiSizeR);
    if(uiSizeR > uiSize){
        vMemFree(vpMem, ucpRight);
        ucpRight = (uint8_t*)vpMemAlloc(vpMem, uiSizeR);
        vUtilFileRead(vpMem, cpFileR, ucpRight, &uiSizeR);
    }
    vpLinesL = vpLinesCtor(spEx, (char*)ucpLeft, uiSizeL);
    vpLinesR = vpLinesCtor(spEx, (char*)ucpRight, uiSizeR);
    if(uiLinesCount(vpLinesL) != uiLinesCount(vpLinesR)){
        goto fail;
    }
    spL = spLinesFirst(vpLinesL);
    spR = spLinesFirst(vpLinesR);
    while(spL){
        if(spL->uiTextLength != spR->uiTextLength){
            goto fail;
        }
        for(ui = 0; ui < spL->uiTextLength; ui++){
            if(ucpLeft[spL->uiCharIndex + ui] != ucpRight[spR->uiCharIndex + ui]){
                goto fail;
            }
        }
        spL = spLinesNext(vpLinesL);
        spR = spLinesNext(vpLinesR);
    }
    bReturn = APG_TRUE;
    fail:;
    vMemFree(vpMem, ucpLeft);
    vMemFree(vpMem, ucpRight);
    vLinesDtor(vpLinesL);
    vLinesDtor(vpLinesR);
    return bReturn;
}

/**
 * \brief Prints exception information from an exception structure.
 * \param  spEx Pointer to a valid exception structure.
 */
void vUtilPrintException(exception* spEx){
    if(bExValidate(spEx)){
        printf("%s:%s(%u):\n%s\n",
                spEx->caFile, spEx->caFunc, spEx->uiLine, spEx->caMsg);
    }else{
        printf("vUtilPrintException: not a valid exception pointer\n");
    }
}

/** \brief Display the memory object's statistics.
 * \param spStats Pointer to a memory statistics structure, returned from \ref vMemStats().
 */
void vUtilPrintMemStats(const mem_stats* spStats) {
    printf("allocations:        %"PRIuMAX"\n", (luint) (spStats->uiAllocations));
    printf("reallocations:      %"PRIuMAX"\n", (luint) (spStats->uiReAllocations));
    printf("frees:              %"PRIuMAX"\n", (luint) (spStats->uiFrees));
    printf("current cells:      %"PRIuMAX"\n", (luint) (spStats->uiCells));
    printf("max cells:          %"PRIuMAX"\n", (luint) (spStats->uiMaxCells));
    printf("current heap bytes: %"PRIuMAX"\n", (luint) (spStats->uiHeapBytes));
    printf("max heap bytes:     %"PRIuMAX"\n", (luint) (spStats->uiMaxHeapBytes));
}

/** \brief Display the vector object's statistics.
 * \param spStats Pointer to a vector statistics structure, returned from \ref vVecStats().
 */
void vUtilPrintVecStats(const vec_stats* spStats) {
    printf("ORIGINAL:\n");
    printf("    element size(bytes):    %"PRIuMAX"\n", (luint) (spStats->uiElementSize));
    printf("    reserved elements:      %"PRIuMAX"\n", (luint) (spStats->uiOriginalElements));
    printf("    reserved bytes:         %"PRIuMAX"\n", (luint) (spStats->uiOriginalBytes));
    printf("CURRENT:\n");
    printf("    reserved elements:      %"PRIuMAX"\n", (luint) (spStats->uiReserved));
    printf("    reserved bytes:         %"PRIuMAX"\n", (luint) (spStats->uiReservedBytes));
    printf("    used elements:          %"PRIuMAX"\n", (luint) (spStats->uiUsed));
    printf("    used bytes:             %"PRIuMAX"\n", (luint) (spStats->uiUsedBytes));
    printf("MAX:\n");
    printf("    max elements:           %"PRIuMAX"\n", (luint) (spStats->uiMaxUsed));
    printf("    max bytes:              %"PRIuMAX"\n", (luint) (spStats->uiMaxUsedBytes));
    printf("STATS:\n");
    printf("    pushed elements:        %"PRIuMAX"\n", (luint) (spStats->uiPushed));
    printf("    popped elements:        %"PRIuMAX"\n", (luint) (spStats->uiPopped));
    printf("    times grown:            %"PRIuMAX"\n", (luint) (spStats->uiGrownCount));
    printf("    elements grown:         %"PRIuMAX"\n", (luint) (spStats->uiGrownElements));
    printf("    bytes grown:            %"PRIuMAX"\n", (luint) (spStats->uiGrownBytes));
}

/** \brief Display one line from a line object.
 * \param spLine Pointer to a \ref line structure returned from spLinesFirst() or spLinesNext();
 */
void vUtilPrintLine(line* spLine){
    char caBuf[16];
    if(spLine){
        printf("line index: %d\n", (int)spLine->uiLineIndex);
        printf("char index: %d\n", (int)spLine->uiCharIndex);
        printf("line length: %d\n", (int)spLine->uiLineLength);
        printf("text length: %d\n", (int)spLine->uiTextLength);
        printf("line end[0]: %s\n", cpUtilPrintChar(spLine->caLineEnd[0], caBuf));
        if(spLine->caLineEnd[1]){
            printf("line end[1]: %s\n", cpUtilPrintChar(spLine->caLineEnd[1], caBuf));
        }
    }else{
        printf("print lines: NULL input\n");
    }
}

/** \brief Display one line from a line_u object.
 * \param spLine Pointer to a \ref line_u structure returned from spLinesuFirst() or spLinesuNext();
 */
void vUtilPrintLineu(line_u* spLine){
    char caBuf[16];
    if(spLine){
        printf("line index: %d\n", (int)spLine->uiLineIndex);
        printf("char index: %d\n", (int)spLine->uiCharIndex);
        printf("line length: %d\n", (int)spLine->uiLineLength);
        printf("text length: %d\n", (int)spLine->uiTextLength);
        printf("line end[0]: %s\n", cpUtilPrintUChar(spLine->uiaLineEnd[0], caBuf));
        if(spLine->uiaLineEnd[1]){
            printf("line end[1]: %s\n", cpUtilPrintUChar(spLine->uiaLineEnd[1], caBuf));
        }
    }else{
        printf("print lines: NULL input\n");
    }
}

/** \brief Generates a string representation for a single character.
 * \param cChar The character to represent
 * \param cpBuf Pointer to a character buffer that must be at least 5 characters in length
 * \return a pointer to cpBuf;
 */
char* cpUtilPrintChar(char cChar, char* cpBuf){
    if(cpBuf){
        while(APG_TRUE){
            if(cChar == 9){
                sprintf(cpBuf, "\\t");
                break;
            }
            if(cChar == 10){
                sprintf(cpBuf, "\\n");
                break;
            }
            if(cChar == 13){
                sprintf(cpBuf, "\\r");
                break;
            }
            if(cChar == 32){
                sprintf(cpBuf, "sp");
                break;
            }
            if(cChar >= 33 && cChar <= 126){
                sprintf(cpBuf, "%c", cChar);
                break;
            }
            sprintf(cpBuf, "0x%02X", (unsigned char)cChar);
            break;
        }
    }
    return cpBuf;
}

/** \brief Generates a string representation for a single Unicode character.
 * \param uiChar The character to represent
 * \param cpBuf Pointer to a character buffer that must be at least 11 characters in length
 * \return a pointer to cpBuf;
 */
char* cpUtilPrintUChar(uint32_t uiChar, char* cpBuf){
    if(cpBuf){
        while(APG_TRUE){
            if(uiChar == 9){
                sprintf(cpBuf, "TAB");
                break;
            }
            if(uiChar == 10){
                sprintf(cpBuf, "LF");
                break;
            }
            if(uiChar == 11){
                sprintf(cpBuf, "VT");
                break;
            }
            if(uiChar == 12){
                sprintf(cpBuf, "FF");
                break;
            }
            if(uiChar == 13){
                sprintf(cpBuf, "CR");
                break;
            }
            if(uiChar == 0x85){
                sprintf(cpBuf, "NEL");
                break;
            }
            if(uiChar == 0x2028){
                sprintf(cpBuf, "LS");
                break;
            }
            if(uiChar == 0x2029){
                sprintf(cpBuf, "PS");
                break;
            }
            if(uiChar == 32){
                sprintf(cpBuf, "sp");
                break;
            }
            if(uiChar >= 33 && uiChar <= 126){
                sprintf(cpBuf, "%c", uiChar);
                break;
            }
            if(uiChar < 0x100){
                sprintf(cpBuf, "0x%02X", uiChar);
            }else{
                sprintf(cpBuf, "0x%04X", uiChar);
            }
            break;
        }
    }
    return cpBuf;
}

/** \brief Convert a conversion type identifier to a readable, printable ASCII string.
 * Conversion type identifiers:
 *  - \ref BINARY
 *  - \ref UTF_8
 *  - \ref UTF_16
 *  - \ref UTF_16BE
 *  - \ref UTF_16LE
 *  - \ref UTF_32
 *  - \ref UTF_32BE
 *  - \ref UTF_32LE
 *  Any of which may be "ORed" with the base64 mask, \ref BASE64.
 * \param uiType The type to convert. May or may not include the base64 bit.
 * \return Pointer to the ASCII string representation.
 */
const char* cpUtilUtfTypeName(aint uiType){
    char* cpReturn;
    abool bBase64 = uiType & BASE64_MASK ? APG_TRUE : APG_FALSE;
    uiType &= TYPE_MASK;
    switch(uiType){
    case BINARY:
        cpReturn = bBase64 ? "(BINARY | BASE64)" : "BINARY";
        break;
    case UTF_8:
        cpReturn = bBase64 ? "(UTF-8 | BASE64)" : "UTF-8";
        break;
    case UTF_16:
        cpReturn = bBase64 ? "(UTF-16 | BASE64)" : "UTF-16";
        break;
    case UTF_16BE:
        cpReturn = bBase64 ? "(UTF-16BE | BASE64)" : "UTF-16BE";
        break;
    case UTF_16LE:
        cpReturn = bBase64 ? "(UTF-16LE | BASE64)" : "UTF-16LE";
        break;
    case UTF_32:
        cpReturn = bBase64 ? "(UTF-32 | BASE64)" : "UTF-32";
        break;
    case UTF_32BE:
        cpReturn = bBase64 ? "(UTF-32BE | BASE64)" : "UTF-32BE";
        break;
    case UTF_32LE:
        cpReturn = bBase64 ? "(UTF-32LE | BASE64)" : "UTF-32LE";
        break;
    default:
        cpReturn = "UNKNOWN";
    }
    return cpReturn;
}

/** \brief Return a human-readable string version of the given value in its true/false sense.
 * \param luiTrue The value to display.
 * \return Pointer to the true/false string.
 */
const char* cpUtilTrueFalse(luint luiTrue) {
    static char* cpTrue = "TRUE";
    static char* cpFalse = "FALSE";
    if(luiTrue){
        return cpTrue;
    }
    return cpFalse;
}

/** \brief Convert an opcode identifier to a human-readable opcode name.
 * \param uiId The opcode identifier to convert.
 * \return Pointer to the opcode name.
 */
const char* cpUtilOpName(aint uiId){
    switch(uiId){
    case ID_ALT:
        return "ALT";
    case ID_CAT:
        return "CAT";
    case ID_REP:
        return "REP";
    case ID_RNM:
        return "RNM";
    case ID_TLS:
        return "TLS";
    case ID_TBS:
        return "TBS";
    case ID_TRG:
        return "TRG";
    case ID_UDT:
        return "UDT";
    case ID_AND:
        return "AND";
    case ID_NOT:
        return "NOT";
    case ID_BKR:
        return "BKR";
    case ID_BKA:
        return "BKA";
    case ID_BKN:
        return "BKN";
    case ID_ABG:
        return "ABG";
    case ID_AEN:
        return "AEN";
    default:
        return "UNKNOWN";
    }
    return NULL;
}

/** \brief Convert the parser state identifier into a human-readable string.
 * \param uiState The parser state:
 *  - \ref ID_ACTIVE
 *  - \ref ID_MATCH
 *  - \ref ID_EMPTY
 *  - \ref ID_NOMATCH
 *  \return Pointer to the string.
 */
static char* cpUtilParserState(aint uiState){
    char* cpReturn = "UNKNOWN";
    switch(uiState){
    case ID_ACTIVE:
        cpReturn = "ACTIVE";
        break;
    case ID_MATCH:
        cpReturn = "MATCH";
        break;
    case ID_NOMATCH:
        cpReturn = "NOMATCH";
        break;
    case ID_EMPTY:
        cpReturn = "EMPTY";
        break;
    }
    return cpReturn;
}

/** \brief Display the parser state in human-readable format to stdout.
 * \param spState Pointer to the parser's state returned from vParserParse().
 */
void vUtilPrintParserState(parser_state* spState) {
    aint uiState;
    printf("  PARSER STATE:\n");
    printf("       success: %s\n", cpUtilTrueFalse((luint)spState->uiSuccess));
    uiState = spState->uiState;
    if ((uiState == ID_MATCH) && (spState->uiPhraseLength == 0)) {
        uiState = ID_EMPTY;
    }
    printf("         state: %s\n", cpUtilParserState(uiState));
//    printf("\n");
    printf(" phrase length: %"PRIuMAX"\n", (luint) spState->uiPhraseLength);
    printf("  input length: %"PRIuMAX"\n", (luint) spState->uiStringLength);
    printf("max tree depth: %"PRIuMAX"\n", (luint) spState->uiMaxTreeDepth);
    printf("     hit count: %"PRIuMAX"\n", (luint) spState->uiHitCount);
}

/**  \brief Display the list of messages in a message object to stdout.
 * \param vpMsgs Pointer to a valid message log object. Previously returned from vpMsgsCtor().
 * If not valid application will silently exit with a \ref BAD_CONTEXT exit code.
 */
void vUtilPrintMsgs(void* vpMsgs){
    if(!bMsgsValidate(vpMsgs)){
        vExContext();
    }
    const char* cpMsg = cpMsgsFirst(vpMsgs);
    while(cpMsg){
        printf("%s\n", cpMsg);
        cpMsg = cpMsgsNext(vpMsgs);
    }
}

void vPrintPPPTMap(uint8_t* ucpMap, aint uiBegin, aint uiLength, const char* cpMode){
    void (*pfnDisplay)(uint8_t) = vDisplayBinary;
    if(cpMode && (cpMode[0] == 'd' || cpMode[0] == 'D')){
        pfnDisplay = vDisplayDecimal;
    }
    aint ui;
    for(ui = 0; ui < uiLength; ui++){
        pfnDisplay(ucpMap[ui + uiBegin]);
    }
    printf("\n");
}

/** \brief Indent by adding the given number of spaces to the output file.
 * \param spFile The open file handle to print to. If NULL, stdout will be used.
 * \param uiIndent The number of spaces to indent.
 */
void vUtilIndent(FILE* spFile, aint uiIndent){
    if(!spFile){
        spFile = stdout;
    }
    if (uiIndent) {
        while (uiIndent--) {
            fprintf(spFile, " ");
        }
    }
}

/** \brief Convert a string of alphabet characters to printable ASCII.
 * \param spFile The open file handle to print to. If NULL, stdout will be used.
 * \param acpChars The string of alphabet characters to interpret.
 * \param uiLength The number of alphabet characters in the string.
 */
void vUtilCharsToAscii(FILE* spFile, const achar* acpChars, aint uiLength) {
    if (uiLength) {
        aint ui;
        for (ui = 0; ui < uiLength; ui += 1) {
            achar aChar = acpChars[ui];
            if (aChar == 38) {
                // escape '&'
                fprintf(spFile, "&#38;");
            }else if(aChar == 60){
                // escape '<'
                fprintf(spFile, "&#60;");
            }else if (aChar >= 32 && aChar <= 126) {
                fprintf(spFile, "%c", (char) aChar);
            } else {
                fprintf(spFile, "&%"PRIuMAX";", (luint) aChar);
            }
        }
    }
}

/** \brief Convert a null-terminated ASCII string to an array of achar characters.
 *
 * This function will allocate memory for the array, relieving the caller of the allocation burden.
 * The allocated memory may be freed with vMemFree( acpBuf ), where acpBuf is the returned pointer.
 * However, the allocated memory will also be freed with vMemDtor() which every application should call
 * for memory clean up anyway.
 *
 * \param vpMem Pointer to a memory context returned from vpMemCtor();
 * \param cpStr Pointer to the null-terminated string to convert.
 * \param uipLen Pointer to an unsigned integer to receive the number of characters in the converted achar array.
 * \return Returns a pointer to the converted achar array.
 * Returns the number of characters in the array if uipLen != NULL.
 *
 */
achar* acpUtilStrToAchar(void* vpMem, const char* cpStr, aint* uipLen){
    if(!bMemValidate(vpMem)){
        vExContext();
    }
    if(!cpStr){
        XTHROW(spMemException(vpMem), "cpStr may not be NULL");
    }
    aint uiStrLen = (aint)strlen(cpStr);
    achar* acpBuf = (achar*)vpMemAlloc(vpMem, ((aint)sizeof(achar) * uiStrLen));
    aint ui = 0;
    for(; ui < uiStrLen; ui++){
        acpBuf[ui] = (achar)((uint8_t)cpStr[ui]);
    }
    if(uipLen){
        *uipLen = uiStrLen;
    }
    return acpBuf;
}

/** \brief Convert an array of achar characters to a null-terminated ASCII string.
 *
 * Any achar characters that are not valid printing ASCII characters will be represented with a period ('.' or 46).
 *
 * This function will allocate memory for the string, relieving the caller of the allocation burden.
 * The allocated memory may be freed with vMemFree(cpStr), where cpStr is the returned pointer.
 * However, the allocated memory will also be freed with vMemDtor() which every application should call
 * for memory clean up anyway.

 * \param vpMem Pointer to a memory context returned from vpMemCtor();
 * \param acpAchar Pointer to an achar array to convert. May not be NULL.
 * \param uiLen The number of characters in the acpAchar array.
 * \return Returns a pointer to the converted, null-terminated string.
 */
const char* cpUtilAcharToStr(void* vpMem, achar* acpAchar, aint uiLen){
    if(!bMemValidate(vpMem)){
        vExContext();
    }
    if(!acpAchar){
        XTHROW(spMemException(vpMem), "acpAchar may not be NULL");
    }
    char* cpStr = (char*)vpMemAlloc(vpMem, ((aint)sizeof(char) * (uiLen + 1)));
    aint ui = 0;
    achar acChar;
    for(; ui < uiLen; ui++){
        acChar = acpAchar[ui];
        if(acChar == 9 || acChar == 10 || acChar == 13 || (acChar >= 32 && acChar <= 126)){
            cpStr[ui] = (char)acChar;
        }else{
            cpStr[ui] = s_cPeriod;
        }
    }
    cpStr[ui] = (char)0;
    return cpStr;
}

/** \brief Convert a null-terminated ASCII string to an apg_phrase.
 *
 * This function will allocate memory for the apg_phrase, relieving the caller of the allocation burden.
 * For a simpler solution with less overhead but with risk of buffer overrun, see acpStrToPhrase() in tools.c.
 * The allocated memory may be freed with vMemFree( spPhrase ), where spPhrase is the returned pointer.
 * However, the allocated memory will also be freed with vMemDtor() which every application should call
 * for memory clean up anyway.
 *
 * \param vpMem Pointer to a memory context returned from vpMemCtor();
 * \param cpStr Pointer to the null-terminated string to convert.
 * \return Returns a pointer to the converted apg_phrase.
 */
apg_phrase* spUtilStrToPhrase(void* vpMem, const char* cpStr){
    if(!bMemValidate(vpMem)){
        vExContext();
    }

    if(!cpStr){
        XTHROW(spMemException(vpMem), "cpStr may not be NULL");
    }
    size_t uiStrLen = strlen(cpStr);
    apg_phrase* spPhrase = (apg_phrase*)vpMemAlloc(vpMem, (aint)(sizeof(apg_phrase) + (sizeof(achar) * uiStrLen)));
    achar* acpBuf = (achar*)&spPhrase[1];
    size_t ui = 0;
    for(; ui < uiStrLen; ui++){
        acpBuf[ui] = (achar)((uint8_t)cpStr[ui]);
    }
    spPhrase->acpPhrase = acpBuf;
    spPhrase->uiLength = (aint)uiStrLen;
    return spPhrase;
}

/** \brief Convert an apg_phrase to a null-terminated ASCII string.
 *
 * Any apg_phrase characters that are not valid printing ASCII characters will be represented with a period ('.' or 46).
 *
 * This function will allocate memory for the string, relieving the caller of the allocation burden.
 * For a simpler solution with less overhead but with risk of buffer overrun, see cpPhraseToStr() in tools.c.
 * The allocated memory may be freed with vMemFree(cpStr), where cpStr is the returned pointer.
 * However, the allocated memory will also be freed with vMemDtor() which every application should call
 * for memory clean up anyway.

 * \param vpMem Pointer to a memory context returned from vpMemCtor();
 * \param spPhrase Pointer to the apg_phrase to convert. May not be NULL.
 * \return Returns a pointer to the converted, null-terminated string.
 */
const char* cpUtilPhraseToStr(void* vpMem, apg_phrase* spPhrase){
    if(!bMemValidate(vpMem)){
        vExContext();
    }

    if(!spPhrase){
        XTHROW(spMemException(vpMem), "spPhrase may not be NULL");
    }
    char* cpStr = (char*)vpMemAlloc(vpMem, ((aint)sizeof(char) * (spPhrase->uiLength + 1)));
    aint ui = 0;
    achar acChar;
    for(; ui < spPhrase->uiLength; ui++){
        acChar = spPhrase->acpPhrase[ui];
        if(acChar == 9 || acChar == 10 || acChar == 13 || (acChar >= 32 && acChar <= 126)){
            cpStr[ui] = (char)acChar;
        }else{
            cpStr[ui] = s_cPeriod;
        }
    }
    cpStr[ui] = (char)0;
    return cpStr;
}

/** \brief Convert a null-terminated ASCII string to an array of 32-bit unsigned integers.
 *
 * This function will allocate memory for the array, relieving the caller of the allocation burden.
 * For a simpler solution with less overhead but with risk of buffer overrun, see uipStrToUint32() in tools.c.
 * The allocated memory may be freed with vMemFree(acpBuf), where acpBuf is the returned pointer.
 * However, the allocated memory will also be freed with vMemDtor() which every application should call
 * for memory clean up anyway.
 *
 * \param vpMem Pointer to a memory context returned from vpMemCtor();
 * \param cpStr Pointer to the null-terminated string to convert.
 * \param uipLen Pointer to an unsigned integer to receive the number of characters in the converted achar array.
 * \return Returns a pointer to the converted 32-bit unsigned integer array.
 *  * Returns the number of integers in the array if uipLen != NULL.
 *
 */
uint32_t* uipUtilStrToUint32(void* vpMem, const char* cpStr, aint* uipLen){
    if(!bMemValidate(vpMem)){
        vExContext();
    }
    if(!cpStr){
        XTHROW(spMemException(vpMem), "cpStr may not be NULL");
    }
    aint uiStrLen = (aint)strlen(cpStr);
    uint32_t* uipBuf = (uint32_t*)vpMemAlloc(vpMem, ((aint)sizeof(uint32_t) * uiStrLen));
    aint ui = 0;
    for(; ui < uiStrLen; ui++){
        uipBuf[ui] = (uint32_t)((uint8_t)cpStr[ui]);
    }
    if(uipLen){
        *uipLen = uiStrLen;
    }
    return uipBuf;
}

/** \brief Convert an array of 32-bit unsigned integers to a null-terminated ASCII string.
 *
 * Any 32-bit unsigned integers that are not valid printing ASCII characters will be represented with a period ('.' or 46).
 *
 * This function will allocate memory for the string, relieving the caller of the allocation burden.
 * For a simpler solution with less overhead but with risk of buffer overrun, see cpUint32ToStr() in tools.c.
 * The allocated memory may be freed with vMemFree(cpStr), where cpStr is the returned pointer.
 * However, the allocated memory will also be freed with vMemDtor() which every application should call
 * for memory clean up anyway.

 * \param vpMem Pointer to a memory context returned from vpMemCtor();
 * \param uipUint Pointer to a 32-bit unsigned integer array to convert. May not be NULL.
 * \param uiLen The number of characters in the uipUint array.
 * \return Returns a pointer to the converted, null-terminated string.
 */
const char* cpUtilUint32ToStr(void* vpMem, const uint32_t* uipUint, aint uiLen){
    if(!bMemValidate(vpMem)){
        vExContext();
    }
    if(!uipUint){
        XTHROW(spMemException(vpMem), "acpAchar may not be NULL");
    }
    char* cpStr = (char*)vpMemAlloc(vpMem, ((aint)sizeof(char) * (uiLen + 1)));
    aint ui = 0;
    uint32_t uiChar;
    for(; ui < uiLen; ui++){
        uiChar = uipUint[ui];
        if(uiChar == 9 || uiChar == 10 || uiChar == 13 || (uiChar >= 32 && uiChar <= 126)){
            cpStr[ui] = (char)uiChar;
        }else{
            cpStr[ui] = s_cPeriod;
        }
    }
    cpStr[ui] = (char)0;
    return cpStr;
}

/** \brief Convert a null-terminated ASCII string to a 32-bit phrase.
 *
 * This function will allocate memory for the phrase, relieving the caller of the allocation burden.
 * For a simpler solution with less overhead but with risk of buffer overrun, see acpStrToPhrase32() in tools.c.
 * The allocated memory may be freed with vMemFree( spPhrase ), where spPhrase is the returned pointer.
 * However, the allocated memory will also be freed with vMemDtor() which every application should call
 * for memory clean up anyway.
 *
 * \param vpMem Pointer to a memory context returned from vpMemCtor();
 * \param cpStr Pointer to the null-terminated string to convert.
 * \return Returns a pointer to the converted 32-bit phrase.
 */
u32_phrase* spUtilStrToPhrase32(void* vpMem, const char* cpStr){
    if(!bMemValidate(vpMem)){
        vExContext();
    }

    if(!cpStr){
        XTHROW(spMemException(vpMem), "cpStr may not be NULL");
    }
    size_t uiStrLen = strlen(cpStr);
    u32_phrase* spPhrase = (u32_phrase*)vpMemAlloc(vpMem, (aint)(sizeof(u32_phrase) + (sizeof(uint32_t) * uiStrLen)));
    uint32_t* acpBuf = (uint32_t*)&spPhrase[1];
    size_t ui = 0;
    for(; ui < uiStrLen; ui++){
        acpBuf[ui] = (uint32_t)((uint8_t)cpStr[ui]);
    }
    spPhrase->uipPhrase = acpBuf;
    spPhrase->uiLength = (aint)uiStrLen;
    return spPhrase;
}

/** \brief Convert an u32_phrase to a null-terminated ASCII string.
 *
 * Any u32_phrase characters that are not valid printing ASCII characters will be represented with a period ('.' or 46).
 *
 * This function will allocate memory for the string, relieving the caller of the allocation burden.
 * For a simpler solution with less overhead but with risk of buffer overrun, see cpPhraseToStr() in tools.c.
 * The allocated memory may be freed with vMemFree(cpStr), where cpStr is the returned pointer.
 * However, the allocated memory will also be freed with vMemDtor() which every application should call
 * for memory clean up anyway.

 * \param vpMem Pointer to a memory context returned from vpMemCtor();
 * \param spPhrase Pointer to the u32_phrase to convert. May not be NULL.
 * \return Returns a pointer to the converted, null-terminated string.
 */
const char* cpUtilPhrase32ToStr(void* vpMem, u32_phrase* spPhrase){
    if(!bMemValidate(vpMem)){
        vExContext();
    }

    if(!spPhrase){
        XTHROW(spMemException(vpMem), "spPhrase may not be NULL");
    }
    char* cpStr = (char*)vpMemAlloc(vpMem, ((aint)sizeof(char) * (spPhrase->uiLength + 1)));
    uint32_t ui = 0;
    uint32_t uiChar;
    for(; ui < spPhrase->uiLength; ui++){
        uiChar = spPhrase->uipPhrase[ui];
        if(uiChar == 9 || uiChar == 10 || uiChar == 13 || (uiChar >= 32 && uiChar <= 126)){
            cpStr[ui] = (char)uiChar;
        }else{
            cpStr[ui] = s_cPeriod;
        }
    }
    cpStr[ui] = (char)0;
    return cpStr;
}

/** \brief Convert the AST to XML.
 *
 * The Abstract Syntax Tree (AST) is converted to XML format.
 * The root node, <\_root\_>, is the parent XML node.
 * Since the grammar's start rule may not be included on the AST, there is no guarantee that the first node
 * of the AST will include all other nodes as children as required by XML.
 * A single string node, <\_string\_>, defines the full input string that was parsed.
 * All of the other matched phrases are defined as sub-strings of the full input string with an offset/length pair.
 * \param vpAst Pointer to the AST object context, returned from \ref vpAstCtor().
 * \param cpType Refers to the format for the display of the input string characters.
 *  - "decimal" (or simply "d" or "D") - the string is a comma- and white space-delimited array of decimal integers
 *  - "hexadecimal" (or simply "h" or "H") - the string is a comma- and white space-delimited array of hexadecimal integers
 *  - "Unicode" (or simply "u" or "U") - the string is UTF-8-encoded XML Unicode<br>
 *    Note that XML does not allow the following characters and an error is generated if present:
 *    - control except TAB (0x09), LF (0x0A), CR (0x0D) and DEL (0x7F)
 *    - 0xFFFE and 0xFFFF
 *    - surrogate-pair range 0xD800 - 0xDFFF
 *    - beyond Unicode range > 0x10FFFF
 * \param cpFileName Name of the file to write the XML to. If NULL, stdout is used.
 * \return True on success, false if an error is detected.
 */
#ifdef APG_AST
static char* s_cpStringNode = "_string_";
static abool bAstValidXmlChar(luint luiChar);
static void vAstDecimalString(FILE* spOut, abool bHex, const achar* acpString, aint uiLen);
static void vAstUnicodeString(void* vpMem, FILE* spOut, const achar* acpString, aint uiLen);
static void vAstMaxChar(const achar* acpString, aint uiLen, luint* uipMaxChar, aint* uipSizeof);
/** \brief Convert the AST records to XML format.
 * \param vpAst Pointer to a valid AST context previously returned from vpAstCtor().
 * If not valid application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpType Select the character type for the parsed phrases:
 * - "u" or "U" for Unicode
 * - "h" or "H" for hexadecimal characters
 * - "d" or "D" for decimal characters
 * - default (NULL or none of the above) decimal characters
 * \param cpFileName Name of the file to convert to.
 * If NULL, stdout is used.
 */
abool bUtilAstToXml(void* vpAst, char* cpType, const char* cpFileName){
    abool bReturn = APG_SUCCESS;
    ast_info sInfo;
    void* vpMem = NULL;
    static FILE* spOut = NULL;
    char caBuf[s_uiBufSize];
    char* cpRoot = "_root_";
    exception e;
    XCTOR(e);
    if(e.try){
        vpMem = vpMemCtor(&e);
        if(cpFileName){
            spOut = fopen(cpFileName, "wb");
            if(!spOut){
                snprintf(caBuf, s_uiBufSize, "can't open file %s for writing", cpFileName);
                XTHROW(&e, caBuf);
            }
        }else{
            spOut = stdout;
        }

        // get the AST
        vAstInfo(vpAst, &sInfo);

        // the XML declaration
        fprintf(spOut, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

        // the root node and descriptive comments
        aint uiIndent = 0;
        fprintf(spOut, "<%s>\n\n", cpRoot);
        fprintf(spOut, "<!-- The <%s> node contains the character codes of the full input string. Its attributes are:\n", s_cpStringNode);
        fprintf(spOut, "     length   - the number of characters in the string\n");
        fprintf(spOut, "     max-char - the maximum character size\n");
        fprintf(spOut, "     sizeof   - the number of bytes in the maximum character\n");
        fprintf(spOut, " -->\n");

        // the input string
        if(cpType == NULL){
            vAstDecimalString(spOut, APG_FALSE, sInfo.acpString, sInfo.uiStringLength);
        }else if(cpType[0] == 'u' || cpType[0] == 'U'){
            vAstUnicodeString(vpMem, spOut, sInfo.acpString, sInfo.uiStringLength);
        }else if(cpType[0] == 'h' || cpType[0] == 'H'){
            vAstDecimalString(spOut, APG_TRUE, sInfo.acpString, sInfo.uiStringLength);
        }else{
            vAstDecimalString(spOut, APG_FALSE, sInfo.acpString, sInfo.uiStringLength);
        }
        fprintf(spOut, "\n<!-- The <rule> node attributes define each rule/UDT the corresponding matched substring phrase.\n");
        fprintf(spOut, "     name   - the name of the rule or UDT\n");
        fprintf(spOut, "     index  - the grammar index of the rule or UDT\n");
        fprintf(spOut, "     udt    - (optional), if \"true\", name refers to a UDT, if \"false\" or absent, name refers to a rule\n");
        fprintf(spOut, "     offset - the offset to the first character in the input string of the matched phrase\n");
        fprintf(spOut, "     length - the number of characters in the matched phrase (may be \"0\" for a matched EMPTY phrase)\n");
        fprintf(spOut, " -->\n");

        ast_record* spRec = sInfo.spRecords;
        ast_record* spEnd = spRec + sInfo.uiRecordCount;
        for(; spRec < spEnd; spRec++){
            if(spRec->uiState == ID_AST_PRE){
                uiIndent += 2;
                vUtilIndent(spOut, uiIndent);
                fprintf(spOut, "<rule name=\"%s\" index=\"%"PRIuMAX"\"", spRec->cpName, (luint)spRec->uiIndex);
                if(spRec->bIsUdt){
                    fprintf(spOut, " udt=\"true\"");
                }
                fprintf(spOut, " offset=\"%"PRIuMAX"\" length=\"%"PRIuMAX"\">\n",
                        (luint)spRec->uiPhraseOffset, (luint)spRec->uiPhraseLength);
            }else{
                vUtilIndent(spOut, uiIndent);
                fprintf(spOut, "</rule>\n");
                uiIndent -= 2;
            }
        }

        // close the root node
        fprintf(spOut, "</%s>\n", cpRoot);
    }else{
        vUtilPrintException(&e);
        bReturn = APG_FAILURE;
    }
    if(spOut && (spOut != stdout)){
        fclose(spOut);
    }
    vMemDtor(vpMem);
    return bReturn;
}

/** \brief Convert all line ending characters.
 *
 * Line ending are \\r\\n, \\n or \\r (CRLF, LF or CR) (0x0D0A, 0x0A or 0x0D).
 * This function will replace any of these line end forms with the `cpEnd` string.
 * The `cpEnd` string will also end the last line, even if the input string has no final line ending.
 *
 * \param spEx Pointer to a valid exception structure.
 * If not valid application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpString Pointer to a null-terminated string whose line ends to translate. May not be NULL or empty.
 * \param cpEnd Pointer to a null-terminated string of characters to add to the end of each line.
 * NOTE: Normally this will be one of the common line ending strings, "\\r\\n", "\\n" or "\\r".
 * However, there is no restriction here. Any string can be used.
 * If NULL or empty, all line endings in the input string will be removed.
 * \param cpFileName Name of the converted file.
 * If NULL, stdout is used.
 */
void vUtilConvertLineEnds(exception* spEx, const char* cpString, const char* cpEnd, const char* cpFileName) {
    if(!bExValidate(spEx)){
        vExContext();
    }
    line* spLine;
    aint uiEndLen = 0;
    FILE* spFile = stdout;
    void* vpLines = NULL;
    if (!cpString) {
            XTHROW(spEx, "input string cannot be NULL");
    }
    aint uiLen = (aint)strlen(cpString);
    if (!uiLen) {
            XTHROW(spEx, "input string cannot be empty");
    }
    if (cpFileName) {
        spFile = fopen(cpFileName, "wb");
        if (!spFile) {
            char caBuf[s_uiBufSize];
            snprintf(caBuf, s_uiBufSize, "unable to open file name '%s' for writing", cpFileName);
            XTHROW(spEx, caBuf);
        }
    }
    if(cpEnd){
        uiEndLen = (aint) strlen(cpEnd);
    }
    vpLines = vpLinesCtor(spEx, cpString, uiLen);
    spLine = spLinesFirst(vpLines);
    while(spLine) {
        fwrite(&cpString[spLine->uiCharIndex], sizeof(char), spLine->uiTextLength, spFile);
        if(uiEndLen){
            fwrite(cpEnd, sizeof(char), uiEndLen, spFile);
        }
        spLine = spLinesNext(vpLines);
    }
    vLinesDtor(vpLines);
    if (spFile && (spFile != stdout)) {
        fclose(spFile);
    }
}

// //////////////////////////////////////////////////////////
// STATIC HELPER FUNCTIONS
// //////////////////////////////////////////////////////////
static void vAstDecimalString(FILE* spOut, abool bHex, const achar* acpString, aint uiLen){
    aint uiCharsPerLine = 10;
    aint ui = 0;
    aint uiChar = 0;
    luint luiMaxChar;
    aint uiSizeof;
    aint uiEnd = uiLen - 1;
    char* cpFmt;
    if(bHex){
        cpFmt = "0x%"PRIXMAX"";
        fprintf(spOut, "<!-- The character codes are represented as comma- and white space-delimited hexadecimal integers. -->\n");
    }else{
        cpFmt = "%"PRIuMAX"";
        fprintf(spOut, "<!-- The character codes are represented as comma- and white space-delimited decimal integers. -->\n");

    }
    vAstMaxChar(acpString, uiLen, &luiMaxChar, &uiSizeof);
    fprintf(spOut, "<%s length=\"%"PRIuMAX"\" max-char=\"%"PRIuMAX"\" sizeof=\"%"PRIuMAX"\">",
            s_cpStringNode, (luint)uiLen, luiMaxChar, (luint)uiSizeof);
    for(; ui < uiLen; ui++){
        fprintf(spOut, cpFmt, (luint)acpString[ui]);
        if(ui < uiEnd){
            fprintf(spOut, ",");
        }
        uiChar++;
        if(uiChar == uiCharsPerLine){
            uiChar = 0;
            fprintf(spOut, "\n");
        }
    }
    if(uiChar != 0){
        fprintf(spOut, "\n");
    }
    fprintf(spOut, "</%s>\n", s_cpStringNode);
}
static void vAstUnicodeString(void* vpMem, FILE* spOut, const achar* acpString, aint uiLen){
    char caBuf[s_uiBufSize];
    fprintf(spOut, "<!-- The character codes are represented as a UTF-8-encoded XML Unicode string.\n");
    fprintf(spOut, "     Note that XML Unicode does not allow the following characters:\n");
    fprintf(spOut, "     - control except TAB(0x09), LF(0x0A), CR(0x0D) and DEL(0x7F)\n");
    fprintf(spOut, "     - 0xFFFE and 0xFFFF\n");
    fprintf(spOut, "     - surrogate-pair range 0xD800 - 0xDFFF\n");
    fprintf(spOut, "     - beyond Unicode range > 0x10FFFF\n");
    fprintf(spOut, " -->\n");
    luint luiMaxChar;
    aint uiSizeof;
    vAstMaxChar(acpString, uiLen, &luiMaxChar, &uiSizeof);
    fprintf(spOut, "<%s length=\"%"PRIuMAX"\" max-char=\"%"PRIuMAX"\" sizeof=\"%"PRIuMAX"\">",
            s_cpStringNode, (luint)uiLen, luiMaxChar, (luint)uiSizeof);
    uint32_t* uipIn32 = (uint32_t*)vpMemAlloc(vpMem, (uiLen * sizeof(uint32_t)));
    aint ui = 0;
    for(; ui < uiLen; ui++){
        if(!bAstValidXmlChar((luint)acpString[ui])){
            snprintf(caBuf, s_uiBufSize, "input string has invalid XML character: offset = %"PRIuMAX": character = %"PRIuMAX"",
                    (luint)ui, (luint)acpString[ui]);
            vMemFree(vpMem, uipIn32);
            XTHROW(spMemException(vpMem), caBuf);
        }
        uipIn32[ui] = (uint32_t)acpString[ui];
    }
    conv_dst sDst = {};
    sDst.uiDataType = UTF_8;
    sDst.bBOM = APG_FALSE;
    void* vpConv = vpConvCtor(spMemException(vpMem));
    vConvUseCodePoints(vpConv, uipIn32, uiLen);
    vConvEncode(vpConv, &sDst);
    fwrite(sDst.ucpData, sizeof(uint8_t), sDst.uiDataLen, spOut);
    fprintf(spOut, "</%s>\n", s_cpStringNode);
    vConvDtor(vpConv);
    vMemFree(vpMem, uipIn32);
}
// /////////////////////////////////////////////////////////////
// STATIC HELPERS
// /////////////////////////////////////////////////////////////

static abool bAstValidXmlChar(luint luiChar){
    abool bReturn = APG_FALSE;
    while(APG_TRUE){
        if(luiChar >= 0 && luiChar < 9){
            break;
        }
        if(luiChar >= 11 && luiChar < 13){
            break;
        }
        if(luiChar >= 14 && luiChar < 32){
            break;
        }
        if(luiChar >= 0xD800 && luiChar < 0xE000){
            break;
        }
        if(luiChar > 0x10FFFF){
            break;
        }
        bReturn = APG_TRUE;
        break;
    }
    return bReturn;
}
static void vAstMaxChar(const achar* acpString, aint uiLen, luint* uipMaxChar, aint* uipSizeof){
    aint ui = 0;
    luint luiChar = 0;
    for(; ui < uiLen; ui++){
        if((luint)acpString[ui] > luiChar){
            luiChar = (luint)acpString[ui];
        }
    }
    *uipMaxChar = luiChar;
    if(luiChar <= 0xFF){
        *uipSizeof = 1;
        return;
    }
    if(luiChar <= 0xFFFF){
        *uipSizeof = 2;
        return;
    }
    if(luiChar <= 0xFFFFFFFF){
        *uipSizeof = 4;
        return;
    }
    if(luiChar <= 0xFFFFFFFFFFFFFFFF){
        *uipSizeof = 4;
        return;
    }

}
#endif
static void vDisplayBinary(uint8_t ucChar){
    uint8_t ucL, ucR;
    ucR = ucChar & 0x0F;
    ucL = (ucChar & 0xF0) >> 4;
    printf("%s %s ", s_cpBinaryVal[ucL], s_cpBinaryVal[ucR]);
}
static void vDisplayDecimal(uint8_t ucChar){
    uint8_t ucL, ucR;
    ucR = ucChar & 0x0F;
    ucL = (ucChar & 0xF0) >> 4;
    printf("%s %s ", s_cpDecimalVal[ucL], s_cpDecimalVal[ucR]);
}
