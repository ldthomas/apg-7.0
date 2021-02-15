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
/** \file trace-out.c
 * \brief All of the trace output code.
 */
#include "./apg.h"
#ifdef APG_TRACE
#include "./lib.h"
#include "./parserp.h"
#include "./tracep.h"
static const aint s_uiMaxPhraseLength = 120;
static aint uiAcharToHex(achar acChar, char* cpHex);
static const char* cpIsControl(achar acChar);
static abool bIsUnicode(achar acChar);
static const char* s_caControlChars[33] =
{"NULL","SOH","STX","ETX","EOT","ENQ","ACK","BEL","BS","\\t",
        "\\n","VT","FF","\\r","SO","SI","DLE","DC1","DC2","DC3",
        "DC4","NAK","SYN","ETB","CAN","EM","SUB","ESC","FS","GS",
        "RS","US","DEL"};

/** \struct html_info
 * \brief Keeps track of the display state for HTML display.
 */
typedef struct{
    trace* spTrace; ///< \brief Pointer to the trace object's context.
    const achar* acpInput; ///< \brief Pointer to the input string.
    char* cpOutput; ///< \brief Pointer to a buffer to build output in.
    aint uiMatched; ///< \brief True if a phrase was matched.
    aint uiRemainder; ///< \brief Length of the remaining input string beyond the current sub-phrase.
    char* cpEmpty; ///< \brief The display for and empty match.
    char* cpLastChar; ///< \brief The display symbol for the end of string character.
} html_info;
static const char* s_cpHtmlHeader;
static const char* s_cpHtmlFooter;

static void vAsciiPpptRecord(trace* spCtx, trace_record* spRec);
static void vHtmlPpptRecord(trace* spCtx, trace_record* spRec);
static void vHtmlHeader(trace* spCtx);
static void vHtmlSeparator(trace* spCtx, aint uiLastIndex);
static void vHtmlRecord(trace* spCtx, trace_record* spRec);
static void vHtmlFooter(trace* spCtx);
static const char* cpHtmlState(trace* spTrace, aint uiState, aint uiPhraseLength);
static const char* cpHtmlOpcode(trace* spCtx, const opcode* spOp, aint uiIndent);
static const char* cpHtmlPhrase(trace* spCtx, aint uiState, aint uiOffset, aint uiPhraseLength);
static void vHtmlPhraseUnicode(html_info* spInfo);

/** \struct display_info
 * \brief Keeps track of the display state for ASCII display.
 */
typedef struct  {
    const achar* acpStr; ///< \brief Pointer to the matched phrase.
    aint uiStrLen; ///< \brief Length of the matched phrase.
    char* cpBuf; ///< \brief Pointer to a scratch buffer.
    int n; ///< \brief Keeps track of the length of the string in the scratch buffer.
} display_info;
static const char* s_cpLineEnd = "$";
static const char* s_cpLineTruncated = "...";
static const char* s_cpEmpty = "(empty)";
static void vAsciiHeader(trace* spCtx);
static void vAsciiSeparator(trace* spCtx, aint uiLastIndex);
static void vAsciiRecord(trace* spCtx, trace_record* spRec);
static void vAsciiFooter(trace* spCtx);
static const char* cpAsciiState(aint uiState, aint uiPhraseLength);
static const char* cpAsciiOpcode(trace* spCtx, const opcode* spOp, aint uiIndent);
static const char* cpAsciiPhrase(trace* spCtx, aint uiState, aint uiOffset, aint uiPhraseLength);
static abool bAsciiStringAscii(display_info* spInfo);

/** \brief Display the trace header.
 *
 */
void vDisplayHeader(trace* spCtx){
    if(spCtx->sConfig.uiOutputType == TRACE_HTML){
        vHtmlHeader(spCtx);
    }else if(spCtx->sConfig.uiOutputType == TRACE_ASCII){
        vAsciiHeader(spCtx);
    }
    // no header for APGEX
}

/** \brief Display one trace record.
 *
 */
void vDisplayRecord(trace* spCtx, trace_record* spRec, abool bIsMatchedPppt){
    if(spCtx->sConfig.uiOutputType == TRACE_HTML){
        if(spCtx->sConfig.bPppt && bIsMatchedPppt){
            vHtmlPpptRecord(spCtx, spRec);
        }else{
            vHtmlRecord(spCtx, spRec);
        }
    }else if(spCtx->sConfig.uiOutputType == TRACE_ASCII){
        if(spCtx->sConfig.bPppt && bIsMatchedPppt){
            vAsciiPpptRecord(spCtx, spRec);
        }else{
            vAsciiRecord(spCtx, spRec);
        }
    }
}

/** \brief Display a separator between trace outputs (apgex only)
 *
 */
void vDisplaySeparator(trace* spCtx, aint uiLastIndex){
    if(spCtx->sConfig.uiOutputType == TRACE_HTML){
        vHtmlSeparator(spCtx, uiLastIndex);
    }else if(spCtx->sConfig.uiOutputType == TRACE_ASCII){
        vAsciiSeparator(spCtx, uiLastIndex);
    }
    // no header for APGEX
}

/** \brief Display the trace footer.
 *
 */
void vDisplayFooter(trace* spCtx){
    if(spCtx->sConfig.uiOutputType == TRACE_HTML){
        vHtmlFooter(spCtx);
    }else if(spCtx->sConfig.uiOutputType == TRACE_ASCII){
        vAsciiFooter(spCtx);
    }
    // no footer for APGEX
}

static aint uiAcharToHex(achar acChar, char* cpHex){
    aint uiReturn = 0;
    while(APG_TRUE){
        if(acChar <= 0xFF){
            uiReturn = (aint)sprintf(cpHex, "x%02"PRIXMAX"", (luint) acChar);
            break;
        }
        if(acChar <= 0xFFFF){
            uiReturn = (aint)sprintf(cpHex, "x%04"PRIXMAX"", (luint) acChar);
            break;
        }
        if(acChar <= 0xFFFFFF){
            uiReturn = (aint)sprintf(cpHex, "x%06"PRIXMAX"", (luint) acChar);
            break;
        }
        if(acChar <= 0xFFFFFFFF){
            uiReturn = (aint)sprintf(cpHex, "x%08"PRIXMAX"", (luint) acChar);
            break;
        }
        if(acChar <= 0xFFFFFFFFFF){
            uiReturn = (aint)sprintf(cpHex, "x%010"PRIXMAX"", (luint) acChar);
            break;
        }
        if(acChar <= 0xFFFFFFFFFFFF){
            uiReturn = (aint)sprintf(cpHex, "x%012"PRIXMAX"", (luint) acChar);
            break;
        }
        if(acChar <= 0xFFFFFFFFFFFFFF){
            uiReturn = (aint)sprintf(cpHex, "x%014"PRIXMAX"", (luint) acChar);
            break;
        }
        uiReturn = (aint)sprintf(cpHex, "x%016"PRIXMAX"", (luint) acChar);
        break;
    }
    return uiReturn;
}

// ASCII FORMAT
static void vAsciiHeader(trace* spCtx){
    fprintf(spCtx->spOut, "%4s|%3s|%3s|%1s|%3s|%3s|operator matched phrase or remaining unmatched string\n", "a", "b", "c", "d", "e", "f");
}

static void vAsciiSeparator(trace* spCtx, aint uiLastIndex){
    fprintf(spCtx->spOut, "Last Index: %"PRIuMAX"\n", (luint)uiLastIndex);
}

static void vAsciiPpptRecord(trace* spCtx, trace_record* spRec){
    fprintf(spCtx->spOut, "%4"PRIuMAX"|", (luint) spRec->uiThisRecord); // uiThisRecord
    fprintf(spCtx->spOut, "%3"PRIuMAX"|", (luint) spRec->uiTreeDepth); // c
    fprintf(spCtx->spOut, "%3d|", spRec->iTraceDepth); // d
    switch(spRec->uiState){
    case ID_MATCH:
        fprintf(spCtx->spOut, "%1s|", "M"); // g
        break;
    case ID_NOMATCH:
        fprintf(spCtx->spOut, "%1s|", "N"); // g
        break;
    case ID_EMPTY:
        fprintf(spCtx->spOut, "%1s|", "E"); // g
        break;
    default:
        fprintf(spCtx->spOut, "%1s|", "-"); // g
        break;
    }
    fprintf(spCtx->spOut, "%3"PRIuMAX"|", (luint) spRec->uiOffset); // e
    fprintf(spCtx->spOut, "%3"PRIuMAX"|", (luint) spRec->uiPhraseLength); // f
    aint ui;
    for(ui = 0; ui < spRec->uiTreeDepth; ui++){
        fprintf(spCtx->spOut, "."); // indent
    }
    fprintf(spCtx->spOut, "PPPT<%s> ", cpAsciiOpcode(spCtx, spRec->spOpcode, 0));
    fprintf(spCtx->spOut, "%s",
            cpAsciiPhrase(spCtx, spRec->uiState, spRec->uiOffset, spRec->uiPhraseLength)); // phrase
}
static void vAsciiRecord(trace* spCtx, trace_record* spRec){
    fprintf(spCtx->spOut, "%4"PRIuMAX"|", (luint) spRec->uiThisRecord); // a
    fprintf(spCtx->spOut, "%3"PRIuMAX"|", (luint) spRec->uiTreeDepth); // c
    fprintf(spCtx->spOut, "%3d|", spRec->iTraceDepth); // d
    fprintf(spCtx->spOut, "%1s|", cpAsciiState(spRec->uiState, spRec->uiPhraseLength)); // g
    fprintf(spCtx->spOut, "%3"PRIuMAX"|", (luint) spRec->uiOffset); // e
    fprintf(spCtx->spOut, "%3"PRIuMAX"|", (luint) spRec->uiPhraseLength); // f
    fprintf(spCtx->spOut, "%s ", cpAsciiOpcode(spCtx, spRec->spOpcode, spRec->uiTreeDepth)); // operator
    fprintf(spCtx->spOut, "%s",
            cpAsciiPhrase(spCtx, spRec->uiState, spRec->uiOffset, spRec->uiPhraseLength)); // phrase
}
static void vAsciiFooter(trace* spCtx){
    fprintf(spCtx->spOut, "\n");
    fprintf(spCtx->spOut, "legend:\n");
    fprintf(spCtx->spOut, "a - line number\n");
    fprintf(spCtx->spOut, "b - tree depth\n");
    fprintf(spCtx->spOut, "c - trace depth\n");
    fprintf(spCtx->spOut, "d - operator state (*)\n");
    fprintf(spCtx->spOut, "e - phrase offset\n");
    fprintf(spCtx->spOut, "f - phrase length\n");
    fprintf(spCtx->spOut, "operator mnemonic - (**)\n");
    fprintf(spCtx->spOut, "matched phrase, if state is M\n");
    fprintf(spCtx->spOut, "(empty), if state is E\n");
    fprintf(spCtx->spOut, "remaining unmatched input string, if state is N or -\n");
    fprintf(spCtx->spOut, "%s - indicates that the input string display has been truncated\n", s_cpLineTruncated);
    fprintf(spCtx->spOut, "%s - indicates the end of string\n", s_cpLineEnd);
    fprintf(spCtx->spOut, "      Beware of possible confusion if \"%s\" or \"%s\" exists in input string.\n",
            s_cpLineTruncated, s_cpLineEnd);
    fprintf(spCtx->spOut, "\n");
    fprintf(spCtx->spOut, "(*)   OPERATOR STATE:\n");
    fprintf(spCtx->spOut, "    - phrase opened\n");
    fprintf(spCtx->spOut, "    M phrase matched\n");
    fprintf(spCtx->spOut, "    E phrase empty (matched with length 0)\n");
    fprintf(spCtx->spOut, "    N phrase not matched\n");
    fprintf(spCtx->spOut, "\n");
    fprintf(spCtx->spOut, "(**)  OPERATOR MNEMONICS:\n");
    fprintf(spCtx->spOut, "      original ABNF operators:\n");
    fprintf(spCtx->spOut, "ALT - alternation\n");
    fprintf(spCtx->spOut, "CAT - concatenation\n");
    fprintf(spCtx->spOut, "REP - repetition\n");
    fprintf(spCtx->spOut, "RNM - rule name\n");
    fprintf(spCtx->spOut, "TRG - terminal range\n");
    fprintf(spCtx->spOut, "TLS - terminal literal string (case insensitive)\n");
    fprintf(spCtx->spOut, "TBS - terminal binary string (case sensitive)\n");
    fprintf(spCtx->spOut, "\n");
    fprintf(spCtx->spOut, "      SABNF super set operators:\n");
    fprintf(spCtx->spOut, "UDT - user-defined terminal\n");
    fprintf(spCtx->spOut, "AND - positive look ahead\n");
    fprintf(spCtx->spOut, "NOT - negative look ahead\n");
    fprintf(spCtx->spOut, "BKA - positive look behind\n");
    fprintf(spCtx->spOut, "BKN - negative look behind\n");
    fprintf(spCtx->spOut, "BKR - back reference\n");
    fprintf(spCtx->spOut, "ABG - anchor - begin of input string\n");
    fprintf(spCtx->spOut, "AEN - anchor - end of input string\n");
}
static const char* cpAsciiState(aint uiState, aint uiPhraseLength) {
    static const char* cpActive = "-";
    static const char* cpMatch = "M";
    static const char* cpNomatch = "N";
    static const char* cpEmpty = "E";
    static const char* cpUnknown = "U";
    if (uiState == ID_ACTIVE) {
        return cpActive;
    }
    if (uiState == ID_NOMATCH) {
        return cpNomatch;
    }
    if (uiState == ID_MATCH) {
        if (uiPhraseLength == 0) {
            return cpEmpty;
        }
        return cpMatch;
    }
    return cpUnknown;
}
static const char* cpAsciiOpcode(trace* spCtx, const opcode* spOp, aint uiIndent) {
    char* cpBuf = spCtx->cpBuf;
    char* cpOp;
    int n = 0;
    aint ui;
    for (ui = 0; ui < (int) uiIndent; ui += 1) {
        n += sprintf(&cpBuf[n], ".");
    }
    switch (spOp->sGen.uiId) {
    case ID_ALT:
        cpOp = "ALT";
        sprintf(&cpBuf[n], "%s(%"PRIuMAX")", cpOp, (luint) spOp->sAlt.uiChildCount);
        break;
    case ID_CAT:
        cpOp = "CAT";
        sprintf(&cpBuf[n], "%s(%"PRIuMAX")", cpOp, (luint) spOp->sCat.uiChildCount);
        break;
    case ID_REP:
        cpOp = "REP";
        if (spOp->sRep.uiMax == APG_INFINITE) {
            sprintf(&cpBuf[n], "%s(%"PRIuMAX"*inf)", cpOp, (luint) spOp->sRep.uiMin);
        } else {
            sprintf(&cpBuf[n], "%s(%"PRIuMAX"*%"PRIuMAX")", cpOp, (luint) spOp->sRep.uiMin, (luint) spOp->sRep.uiMax);
        }
        break;
    case ID_RNM:
        cpOp = "RNM";
        sprintf(&cpBuf[n], "%s(%s)", cpOp, spOp->sRnm.spRule->cpRuleName);
        break;
    case ID_TRG:
        cpOp = "TRG";
        sprintf(&cpBuf[n], "%s[x%"PRIXMAX"-%"PRIXMAX"]", cpOp, (luint) spOp->sTrg.acMin, (luint) spOp->sTrg.acMax);
        break;
    case ID_TBS:
        cpOp = "TBS";
        n += sprintf(&cpBuf[n], "%s[", cpOp);
        if (spOp->sTbs.uiStrLen > 3) {
            for (ui = 0; ui < 3; ui += 1) {
                if (ui > 0) {
                    n += sprintf(&cpBuf[n], ", ");
                }
                n += sprintf(&cpBuf[n], "x%"PRIXMAX"", (luint) spOp->sTbs.acpStrTbl[ui]);
            }
            n += sprintf(&cpBuf[n], ", ...");
        } else {
            for (ui = 0; ui < spOp->sTbs.uiStrLen; ui += 1) {
                if (ui > 0) {
                    n += sprintf(&cpBuf[n], ", ");
                }
                n += sprintf(&cpBuf[n], "x%"PRIXMAX"", (luint) spOp->sTbs.acpStrTbl[ui]);
            }
        }
        sprintf(&cpBuf[n], "]");
        break;
    case ID_TLS:
        cpOp = "TLS";
        n += sprintf(&cpBuf[n], "%s(", cpOp);
        if (spOp->sTls.uiStrLen > 3) {
            for (ui = 0; ui < 3; ui += 1) {
                cpBuf[n++]= (char) spOp->sTls.acpStrTbl[ui];
            }
            n += sprintf(&cpBuf[n], ", ...");
        } else {
            for (ui = 0; ui < spOp->sTls.uiStrLen; ui += 1) {
                cpBuf[n++]= (char) spOp->sTls.acpStrTbl[ui];
            }
        }
        sprintf(&cpBuf[n], ")");
        break;
    case ID_UDT:
        cpOp = "UDT";
        sprintf(&cpBuf[n], "%s(%s)", cpOp, spOp->sUdt.spUdt->cpUdtName);
        break;
    case ID_AND:
        cpOp = "AND";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    case ID_NOT:
        cpOp = "NOT";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    case ID_BKR:
        cpOp = "BKR";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    case ID_BKA:
        cpOp = "BKA";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    case ID_BKN:
        cpOp = "BKN";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    case ID_ABG:
        cpOp = "ABG";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    case ID_AEN:
        cpOp = "AEN";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    default:
        cpOp = "UNK";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    }
    return cpBuf;
}
static abool bAsciiStringAscii(display_info* spInfo){
    abool bReturn = APG_FAILURE;
    aint ui;
    for(ui = 0; ui < spInfo->uiStrLen; ui++){
        achar acChar = spInfo->acpStr[ui];
        if (acChar >= 32 && acChar <= 126){
            spInfo->cpBuf[spInfo->n++] = (char)acChar;
        }else if(acChar == 9){
            spInfo->n += sprintf(&spInfo->cpBuf[spInfo->n], "\\t");
        }else if(acChar == 10){
            spInfo->n += sprintf(&spInfo->cpBuf[spInfo->n], "\\n");
        }else if(acChar == 13){
            spInfo->n += sprintf(&spInfo->cpBuf[spInfo->n], "\\r");
        }else{
            spInfo->n += uiAcharToHex(acChar, &spInfo->cpBuf[spInfo->n]);
        }
        if(spInfo->n >= s_uiMaxPhraseLength){
            goto truncate;
        }
    }
    bReturn = APG_SUCCESS;
    truncate:;
    return bReturn;
}
static const char* cpAsciiPhrase(trace* spCtx, aint uiState, aint uiOffset, aint uiPhraseLength){
    display_info sInfo;
    sInfo.acpStr = spCtx->spParserCtx->acpInputString + uiOffset;
    sInfo.uiStrLen = spCtx->spParserCtx->uiInputStringLength - uiOffset;
    sInfo.cpBuf = spCtx->cpBuf;
    sInfo.n = 0;
    if((uiState == ID_ACTIVE) || (uiState == ID_NOMATCH)){
        if(bAsciiStringAscii(&sInfo)){
            sInfo.n += sprintf(&sInfo.cpBuf[sInfo.n], "%s", s_cpLineEnd);
        }else{
            sInfo.n += sprintf(&sInfo.cpBuf[sInfo.n], "%s", s_cpLineTruncated);
        }
    }else{
        if(uiPhraseLength){
            sInfo.uiStrLen = uiPhraseLength;
            bAsciiStringAscii(&sInfo);
        }else{
            sInfo.n += sprintf(&sInfo.cpBuf[sInfo.n], "%s", s_cpEmpty);
        }
    }
    sInfo.cpBuf[sInfo.n++] = 10;
    sInfo.cpBuf[sInfo.n] = 0;
    return spCtx->cpBuf;
}

// HTML FORMAT
static void vHtmlHeader(trace* spCtx){
    // output the page header
    fprintf(spCtx->spOut, "%s", s_cpHtmlHeader);

    // open the table
    fprintf(spCtx->spOut, "%s", "<table class=\"apg-trace\">\n");
    fprintf(spCtx->spOut,
            "<tr><th>(a)</th><th>(b)</th><th>(c)</th><th>(d)</th><th>(e)</th><th>(f)</th><th>operator</th><th>phrase</th></tr>\n");
}
static void vHtmlSeparator(trace* spCtx, aint uiLastIndex){
    fprintf(spCtx->spOut,
            "<tr><td colspan=\"9\">Last Index: %"PRIuMAX"</td></tr>\n", (luint)uiLastIndex);
}
static void vHtmlPpptRecord(trace* spCtx, trace_record* spRec){
    fprintf(spCtx->spOut, "%s", "<tr>");
    fprintf(spCtx->spOut, "<td>%"PRIuMAX"</td>", (luint) spRec->uiThisRecord); // a
    fprintf(spCtx->spOut, "<td>%"PRIuMAX"</td>", (luint) spRec->uiTreeDepth); // c
    fprintf(spCtx->spOut, "<td>%d</td>", spRec->iTraceDepth); // d
    fprintf(spCtx->spOut, "<td>%s</td>", cpHtmlState(spCtx, spRec->uiState, spRec->uiPhraseLength)); // g
    fprintf(spCtx->spOut, "<td>%"PRIuMAX"</td>", (luint) spRec->uiOffset); // e
    fprintf(spCtx->spOut, "<td>%"PRIuMAX"</td>", (luint) spRec->uiPhraseLength); // f
    aint ui = 0;
    fprintf(spCtx->spOut, "<td>");
    for(; ui < spRec->uiTreeDepth; ui++){
        fprintf(spCtx->spOut, ".");
    }
    fprintf(spCtx->spOut, "PPPT&lt;%s&gt;</td>", cpHtmlOpcode(spCtx, spRec->spOpcode, 0)); // operator
    fprintf(spCtx->spOut, "<td>%s</td>",
            cpHtmlPhrase(spCtx, spRec->uiState, spRec->uiOffset, spRec->uiPhraseLength)); // phrase
    fprintf(spCtx->spOut, "%s", "</tr>\n");
}
static void vHtmlRecord(trace* spCtx, trace_record* spRec){
    fprintf(spCtx->spOut, "%s", "<tr>");
    fprintf(spCtx->spOut, "<td>%"PRIuMAX"</td>", (luint) spRec->uiThisRecord); // a
    fprintf(spCtx->spOut, "<td>%"PRIuMAX"</td>", (luint) spRec->uiTreeDepth); // c
    fprintf(spCtx->spOut, "<td>%d</td>", spRec->iTraceDepth); // d
    fprintf(spCtx->spOut, "<td>%s</td>", cpHtmlState(spCtx, spRec->uiState, spRec->uiPhraseLength)); // g
    fprintf(spCtx->spOut, "<td>%"PRIuMAX"</td>", (luint) spRec->uiOffset); // e
    fprintf(spCtx->spOut, "<td>%"PRIuMAX"</td>", (luint) spRec->uiPhraseLength); // f
    fprintf(spCtx->spOut, "<td>%s</td>", cpHtmlOpcode(spCtx, spRec->spOpcode, spRec->uiTreeDepth)); // operator
    fprintf(spCtx->spOut, "<td>%s</td>",
            cpHtmlPhrase(spCtx, spRec->uiState, spRec->uiOffset, spRec->uiPhraseLength)); // phrase
    fprintf(spCtx->spOut, "%s", "</tr>\n");
}
static void vHtmlFooter(trace* spCtx){
    // close the table
    fprintf(spCtx->spOut, "%s", "</table>\n");

    // output the page footer
    fprintf(spCtx->spOut, "%s", s_cpHtmlFooter);
}

static const char* cpHtmlState(trace* spTrace, aint uiState, aint uiPhraseLength) {
    if (uiState == ID_ACTIVE) {
        return "<span class=\"apg-active\">&darr;&nbsp;</span>";
    }
    if (uiState == ID_NOMATCH) {
        return "<span class=\"apg-nomatch\">&uarr;N</span>";
    }
    if (uiState == ID_MATCH) {
        if (uiPhraseLength == 0) {
            return "<span class=\"apg-empty\">&uarr;E</span>";
        }
        aint* uipLookaround = (aint*)vpVecLast(spTrace->vpLookaroundStack);
        if(uipLookaround){
            if(*uipLookaround == ID_LOOKAROUND_AHEAD){
                return "<span class=\"apg-lh-match\">&uarr;M</span>";
            }
            return "<span class=\"apg-lb-match\">&uarr;M</span>";
        }
        return "<span class=\"apg-match\">&uarr;M</span>";
    }
    return "<span class=\"apg-nomatch\">&#10008;</span>";
}
static const char* cpHtmlOpcode(trace* spCtx, const opcode* spOp, aint uiIndent) {
    char* cpBuf = spCtx->cpBuf;
    char* cpOp;
    int n = 0;
    aint ui;
    for (ui = 0; ui < (int) uiIndent; ui += 1) {
        n += sprintf(&cpBuf[n], ".");
    }
    switch (spOp->sGen.uiId) {
    case ID_ALT:
        cpOp = "ALT";
        sprintf(&cpBuf[n], "%s(%"PRIuMAX")", cpOp, (luint) spOp->sAlt.uiChildCount);
        break;
    case ID_CAT:
        cpOp = "CAT";
        sprintf(&cpBuf[n], "%s(%"PRIuMAX")", cpOp, (luint) spOp->sCat.uiChildCount);
        break;
    case ID_REP:
        cpOp = "REP";
        if (spOp->sRep.uiMax == APG_INFINITE) {
            sprintf(&cpBuf[n], "%s(%"PRIuMAX"*&infin;)", cpOp, (luint) spOp->sRep.uiMin);
        } else {
            sprintf(&cpBuf[n], "%s(%"PRIuMAX"*%"PRIuMAX")", cpOp, (luint) spOp->sRep.uiMin, (luint) spOp->sRep.uiMax);
        }
        break;
    case ID_RNM:
        cpOp = "RNM";
        sprintf(&cpBuf[n], "%s(%s)", cpOp, spOp->sRnm.spRule->cpRuleName);
        break;
    case ID_TRG:
        cpOp = "TRG";
        sprintf(&cpBuf[n], "%s[x%"PRIXMAX"-%"PRIXMAX"]", cpOp, (luint) spOp->sTrg.acMin, (luint) spOp->sTrg.acMax);
        break;
    case ID_TBS:
        cpOp = "TBS";
        n += sprintf(&cpBuf[n], "%s[", cpOp);
        if (spOp->sTbs.uiStrLen > 3) {
            for (ui = 0; ui < 3; ui += 1) {
                if (ui > 0) {
                    n += sprintf(&cpBuf[n], ", ");
                }
                n += sprintf(&cpBuf[n], "x%"PRIXMAX"", (luint) spOp->sTbs.acpStrTbl[ui]);
            }
            n += sprintf(&cpBuf[n], ", ...");
        } else {
            for (ui = 0; ui < spOp->sTbs.uiStrLen; ui += 1) {
                if (ui > 0) {
                    n += sprintf(&cpBuf[n], ", ");
                }
                n += sprintf(&cpBuf[n], "x%"PRIXMAX"", (luint) spOp->sTbs.acpStrTbl[ui]);
            }
        }
        sprintf(&cpBuf[n], "]");
        break;
    case ID_TLS:
        cpOp = "TLS";
        n += sprintf(&cpBuf[n], "%s(", cpOp);
        if (spOp->sTls.uiStrLen > 3) {
            for (ui = 0; ui < 3; ui += 1) {
                n += sprintf(&cpBuf[n], "&#%"PRIuMAX";", (luint) spOp->sTls.acpStrTbl[ui]);
            }
            n += sprintf(&cpBuf[n], ", ...");
        } else {
            for (ui = 0; ui < spOp->sTls.uiStrLen; ui += 1) {
                n += sprintf(&cpBuf[n], "&#%"PRIuMAX";", (luint) spOp->sTls.acpStrTbl[ui]);
            }
        }
        sprintf(&cpBuf[n], ")");
        break;
    case ID_UDT:
        cpOp = "UDT";
        sprintf(&cpBuf[n], "%s(%s)", cpOp, spOp->sUdt.spUdt->cpUdtName);
        break;
    case ID_AND:
        cpOp = "AND";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    case ID_NOT:
        cpOp = "NOT";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    case ID_BKR:
        cpOp = "BKR";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    case ID_BKA:
        cpOp = "BKA";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    case ID_BKN:
        cpOp = "BKN";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    case ID_ABG:
        cpOp = "ABG";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    case ID_AEN:
        cpOp = "AEN";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    default:
        cpOp = "UNK";
        sprintf(&cpBuf[n], "%s", cpOp);
        break;
    }
    return cpBuf;
}
/** Attempts to render characters as UTF-32.
 * - Characters 0x0-0x7F are rendered as ASCII with special display for control characters.
 * - Characters 0x80-0xD7FF and 0xE000-0xFFFF are sent to the browser to render as Unicode if it can.
 * - Characters 0xD800-0xDFFF (surrogate pairs) are rendered as hex.
 * - Characters > 0xFFFF are rendered as hex.
 */
static void vHtmlPhraseUnicode(html_info* spInfo){
    const achar* acpOut = spInfo->acpInput;
    char* cpBuf = spInfo->cpOutput;
    const char* cpControl;
    int n = 0;
    aint ui;
    n += sprintf(&cpBuf[n], "%s", spInfo->cpEmpty);
    if (spInfo->uiMatched > 0) {
        aint* uipLookaround = (aint*)vpVecLast(spInfo->spTrace->vpLookaroundStack);
        if(uipLookaround){
            if(*uipLookaround == ID_LOOKAROUND_AHEAD){
                n += sprintf(&cpBuf[n], "<span class=\"apg-lh-match\">");
            }else{
                n += sprintf(&cpBuf[n], "<span class=\"apg-lb-match\">");
            }

        }else{
            n += sprintf(&cpBuf[n], "<span class=\"apg-match\">");
        }
        for (ui = 0; ui < spInfo->uiMatched; ui++, acpOut++) {
            cpControl = cpIsControl(*acpOut);
            if(cpControl){
                n += sprintf(&cpBuf[n], "<span class=\"apg-ctrl-char\">%s</span>", cpControl);
            }else if(*acpOut == 32){
                n += sprintf(&cpBuf[n], "&nbsp;");
            }else if(bIsUnicode(*acpOut)){
                n += sprintf(&cpBuf[n], "&#%"PRIuMAX";", (luint) *acpOut);
            }else{
                n += (int)uiAcharToHex(*acpOut, &cpBuf[n]);
            }
        }
        n += sprintf(&cpBuf[n], "</span>");
    }
    if (spInfo->uiRemainder > 0) {
        n += sprintf(&cpBuf[n], "<span class=\"apg-remainder\">");
        for (ui = 0; ui < spInfo->uiRemainder; ui++, acpOut++) {
            cpControl = cpIsControl(*acpOut);
            if(cpControl){
                n += sprintf(&cpBuf[n], "<span class=\"apg-ctrl-char\">%s</span>", cpControl);
            }else if(*acpOut == 32){
                n += sprintf(&cpBuf[n], "&nbsp;");
            }else if(bIsUnicode(*acpOut)){
                n += sprintf(&cpBuf[n], "&#%"PRIuMAX";", (luint) *acpOut);
            }else{
                n += (int)uiAcharToHex(*acpOut, &cpBuf[n]);
            }
        }
        n += sprintf(&cpBuf[n], "</span>");
    }
    n += sprintf(&cpBuf[n], "%s", spInfo->cpLastChar);
}
static const char* cpHtmlPhrase(trace* spCtx, aint uiState, aint uiOffset, aint uiPhraseLength) {
    aint uiMatchedLen, uiRemainder, uiLeft;
    html_info sInfo;
    char *cpEmpty, *cpLastChar;
    if (uiState == ID_MATCH && uiPhraseLength == 0) {
        cpEmpty = "<span class=\"apg-empty\">&#120576</span>";
    } else {
        cpEmpty = "";
    }
    uiLeft = spCtx->spParserCtx->uiSubStringEnd - uiOffset;
    cpLastChar = "<span class=\"apg-line-end\">&bull;</span>";
    if (uiLeft >= s_uiMaxPhraseLength) {
        uiLeft = s_uiMaxPhraseLength;
        cpLastChar = "<span class=\"apg-line-end\">&hellip;</span>";
    }
    uiMatchedLen = 0;
    if (uiState == ID_MATCH) {
        uiMatchedLen = uiPhraseLength < uiLeft ? uiPhraseLength : uiLeft;
    }
    uiLeft -= uiMatchedLen;
    uiRemainder = s_uiMaxPhraseLength - uiMatchedLen;
    if(uiLeft < uiRemainder){
        uiRemainder = uiLeft;
    }

    sInfo.spTrace = spCtx;
    sInfo.acpInput = &spCtx->spParserCtx->acpInputString[uiOffset];
    sInfo.cpEmpty = cpEmpty;
    sInfo.cpLastChar = cpLastChar;
    sInfo.cpOutput = spCtx->cpBuf;
    sInfo.uiMatched = uiMatchedLen;
    sInfo.uiRemainder = uiRemainder;
    vHtmlPhraseUnicode(&sInfo);
    return sInfo.cpOutput;
}

static const char* cpIsControl(achar acChar){
    if(acChar >= 0 && acChar <=31){
        return s_caControlChars[(luint)acChar];
    }
    if(acChar == 127){
        return s_caControlChars[32];
    }
    return NULL;
}
static abool bIsUnicode(achar acChar){
    if(acChar >= 33 && acChar <=126){
        return APG_TRUE;
    }
    if(acChar >= 0x80 && acChar <=0xd7ff){
        return APG_TRUE;
    }
    if(acChar >= 0xe000 && acChar <= 0xffff){
        return APG_TRUE;
    }
    return APG_FALSE;
}

static const char* s_cpHtmlHeader = "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<title>trace</title>\n"
        "<style>\n"
        ".apg-mono {\n"
        "  font-family: monospace;\n"
        "}\n"
        ".apg-active {\n"
        "  font-weight: bold;\n"
        "  color: #000000;\n"
        "}\n"
        ".apg-match {\n"
        "  font-weight: bold;\n"
        "  background-color: #6680FF;\n"
        "  color: white;\n"
        "}\n"
        ".apg-empty {\n"
        "  font-weight: bold;\n"
        "  background-color: #0fbd0f;\n"
        "  color: white;\n"
        "}\n"
        ".apg-nomatch {\n"
        "  font-weight: bold;\n"
        "  background-color: #FF4000;\n"
        "  color: white;\n"
        "}\n"
        ".apg-lh-match {\n"
        "  font-weight: bold;\n"
        "  background-color: #D966FF;\n"
        "  color: white;\n"
        "}\n"
        ".apg-lb-match {\n"
        "  font-weight: bold;\n"
        "  background-color: #FF944D;\n"
        "  color: white;\n"
        "}\n"
        ".apg-remainder {\n"
        "  font-weight: bold;\n"
        "  color: gray;/* #999999 */\n"
        "}\n"
        ".apg-ctrl-char {\n"
        "  font-weight: bolder;\n"
        "  font-style: italic;\n"
        "  font-size: .8em;\n"
        "  color: black;\n"
        "}\n"
        ".apg-line-end {\n"
        "  font-weight: bold;\n"
        "  color: #000000;\n"
        "}\n"
        ".apg-error {\n"
        "  font-weight: bold;\n"
        "  color: #FF4000;\n"
        "}\n"
        ".apg-phrase {\n"
        "  color: #000000;\n"
        "  background-color: #8caae6;\n"
        "}\n"
        ".apg-empty-phrase {\n"
        "  color: #0fbd0f;\n"
        "}\n"
        "table.apg-state {\n"
        "  font-family: monospace;\n"
        "  margin-top: 5px;\n"
        "  font-size: 11px;\n"
        "  line-height: 130%;\n"
        "  text-align: left;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-state th,\n"
        "table.apg-state td {\n"
        "  text-align: left;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-state th:nth-last-child(2),\n"
        "table.apg-state td:nth-last-child(2) {\n"
        "  text-align: right;\n"
        "}\n"
        "table.apg-state caption {\n"
        "  font-size: 125%;\n"
        "  line-height: 130%;\n"
        "  font-weight: bold;\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-stats {\n"
        "  font-family: monospace;\n"
        "  margin-top: 5px;\n"
        "  font-size: 11px;\n"
        "  line-height: 130%;\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-stats th,\n"
        "table.apg-stats td {\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-stats caption {\n"
        "  font-size: 125%;\n"
        "  line-height: 130%;\n"
        "  font-weight: bold;\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-trace {\n"
        "  font-family: monospace;\n"
        "  margin-top: 5px;\n"
        "  font-size: 11px;\n"
        "  line-height: 130%;\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-trace caption {\n"
        "  font-size: 125%;\n"
        "  line-height: 130%;\n"
        "  font-weight: bold;\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-trace th,\n"
        "table.apg-trace td {\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-trace th:last-child,\n"
        "table.apg-trace th:nth-last-child(2),\n"
        "table.apg-trace td:last-child,\n"
        "table.apg-trace td:nth-last-child(2) {\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-grammar {\n"
        "  font-family: monospace;\n"
        "  margin-top: 5px;\n"
        "  font-size: 11px;\n"
        "  line-height: 130%;\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-grammar caption {\n"
        "  font-size: 125%;\n"
        "  line-height: 130%;\n"
        "  font-weight: bold;\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-grammar th,\n"
        "table.apg-grammar td {\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-grammar th:last-child,\n"
        "table.apg-grammar td:last-child {\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-rules {\n"
        "  font-family: monospace;\n"
        "  margin-top: 5px;\n"
        "  font-size: 11px;\n"
        "  line-height: 130%;\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-rules caption {\n"
        "  font-size: 125%;\n"
        "  line-height: 130%;\n"
        "  font-weight: bold;\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-rules th,\n"
        "table.apg-rules td {\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-rules a {\n"
        "  color: #003399 !important;\n"
        "}\n"
        "table.apg-rules a:hover {\n"
        "  color: #8caae6 !important;\n"
        "}\n"
        "table.apg-attrs {\n"
        "  font-family: monospace;\n"
        "  margin-top: 5px;\n"
        "  font-size: 11px;\n"
        "  line-height: 130%;\n"
        "  text-align: center;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-attrs caption {\n"
        "  font-size: 125%;\n"
        "  line-height: 130%;\n"
        "  font-weight: bold;\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-attrs th,\n"
        "table.apg-attrs td {\n"
        "  text-align: center;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-attrs th:nth-child(1),\n"
        "table.apg-attrs th:nth-child(2),\n"
        "table.apg-attrs th:nth-child(3) {\n"
        "  text-align: right;\n"
        "}\n"
        "table.apg-attrs td:nth-child(1),\n"
        "table.apg-attrs td:nth-child(2),\n"
        "table.apg-attrs td:nth-child(3) {\n"
        "  text-align: right;\n"
        "}\n"
        "table.apg-attrs a {\n"
        "  color: #003399 !important;\n"
        "}\n"
        "table.apg-attrs a:hover {\n"
        "  color: #8caae6 !important;\n"
        "}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n";
static const char* s_cpHtmlFooter =
        "<p class=\"apg-mono\">legend:<br>\n"
                "(a)&nbsp;-&nbsp;this line number<br>\n"
                "(b)&nbsp;-&nbsp;tree depth<br>\n"
                "(c)&nbsp;-&nbsp;trace depth<br>\n"
                "(d)&nbsp;-&nbsp;operator state<br>\n"
                "(e)&nbsp;-&nbsp;phrase offset<br>\n"
                "(f)&nbsp;-&nbsp;phrase length<br>\n"
                "&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;<span class=\"apg-active\">&darr;</span>&nbsp;&nbsp;phrase opened<br>\n"
                "&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;<span class=\"apg-match\">&uarr;M</span> phrase matched<br>\n"
                "&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;<span class=\"apg-empty\">&uarr;E</span> phrase matched empty (phrase length = 0)<br>\n"
                "&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;<span class=\"apg-nomatch\">&uarr;N</span> phrase not matched<br>\n"
                "operator&nbsp;-&nbsp;ALT, CAT, REP, RNM, TRG, TLS, TBS<sup>&dagger;</sup>, UDT, AND, NOT, BKA, BKN, BKR, ABG, AEN<sup>&Dagger;</sup><br>\n"
                "phrase&nbsp;&nbsp;&nbsp;-&nbsp;up to 120 characters of the phrase being matched<br>\n"
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;<span class=\"apg-match\">matched characters</span><br>\n"
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;<span class=\"apg-lh-match\">matched characters in look ahead mode</span><br>\n"
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;<span class=\"apg-lb-match\">matched characters in look behind mode</span><br>\n"
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;<span class=\"apg-remainder\">remainder characters(not yet examined by parser)</span><br>\n"
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;<span class=\"apg-ctrl-char\">control characters, TAB, LF, CR, etc. (ASCII mode only)</span><br>\n"
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;<span class=\"apg-empty\">&#120634;</span> empty string<br>\n"
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;<span class=\"apg-line-end\">&bull;</span> end of input string<br>\n"
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-&nbsp;<span class=\"apg-line-end\">&hellip;</span> input string display truncated<br>\n"
                "</p>\n"
                "<p class=\"apg-mono\">\n"
                "<sup>&dagger;</sup>original ABNF operators:<br>\n"
                "ALT - alternation<br>\n"
                "CAT - concatenation<br>\n"
                "REP - repetition<br>\n"
                "RNM - rule name<br>\n"
                "TRG - terminal range<br>\n"
                "TLS - terminal literal string (case insensitive)<br>\n"
                "TBS - terminal binary string (case sensitive)<br>\n"
                "<br>\n"
                "<sup>&Dagger;</sup>super set SABNF operators:<br>\n"
                "UDT - user-defined terminal<br>\n"
                "AND - positive look ahead<br>\n"
                "NOT - negative look ahead<br>\n"
                "BKA - positive look behind<br>\n"
                "BKN - negative look behind<br>\n"
                "BKR - back reference<br>\n"
                "ABG - anchor - begin of input string<br>\n"
                "AEN - anchor - end of input string<br>\n"
                "</p>\n"
                "</body>\n"
                "</html>\n";

#endif /* APG_TRACE */
