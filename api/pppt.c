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
/** \file pppt.c
 * \brief All of the code for generating Partially-Predictive Parsing Tables (PPPT).
 *
 * PPPTs can greatly reduce the number of node hits in the traversal of the parse tree
 * and thus greatly reduce the computation times. However, not all grammars lend themselves to this approach.
 * In particular, grammars with a large number of alphabet characters in the grammar will produce extremely large
 * PPPTs. In some cases, impractically large or even impossibly large.
 *
 * For example, if parsing a grammar that uses the full range of UTF-32 characters the alphabet character range
 * is 0x00 - 0x10FFFF. Not a good fit for PPPTs.
 */

// sanity check assertion
//#define ASSERT_PPPT
#ifdef ASSERT_PPPT
#include <assert.h>
#endif /* ASSERT_PPPT */

#include "../api/api.h"
#include "../api/apip.h"
#include "../api/attributes.h"

//#define TRACE_PPPT 1
#ifdef TRACE_PPPT
#include "../library/parserp.h"
#include "../utilities/utilities.h"
static aint s_uiTreeDepth = 0;
static void vRuleOpen(api* spApi, aint uiRuleIndex);
static void vRuleLeaf(api* spApi, aint uiRuleIndex, uint8_t* ucpMap);
static void vRuleClose(api* spApi, aint uiRuleIndex);
static void vOpcodeOpen(api* spApi, api_op* spOp);
static void vOpcodeClose(api* spApi, api_op* spOp);
static void vIndent(aint uiIndent);
static void vPrintMap(api* spApi, uint8_t* ucpMap);
#define TRACE_RULE_OPEN(n) vRuleOpen(spApi, (n))
#define TRACE_RULE_LEAF(n, m) vRuleLeaf(spApi, (n), (m))
#define TRACE_RULE_CLOSE(n) vRuleClose(spApi, (n))
#define TRACE_OPCODE_OPEN(o) vOpcodeOpen(spApi, (o))
#define TRACE_OPCODE_CLOSE(o) vOpcodeClose(spApi, (o))
#else
#define TRACE_RULE_OPEN(n)
#define TRACE_RULE_LEAF(n, m)
#define TRACE_RULE_CLOSE(n)
#define TRACE_OPCODE_OPEN(o)
#define TRACE_OPCODE_CLOSE(o)
#endif /* TRACE_PPPT */

static void vSetMapValGen(uint8_t* ucpMap, luint luiOffset, luint luiChar, uint8_t ucVal);
static uint8_t ucGetMapValGen(api* spApi, uint8_t* ucpMap, luint luiOffset, luint luiChar);
static void vGetMaps(api* spApi);
static int iCompOps(const void* vpL, const void* vpR);
static int iCompName(const void* vpL, const void* vpR);
static int iNameInsensitiveCompare(char* cpL, char* cpR);
static int iMatchRule(api_rule* spRule, aint uiRuleCount, char* cpName);
static void vCopyMap(uint8_t* ucpDst, uint8_t* ucpSrc, aint uiLen);
static void vClearMap(uint8_t* ucpMap, aint uiLen);
static void vRuleMap(api* spApi, aint uiRuleIndex, uint8_t* ucpMap);
static void vAltMap(api* spApi, api_op* spOp, uint8_t* ucpMap);
static void vCatMap(api* spApi, api_op* spOp, uint8_t* ucpMap);
static void vRepMap(api* spApi, api_op* spOp, uint8_t* ucpMap);
static void vOpcodeMap(api* spApi, api_op* spOp, uint8_t* ucpMap);

/** \brief Compute the Partially-Predictive Parsing Tables.
 *
 * \param vpCtx Context pointer previously returned from vpApiCtor().
 * \param cppProtectedRules An array of string pointers pointing to the rule names to protect.
 * Protection means that the rule is generated, even if the PPPT would have been deterministic prior to calling the rule.
 * May be NULL, in which case no rules are protected.
 * \param uiProtectedRules The number of protected rules.
 * May be 0, in which case no rules are protected.
 */
void vApiPppt(void* vpCtx, char** cppProtectedRules, aint uiProtectedRules){
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spApi = (api*) vpCtx;
    aint ui;
    api_op* spOp = NULL;
    if (!spApi->bSemanticsValid) {
        XTHROW(spApi->spException,
                "attempted PPPT construction but opcodes (vApiOpcodes()) have not been constructed");
    }

    // PPPT sizes computed in semantics - vApiOpcodes()
    // test to see if the maps are impossibly large
    if(spApi->luiAcharMax == (luint)-1){
        XTHROW(spApi->spException, "Partially-Predictive Parsing Tables cannot be used for this grammar. The maximum character is too large - 0xFFFFFFFFFFFFFFFF");
    }
    if(cppProtectedRules && uiProtectedRules){
        // protect all rules in protection list
        api_rule saRules[spApi->uiRuleCount];
        memcpy((void*)saRules, (void*)spApi->spRules, sizeof(saRules));
        qsort((void*)saRules, (size_t)spApi->uiRuleCount, sizeof(api_rule), iCompName);
        for(ui = 0; ui < uiProtectedRules; ui++){
            int iIndex = iMatchRule(saRules, spApi->uiRuleCount, cppProtectedRules[ui]);
            if(iIndex == -1){
                char caBuf[256];
                snprintf(caBuf, 256, "PPPT protected rules: %s is not a valid rule name", cppProtectedRules[ui]);
                XTHROW(spApi->spException, caBuf);
            }else{
                spApi->spRules[saRules[iIndex].uiIndex].bProtected = APG_TRUE;
            }
        }
        if(uiMsgsCount(spApi->vpLog)){
            XTHROW(spApi->spException, "PPPT protected rules have invalid rule names");
        }
    }

    // allocate and create the character, empty and undecided maps
    spApi->ucpPpptUndecidedMap = (uint8_t*)vpMemAlloc(spApi->vpMem, spApi->luiPpptMapSize * sizeof(uint8_t));
    memset((void*)spApi->ucpPpptUndecidedMap, ID_PPPT_ACTIVE, spApi->luiPpptMapSize * sizeof(uint8_t));
    spApi->ucpPpptEmptyMap = (uint8_t*)vpMemAlloc(spApi->vpMem, spApi->luiPpptMapSize * sizeof(uint8_t));
    memset((void*)spApi->ucpPpptEmptyMap, 0, spApi->luiPpptMapSize * sizeof(uint8_t));

    luint lu;
    spOp = spApi->spOpcodes;
    for(ui = 0; ui < spApi->uiOpcodeCount; ui++, spOp++){
        if(spOp->uiId == ID_TRG) {
            lu = spOp->luiMin;
            for(; lu <= spOp->luiMax; lu++){
                vSetMapValGen(spApi->ucpPpptEmptyMap, spApi->luiAcharMin, lu, ID_PPPT_EMPTY);
            }
        }else if(spOp->uiId == ID_TBS){
            vSetMapValGen(spApi->ucpPpptEmptyMap, spApi->luiAcharMin, *spOp->luipAchar, ID_PPPT_EMPTY);
        }else if(spOp->uiId == ID_TLS){
            if(spOp->uiAcharLength){
                vSetMapValGen(spApi->ucpPpptEmptyMap, spApi->luiAcharMin, *spOp->luipAchar, ID_PPPT_EMPTY);
                if(*spOp->luipAchar >= 97 && *spOp->luipAchar <= 122){
                    vSetMapValGen(spApi->ucpPpptEmptyMap, spApi->luiAcharMin, (*spOp->luipAchar - 32), ID_PPPT_EMPTY);
                }
            }
        }
    }
    vSetMapValGen(spApi->ucpPpptEmptyMap, spApi->luiAcharMin, spApi->luiAcharEos, ID_PPPT_EMPTY);

    // !!!! DEBUG
#ifdef TRACE_PPPT
    printf("\n");
    printf("CHARACTER MAP\n");
    printf("character | char val | undec val\n");
    printf("----------|----------|----------\n");
    uint8_t uc;
    for(lu = spApi->luiAcharMin; lu <= spApi->luiAcharMax; lu++){
        uc =0;
        printf("%9"PRIuMAX" | %8"PRIuMAX" | ", (luint)lu, (luint)uc);
        uc = ucGetMapValGen(spApi, spApi->ucpPpptUndecidedMap, spApi->luiAcharMin, lu);
        printf("%9"PRIuMAX"\n", (luint)uc);
    }
    uc =0;
    printf("%9s | %8"PRIuMAX" | ", "EOS", (luint)uc);
    uc = ucGetMapValGen(spApi, spApi->ucpPpptUndecidedMap, spApi->luiAcharMin, spApi->luiAcharEos);
    printf("%9"PRIuMAX"\n", (luint)uc);
#endif /* TRACE_PPPT */
    // !!!! DEBUG


    // set the opcode map indexes
    aint uiIndex = 0;
    aint uiMapSize = spApi->luiPpptMapSize;
    for(ui = 0; ui < spApi->uiRuleCount; ui++){
        spApi->spRules[ui].uiPpptIndex = uiIndex++ * uiMapSize;
    }
    spOp = spApi->spOpcodes;
    for(ui = 0; ui < spApi->uiOpcodeCount; ui++, spOp++){
        switch (spOp->uiId) {
        case ID_RNM:
            spOp->uiPpptIndex = spApi->spRules[spOp->uiIndex].uiPpptIndex;
            break;
        case ID_ALT:
        case ID_CAT:
        case ID_REP:
        case ID_TRG:
        case ID_TLS:
        case ID_TBS:
        case ID_AND:
        case ID_NOT:
            spOp->uiPpptIndex = uiIndex++ * uiMapSize;
            break;
            // these opcodes have no PPPT map
            // we can't predict what the user will do in a UDT
        case ID_UDT:
            // case insensitivity BKR it is possible for BKR to accept characters outside of AcharMin and AcharMax
        case ID_BKR:
            // look behind is iterative and impossible (or extremely difficult) to determine a PPPT map
        case ID_BKA:
        case ID_BKN:
            // anchor opcodes only examine the character position, not the character value
        case ID_ABG:
        case ID_AEN:
            break;
        default:
            XTHROW(spApi->spException, "unrecognized operator ID");
            break;
        }
    }

    // allocate the PPPT table
    spApi->ucpPpptTable = (uint8_t*)vpMemAlloc(spApi->vpMem, spApi->luiPpptTableLength);
    memset((void*)spApi->ucpPpptTable, 0, spApi->luiPpptTableLength);

    // compute all maps in the PPPT table
    vGetMaps(spApi);

    // success
    spApi->bUsePppt = APG_TRUE;
}

/** \brief Compute the size of the PPPT maps and the number of bytes for the entire table.
 *
 * This function may be called after vApiOpcodes() and before vApiPppt() to determine if the application
 * has sufficient memory to handle the PPPTs.
 * vApiOpcodes() is where the PPPT sizes are computed.
 * The tables are not allocated until vApiPppt().
 * \param vpCtx Context pointer previously returned from vpApiCtor().
 * \param spSize Pointer to a pppt_size structure to receive the size information.
 */
void vApiPpptSize(void *vpCtx, pppt_size* spSize){
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spApi = (api*) vpCtx;
    if (!spApi->bSemanticsValid) {
        XTHROW(spApi->spException, "this function may not be called prior to vApiOpcodes()");
    }
    if(!spSize){
        XTHROW(spApi->spException, "size pointer, spSize, may not be NULL");
    }
    spSize->luiAcharMax = spApi->luiAcharMax;
    spSize->luiAcharMin = spApi->luiAcharMin;
    spSize->luiMapSize = spApi->luiPpptMapSize;
    spSize->luiMaps = spApi->luiPpptMapCount;
    spSize->luiTableSize = spApi->luiPpptTableLength;
}

static void vSetMapValGen(uint8_t* ucpMap, luint luiOffset, luint luiChar, uint8_t ucVal){
    ucpMap[luiChar - luiOffset] = ucVal;
//    luint luiRelChar = luiChar - luiOffset;
//    luint luiMapIndex = luiRelChar >> 2;
//    luint luiValShift = (3 - (luiRelChar - (luiMapIndex << 2))) << 1;
//    ucpMap[luiMapIndex] &= ~(3 << luiValShift);
//    ucpMap[luiMapIndex] |= ucVal << luiValShift;
}
//static uint8_t s_ucGetMask[4] = {0xC0, 0x30, 0xC,0x3};
//static uint8_t s_ucGetShift[4] = {6,4,2,0};
static uint8_t ucGetMapValGen(api* spApi, uint8_t* ucpMap, luint luiOffset, luint luiChar){
    // sanity check
    if((luiChar < luiOffset) || ((luiChar - luiOffset) >= (luint)spApi->luiPpptMapSize)){
        XTHROW(spApi->spException, "bad character value");
    }
    return ucpMap[luiChar - luiOffset];
//    luint luiRelChar = luiChar - luiOffset;
//    luint luiMapIndex = luiRelChar >> 2;
//    uint8_t ucByteIndex = (uint8_t)(luiRelChar - (luiMapIndex << 2));
//    return (ucpMap[luiMapIndex] & s_ucGetMask[ucByteIndex]) >> s_ucGetShift[ucByteIndex];
}

static void vCopyMap(uint8_t* ucpDst, uint8_t* ucpSrc, aint uiLen){
    uint8_t* ucpEnd = ucpDst + uiLen;
    while(ucpDst < ucpEnd){
        *ucpDst++ = *ucpSrc++;
    }
}
static void vClearMap(uint8_t* ucpMap, aint uiLen){
    memset((void*)ucpMap, 0, sizeof(uint8_t) * uiLen);
}

static void vGetMaps(api* spApi){
    aint ui;
    uint8_t ucaMap[spApi->luiPpptMapSize];
    api_rule saRules[spApi->uiRuleCount];
    memcpy((void*)saRules, (void*)spApi->spRules, sizeof(saRules));

    // order the rules ascending on their opcode count - want to process the smallest rules first.
    qsort((void*)saRules, (size_t)spApi->uiRuleCount, sizeof(api_rule), iCompName);
    qsort((void*)saRules, (size_t)spApi->uiRuleCount, sizeof(api_rule), iCompOps);

#ifdef TRACE_PPPT
    api_rule* spRule = saRules;
    printf("opcode count | name\n");
    printf("------------ | ----\n");
    for(ui = 0; ui < spApi->uiRuleCount; ui++, spRule++){
        printf("%12lu | %s\n", (luint)spRule->uiOpCount, spRule->cpName);
    }
#endif /* TRACE_PPPT */

    // compute PPPTs for all opcodes of all rules
    for(ui = 0; ui < spApi->uiRuleCount; ui++){
        vRuleMap(spApi, saRules[ui].uiIndex, ucaMap);
    }
}

static void vRuleMap(api* spApi, aint uiRuleIndex, uint8_t* ucpMap){
    TRACE_RULE_OPEN(uiRuleIndex);
    uint8_t* ucpRuleMap = spApi->ucpPpptTable + spApi->spRules[uiRuleIndex].uiPpptIndex;
    if(spApi->spRules[uiRuleIndex].bIsComplete){
        vCopyMap(ucpMap, ucpRuleMap, spApi->luiPpptMapSize);
        TRACE_RULE_CLOSE(uiRuleIndex);
    }else if(spApi->spRules[uiRuleIndex].bIsOpen){
        memcpy((void*)ucpMap, (void*)spApi->ucpPpptUndecidedMap, (sizeof(uint8_t) * spApi->luiPpptMapSize));
        TRACE_RULE_LEAF(uiRuleIndex, ucpMap);
    }else{
        spApi->spRules[uiRuleIndex].bIsOpen = APG_TRUE;
        api_op* spOp = &spApi->spOpcodes[spApi->spRules[uiRuleIndex].uiOpOffset];
        vOpcodeMap(spApi, spOp, ucpMap);
        if(spApi->spRules[uiRuleIndex].bProtected){
            memcpy((void*)ucpRuleMap, (void*)spApi->ucpPpptUndecidedMap, (sizeof(uint8_t) * spApi->luiPpptMapSize));
        }else{
            memcpy((void*)ucpRuleMap, (void*)ucpMap, (sizeof(uint8_t) * spApi->luiPpptMapSize));
        }
        vCopyMap(ucpMap, ucpRuleMap, spApi->luiPpptMapSize);
        spApi->spRules[uiRuleIndex].bIsComplete = APG_TRUE;
        spApi->spRules[uiRuleIndex].bIsOpen = APG_FALSE;
        TRACE_RULE_CLOSE(uiRuleIndex);
    }
}

/** Evaluated the PPPT for the ALT opcode.
 * Evaluates the PPPT for each child.
 * For each character, accept the first non-NOMATCH value.
 * If none are found, defaults to NOMATCH.
 */
static void vAltMap(api* spApi, api_op* spOp, uint8_t* ucpMap){
    aint ui;
    luint lu;
    uint8_t ucChildVal;
    aint uiCount = spOp->uiChildCount;
    api_op* spChildOp;
    uint8_t ucaChildren[spApi->luiPpptMapSize * spOp->uiChildCount];
    uint8_t* ucpChild = ucaChildren;
    for (ui = 0; ui < uiCount; ui++, ucpChild += spApi->luiPpptMapSize) {
        spChildOp = &spApi->spOpcodes[spOp->uipChildIndex[ui]];
        vOpcodeMap(spApi, spChildOp, ucpChild);
    }
    for(lu = spApi->luiAcharMin; lu <= spApi->luiAcharEos; lu++){
        ucpChild = ucaChildren;
        for (ui = 0; ui < uiCount; ui++, ucpChild += spApi->luiPpptMapSize) {
            ucChildVal = ucGetMapValGen(spApi, ucpChild, spApi->luiAcharMin, lu);
            if(ucChildVal != ID_PPPT_NOMATCH){
                vSetMapValGen(ucpMap, spApi->luiAcharMin, lu, ucChildVal);
                break;
            }
        }
    }
}

/** Evaluate the PPPT for the CAT opcode.
 * Evaluate the PPPT for the first child.
 * If not NOMATCH then the CAT op value is ACTIVE.
 * Else, defaults to NOMATCH.
 */
static void vCatMap(api* spApi, api_op* spOp, uint8_t* ucpMap){
    luint lu;
    aint ui;
    api_op* spChildOp;
    uint8_t ucChildVal;
    aint uiCount = spOp->uiChildCount;
    uint8_t ucaChildren[spApi->luiPpptMapSize * spOp->uiChildCount];
    uint8_t* ucpChild = ucaChildren;
    // get the maps for all children
    for (ui = 0; ui < uiCount; ui++, ucpChild += spApi->luiPpptMapSize) {
        spChildOp = &spApi->spOpcodes[spOp->uipChildIndex[ui]];
        vOpcodeMap(spApi, spChildOp, ucpChild);
    }
    for(lu = spApi->luiAcharMin; lu <= spApi->luiAcharEos; lu++){
        // only use the first child to evaluate CAT map
        ucChildVal = ucGetMapValGen(spApi, ucaChildren, spApi->luiAcharMin, lu);
        if(ucChildVal != ID_PPPT_NOMATCH){
            vSetMapValGen(ucpMap, spApi->luiAcharMin, lu, ID_PPPT_ACTIVE);
        }
    }
}
static void vRepMap(api* spApi, api_op* spOp, uint8_t* ucpMap){
    luint lu;
    uint8_t ucChildVal;
    uint8_t ucaChildMap[spApi->luiPpptMapSize];
    vOpcodeMap(spApi, (spOp + 1), ucaChildMap);
    for(lu = spApi->luiAcharMin; lu <= spApi->luiAcharEos; lu++){
        ucChildVal = ucGetMapValGen(spApi, ucaChildMap, spApi->luiAcharMin, lu);
        if(ucChildVal == ID_PPPT_EMPTY){
            vSetMapValGen(ucpMap, spApi->luiAcharMin, lu, ID_PPPT_EMPTY);
        }else if(ucChildVal == ID_PPPT_NOMATCH){
            if(spOp->luiMin == 0){
                vSetMapValGen(ucpMap, spApi->luiAcharMin, lu, ID_PPPT_EMPTY);
            }else{
                vSetMapValGen(ucpMap, spApi->luiAcharMin, lu, ID_PPPT_NOMATCH);
            }
        }else{
            vSetMapValGen(ucpMap, spApi->luiAcharMin, lu, ID_PPPT_ACTIVE);
        }
    }
}
static void vOpcodeMap(api* spApi, api_op* spOp, uint8_t* ucpMap){
    TRACE_OPCODE_OPEN(spOp);
    luint lu;
    vClearMap(ucpMap, spApi->luiPpptMapSize);
    switch (spOp->uiId) {
    case ID_ALT:
        vAltMap(spApi, spOp, ucpMap);
        break;
    case ID_CAT:
        vCatMap(spApi, spOp, ucpMap);
        break;
    case ID_REP:
        vRepMap(spApi, spOp, ucpMap);
        break;
    case ID_RNM:
        vRuleMap(spApi, spOp->uiIndex, ucpMap);
        break;
    case ID_AND:
        vOpcodeMap(spApi, (spOp + 1), ucpMap);
        for(lu = spApi->luiAcharMin; lu <= spApi->luiAcharEos; lu++){
            uint8_t ucVal = ucGetMapValGen(spApi, ucpMap, spApi->luiAcharMin, lu);
            if(ucVal == ID_PPPT_MATCH){
                vSetMapValGen(ucpMap, spApi->luiAcharMin, lu, ID_PPPT_EMPTY);
            }
        }
        break;
    case ID_NOT:
        vOpcodeMap(spApi, (spOp + 1), ucpMap);
        // reverse NOMATCH/MATCH
        for(lu = spApi->luiAcharMin; lu <= spApi->luiAcharEos; lu++){
            uint8_t ucVal = ucGetMapValGen(spApi, ucpMap, spApi->luiAcharMin, lu);
            if(ucVal == ID_PPPT_MATCH){
                vSetMapValGen(ucpMap, spApi->luiAcharMin, lu, ID_PPPT_NOMATCH);
            }else if(ucVal == ID_PPPT_NOMATCH){
                vSetMapValGen(ucpMap, spApi->luiAcharMin, lu, ID_PPPT_EMPTY);
            }
        }
        break;
    case ID_TLS:
#ifdef ASSERT_PPPT
        if(*spOp->luipAchar >= 65 && *spOp->luipAchar <= 90){
            assert(APG_FALSE);
        }
#endif /* ASSERT_PPPT */
        if(spOp->uiAcharLength > 1){
            vSetMapValGen(ucpMap, spApi->luiAcharMin, *spOp->luipAchar, ID_PPPT_ACTIVE);
            if(*spOp->luipAchar >= 97 && *spOp->luipAchar <= 122){
                vSetMapValGen(ucpMap, spApi->luiAcharMin, (*spOp->luipAchar - 32), ID_PPPT_ACTIVE);
            }
        }else if(spOp->uiAcharLength == 1){
            vSetMapValGen(ucpMap, spApi->luiAcharMin, *spOp->luipAchar, ID_PPPT_MATCH);
            if(*spOp->luipAchar >= 97 && *spOp->luipAchar <= 122){
                vSetMapValGen(ucpMap, spApi->luiAcharMin, (*spOp->luipAchar - 32), ID_PPPT_MATCH);
            }
        }else {
            vCopyMap(ucpMap, spApi->ucpPpptEmptyMap, spApi->luiPpptMapSize);
        }
        break;
    case ID_TBS:
        if(spOp->uiAcharLength > 1){
            vSetMapValGen(ucpMap, spApi->luiAcharMin, *spOp->luipAchar, ID_PPPT_ACTIVE);
        }else{
            vSetMapValGen(ucpMap, spApi->luiAcharMin, *spOp->luipAchar, ID_PPPT_MATCH);
        }
        break;
    case ID_TRG:
        for(lu = spOp->luiMin; lu <= spOp->luiMax; lu++){
            vSetMapValGen(ucpMap, spApi->luiAcharMin, lu, ID_PPPT_MATCH);
        }
        break;
        // these opcodes have no PPPT map, so pass undecided (ACTIVE) up to the parent
    case ID_ABG:
    case ID_AEN:
    case ID_BKR:
    case ID_BKA:
    case ID_UDT:
    case ID_BKN:
        vCopyMap(ucpMap, spApi->ucpPpptUndecidedMap, spApi->luiPpptMapSize);
        break;
    default:
        XTHROW(spApi->spException, "unrecognized operator ID");
        break;
    }
    switch (spOp->uiId) {
    case ID_ALT:
    case ID_CAT:
    case ID_REP:
    case ID_AND:
    case ID_NOT:
    case ID_TLS:
    case ID_TBS:
    case ID_TRG:
        // copy this result to the proper opcode index in the PPPT table
        vCopyMap((spApi->ucpPpptTable + spOp->uiPpptIndex), ucpMap, spApi->luiPpptMapSize);
#ifdef ASSERT_PPPT
        assert(((luint)spOp->uiPpptIndex + spApi->luiPpptMapSize) <= spApi->luiPpptTableLength);
#endif /* ASSERT_PPPT */
        break;
    case ID_RNM: // this map is saved in vRuleMap()
    case ID_UDT: // no way to predict what characters the user will accept in a UDT
    case ID_BKR: // case insensitivity makes it possible for back reference to accept characters outside of min and max
    case ID_BKA: // the look-behind algorithm is iterative - no way to predict its behavior - always ACTIVE
    case ID_BKN: // the look-behind algorithm is iterative - no way to predict its behavior - always ACTIVE
    case ID_ABG: // looks only at the character position, not the character itself
    case ID_AEN: // looks only at the character position, not the character itself
        // no map for these opcodes
        break;
    }
    TRACE_OPCODE_CLOSE(spOp);
}
static int iCompOps(const void* vpL, const void* vpR){
    api_rule* spRuleL = (api_rule*)vpL;
    api_rule* spRuleR = (api_rule*)vpR;
    if(spRuleL->uiOpCount < spRuleR->uiOpCount){
        return -1;
    }
    if(spRuleL->uiOpCount > spRuleR->uiOpCount){
        return 1;
    }
    return 0;
}
static int iNameInsensitiveCompare(char* cpL, char* cpR){
    aint uiLenL = strlen(cpL);
    aint uiLenR = strlen(cpR);
    char l, r;
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
static int iCompName(const void* vpL, const void* vpR) {
    api_rule* spL = (api_rule*) vpL;
    api_rule* spR = (api_rule*) vpR;
    return iNameInsensitiveCompare(spL->cpName, spR->cpName);
}

static int iMatchRule(api_rule* spRule, aint uiRuleCount, char* cpName){
    // Bottenbruch binary search: https://en.wikipedia.org/wiki/Binary_search_algorithm
    int iL, iR, iM;
    int iComp;
    iL = 0;
    iR = uiRuleCount - 1;
    while(iL <= iR){
        iM = ((iR - iL) / 2 + iL);
        iComp = iNameInsensitiveCompare(spRule[iM].cpName, cpName);
        if(iComp > 0){
            iR = iM - 1;
        }else if(iComp < 0){
            iL = iM + 1;
        }else{
            return iM;
        }
    }
    return -1;
}

#ifdef TRACE_PPPT
static const char* cpMapVal(uint8_t ucVal){
    static char* caVal[5] = {"N", "M", "E", "A", "U"};
//    static char* caVal[5] = {"n", "m", "e", "a", "U"};
    if(ucVal < 4){
        return caVal[ucVal];
    }
    return caVal[4];
}
static void vPrintMap(api* spApi, uint8_t* ucpMap){
    luint lu;
    uint8_t ucVal;
    for(lu = spApi->luiAcharMin; lu <= spApi->luiAcharEos; lu++){
        ucVal = ucGetMapValGen(spApi, ucpMap, spApi->luiAcharMin, lu);
        printf(" %"PRIuMAX"%s", lu, cpMapVal(ucVal));
//        if(ucVal == ID_PPPT_NOMATCH || ucVal == ID_PPPT_MATCH){
//            printf(" %"PRIuMAX"%s", lu, cpMapVal(ucVal));
//        }
    }
    printf("\n");
}
static void vRuleOpen(api* spApi, aint uiRuleIndex) {
    vIndent(s_uiTreeDepth);
    printf("%s: open\n", spApi->spRules[uiRuleIndex].cpName);
    s_uiTreeDepth++;
}
static void vRuleLeaf(api* spApi, aint uiRuleIndex, uint8_t* ucpMap) {
    s_uiTreeDepth--;
    vIndent(s_uiTreeDepth);
    printf("%s: leaf", spApi->spRules[uiRuleIndex].cpName);
    vPrintMap(spApi, ucpMap);
}
static void vRuleClose(api* spApi, aint uiRuleIndex) {
    s_uiTreeDepth--;
    vIndent(s_uiTreeDepth);
    printf("%s:", spApi->spRules[uiRuleIndex].cpName);
    uint8_t* ucpMap = spApi->ucpPpptTable + spApi->spRules[uiRuleIndex].uiPpptIndex;
    vPrintMap(spApi, ucpMap);
}
static void vOpcodeOpen(api* spApi, api_op* spOp) {
    vIndent(s_uiTreeDepth);
    printf("%s: open\n", cpUtilOpName(spOp->uiId));
    s_uiTreeDepth++;
}
static void vOpcodeClose(api* spApi, api_op* spOp) {
    s_uiTreeDepth--;
    vIndent(s_uiTreeDepth);
    printf("%s:", cpUtilOpName(spOp->uiId));
    uint8_t* ucpMap = spApi->ucpPpptTable + spOp->uiPpptIndex;
    vPrintMap(spApi, ucpMap);
}
static void vIndent(aint uiIndent) {
    while (uiIndent--) {
        printf(".");
    }
}
#endif /* TRACE_PPPT */

