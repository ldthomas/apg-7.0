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
/** \anchor apictor */

/** \file api.c
 * \brief Some basic functions of the APG Application Programming Interface.
 *
 * NOTE: The macro APG_AST must be defined for compliation of the APG A{I;
 * The Abstract Syntax Tree (AST) is used for syntax translation. APG_AST must be defined to allow its use.
 */

#include "./api.h"
#include "./apip.h"
#include "./attributes.h"

static const void* s_vpMagicNumber = (void*)"API";

static void vRulesFooter(FILE *spOut);
static void vRulesHeader(FILE *spOut);
static int iCompRules(const void *vpL, const void *vpR);
static int iCompUdts(const void *vpL, const void *vpR);

/** \brief Construct an API component context (object).
 * \param spEx Pointer to a valid exception object. See XCTOR().
 * If invalid, the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Pointer to an API context. Exception thrown on memory allocation error.
 */
void* vpApiCtor(exception* spEx) {
    if(!bExValidate(spEx)){
        vExContext();
    }
    void* vpMem = vpMemCtor(spEx);
    api* spCtx = (api*) vpMemAlloc(vpMem, (aint) sizeof(api));
    memset((void*) spCtx, 0, sizeof(api));
    spCtx->vpMem = vpMem;
    spCtx->spException = spEx;
    spCtx->vpLog = vpMsgsCtor(spEx);

    // create the input vector
    spCtx->vpAltStack = vpVecCtor(spCtx->vpMem, sizeof(alt_data), 100);
    spCtx->vpVecGrammar = vpVecCtor(spCtx->vpMem, (aint) sizeof(achar), 512);
    spCtx->vpVecInput = vpVecCtor(spCtx->vpMem, (aint) sizeof(char), 5120);
    spCtx->vpVecTempChars = vpVecCtor(spCtx->vpMem, (aint) sizeof(char), 1024);
    char cZero = 0;
    spCtx->cpInput = (char*)vpVecPush(spCtx->vpVecInput, &cZero);
    spCtx->uiInputLength = 0;

    spCtx->vpValidate = s_vpMagicNumber;
    return (void*) spCtx;
}

/** \brief The API component destructor
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \return Silently ignores NULL context pointer.
 * However, if non-NULL the API context pointer must be valid.
 */
void vApiDtor(void *vpCtx) {
    if(vpCtx){
        api *spCtx = (api*) vpCtx;
        if (spCtx->vpValidate == s_vpMagicNumber) {
            void *vpMem = spCtx->vpMem;
            vLinesDtor(spCtx->vpLines);
            vMsgsDtor(spCtx->vpLog);
            vParserDtor(spCtx->vpParser);
            memset(vpCtx, 0, sizeof(api));
            vMemDtor(vpMem);
        }else{
            vExContext();
        }
    }
}

/** \brief Validates an API context pointer.
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \return True is the context pointer is valid, false otherwise.
 */
abool bApiValidate(void *vpCtx){
    if(vpCtx && ((api*)vpCtx)->vpValidate == s_vpMagicNumber){
        return APG_TRUE;
    }
    return APG_FALSE;
}

/** \brief Get the internal message log.
 *
 * User may want to display or otherwise use the list of messages.
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \return Pointer to a message log context.
 */
void* vpApiGetErrorLog(void *vpCtx) {
    api *spApi = (api*) vpCtx;
    if (vpCtx && (spApi->vpValidate == s_vpMagicNumber)) {
        return spApi->vpLog;
    }
    vExContext();
    return NULL;
}

/** \brief Display the grammar rules in human-readable, HTML format.
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param cpFileName - Name of the file to display on.
 * Any directories in the path name must exist.
 * If NULL, stdout is used.
 */
void vApiRulesToHtml(void *vpCtx, const char *cpFileName) {
    api *spApi = (api*) vpCtx;
    if (!vpCtx || (spApi->vpValidate != s_vpMagicNumber)) {
        vExContext();
    }
    FILE *spOut = stdout;
    attrs_ctx *spAttrsCtx = (attrs_ctx*) spApi->vpAttrsCtx;
    if (!spApi->bAttributesValid || !spAttrsCtx) {
        XTHROW(spApi->spException,
                "cannot display rule dependencies without attributes (bApiAttrs())");
    }
    if (cpFileName) {
        spOut = fopen(cpFileName, "wb");
        if (!spOut) {
            char caBuf[126];
            snprintf(caBuf, 126, "cannot output open file name %s for writing", cpFileName);
            XTHROW(spApi->spException, caBuf);
        }
    }
    aint uiRuleCount = spApi->uiRuleCount;
    aint uiUdtCount = spApi->uiUdtCount;
    api_rule *spRules = spApi->spRules;
    api_udt *spUdt = spApi->spUdts;
    api_attr_w *spAttr = spAttrsCtx->spAttrs;
    aint ui, uj, uiCount;
    abool bFirst;
    api_rule saRules[uiRuleCount];

    vRulesHeader(spOut);
    // For each rule, find and display the list of rules that it refers to
    // and the list of rules that refer to it,
    for (ui = 0; ui < uiRuleCount; ui++, spAttr++) {
        fprintf(spOut, "rulesData[%"PRIuMAX"] = {\n", (luint) ui);
        fprintf(spOut, "name: \"%s\",\n", spAttr->cpRuleName);
        fprintf(spOut, "index: %"PRIuMAX",\n", (luint) spAttr->uiRuleIndex);
        fprintf(spOut, "to: [");
        uiCount = 0;
        for (uj = 0; uj < uiRuleCount; uj++) {
            if (spAttr->bpRefersTo[uj]) {
                saRules[uiCount] = spRules[uj];
                uiCount++;
            }
        }
        if (uiCount) {
            qsort(saRules, uiCount, sizeof(api_rule), iCompRules);
            bFirst = APG_TRUE;
            for (uj = 0; uj < uiCount; uj++) {
                if (bFirst) {
                    fprintf(spOut, "\"%s\"", saRules[uj].cpName);
                    bFirst = APG_FALSE;
                } else {
                    fprintf(spOut, ", \"%s\"", saRules[uj].cpName);
                }
            }
        }
        fprintf(spOut, "],\n");
        fprintf(spOut, "by: [");
        uiCount = 0;
        for (uj = 0; uj < uiRuleCount; uj++) {
            if (spAttr->bpIsReferencedBy[uj]) {
                saRules[uiCount] = spRules[uj];
                uiCount++;
            }
        }
        if (uiCount) {
            qsort(saRules, uiCount, sizeof(api_rule), iCompRules);
            bFirst = APG_TRUE;
            for (uj = 0; uj < uiCount; uj++) {
                if (bFirst) {
                    fprintf(spOut, "\"%s\"", saRules[uj].cpName);
                    bFirst = APG_FALSE;
                } else {
                    fprintf(spOut, ", \"%s\"", saRules[uj].cpName);
                }
            }
        }
        fprintf(spOut, "]};\n");
    }
    if (uiUdtCount) {
        // For each UDT, find and display the list of rules that refer to it,
        spAttr = spAttrsCtx->spAttrs;
        for (ui = 0; ui < uiUdtCount; ui++, spUdt++) {
            fprintf(spOut, "udtsData[%"PRIuMAX"] = {\n", (luint) ui);
            fprintf(spOut, "name: \"%s\",\n", spUdt->cpName);
            fprintf(spOut, "index: %"PRIuMAX",\n", (luint) spUdt->uiIndex);
            fprintf(spOut, "by: [");
            uiCount = 0;
            spAttr = spAttrsCtx->spAttrs;
            for (uj = 0; uj < uiRuleCount; uj++, spAttr++) {
                if (spAttr->bpRefersToUdt[ui]) {
                    saRules[uiCount] = spRules[uj];
                    uiCount++;
                }
            }
            if (uiCount) {
                qsort(saRules, uiCount, sizeof(api_rule), iCompRules);
                bFirst = APG_TRUE;
                for (uj = 0; uj < uiCount; uj++) {
                    if (bFirst) {
                        fprintf(spOut, "\"%s\"", saRules[uj].cpName);
                        bFirst = APG_FALSE;
                    } else {
                        fprintf(spOut, ", \"%s\"", saRules[uj].cpName);
                    }
                }
            }

            fprintf(spOut, "]};\n");
        }
    }
    vRulesFooter(spOut);
    if(spOut != stdout){
        fclose(spOut);
    }
}
/** \brief Display rules and UDTs in human-readable format in ASCII format.
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param cpMode (note: the first character (case-insensitive) of the following options is all that is needed)
 *  - "index" sort rule names index (the order they appear in the grammar syntax)
 *  - "alpha" sort rule names alphabetically
 *  - NULL, empty string or any string not beginning with "i" or "a", defaults to "index"
 * \param cpFileName - Name of the file to display on.
 * Any directories in the path name must exist.
 * If NULL, stdout is used.
 */
void vApiRulesToAscii(void *vpCtx, const char *cpMode, const char *cpFileName) {
    api *spApi = (api*) vpCtx;
    if (!vpCtx || (spApi->vpValidate != s_vpMagicNumber)) {
        vExContext();
    }
    FILE *spOut = stdout;
    if (!spApi->bSemanticsValid) {
        XTHROW(spApi->spException,
                "cannot display rules until semantic phase is complete (bApiOpcodes())");
    }
    aint uiRuleCount = spApi->uiRuleCount;
    aint uiUdtCount = spApi->uiUdtCount;
    api_rule saRules[uiRuleCount];
    api_udt saUdts[uiUdtCount];
    aint ui;
    abool bAlpha = APG_FALSE;
    if (cpFileName) {
        spOut = fopen(cpFileName, "wb");
        if (!spOut) {
            char caBuf[126];
            snprintf(caBuf, 126, "cannot open file name %s for writing", cpFileName);
            XTHROW(spApi->spException, caBuf);
        }
    }
    if (cpMode) {
        if (*cpMode == 'a' || *cpMode == 'A') {
            bAlpha = APG_TRUE;
        }
    }
    for (ui = 0; ui < uiRuleCount; ui++) {
        saRules[ui] = spApi->spRules[ui];
    }
    if (bAlpha) {
        qsort((void*) saRules, (size_t) uiRuleCount, sizeof(api_rule), iCompRules);
        fprintf(spOut, "RULES BY ALPHABET\n");
    } else {
        fprintf(spOut, "RULES BY INDEX\n");
    }
    fprintf(spOut, " index | rule name\n");
    fprintf(spOut, "-------|----------\n");
    for (ui = 0; ui < uiRuleCount; ui++) {
        fprintf(spOut, "%6"PRIuMAX" | %s\n", (luint) saRules[ui].uiIndex, saRules[ui].cpName);
    }
    if (uiUdtCount) {
        fprintf(spOut, "\n");
        for (ui = 0; ui < uiUdtCount; ui++) {
            saUdts[ui] = spApi->spUdts[ui];
        }
        if (bAlpha) {
            qsort((void*) saUdts, (size_t) uiUdtCount, sizeof(api_udt), iCompUdts);
            fprintf(spOut, "UDTS BY ALPHABET\n");
        } else {
            fprintf(spOut, "UDTS BY INDEX\n");
        }
        fprintf(spOut, "index |  UDT name\n");
        fprintf(spOut, "------|----------\n");
        for (ui = 0; ui < uiUdtCount; ui++) {
            fprintf(spOut, "%6"PRIuMAX"| %s\n", (luint) saUdts[ui].uiIndex, saUdts[ui].cpName);
        }
    }
    fprintf(spOut, "\n");
    if (spOut != stdout) {
        fclose(spOut);
    }
}
/** \brief Display all opcodes in human-readable format
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param cpFileName - Name of the file to display on.
 * Any directories in the path name must exist.
 * If NULL, stdout is used.
 */
void vApiOpcodesToAscii(void *vpCtx, const char* cpFileName) {
    api *spApi = (api*) vpCtx;
    if (!vpCtx || (spApi->vpValidate != s_vpMagicNumber)) {
        vExContext();
    }
    if (!spApi->bSemanticsValid && spApi->vpParser) {
        XTHROW(spApi->spException,
                "cannot display opcodes until semantic phase is complete (bApiOpcodes())");
    }
    FILE *spOut = stdout;
    if(cpFileName){
        spOut = fopen(cpFileName, "wb");
        if (!spOut) {
            char caBuf[126];
            snprintf(caBuf, 126, "cannot open file name %s for writing", cpFileName);
            XTHROW(spApi->spException, caBuf);
        }
    }
    // opcodes
    aint uiRuleCount = spApi->uiRuleCount;
    aint ui, uj, uk, uc;
    aint *uipBeg;
    luint *luipBeg;
    api_rule *spRule;
    api_op *spOp = spApi->spOpcodes;
    fprintf(spOut, "OPCODES\n");
    uk = 0;
    for (ui = 0; ui < uiRuleCount; ui++) {
        spRule = &spApi->spRules[ui];
        fprintf(spOut, "rule: %"PRIuMAX": %s\n", (luint) ui, spRule->cpName);
        for (uj = 0; uj < spRule->uiOpCount; uj++, uk++) {
            spOp = &spApi->spOpcodes[uk];
            switch (spOp->uiId) {
            case ID_ALT:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "ALT: ");
                fprintf(spOut, "children: %"PRIuMAX":", (luint) spOp->uiChildCount);
                for (uc = 0; uc < spOp->uiChildCount; uc++, uipBeg++) {
                    if (uc == 0) {
                        fprintf(spOut, " %"PRIuMAX"", (luint) spOp->uipChildIndex[uc]);
                    } else {
                        fprintf(spOut, ", %"PRIuMAX"", (luint) spOp->uipChildIndex[uc]);

                    }
                }
                fprintf(spOut, "\n");
                break;
            case ID_CAT:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "CAT: ");
                fprintf(spOut, "children: %"PRIuMAX":", (luint) spOp->uiChildCount);
                for (uc = 0; uc < spOp->uiChildCount; uc++, uipBeg++) {
                    if (uc == 0) {
                        fprintf(spOut, " %"PRIuMAX"", (luint) spOp->uipChildIndex[uc]);
                    } else {
                        fprintf(spOut, ", %"PRIuMAX"", (luint) spOp->uipChildIndex[uc]);

                    }
                }
                fprintf(spOut, "\n");
                break;
            case ID_REP:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "REP: ");
                fprintf(spOut, "min: %"PRIuMAX": ", spOp->luiMin);
                if (spOp->luiMax == (luint) -1) {
                    fprintf(spOut, "max: infinity");
                } else {
                    fprintf(spOut, "max: %"PRIuMAX"", spOp->luiMax);
                }
                fprintf(spOut, "\n");
                break;
            case ID_RNM:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "RNM: ");
                fprintf(spOut, "%s", spApi->spRules[spOp->uiIndex].cpName);
                fprintf(spOut, "\n");
                break;
            case ID_TBS:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "TBS: ");
                luipBeg = spOp->luipAchar;
                fprintf(spOut, "\'");
                for (uc = 0; uc < spOp->uiAcharLength; uc++, luipBeg++) {
                    if (*luipBeg >= 32 && *luipBeg <= 126) {
                        fprintf(spOut, "%c", (char) *luipBeg);
                    } else {
                        fprintf(spOut, "0x%.2"PRIXMAX"", *luipBeg);
                    }
                }
                fprintf(spOut, "\'");
                fprintf(spOut, "\n");
                break;
            case ID_TLS:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "TLS: ");
                fprintf(spOut, "\"");
                luipBeg = spOp->luipAchar;
                for (uc = 0; uc < spOp->uiAcharLength; uc++, luipBeg++) {
                    fprintf(spOut, "%c", (char) *luipBeg);
                }
                fprintf(spOut, "\"");
                fprintf(spOut, "\n");
                break;
            case ID_TRG:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "TRG: ");
                fprintf(spOut, "min: %"PRIuMAX": ", spOp->luiMin);
                fprintf(spOut, "max: %"PRIuMAX"", spOp->luiMax);
                fprintf(spOut, "\n");
                break;
            case ID_UDT:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "UDT: ");
                fprintf(spOut, "%s", spApi->spUdts[spOp->uiIndex].cpName);
                fprintf(spOut, "\n");
                break;
            case ID_AND:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "AND: ");
                fprintf(spOut, "\n");
                break;
            case ID_NOT:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "NOT: ");
                fprintf(spOut, "\n");
                break;
            case ID_BKA:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "BKA: ");
                fprintf(spOut, "\n");
                break;
            case ID_BKN:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "BKN: ");
                fprintf(spOut, "\n");
                break;
            case ID_BKR:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "BKR: ");
                if (spOp->uiCase == ID_BKR_CASE_I) {
                    fprintf(spOut, "\\%%i");
                } else {
                    fprintf(spOut, "\\%%s");
                }
                if (spOp->uiMode == ID_BKR_MODE_U) {
                    fprintf(spOut, "%%u");
                } else {
                    fprintf(spOut, "%%p");
                }
                aint uiIndex = spOp->uiBkrIndex;
                if (uiIndex < spApi->uiRuleCount) {
                    fprintf(spOut, "%s", spApi->spRules[uiIndex].cpName);
                } else {
                    uiIndex -= spApi->uiRuleCount;
                    fprintf(spOut, "%s", spApi->spUdts[uiIndex].cpName);
                }
                fprintf(spOut, "\n");
                break;
            case ID_ABG:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "ABG: ");
                fprintf(spOut, "\n");
                break;
            case ID_AEN:
                fprintf(spOut, "%"PRIuMAX": ", (luint) uk);
                fprintf(spOut, "AEN: ");
                fprintf(spOut, "\n");
                break;
            }
        }
        fprintf(spOut, "\n");
    }
    if(spOut != stdout){
        fclose(spOut);
    }
}

/** \brief Display all rule attributes.
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param cpMode (note: the first character (case-insensitive) of the following options is all that is needed)
 *  - "index" sort attributes by rule name index (the order they appear in the grammar syntax)
 *  - "alpha" sort attributes by rule name alphabetically
 *  - "type" sort attributes by type (non-recursive, recursive, etc.). Rules are alphabetical within each type.
 *  - NULL, empty string or any string not beginning with "a" or "t" defaults to "index"
 * \param cpFileName - Name of the file to display on.
 * Any directories in the path name must exist.
 * If NULL, stdout is used.
 */
void vApiAttrsToAscii(void *vpCtx, const char *cpMode, const char* cpFileName) {
    api *spApi = (api*) vpCtx;
    if (!vpCtx || (spApi->vpValidate != s_vpMagicNumber)) {
        vExContext();
    }
    if (!spApi->bAttributesValid) {
        XTHROW(spApi->spException,
                "no attributes available - call the attributes constructor vApiAttrs()");
    }
    FILE *spOut = stdout;
    if(cpFileName){
        spOut = fopen(cpFileName, "wb");
        if (!spOut) {
            char caBuf[126];
            snprintf(caBuf, 126, "cannot open file name %s for writing", cpFileName);
            XTHROW(spApi->spException, caBuf);
        }
    }
    attrs_ctx *spAtt = (attrs_ctx*) spApi->vpAttrsCtx;
    if(cpMode){
        if (cpMode[0] == 'a' || cpMode[0] == 'A') {
            vAttrsByName(spAtt, spOut);
        } else if (cpMode[0] == 't' || cpMode[0] == 'T') {
            vAttrsByType(spAtt, spOut);
        } else {
            // default to index
            vAttrsByIndex(spAtt, spOut);
        }
    }else{
        // default to index
        vAttrsByIndex(spAtt, spOut);
    }
    if(spOut != stdout){
        fclose(spOut);
    }
}

/** \brief Quicky way to generate a parser from a grammar file.
 *
 * Calls all of the intermediate steps in one function.
 * Input is limited to a single file.
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param cpFileName - Name of the grammar file.
 * \param bStrict If true, only strictly ABNF (RFC 5234 & RFC7405) grammars allowed.
 * \param bPppt If true, Partially-Predictive Parsing Tables (PPPTs) are generated.
 * Note that in this single, collective call to generate a parser there is no
 * opportunity to protect any rules from PPPT replacement. If any rules need protecting
 * it will be necessary to do the full sequence of API calls.
 */
void vApiFile(void *vpCtx, const char *cpFileName, abool bStrict, abool bPppt){
    api *spApi = (api*) vpCtx;
    if (!vpCtx || (spApi->vpValidate != s_vpMagicNumber)) {
        vExContext();
    }
    vApiInClear(vpCtx);
    cpApiInFile(vpCtx, cpFileName);
    vApiInValidate(vpCtx, bStrict);
    vApiSyntax(vpCtx, bStrict);
    vApiOpcodes(vpCtx);
    spApiAttrs(vpCtx, NULL);
    if(bPppt){
        vApiPppt(vpCtx, NULL, 0);
    }
}
/** \brief Quicky way to generate a parser from a grammar string.
 *
 * Calls all of the intermediate steps in one function.
 * Input is limited to a single file.
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param cpString - Pointer to a string which contains the entire grammar.
 * \param bStrict If true, only strictly ABNF (RFC 5234 & RFC7405) grammars allowed.
 * \param bPppt If true, Partially-Predictive Parsing Tables (PPPTs) are generated.
 * Note that in this single, collective call to generate a parser there is no
 * opportunity to protect any rules from PPPT replacement. If any rules need protecting
 * it will be necessary to do the full sequence of API calls.
 */
void vApiString(void *vpCtx, const char *cpString, abool bStrict, abool bPppt){
    api *spApi = (api*) vpCtx;
    if (!vpCtx || (spApi->vpValidate != s_vpMagicNumber)) {
        vExContext();
    }
    vApiInClear(spApi);
    cpApiInString(spApi, cpString);
    vApiInValidate(vpCtx, bStrict);
    vApiSyntax(vpCtx, bStrict);
    vApiOpcodes(vpCtx);
    spApiAttrs(vpCtx, NULL);
    if(bPppt){
        vApiPppt(vpCtx, NULL, 0);
    }
}

/** \brief Prints an HTML header to an open file.
 * \param[out] spFile - pointer to an open file handle.
 * \param cpTitle - the HTML page title. If NULL a default will be used.
 * \return void
 */
void vHtmlHeader(FILE *spFile, const char *cpTitle) {
    if (spFile) {
        if (!cpTitle) {
            cpTitle = "APG generated HTML";
        }
        fprintf(spFile, "<!DOCTYPE html>\n");
        fprintf(spFile, "<html lang=\"en\">\n");
        fprintf(spFile, "<meta charset=\"utf-8\">\n");
        fprintf(spFile, "<title>\n");
        fprintf(spFile, "%s", cpTitle);
        fprintf(spFile, "</title>\n<style>\n");
        fprintf(spFile, "body{font-family: monospace; font-size: 1em;}\n");
        fprintf(spFile, "kbd{font-weight: bold; font-style: italic; color: red;}\n"); // errors
        fprintf(spFile, "var{color: #8A2BE2;}\n"); // control characters
        fprintf(spFile, "th{text-align: left;}\n");
        fprintf(spFile, "</style>\n");
        fprintf(spFile, "<body>\n");
    }
}

/** \brief Prints an HTML footer to an open file.
 * \param[out] spFile - pointer to an open file handle.
 * \return void
 */
void vHtmlFooter(FILE *spFile) {
    if (spFile) {
        fprintf(spFile, "</body>\n");
        fprintf(spFile, "</html>\n");
    }
}

static void vRulesHeader(FILE *spOut) {
    fprintf(spOut, "<!DOCTYPE html>\n");
    fprintf(spOut, "<!-- LICENSE:\n");
    fprintf(spOut, "-->\n");
    fprintf(spOut, "<html>\n");
    fprintf(spOut, " <head>\n");
    fprintf(spOut, "   <title>Rule Dependencies</title>\n");
    fprintf(spOut, "   <meta charset=\"UTF-8\">\n");
    fprintf(spOut, "   <meta name=\"viewport\" sContent=\"width=device-width, initial-scale=1.0\">\n");
    fprintf(spOut, "   <style>\n");
    fprintf(spOut, "     td{\n");
    fprintf(spOut, "         vertical-align: top;\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     caption{\n");
    fprintf(spOut, "         text-align: left;\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     ul{\n");
    fprintf(spOut, "         margin: 0;\n");
    fprintf(spOut, "         list-style: none;\n");
    fprintf(spOut, "         padding-left: 5px;\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     li{\n");
    fprintf(spOut, "         font-size: .8em;\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     .bold{\n");
    fprintf(spOut, "         font-weight: bold;\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     .tableButton, .closeButton{\n");
    fprintf(spOut, "         background-color:#ffffff;\n");
    fprintf(spOut, "         -moz-border-radius:28px;\n");
    fprintf(spOut, "         -webkit-border-radius:28px;\n");
    fprintf(spOut, "         border-radius:28px;\n");
    fprintf(spOut, "         border:1px solid #000000;\n");
    fprintf(spOut, "         cursor:pointer;\n");
    fprintf(spOut, "         color:#000000;\n");
    fprintf(spOut, "         font-family:Arial;\n");
    fprintf(spOut, "         font-size:12px;\n");
    fprintf(spOut, "         font-weight:bold;\n");
    fprintf(spOut, "         padding:1px 18px;\n");
    fprintf(spOut, "         text-decoration:none;\n");
    fprintf(spOut, "         outline: none;\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     .tableButton:hover, .closeButton:hover {\n");
    fprintf(spOut, "         background-color:lightgray;\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     .closeButton{\n");
    fprintf(spOut, "         margin: 8px 0px;\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "   </style>\n");
    fprintf(spOut, " </head>\n");
    fprintf(spOut, " <body>\n");
    fprintf(spOut, "   <div id=\"rulesTable\"></div>\n");
    fprintf(spOut, "   <div id=\"udtsTable\"></div>\n");
    fprintf(spOut, "   <script>\n");
    fprintf(spOut, "     var ASC = 0;\n");
    fprintf(spOut, "     var DESC = 1;\n");
    fprintf(spOut, "     var rulesNameToggle = ASC;\n");
    fprintf(spOut, "     var rulesIndexToggle = DESC;\n");
    fprintf(spOut, "     var udtsNameToggle = ASC;\n");
    fprintf(spOut, "     var udtsIndexToggle = DESC;\n");
    fprintf(spOut, "     var rulesData = [];\n");
    fprintf(spOut, "     var udtsData = [];\n");
    fprintf(spOut, "     function toggle(id) {\n");
    fprintf(spOut, "       var x = document.getElementById(id);\n");
    fprintf(spOut, "       if (x.style.display === \"none\") {\n");
    fprintf(spOut, "         x.style.display = \"block\";\n");
    fprintf(spOut, "       } else {\n");
    fprintf(spOut, "         x.style.display = \"none\";\n");
    fprintf(spOut, "       }\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     function closeAllRules() {\n");
    fprintf(spOut, "       for (var i = 0; i < rulesData.length; i++) {\n");
    fprintf(spOut, "         x = document.getElementById(\"to\" + i);\n");
    fprintf(spOut, "         if (x) {\n");
    fprintf(spOut, "           x.style.display = \"none\";\n");
    fprintf(spOut, "         }\n");
    fprintf(spOut, "         x = document.getElementById(\"by\" + i);\n");
    fprintf(spOut, "         if (x) {\n");
    fprintf(spOut, "           x.style.display = \"none\";\n");
    fprintf(spOut, "         }\n");
    fprintf(spOut, "       }\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     function closeAllUdts() {\n");
    fprintf(spOut, "       for (var i = 0; i < udtsData.length; i++) {\n");
    fprintf(spOut, "         x = document.getElementById(\"udt\" + i);\n");
    fprintf(spOut, "         if (x) {\n");
    fprintf(spOut, "           x.style.display = \"none\";\n");
    fprintf(spOut, "         }\n");
    fprintf(spOut, "       }\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     function nameSortAscending(l, r) {\n");
    fprintf(spOut, "       var li = l.name.toUpperCase();\n");
    fprintf(spOut, "       var ri = r.name.toUpperCase();\n");
    fprintf(spOut, "       var ret = 0;\n");
    fprintf(spOut, "       if (li > ri) {\n");
    fprintf(spOut, "         ret = 1;\n");
    fprintf(spOut, "       } else if (li < ri) {\n");
    fprintf(spOut, "         ret = -1;\n");
    fprintf(spOut, "       }\n");
    fprintf(spOut, "       return ret;\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     function nameSortDescending(l, r) {\n");
    fprintf(spOut, "       return -1 * nameSortAscending(l, r);\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     function tableSort(data, col) {\n");
    fprintf(spOut, "       if (data === \"rules\") {\n");
    fprintf(spOut, "         if (col === \"index\") {\n");
    fprintf(spOut, "           if (rulesIndexToggle === ASC) {\n");
    fprintf(spOut, "             rulesData.sort((l,r)=>(l.index - r.index));\n");
    fprintf(spOut, "             rulesIndexToggle = DESC;\n");
    fprintf(spOut, "           } else if (rulesIndexToggle === DESC) {\n");
    fprintf(spOut, "             rulesData.sort((l,r)=>(r.index - l.index));\n");
    fprintf(spOut, "             rulesIndexToggle = ASC;\n");
    fprintf(spOut, "           }\n");
    fprintf(spOut, "         } else if (col === \"name\") {\n");
    fprintf(spOut, "           if (rulesNameToggle === ASC) {\n");
    fprintf(spOut, "             rulesData.sort(nameSortAscending);\n");
    fprintf(spOut, "             rulesNameToggle = DESC;\n");
    fprintf(spOut, "           } else if (rulesNameToggle === DESC) {\n");
    fprintf(spOut, "             rulesData.sort(nameSortDescending);\n");
    fprintf(spOut, "             rulesNameToggle = ASC;\n");
    fprintf(spOut, "           }\n");
    fprintf(spOut, "         }\n");
    fprintf(spOut, "         rulesGen();\n");
    fprintf(spOut, "       } else if (data === \"udts\") {\n");
    fprintf(spOut, "         if (col === \"index\") {\n");
    fprintf(spOut, "           if (udtsIndexToggle === ASC) {\n");
    fprintf(spOut, "             udtsData.sort((l,r)=>(l.index - r.index));\n");
    fprintf(spOut, "             udtsIndexToggle = DESC;\n");
    fprintf(spOut, "           } else if (udtsIndexToggle === DESC) {\n");
    fprintf(spOut, "             udtsData.sort((l,r)=>(r.index - l.index));\n");
    fprintf(spOut, "             udtsIndexToggle = ASC;\n");
    fprintf(spOut, "           }\n");
    fprintf(spOut, "         } else if (col === \"name\") {\n");
    fprintf(spOut, "           if (udtsNameToggle === ASC) {\n");
    fprintf(spOut, "             udtsData.sort(nameSortAscending);\n");
    fprintf(spOut, "             udtsNameToggle = DESC;\n");
    fprintf(spOut, "           } else if (udtsNameToggle === DESC) {\n");
    fprintf(spOut, "             udtsData.sort(nameSortDescending);\n");
    fprintf(spOut, "             udtsNameToggle = ASC;\n");
    fprintf(spOut, "           }\n");
    fprintf(spOut, "         }\n");
    fprintf(spOut, "         udtsGen();\n");
    fprintf(spOut, "       }\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     function rulesGen() {\n");
    fprintf(spOut, "       var html = \"\";\n");
    fprintf(spOut, "       html += '<table id=\"rulesTable\">';\n");
    fprintf(spOut, "       html += '<caption><strong>Rule Dependencies</strong><br>';\n");
    fprintf(spOut,
            "       html += '<button class=\"closeButton\" onclick=\"closeAllRules()\">close all rules</button>';\n");
    fprintf(spOut, "       html += '<caption/>';\n");
    fprintf(spOut,
            "       html += '<tr><td class=\"tableButton\" onclick=\"tableSort(\\'rules\\', \\'index\\')\">index</td>';\n");
    fprintf(spOut,
            "       html += '<td class=\"tableButton\" onclick=\"tableSort(\\'rules\\', \\'name\\')\">name</td>';\n");
    fprintf(spOut, "       html += '<td class=\"bold\">dependencies</td></tr>';\n");
    fprintf(spOut, "       for (var i = 0; i < rulesData.length; i++) {\n");
    fprintf(spOut, "         var data = rulesData[i];\n");
    fprintf(spOut, "         html += \"<tr><td>\" + data.index + \"</td><td>\" + data.name + \"</td>\";\n");
    fprintf(spOut, "         if (data.to.length > 0) {\n");
    fprintf(spOut, "           var to = \"to\" + i;\n");
    fprintf(spOut,
            "           html += '<td><button class=\"tableButton\" onclick=\"toggle(\\'' + to + '\\')\">refers to</button><br>';\n");
    fprintf(spOut, "           html += '<ul id=\"' + to + '\">';\n");
    fprintf(spOut, "           for (var j = 0; j < data.to.length; j++) {\n");
    fprintf(spOut, "             html += \"<li>\" + data.to[j] + \"</li>\";\n");
    fprintf(spOut, "           }\n");
    fprintf(spOut, "           html += '</ul>';\n");
    fprintf(spOut, "           html += \"</td></tr>\";\n");
    fprintf(spOut, "         } else {\n");
    fprintf(spOut, "           html += \"<td><i>no referals</i></td></tr>\";\n");
    fprintf(spOut, "         }\n");
    fprintf(spOut, "         if (data.by.length > 0) {\n");
    fprintf(spOut, "           var by = \"by\" + i;\n");
    fprintf(spOut,
            "           html += '<tr><td></td><td></td><td><button class=\"tableButton\" onclick=\"toggle(\\'' + by + '\\')\">referenced by</button><br>';\n");
    fprintf(spOut, "           html += '<ul id=\"' + by + '\">';\n");
    fprintf(spOut, "           for (var j = 0; j < data.by.length; j++) {\n");
    fprintf(spOut, "             html += \"<li>\" + data.by[j] + \"</li>\";\n");
    fprintf(spOut, "           }\n");
    fprintf(spOut, "           html += '</ul>';\n");
    fprintf(spOut, "           html += \"</td><tr>\";\n");
    fprintf(spOut, "         } else {\n");
    fprintf(spOut, "           html += \"<tr><td></td><td></td><td><i>not referenced</i></td></tr>\";\n");
    fprintf(spOut, "         }\n");
    fprintf(spOut, "       }\n");
    fprintf(spOut, "       html += '</table>';\n");
    fprintf(spOut, "       var d = document.getElementById(\"rulesTable\");\n");
    fprintf(spOut, "       d.innerHTML = html;\n");
    fprintf(spOut, "       closeAllRules();\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     function udtsGen() {\n");
    fprintf(spOut, "       if (udtsData.length > 0) {\n");
    fprintf(spOut, "         var html = \"\";\n");
    fprintf(spOut, "         html += \"<p></p>\";\n");
    fprintf(spOut, "         html += '<table id=\"rulesTable\">';\n");
    fprintf(spOut, "         html += '<caption><strong>UDT Dependencies</strong><br>';\n");
    fprintf(spOut,
            "         html += '<button class=\"closeButton\" onclick=\"closeAllUdts()\">close all UDTS</button>';\n");
    fprintf(spOut, "         html += '<caption/>';\n");
    fprintf(spOut,
            "         html += '<tr><td class=\"tableButton\" onclick=\"tableSort(\\'udts\\', \\'index\\')\">index</td>';\n");
    fprintf(spOut,
            "         html += '<td class=\"tableButton\" onclick=\"tableSort(\\'udts\\', \\'name\\')\">name</td>';\n");
    fprintf(spOut, "         html += '<td class=\"bold\">dependencies</td><tr>';\n");
    fprintf(spOut, "         for (var i = 0; i < udtsData.length; i++) {\n");
    fprintf(spOut, "           var data = udtsData[i];\n");
    fprintf(spOut, "           html += \"<tr><td>\" + data.index + \"</td><td>\" + data.name + \"</td>\";\n");
    fprintf(spOut, "           if (data.by.length > 0) {\n");
    fprintf(spOut, "             var by = \"udt\" + i;\n");
    fprintf(spOut,
            "             html += '<td><button class=\"tableButton\" onclick=\"toggle(\\'' + by + '\\')\">referenced by</button><br>';\n");
    fprintf(spOut, "             html += '<ul id=\"' + by + '\">';\n");
    fprintf(spOut, "             for (var j = 0; j < data.by.length; j++) {\n");
    fprintf(spOut, "               html += \"<li>\" + data.by[j] + \"</li>\";\n");
    fprintf(spOut, "             }\n");
    fprintf(spOut, "             html += '</ul>';\n");
    fprintf(spOut, "             html += \"</td><tr>\";\n");
    fprintf(spOut, "           } else {\n");
    fprintf(spOut, "             html += \"<td><i>not referenced</i></td></tr>\";\n");
    fprintf(spOut, "           }\n");
    fprintf(spOut, "         }\n");
    fprintf(spOut, "         html += '</table>';\n");
    fprintf(spOut, "         var d = document.getElementById(\"udtsTable\");\n");
    fprintf(spOut, "         d.innerHTML = html;\n");
    fprintf(spOut, "         closeAllUdts();\n");
    fprintf(spOut, "       }\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     function setup() {\n");
    fprintf(spOut, "       if (rulesData.length > 0) {\n");
    fprintf(spOut, "         rulesGen();\n");
    fprintf(spOut, "       }\n");
    fprintf(spOut, "       if (udtsData.length > 0) {\n");
    fprintf(spOut, "         udtsGen();\n");
    fprintf(spOut, "       }\n");
    fprintf(spOut, "     }\n");
    fprintf(spOut, "     window.onload = setup;\n");
}
static void vRulesFooter(FILE *spOut) {
    fprintf(spOut, "</script>\n");
    fprintf(spOut, "</body>\n");
    fprintf(spOut, "</html>\n");
}
static int iCompRules(const void *vpL, const void *vpR) {
    api_rule *spL = (api_rule*) vpL;
    api_rule *spR = (api_rule*) vpR;
    return strcmp(spL->cpName, spR->cpName);
}
static int iCompUdts(const void *vpL, const void *vpR) {
    api_udt *spL = (api_udt*) vpL;
    api_udt *spR = (api_udt*) vpR;
    return strcmp(spL->cpName, spR->cpName);
}
