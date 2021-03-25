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
/** \file attributes.c
 * \brief The main functions driving the attributes determination.
 *
 * For each rule in the SABNF grammar the rule attributes are:<br>
 *  - left - if true, rule is left recursive (fatal)
 *  - nested - if true, rule is nested recursive (is not a regular expression)
 *  - right - if true, rule is right recursive
 *  - cyclic - if true, at least one branch has no terminal nodes (fatal)
 *  - empty - if true, the rule matches the empty string
 *  - finite - if false, the rule only matches infinite strings (fatal)
*/

#include "./api.h"
#include "./apip.h"
#include "./attributes.h"

static const char* s_cpTrue = "yes";
static const char* s_cpFalse = "no";
static const char* s_cpFatal = "error";
static const char* s_cpEmpty = "empty";
static const char* s_cpUndef = "undef";
static const void* s_vpMagicNumber = (void*)"attributes";

static int iCompName(const void* vpL, const void* vpR);
static int iCompType(const void* vpL, const void* vpR);
static const char * cpEmpty(abool aTF);
static const char * cpBool(abool aTF);
static const char * cpShouldBeTrue(abool aTF);
static const char * cpShouldBeFalse(abool aTF);
static attrs_ctx* spAttrsCtor(api* spApi);

/**
 * \brief Computes the grammar's attributes.
 *
 * For each rule in the SABNF grammar the rule attributes are:<br>
 *  - left - if true, rule is left recursive (fatal)
 *  - nested - if true, rule is nested recursive (is not a regular expression)
 *  - right - if true, rule is right recursive
 *  - cyclic - if true, at least one branch has no terminal nodes (fatal)
 *  - empty - if true, the rule matches the empty string
 *  - finite - if false, the rule only matches infinite strings (fatal)
 *
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param uipCount - Pointer to an integer. Set to the number (count) of attributes.
 * This will always be the number of rules in the grammar.
 * May be NULL if no count is wanted.
 * \return Returns a pointer to an array of api_attr structures, one for each rule in the grammar.
 * Throws exception on fatal error.
 */
abool bApiAttrs(void* vpCtx) {
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spApi = (api*) vpCtx;
    // validate the prerequisites
    if(!spApi->bSemanticsValid){
        XTHROW(spApi->spException,
                "attempting to compute attributes before semantics (opcodes) are complete");
    }

    attrs_ctx* spAtt = NULL;
    spAtt = spAttrsCtor(spApi);

    // compute rule dependencies
    vRuleDependencies(spAtt);

    // compute the rule attributes
    vRuleAttributes(spAtt);

    spApi->bAttributesComputed = APG_TRUE;
    if(spAtt->uiErrorCount){
        return APG_FALSE;
    }
    spApi->bAttributesValid = APG_TRUE;
    return APG_TRUE;
}

/** \brief Construct an attribute object.
 *
 * This is a "sub-object". Convenient for the various pieces of work needed to be done.
 * However, it shares the memory component with the API and no destructor is needed.
 * Fatal errors result in exceptions thrown to the caller's main program using the API.
 * \param spApi - Pointer to an API context previously returned from vpApiCtor().
 * Note that this is used internally and is already cast as an API object pointer.
 * \return Pointer to the attributes context. Throws exception on fatal error.
 */
static attrs_ctx* spAttrsCtor(api* spApi) {
    if(!bApiValidate(spApi)){
        vExContext();
    }
    attrs_ctx* spCtx = NULL;
    aint ui;
    spCtx = (attrs_ctx*) vpMemAlloc(spApi->vpMem, (aint) sizeof(attrs_ctx));
    memset((void*) spCtx, 0, sizeof(attrs_ctx));
    spCtx->vpMem = spApi->vpMem;
    spCtx->spException = spMemException(spApi->vpMem);
    spCtx->spApi = spApi;
    spApi->vpAttrsCtx = (void*)spCtx;

    // allocate the working array of attributes
    spCtx->spWorkingAttrs = (api_attr_w*) vpMemAlloc(spCtx->vpMem, (aint) ((aint)sizeof(api_attr_w) * spApi->uiRuleCount));
    memset((void*) spCtx->spWorkingAttrs, 0, (aint) ((aint)sizeof(api_attr_w) * spApi->uiRuleCount));

    // allocate the final array of attributes
    spCtx->spAttrs = (api_attr_w*) vpMemAlloc(spCtx->vpMem, (aint) ((aint)sizeof(api_attr_w) * spApi->uiRuleCount));
    memset((void*) spCtx->spAttrs, 0, (aint) ((aint)sizeof(api_attr_w) * spApi->uiRuleCount));

    // allocate the array of public attributes
    spCtx->spPublicAttrs = (api_attr*) vpMemAlloc(spCtx->vpMem, (aint) ((aint)sizeof(api_attr) * spApi->uiRuleCount));
    memset((void*) spCtx->spPublicAttrs, 0, (aint) ((aint)sizeof(api_attr) * spApi->uiRuleCount));

    // allocate the array of public error attributes
    spCtx->spErrorAttrs = (api_attr*) vpMemAlloc(spCtx->vpMem, (aint) ((aint)sizeof(api_attr) * spApi->uiRuleCount));
    memset((void*) spCtx->spErrorAttrs, 0, (aint) ((aint)sizeof(api_attr) * spApi->uiRuleCount));

    // initialize the working array of attributes
    for (ui = 0; ui < spApi->uiRuleCount; ui++) {
        spCtx->spWorkingAttrs[ui].cpRuleName = spApi->spRules[ui].cpName;
        spCtx->spWorkingAttrs[ui].uiRuleIndex = spApi->spRules[ui].uiIndex;

        // get the array of "rules referred to"
        spCtx->spWorkingAttrs[ui].bpRefersTo = (abool*) vpMemAlloc(spCtx->vpMem,
                (aint) (sizeof(abool) * spApi->uiRuleCount));
        memset((void*) spCtx->spWorkingAttrs[ui].bpRefersTo, 0, (sizeof(abool) * spApi->uiRuleCount));

        // get the array of "referenced by rules"
        spCtx->spWorkingAttrs[ui].bpIsReferencedBy = (abool*) vpMemAlloc(spCtx->vpMem,
                (aint) (sizeof(abool) * spApi->uiRuleCount));
        memset((void*) spCtx->spWorkingAttrs[ui].bpIsReferencedBy, 0, (sizeof(abool) * spApi->uiRuleCount));

        if(spApi->uiUdtCount){
            // get the array of "UDTs referred to"
            spCtx->spWorkingAttrs[ui].bpRefersToUdt = (abool*) vpMemAlloc(spCtx->vpMem,
                    (aint) (sizeof(abool) * spApi->uiUdtCount));
            memset((void*) spCtx->spWorkingAttrs[ui].bpRefersToUdt, 0, (sizeof(abool) * spApi->uiUdtCount));
        }
    }

    // create the vector of mutually-recursive group numbers
    spCtx->vpVecGroupNumbers = vpVecCtor(spCtx->vpMem, (aint)sizeof(aint), 10);

    // success
    spCtx->vpValidate = s_vpMagicNumber;
    return spCtx;
}

/** \brief The API object destructor.
 *
 * Since this is a "sub-object" which shares the memory context with its parent API object
 * this destructor simply frees all memory associated with it and clears the context.
 * The API destructor will also free all memory associated with this object.
 * \param vpCtx Pointer to an attributes context returned from spAttrsCtor().
 * NULL is silently ignored. However if non-NULL it must be a valid attributes context pointer.
 */
void vAttrsDtor(void* vpCtx) {
    if (vpCtx ) {
        attrs_ctx* spCtx = (attrs_ctx*) vpCtx;
        if(spCtx->vpValidate == s_vpMagicNumber){
            void* vpMem = spCtx->vpMem;
            vVecDtor(spCtx->vpVecGroupNumbers);
            aint ui = 0;
            for(; ui < spCtx->spApi->uiRuleCount; ui++){
                vMemFree(vpMem, spCtx->spWorkingAttrs[ui].bpRefersTo);
                vMemFree(vpMem, spCtx->spWorkingAttrs[ui].bpIsReferencedBy);
                vMemFree(vpMem, spCtx->spWorkingAttrs[ui].bpRefersToUdt);
           }
            vMemFree(vpMem, spCtx->spWorkingAttrs);
            vMemFree(vpMem, spCtx->spAttrs);
            vMemFree(vpMem, spCtx->spPublicAttrs);
            vMemFree(vpMem, spCtx->spErrorAttrs);
            spCtx->spApi->vpAttrsCtx = NULL;
            memset(vpCtx, 0, sizeof(attrs_ctx));
            vMemFree(vpMem, vpCtx);
        }else{
            vExContext();
        }
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
    if (!vpCtx || !bApiValidate(vpCtx)) {
        vExContext();
    }
    api *spApi = (api*) vpCtx;
    if (!spApi->bAttributesComputed) {
        XTHROW(spApi->spException,
                "no attributes available - must first call bApiAttrs()");
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
            vAttrsByName(spAtt->spPublicAttrs, spApi->uiRuleCount, spOut);
        } else if (cpMode[0] == 't' || cpMode[0] == 'T') {
            vAttrsByType(spAtt->spPublicAttrs, spApi->uiRuleCount, spOut);
        } else {
            // default to index
            vAttrsByIndex(spAtt->spPublicAttrs, spApi->uiRuleCount, spOut);
        }
    }else{
        // default to index
        vAttrsByIndex(spAtt->spPublicAttrs, spApi->uiRuleCount, spOut);
    }
    if(spOut != stdout){
        fclose(spOut);
    }
}

/** \brief Display all rule attributes with errors.
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
void vApiAttrsErrorsToAscii(void *vpCtx, const char *cpMode, const char* cpFileName) {
    if (!vpCtx || !bApiValidate(vpCtx)) {
        vExContext();
    }
    api *spApi = (api*) vpCtx;
    if (!spApi->bAttributesComputed) {
        XTHROW(spApi->spException,
                "no attributes available - must first call bApiAttrs()");
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
    fprintf(spOut, "ATTRIBUTE ERRORS\n");
    if(spAtt->uiErrorCount){
        if(cpMode){
            if (cpMode[0] == 'a' || cpMode[0] == 'A') {
                vAttrsByName(spAtt->spErrorAttrs, spAtt->uiErrorCount, spOut);
            } else if (cpMode[0] == 't' || cpMode[0] == 'T') {
                vAttrsByType(spAtt->spErrorAttrs, spAtt->uiErrorCount, spOut);
            } else {
                // default to index
                vAttrsByIndex(spAtt->spErrorAttrs, spAtt->uiErrorCount, spOut);
            }
        }else{
            // default to index
            vAttrsByIndex(spAtt->spErrorAttrs, spAtt->uiErrorCount, spOut);
        }
    }else{
        fprintf(spOut, "<none>\n");
    }
    if(spOut != stdout){
        fclose(spOut);
    }
}

static void vPrintOneAttrByName(api_attr* a, FILE* spStream){
    fprintf(spStream, "%7s |%7s |%7s |%7s |%7s |%7s | %s\n",
            cpShouldBeFalse(a->bLeft), cpBool(a->bNested), cpBool(a->bRight),
            cpShouldBeFalse(a->bCyclic),
            cpEmpty(a->bEmpty), cpShouldBeTrue(a->bFinite),
            a->cpRuleName);
}
static void vPrintOneAttrByType(api_attr* a, FILE* spStream){
    if(a->uiRecursiveType == ID_ATTR_MR){
        fprintf(spStream, "%7s |%7s |%7s |%7s |%7s |%7s |%7"PRIuMAX" |%7s | %s\n",
                cpShouldBeFalse(a->bLeft), cpBool(a->bNested), cpBool(a->bRight),
                cpShouldBeFalse(a->bCyclic),
                cpEmpty(a->bEmpty), cpShouldBeTrue(a->bFinite),
                (luint)a->uiMRGroup, cpType(a->uiRecursiveType), a->cpRuleName);
    }else{
        fprintf(spStream, "%7s |%7s |%7s |%7s |%7s |%7s |        |%7s | %s\n",
                cpShouldBeFalse(a->bLeft), cpBool(a->bNested), cpBool(a->bRight),
                cpShouldBeFalse(a->bCyclic),
                cpEmpty(a->bEmpty), cpShouldBeTrue(a->bFinite),
                cpType(a->uiRecursiveType), a->cpRuleName);
    }
}
/** \brief Display the attributes sorted by attribute type.
 * \param spAtt Pointer to an attributes context returned from spAttrsCtor().
 * This function is only called internally, hence the context pointer is already cast correctly.
 * \param spStream An open stream to display to.
 */
void vAttrsByType(api_attr* spAttrs, aint uiCount, FILE* spStream){
    aint ui;
    api_attr sAttrs[uiCount];
    for(ui = 0; ui < uiCount; ui++){
        sAttrs[ui] = spAttrs[ui];
    }
    qsort((void*)&sAttrs[0], uiCount, sizeof(api_attr), iCompName);
    qsort((void*)&sAttrs[0], uiCount, sizeof(api_attr), iCompType);
    fprintf(spStream, "ATTRIBUTES BY TYPE\n");
    fprintf(spStream, "   left | nested |  right | cyclic |  empty | finite |  group |   type |   name\n");
    fprintf(spStream, "--------|--------|--------|--------|--------|--------|--------|--------|--------\n");
    for(ui = 0; ui < uiCount; ui++){
        vPrintOneAttrByType(&sAttrs[ui], spStream);
    }
    fprintf(spStream, "\n");
}
/** \brief Display the attributes sorted by rule name.
 * \param spAtt Pointer to an attributes context returned from spAttrsCtor().
 * This function is only called internally, hence the context pointer is already cast correctly.
 * \param spStream An open stream to display to.
 */
void vAttrsByName(api_attr* spAttrs, aint uiCount, FILE* spStream){
    aint ui;
    api_attr sAttrs[uiCount];
    for(ui = 0; ui < uiCount; ui++){
        sAttrs[ui] = spAttrs[ui];
    }
    qsort((void*)&sAttrs[0], uiCount, sizeof(api_attr), iCompName);

    fprintf(spStream, "ATTRIBUTES BY NAME\n");
    fprintf(spStream, "   left | nested |  right | cyclic |  empty | finite |   name\n");
    fprintf(spStream, "--------|--------|--------|--------|--------|--------|--------\n");
    for(ui = 0; ui < uiCount; ui++){
        vPrintOneAttrByName(&sAttrs[ui], spStream);
    }
    fprintf(spStream, "\n");
}
/** \brief Display the attributes sorted by rule index.
 * \param spAtt Pointer to an attributes context returned from spAttrsCtor().
 * This function is only called internally, hence the context pointer is already cast correctly.
 * \param spStream An open stream to display to.
 */
void vAttrsByIndex(api_attr* spAttrs, aint uiCount, FILE* spStream){
    aint ui;
    fprintf(spStream, "ATTRIBUTES BY INDEX\n");
    fprintf(spStream, "   left | nested |  right | cyclic |  empty | finite |   name\n");
    fprintf(spStream, "--------|--------|--------|--------|--------|--------|--------\n");
    for(ui = 0; ui < uiCount; ui++){
        vPrintOneAttrByName(&spAttrs[ui], spStream);
    }
    fprintf(spStream, "\n");
}

static int iCompName(const void* vpL, const void* vpR) {
    api_attr* spL = (api_attr*) vpL;
    api_attr* spR = (api_attr*) vpR;
    aint uiLenL = strlen(spL->cpRuleName);
    aint uiLenR = strlen(spR->cpRuleName);
    char l, r;
    const char* cpL = spL->cpRuleName;
    const char* cpR = spR->cpRuleName;
    aint uiLesser = uiLenL < uiLenR ? uiLenL : uiLenR;
    while (uiLesser--) {
        l = *cpL;
        if (l >= 65 && l <= 90) {
            l += 32;
        }
        r = *cpR;
        if (r >= 65 && r <= 90) {
            r += 32;
        }
        if (l < r) {
            return -1;
        }
        if (l > r) {
            return 1;
        }
        cpL++;
        cpR++;
    }
    if (uiLenL < uiLenR) {
        return -1;
    }
    if (uiLenL > uiLenR) {
        return 1;
    }
    return 0;
}
static const char * cpBool(abool aTF){
    if(aTF == APG_TRUE){
        return s_cpTrue;
    }
    if(aTF == APG_FALSE){
        return s_cpFalse;
    }
    return s_cpUndef;
}
static const char * cpEmpty(abool aTF){
    if(aTF == APG_TRUE){
        return s_cpEmpty;
    }
    if(aTF == APG_FALSE){
        return s_cpFalse;
    }
    return s_cpUndef;
}
static const char * cpShouldBeTrue(abool aTF){
    if(aTF == APG_TRUE){
        return s_cpTrue;
    }
    if(aTF == APG_FALSE){
        return s_cpFatal;
    }
    return s_cpUndef;
}
static const char * cpShouldBeFalse(abool aTF){
    if(aTF == APG_TRUE){
        return s_cpFatal;
    }
    if(aTF == APG_FALSE){
        return s_cpFalse;
    }
    return s_cpUndef;
}
static int iCompType(const void* vpL, const void* vpR) {
    api_attr* spL = (api_attr*)vpL;
    api_attr* spR = (api_attr*)vpR;
    if(spL->uiRecursiveType == ID_ATTR_MR && spR->uiRecursiveType == ID_ATTR_MR){
        if(spL->uiMRGroup < spR->uiMRGroup){
            return -1;
        }
        if(spL->uiMRGroup > spR->uiMRGroup){
            return 1;
        }
        return 0;
    }
    if(spL->uiRecursiveType < spR->uiRecursiveType){
        return -1;
    }
    if(spL->uiRecursiveType > spR->uiRecursiveType){
        return 1;
    }
    return 0;
}
/** \brief Convert an attribute type ID to an ASCII string.
 * \param uiId An attribute type ID (ID_ATTR_N, ID_ATTR_R or ID_ATTR_MF)
 */
const char * cpType(aint uiId){
    char* cpReturn = "UNKNOWN";
    switch(uiId){
    case ID_ATTR_N:
        cpReturn = "N";
        break;
    case ID_ATTR_R:
        cpReturn = "R";
        break;
    case ID_ATTR_MR:
        cpReturn = "MR";
        break;
    }
    return cpReturn;
}

