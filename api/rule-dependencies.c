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
/** \file rule-dependencies.c
 * \brief For each rule, R, determines the list of other rules R references in its syntax tree.
 *
 * Additionally, for each rule, R, determines the list of rules that refer back to R in their respective syntax trees.
 *
 */

#include "./api.h"
#include "./apip.h"
#include "./attributes.h"

/**
 * Define DEPENDENCIES_DEBUG to print lots of debugging information
 */
//#define DEPENDENCIES_DEBUG 1
#ifdef DEPENDENCIES_DEBUG
static void vPrintReferences(attrs_ctx* spAtt);
static void vPrintReferals(attrs_ctx* spAtt);
static void vPrintRulesByType(attrs_ctx* spAtt);
#endif /* DEPENDENCIES_DEBUG */

// scans the (single-expansion) syntax tree of each rule to find out which other rules it refers to
static void vScan(attrs_ctx* spAtt, aint uiRuleIndex, api_attr_w* spAttr, abool* bpIsScanned);

/** \brief Compute each rule's dependencies on the other rules, and possibly on itself if the rule is recursive.
 *
 * \param spAtt Pointer to the attribute context.
 */
void vRuleDependencies(attrs_ctx* spAtt) {
    // find all rules referred to by each rule
    aint ui, uj, uiNewGroup, uiGroupNumber;
    api_attr_w* spAttri;
    api_attr_w* spAttrj;
    aint uiRuleCount = spAtt->spApi->uiRuleCount;
    abool baIsScanned[uiRuleCount];

    // scan each rule to see which rules it refers to
    for (ui = 0; ui < uiRuleCount; ui++) {
        memset((void*)baIsScanned, 0, (sizeof(abool) * uiRuleCount));
        vScan(spAtt, ui, &spAtt->spWorkingAttrs[ui], baIsScanned);
    }

    // for each rule, find all rules that reference it
    for (ui = 0; ui < uiRuleCount; ui++) {
        for (uj = 0; uj < uiRuleCount; uj++) {
            if(ui != uj){
                if(spAtt->spWorkingAttrs[uj].bpRefersTo[ui]){
                    spAtt->spWorkingAttrs[ui].bpIsReferencedBy[uj] = APG_TRUE;
                }
            }
        }
    }

    // find the non-recursive and recursive types
    for (ui = 0; ui < spAtt->spApi->uiRuleCount; ui++) {
        spAtt->spWorkingAttrs[ui].uiRecursiveType = ID_ATTR_N;
        if (spAtt->spWorkingAttrs[ui].bpRefersTo[ui]) {
            spAtt->spWorkingAttrs[ui].uiRecursiveType = ID_ATTR_R;
        }
    }

    // find the mutually-recursive groups
    uiGroupNumber = 0;
    for (ui = 0; ui < uiRuleCount; ui++) {
        spAttri = &spAtt->spWorkingAttrs[ui];
        if (spAttri->uiRecursiveType == ID_ATTR_R) {
            uiNewGroup = APG_TRUE;
            for (uj = 0; uj < spAtt->spApi->uiRuleCount; uj++) {
                if(ui != uj){
                    spAttrj = &spAtt->spWorkingAttrs[uj];
                    if (spAttrj->uiRecursiveType == ID_ATTR_R) {
                        if (spAttri->bpRefersTo[uj] && spAttrj->bpRefersTo[ui]) {
                            if (uiNewGroup) {
                                uiGroupNumber++;
                                vpVecPush(spAtt->vpVecGroupNumbers, &uiGroupNumber);
                                spAttri->uiRecursiveType = ID_ATTR_MR;
                                spAttri->uiMRGroup = uiGroupNumber;
                                uiNewGroup = APG_FALSE;
                            }
                            spAttrj->uiRecursiveType = ID_ATTR_MR;
                            spAttrj->uiMRGroup = uiGroupNumber;
                        }
                    }
                }
            }
        }
    }

#ifdef DEPENDENCIES_DEBUG
    vPrintReferences(spAtt);
    vPrintReferals(spAtt);
    vPrintRulesByType(spAtt);
#endif /* DEPENDENCIES_DEBUG */
}

static void vScan(attrs_ctx* spAtt, aint uiRuleIndex, api_attr_w* spAttr, abool* bpIsScanned){
    api* spApi = spAtt->spApi;
    aint uiRuleCount = spApi->uiRuleCount;
    api_rule* spRule = &spApi->spRules[uiRuleIndex];
    api_op* spOp = &spApi->spOpcodes[spRule->uiOpOffset];
    api_op* spOpEnd = &spOp[spRule->uiOpCount];
    bpIsScanned[uiRuleIndex] = APG_TRUE;
    for (; spOp < spOpEnd; spOp++) {
        if (spOp->uiId == ID_RNM) {
            spAttr->bpRefersTo[spOp->uiIndex] = APG_TRUE;
            if(!bpIsScanned[spOp->uiIndex]){
                vScan(spAtt, spOp->uiIndex, spAttr, bpIsScanned);
            }
        }else if(spOp->uiId == ID_UDT){
            spAttr->bpRefersToUdt[spOp->uiIndex] = APG_TRUE;
        }
        else if (spOp->uiId == ID_BKR) {
            if(spOp->uiBkrIndex < uiRuleCount){
                spAttr->bpRefersTo[spOp->uiBkrIndex] = APG_TRUE;
                if(!bpIsScanned[spOp->uiBkrIndex]){
                    vScan(spAtt, spOp->uiBkrIndex, spAttr, bpIsScanned);
                }
            }else{
                spAttr->bpRefersToUdt[uiRuleCount - spOp->uiBkrIndex] = APG_TRUE;
            }
        }
    }
}

#ifdef DEPENDENCIES_DEBUG
static void vPrintReferences(attrs_ctx* spAtt) {
    aint ui, uj, uiCount;
    api_attr_w* spAttri;
    api_attr_w* spAttrj;
    printf("RULE REFERENCES\n");
    for (ui = 0; ui < spAtt->spApi->uiRuleCount; ui++) {
        spAttri = &spAtt->spWorkingAttrs[ui];
        printf("%s refers to:\n", spAttri->cpRuleName);
        uiCount = 0;
        for (uj = 0; uj < spAtt->spApi->uiRuleCount; uj++) {
            spAttrj = &spAtt->spWorkingAttrs[uj];
            if (spAttri->bpRefersTo[spAttrj->uiRuleIndex]) {
                uiCount++;
                printf("%4lu | %s\n", (luint)uj, spAttrj->cpRuleName);
            }
        }
        if (!uiCount) {
            printf("  %s\n", "<none>");
        }
    }
    printf("\n");
}

static void vPrintReferals(attrs_ctx* spAtt) {
    aint ui, uj, uiCount;
    api_attr_w* spAttri;
    api_attr_w* spAttrj;
    printf("RULE REFERALS\n");
    for (ui = 0; ui < spAtt->spApi->uiRuleCount; ui++) {
        spAttri = &spAtt->spWorkingAttrs[ui];
        printf("%s is referenced by:\n", spAttri->cpRuleName);
        uiCount = 0;
        for (uj = 0; uj < spAtt->spApi->uiRuleCount; uj++) {
            spAttrj = &spAtt->spWorkingAttrs[uj];
            if (spAttri->bpIsReferencedBy[spAttrj->uiRuleIndex]) {
                uiCount++;
                printf("%4lu | %s\n", (luint)uj, spAttrj->cpRuleName);
            }
        }
        if (!uiCount) {
            printf("  %s\n", "<none>");
        }
    }
    if(spAtt->spApi->uiUdtCount){
        api_udt* spUdt;
        for (ui = 0; ui < spAtt->spApi->uiUdtCount; ui++) {
            spUdt = &spAtt->spApi->spUdts[ui];
//            spAttri = &spAtt->spWorkingAttrs[ui];
            printf("%s is referenced by:\n", spUdt->cpName);
            uiCount = 0;
            for (uj = 0; uj < spAtt->spApi->uiRuleCount; uj++) {
                spAttrj = &spAtt->spWorkingAttrs[uj];
                if (spAttrj->bpRefersToUdt[spUdt->uiIndex]) {
                    uiCount++;
                    printf("%4lu | %s\n", (luint)uj, spAttrj->cpRuleName);
                }
            }
            if (!uiCount) {
                printf("  %s\n", "<none>");
            }
        }
    }
    printf("\n");
}

static void vPrintRulesByType(attrs_ctx* spAtt) {
    api* spApi = spAtt->spApi;
    aint ui, uj, uiCount;
    aint uiTotal = 0;
    printf("ID_ATTR_N (non-recursive, never refers to itself)\n");
    for (ui = 0, uiCount = 0; ui < spApi->uiRuleCount; ui++) {
        if (spAtt->spWorkingAttrs[ui].uiRecursiveType == ID_ATTR_N) {
            uiCount++;
        }
    }
    if (uiCount) {
        for (ui = 0; ui < spApi->uiRuleCount; ui++) {
            if (spAtt->spWorkingAttrs[ui].uiRecursiveType == ID_ATTR_N) {
                printf("%4lu | %s\n", (luint) ui, spAtt->spWorkingAttrs[ui].cpRuleName);
                uiTotal++;
            }
        }
    } else {
        printf("<none>\n");
    }
    printf("\n");
    printf("ID_ATTR_R (recursive, refers to itself)\n");
    for (ui = 0, uiCount = 0; ui < spApi->uiRuleCount; ui++) {
        if ((spAtt->spWorkingAttrs[ui].uiRecursiveType == ID_ATTR_R)) {
            uiCount++;
        }
    }
    if (uiCount) {
        for (ui = 0; ui < spApi->uiRuleCount; ui++) {
            if (spAtt->spWorkingAttrs[ui].uiRecursiveType == ID_ATTR_R) {
                printf("%4lu | %s\n", (luint) ui, spAtt->spWorkingAttrs[ui].cpRuleName);
                uiTotal++;
            }
        }
    } else {
        printf("<none>\n");
    }
    printf("\n");
    printf(
            "ID_ATTR_MR (mutually-recursive, within a set, each rule refers to every other rule in the set, including itself)\n");
    for (ui = 0, uiCount = 0; ui < spApi->uiRuleCount; ui++) {
        if ((spAtt->spWorkingAttrs[ui].uiRecursiveType == ID_ATTR_MR)) {
            if (spAtt->spWorkingAttrs[ui].uiMRGroup > uiCount) {
                uiCount = spAtt->spWorkingAttrs[ui].uiMRGroup;
            }
        }
    }
    if (uiCount) {
        printf("index|   set| name\n");
        printf("-----|------|-----\n");
        for (uj = 1; uj <= uiCount; uj++) {
            for (ui = 0; ui < spApi->uiRuleCount; ui++) {
                if ((spAtt->spWorkingAttrs[ui].uiMRGroup == uj)) {
                    printf("%4lu | %4lu | %s\n", (luint) ui, (luint) uj, spAtt->spWorkingAttrs[ui].cpRuleName);
                    uiTotal++;
                }
            }
        }
    } else {
        printf("<none>\n");
    }
    printf("\n");
}
#endif /* DEPENDENCIES_DEBUG */
