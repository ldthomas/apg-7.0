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
/** \file input.c
 * \brief The API input functions. These functions assist with retrieving the ABNF grammar or grammars for processing.
 */

#include "./api.h"
#include "./apip.h"
#include "./attributes.h"


#undef CR
#undef LF
#undef TAB
#define CR 13
#define LF 10
#define TAB 9
#define BUF_SIZE 128

static void vPushInvalidChar(api* spCtx, aint uiCharIndex, uint8_t ucChar, const char* cpMsg);

static const char* s_cpMsgInvalid =
        "valid ABNF characters are, 0x09, 0x0A, 0x0D and 0x20-7E only";
static const char* s_cpMsgCRLF = "invalid line ending - must be CRLF (\\r\\n, 0x0D0A) - strict specified";
static const char* s_cpMsgEOF = "invalid line ending - last line has no line ending";
static char s_cZero = 0;

/** \brief Clears the input and related memory.
 *
 * The user must call this to clear any previous input grammar before reusing the API object for another job.
 *
 * \param[in] vpCtx - Pointer to an API context previously returned from vpApiCtor().
 */
void vApiInClear(void* vpCtx) {
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spCtx = (api*) vpCtx;
    vMsgsClear(spCtx->vpLog);
    vVecClear(spCtx->vpAltStack);
    vVecClear(spCtx->vpVecInput);
    vLinesDtor(spCtx->vpLines);
    vAttrsDtor(spCtx->vpAttrsCtx);
    vParserDtor(spCtx->vpParser);
    vMemFree(spCtx->vpMem, spCtx->spOpcodes);
    spCtx->spOpcodes = NULL;
    vMemFree(spCtx->vpMem, spCtx->spRules);
    spCtx->spRules = NULL;
    vMemFree(spCtx->vpMem, spCtx->spUdts);
    spCtx->spUdts = NULL;
    spCtx->uiRuleCount = 0;
    spCtx->uiUdtCount = 0;
    vMemFree(spCtx->vpMem, spCtx->luipAcharTable);
    spCtx->luipAcharTable = NULL;
    spCtx->uiAcharTableLength = 0;
    vMemFree(spCtx->vpMem, spCtx->uipChildIndexTable);
    spCtx->uipChildIndexTable = NULL;
    vMemFree(spCtx->vpMem, spCtx->ucpPpptTable);
    spCtx->ucpPpptTable = NULL;
    vMemFree(spCtx->vpMem, spCtx->vpOutputAcharTable);
    spCtx->vpOutputAcharTable = NULL;
    vMemFree(spCtx->vpMem, spCtx->vpOutputParserInit);
    spCtx->vpOutputParserInit = NULL;
    vMemFree(spCtx->vpMem, spCtx->ucpPpptUndecidedMap);
    spCtx->ucpPpptUndecidedMap = NULL;
    vMemFree(spCtx->vpMem, spCtx->ucpPpptEmptyMap);
    spCtx->ucpPpptEmptyMap = NULL;
    spCtx->uiChildIndexTableLength = 0;
    vMemFree(spCtx->vpMem, spCtx->cpStringTable);
    spCtx->cpStringTable = NULL;
    spCtx->vpLines = NULL;
    spCtx->cpInput = (char*)vpVecPush(spCtx->vpVecInput, &s_cZero);
    spCtx->uiInputLength = 0;
    spCtx->bAttributesValid = APG_FALSE;
    spCtx->bInputValid = APG_FALSE;
    spCtx->bSyntaxValid = APG_FALSE;
    spCtx->bSemanticsValid = APG_FALSE;
    spCtx->bUsePppt = APG_FALSE;
}

/** \brief Reads an SABNF grammar byte stream from a file.
 *
 * May be called multiple times.
 * Successive calls will append data to the previous grammar result.
 * May be mixed with calls to cpApiInString().
 * \param[in] vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param[in] cpFileName - Name of the file to read.
 * \return Pointer to the cumulative, null-terminated SABNF grammar string.
 */
const char* cpApiInFile(void* vpCtx, const char* cpFileName) {
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spCtx = (api*) vpCtx;
    char caBuf[BUF_SIZE];
    FILE* spFile = NULL;
    aint uiRead;
    char caReadBuf[1024];
    size_t iBufSize = 1024;
    vMsgsClear(spCtx->vpLog);
    spCtx->bAttributesValid = APG_FALSE;
    spCtx->bInputValid = APG_FALSE;
    spCtx->bSyntaxValid = APG_FALSE;
    spCtx->bSemanticsValid = APG_FALSE;
    if (!cpFileName) {
        XTHROW(spCtx->spException, "file name pointer cannot be NULL");
    }
    // open the file
    spFile = fopen(cpFileName, "rb");
    if (!spFile) {
        snprintf(caBuf, BUF_SIZE, "unable to open file name %s for reading", cpFileName);
        XTHROW(spCtx->spException, caBuf);
    }
    // pop the null-term
    vpVecPop(spCtx->vpVecInput);
    while((uiRead = (aint)fread(caReadBuf, sizeof(char), iBufSize, spFile)) > 0){
        vpVecPushn(spCtx->vpVecInput, caReadBuf, uiRead);
    }
    fclose(spFile);
    // push the null-term
    spCtx->uiInputLength = uiVecLen(spCtx->vpVecInput);
    vpVecPush(spCtx->vpVecInput, &s_cZero);
    spCtx->cpInput = (char*) vpVecFirst(spCtx->vpVecInput);
    vLinesDtor(spCtx->vpLines);
    spCtx->vpLines = vpLinesCtor(spCtx->spException, spCtx->cpInput, spCtx->uiInputLength);
    return spCtx->cpInput;
}

/** \brief Reads an SABNF grammar byte stream from a string.
 *
 * May be called multiple times.
 * Successive calls will append data to the previous SABNF grammar result.
 * May be mixed with calls to cpApiInFile().
 * \param[in] vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param[in] cpString - Pointer to the string to read.
 * \return Pointer to the cumulative, null-terminated SABNF grammar string.
 */
const char* cpApiInString(void* vpCtx, const char* cpString) {
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spCtx = (api*) vpCtx;
    vMsgsClear(spCtx->vpLog);
    spCtx->bAttributesValid = APG_FALSE;
    spCtx->bInputValid = APG_FALSE;
    spCtx->bSyntaxValid = APG_FALSE;
    spCtx->bSemanticsValid = APG_FALSE;
    aint uiLen;
    if (!cpString) {
        XTHROW(spCtx->spException, "input string cannot be NULL");
    }
    uiLen = (aint)strlen(cpString);
    if (!uiLen) {
        XTHROW(spCtx->spException, "input string cannot be empty");
    }
    // pop the null-term
    vpVecPop(spCtx->vpVecInput);
    vpVecPushn(spCtx->vpVecInput, (void*) cpString, uiLen);
    // push the null-term
    spCtx->uiInputLength = uiVecLen(spCtx->vpVecInput);
    vpVecPush(spCtx->vpVecInput, &s_cZero);
    spCtx->cpInput = (char*) vpVecFirst(spCtx->vpVecInput);
    vLinesDtor(spCtx->vpLines);
    spCtx->vpLines = vpLinesCtor(spCtx->spException, spCtx->cpInput, spCtx->uiInputLength);
    return spCtx->cpInput;
}

/** \brief Scans the input SABNF grammar for invalid characters and line ends.
 *
 * Constructs a `lines` object for dealing with finding and handling lines.
 * \param[in] vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param[in] bStrict - If true, validate as strict ABNF.
 * If APG_TRUE, validate as strict ABNF (RFC5234 & RFC7405).
 * Otherwise, validate as SABNF.
 * \return Throws exception on error.
 */
void vApiInValidate(void* vpCtx, abool bStrict) {
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spCtx = (api*) vpCtx;
    char caBuf[BUF_SIZE];
    aint ui, uii, uiLines, uiEndLen;
    line* spThis;
    line* spLines;
    if (!spCtx->cpInput) {
        snprintf(caBuf, BUF_SIZE,
                "no input grammar, see bApiInFile() & uiApiInString()\n");
        XTHROW(spCtx->spException, caBuf);
    }
    spCtx->bInputValid = APG_TRUE;
    spLines = spLinesFirst(spCtx->vpLines);
    uiLines = uiLinesCount(spCtx->vpLines);
    for (ui = 0; ui < uiLines; ui += 1) {
        spThis = &spLines[ui];
        for (uii = 0; uii < spThis->uiTextLength; uii += 1) {
            uint8_t ucChar = (uint8_t) spCtx->cpInput[spThis->uiCharIndex + uii];
            if (ucChar != TAB) {
                if (ucChar < 32 || ucChar > 126) {
                    // invalid character
                    spCtx->bInputValid = APG_FALSE;
                    vPushInvalidChar(spCtx, (spThis->uiCharIndex + uii), ucChar, s_cpMsgInvalid);
                }
            }
        }
        uiEndLen = (aint) strlen(spThis->caLineEnd);
        if (uiEndLen == 0) {
            // EOF
            spCtx->bInputValid = APG_FALSE;
            vPushInvalidChar(spCtx, (spThis->uiCharIndex + uii - 1), 0, s_cpMsgEOF);
        } else if (uiEndLen == 1) {
            if (bStrict) {
                // not CRLF
                spCtx->bInputValid = APG_FALSE;
                vPushInvalidChar(spCtx, (spThis->uiCharIndex + uii), (uint8_t) spThis->caLineEnd[0],
                        s_cpMsgCRLF);
            }
        }
    }
    if(!spCtx->bInputValid){
        XTHROW(spCtx->spException, "grammar has invalid characters");
    }
}

/** \brief Display the input lines with line numbers and character offsets.
 *
 * Writes the input grammar in ASCII format to a file.
 * - Valid control characters are single quoted, e.g. '\\t', '\\n', '\\r'
 * - Invalid characters are double quoted hex, e.g. '\\xHH'
 * - Invalid last line with no line ending is indicated as "EOF"
 * \param[in] vpCtx - pointer to an API context previously returned from vpApiCtor().
 * \param[in] cpFileName - Name of the file to write the result to. If NULL, stdout is used.
 */
void vApiInToAscii(void* vpCtx, const char* cpFileName) {
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spCtx = (api*) vpCtx;
    char caBuf[BUF_SIZE];
    FILE* spFile = stdout;
    aint uii;
    line* spLine;
    uint8_t ucChar;
    if (cpFileName) {
        spFile = fopen(cpFileName, "wb");
        if (!spFile) {
            snprintf(caBuf, BUF_SIZE, "can't open file %s for writing", cpFileName);
            XTHROW(spCtx->spException, caBuf);
        }
    }
    spLine = spLinesFirst(spCtx->vpLines);
    while(spLine) {
        fprintf(spFile, "%"PRIuMAX"(%"PRIuMAX"):", (luint) spLine->uiLineIndex, (luint) spLine->uiCharIndex);
        for (uii = 0; uii < spLine->uiTextLength; uii += 1) {
            ucChar = (uint8_t) spCtx->cpInput[spLine->uiCharIndex + uii];
            if (ucChar == TAB) {
                fprintf(spFile, "\\t"); // TAB
            } else if (ucChar >= 32 && ucChar <= 126) {
                fprintf(spFile, "%c", ucChar); // good char
            } else {
                fprintf(spFile, "\"\\x%02X\"", ucChar); // bad char
            }
        }
        ucChar = spLine->caLineEnd[0];
        if (ucChar == 0) {
            fprintf(spFile, "\\EOF\n"); // bad line ending
        } else {
            fprintf(spFile, "%s", ucChar == LF ? "\\n" : "\\r");
            ucChar = spLine->caLineEnd[1];
            if (ucChar) {
                fprintf(spFile, "%s", ucChar == LF ? "\\n" : "\\r");
            }
            fprintf(spFile, "\n");
        }
        spLine = spLinesNext(spCtx->vpLines);
    }
    if (spFile != stdout) {
        fclose(spFile);
    }
}

/** \brief Display the input lines with line numbers and character offsets.
 *
 * Writes the input grammar as an HTML page to a file.
 * - Valid control characters are stylized as TAB, LF and CR
 * - Invalid characters are error stylized in hex, e.g. \xHH
 * - Invalid last line with no line ending is error stylized as EOF
 * \param[in] vpCtx - pointer to an API context previously returned from vpApiCtor().
 * \param[in] cpFileName - name of the file to write the result to.
 * \param[in] cpTitle - HTML title. If NULL, a default page title is used.
 */
void vApiInToHtml(void* vpCtx, const char* cpFileName, const char* cpTitle) {
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spCtx = (api*) vpCtx;
    char caBuf[BUF_SIZE];
    FILE* spFile = NULL;
    aint uii;
    line* spLine;
    uint8_t ucChar;
    if (!cpFileName) {
        XTHROW(spCtx->spException, "file name cannot be NULL");
    }
    spFile = fopen(cpFileName, "wb");
    if (!spFile) {
        snprintf(caBuf, BUF_SIZE, "fopen() failed for file name %s for writing", cpFileName);
        XTHROW(spCtx->spException, caBuf);
    }
    if (!cpTitle) {
        cpTitle = "SABNF Grammar";
    }
    vHtmlHeader(spFile, cpTitle);
    fprintf(spFile, "%s", "<table>\n");
    fprintf(spFile, "%s", "<table><tr><th>line<br/>index</th><th>char<br>offset</th><th>line<br/>text</th></tr>\n");
    spLine = spLinesFirst(spCtx->vpLines);
    while(spLine){
        fprintf(spFile, "%s%"PRIuMAX"%s%"PRIuMAX"%s", "<tr><td>", (luint) spLine->uiLineIndex, "</td><td>",
                (luint) spLine->uiCharIndex, "</td><td>");
        for (uii = 0; uii < spLine->uiTextLength; uii += 1) {
            ucChar = (uint8_t) spCtx->cpInput[spLine->uiCharIndex + uii];
            if (ucChar == TAB) {
                fprintf(spFile, "%s", "<var>TAB</var>");
            } else if (ucChar >= 32 && ucChar <= 126) {
                fprintf(spFile, "%c", ucChar);
            } else {
                fprintf(spFile, "%s%02X%s", "<kbd>\\x", ucChar, "</kbd>");
            }
        }
        ucChar = spLine->caLineEnd[0];
        if (ucChar == 0) {
            fprintf(spFile, "%s", "<kbd>EOF</kbd>");
        } else {
            fprintf(spFile, "%s%s%s", "<var>", ((ucChar == LF) ? "LF" : "CR"), "</var>");
            ucChar = spLine->caLineEnd[1];
            if (ucChar) {
                fprintf(spFile, "%s%s%s", "<var>", ((ucChar == LF) ? "LF" : "CR"), "</var>");
            }
        }
        fprintf(spFile, "%s\n", "</td></tr>");
        spLine = spLinesNext(spCtx->vpLines);
    }
    fprintf(spFile, "%s\n", "</table>");
    vHtmlFooter(spFile);
    if (spFile) {
        fclose(spFile);
    }
}

/** \brief Finds the grammar line associated with a character index and formats an error message to the error log.
 * \param[in] spCtx - pointer to an API context previously returned from vpApiCtor().
 * \param uiCharIndex The index of the character whose line number is desired.
 * \param cpSrc An string idendifying the caller.
 * \param cpMsg The error message.
 */
void vLineError(api* spCtx, aint uiCharIndex, const char* cpSrc, const char* cpMsg) {
    aint uiLine, uiRelIndex;
    aint uiBufSize = 2014;
    char cZero = 0;
    char cLF = 10;
    char caBuf[uiBufSize];
    aint ui;
    char* cpText;
    char* cpCtrl;
    uint8_t ucChar;
    void* vpVecTemp = NULL;

    // generate the line location information
    vpVecTemp = spCtx->vpVecTempChars;
    vVecClear(vpVecTemp);
    // pop the final null term if any
    if (bLinesFindLine(spCtx->vpLines, uiCharIndex, &uiLine, &uiRelIndex)) {
        line* spLine = spLinesFirst(spCtx->vpLines);
        spLine = &spLine[uiLine];

        // generate the error message
        aint uiRelCharIndex = uiCharIndex - spLine->uiCharIndex;
        snprintf(caBuf, uiBufSize, "%s: line index: %"PRIuMAX": rel char index: %"PRIuMAX": %s",
                cpSrc, (luint) uiLine, (luint) uiRelCharIndex, cpMsg);
        vpVecPushn(vpVecTemp, (void*) caBuf, (aint) strlen(caBuf));
        vpVecPush(vpVecTemp, (void*)&cLF);

        // generate the line text
        int n = 0;
        for(; n < strlen(cpSrc); n++){
            caBuf[n] = ' ';
        }
        caBuf[n++] = ':';
        caBuf[n++] = ' ';
        caBuf[n] = 0;
        vpVecPushn(vpVecTemp, (void*) caBuf, (aint) strlen(caBuf));
        cpText = &spCtx->cpInput[spLine->uiCharIndex];
        for (ui = 0; ui < spLine->uiLineLength; ui += 1) {
            ucChar = cpText[ui];
            if (ucChar >= 32 && ucChar <= 126) {
                vpVecPush(vpVecTemp, (void*) &ucChar);
            } else if (ucChar == TAB) {
                cpCtrl = "\\t";
                vpVecPushn(vpVecTemp, (void*) cpCtrl, (aint) strlen(cpCtrl));
            } else if (ucChar == LF) {
                cpCtrl = "\\n";
                vpVecPushn(vpVecTemp, (void*) cpCtrl, (aint) strlen(cpCtrl));
            } else if (ucChar == CR) {
                cpCtrl = "\\n";
                vpVecPushn(vpVecTemp, (void*) cpCtrl, (aint) strlen(cpCtrl));
            } else {
                sprintf(caBuf, "\\x%02X", ucChar);
                vpVecPushn(vpVecTemp, (void*) caBuf, (aint) strlen(caBuf));
            }
        }
        vpVecPush(vpVecTemp, (void*) &cZero);
    } else {
        snprintf(caBuf, uiBufSize, "%s: char index out of range: %"PRIuMAX": %s",
                cpSrc, (luint) uiCharIndex, cpMsg);
        vpVecPushn(vpVecTemp, (void*) caBuf, (aint) (strlen(caBuf) + 1));
    }

    // log the line error
    vMsgsLog(spCtx->vpLog, vpVecFirst(vpVecTemp));
    vVecClear(vpVecTemp);
}

static void vPushInvalidChar(api* spCtx, aint uiCharIndex, uint8_t ucChar, const char* cpMsg) {
    char caBuf[256];
    // create the error message in a temp buffer
    snprintf(caBuf, 256, "invalid character: 0x%"PRIXMAX": %s", (luint) ucChar, cpMsg);
    vLineError(spCtx, uiCharIndex, "validate", caBuf);
}
