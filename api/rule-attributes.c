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
/** \file rule-attributes.c
 *
 * \brief Determines each rule's recursive attributes &ndash; left, nested, right and cyclic &ndash;
 * and non-recursive attributes &ndash; empty and finite.
 *
 *  - left - if true, rule is left recursive (fatal)
 *  - nested - if true, rule is nested recursive (is not a regular expression)
 *  - right - if true, rule is right recursive
 *  - cyclic - if true, at least one branch has no terminal nodes (fatal)
 *  - empty - if true, the rule matches the empty string
 *  - finite - if false, the rule only matches infinite strings (fatal)
 *
 */
#include "../api/api.h"
#include "../api/apip.h"
#include "../api/attributes.h"

// when TRACE is defined, node information is printed for each node on the syntax tree of each rule
/** \def TRACE_RULE_OPEN
 * Displays the rule name when it is opened.
 * \param x - the attribute component context handle
 * \param n - the rule name
 */
/** \def TRACE_RULE_CLOSE
 * Displays the rule name when it is opened.
 * \param x - the attribute component context handle
 * \param n - the rule name
 * \param a - pointer to the computed attributes structure
 */
/** TRACE_ATTRIBUTES
 * Uncomment, or otherwise define this macro to turn on tracing of the attributes computations.
 * Displays the Single-Expansion Parse Tree for each rule.
 */
//#define TRACE_ATTRIBUTES 1
#ifdef TRACE_ATTRIBUTES
#include "./parserp.h"
#include "../utilities/utilities.h"
static aint s_uiTreeDepth = 0;
static void vRuleOpen(attrs_ctx* spAtt, const char* cpRuleName);
static void vRuleClose(attrs_ctx* spAtt, const char* cpRuleName, api_attr_w* spRuleAttr);
static void vOpcodeOpen(attrs_ctx* spAtt, api_op* spOp);
static void vOpcodeClose(attrs_ctx* spAtt, api_op* spOp, api_attr_w* spOpAttr);
static void vIndent(aint uiIndent);
#define TRACE_RULE_OPEN(x,n) vRuleOpen((x), (n))
#define TRACE_RULE_CLOSE(x,n,a) vRuleClose((x), (n), (a))
#define TRACE_OPCODE_OPEN(x,o) vOpcodeOpen((x), (o))
#define TRACE_OPCODE_CLOSE(x,o,a) vOpcodeClose((x), (o), (a))
#define TRACE_OPCODE_CLOSE(x,o,a) vOpcodeClose((x), (o), (a))
#define TRACE_PRINT_ATTRS(x,t) vPrintAttrs((x), (t))
#else
#define TRACE_RULE_OPEN(x,n)
#define TRACE_RULE_CLOSE(x,n,a)
#define TRACE_OPCODE_OPEN(x,o)
#define TRACE_PRINT_ATTRS(x,t)
#define TRACE_OPCODE_CLOSE(x,o,a)
#endif /* TRACE_ATTRIBUTES */

static void vAttrsInit(api_attr_w* spAttrs);
static void vRuleAttrs(attrs_ctx* spAtt, aint uiRuleIndex, api_attr_w* spAttrs);
static void vBkrAttrs(attrs_ctx* spAtt, api_op* spOp, api_attr_w* spAttrs);
static void vOpcodeAttrs(attrs_ctx* spAtt, api_op* spOp, api_attr_w* spAttrs);
static void vAltAttrs(attrs_ctx* spAtt, api_op* spOp, api_attr_w* spAttrs);
static void vCatAttrs(attrs_ctx* spAtt, api_op* spOp, api_attr_w* spAttrs);
static abool bIsCatLeft(api_attr_w* spChild, aint uiCount);
static abool bIsCatNested(api_attr_w* spChild, aint uiCount);
static abool bIsCatRight(api_attr_w* spChild, aint uiCount);
static abool bIsCatEmpty(api_attr_w* spChild, aint uiCount);
static abool bIsCatFinite(api_attr_w* spChild, aint uiCount);
static abool bIsCatCyclic(api_attr_w* spChild, aint uiCount);

/** \brief Computes the attributes of each rule in the grammar.
 *
 * Attributes:
 *  - left recursive
 *  - nested recursive (matched parentheses would be an example.)
 *  - right recursive
 *  - cyclic (a rule refers only to itself)
 *  - empty - the rule is allowed to match empty strings
 *  - infinite - the rule matches only infinite strings.
 *
 *  \param spAtt Context pointer to the attributes object.
 */
void vRuleAttributes(attrs_ctx* spAtt) {
    aint ui, uj;
    api_attr_w sAttr;
    aint uiRuleCount = spAtt->spApi->uiRuleCount;
    for(ui = 0; ui < uiRuleCount; ui++){
        // initialize ALL working attributes for a fresh start on each rule
        for(uj = 0; uj < uiRuleCount; uj++){
            vAttrsInit(&spAtt->spWorkingAttrs[uj]);
        }

        // compute the attributes for this rule
        spAtt->uiStartRule = ui;
        vRuleAttrs(spAtt, ui, &sAttr);

        // save off the computed values in a permanent location
        spAtt->spAttrs[ui] = spAtt->spWorkingAttrs[ui];
    }

    // check for errors
    spAtt->uiErrorCount = 0;
    for(ui = 0; ui < uiRuleCount; ui++){
        api_attr_w* a = &spAtt->spAttrs[ui];
        if(a->bLeft|| !a->bFinite || a->bCyclic){
            spAtt->uiErrorCount++;
        }
    }
}

static void vAttrsInit(api_attr_w* spAttrs){
    spAttrs->bLeft = APG_FALSE;
    spAttrs->bNested = APG_FALSE;
    spAttrs->bRight = APG_FALSE;
    spAttrs->bCyclic = APG_TRUE;
    spAttrs->bEmpty = APG_FALSE;
    spAttrs->bFinite = APG_FALSE;
    spAttrs->bIsOpen = APG_FALSE;
    spAttrs->bIsComplete = APG_FALSE;
}

static void vRuleAttrs(attrs_ctx* spAtt, aint uiRuleIndex, api_attr_w* spAttrs) {
    api_attr_w* spRuleAttrs;
    api_op* spOp;

    TRACE_RULE_OPEN(spAtt, spAtt->spWorkingAttrs[uiRuleIndex].cpRuleName);
    spRuleAttrs = &spAtt->spWorkingAttrs[uiRuleIndex];
    while (APG_TRUE) {
        if (spRuleAttrs->bIsComplete) {
            // use final attributes
            *spAttrs = *spRuleAttrs;
            break;
        }
        if (spRuleAttrs->bIsOpen) {
            // use recursive leaf values
            if(uiRuleIndex == spAtt->uiStartRule){
                spAttrs->bLeft = APG_TRUE;
                spAttrs->bRight = APG_TRUE;
                spAttrs->bCyclic = APG_TRUE;
            }
            break;
        }
        // open the rule and traverse it
        spRuleAttrs->bIsOpen = APG_TRUE;
        spOp = &spAtt->spApi->spOpcodes[spAtt->spApi->spRules[uiRuleIndex].uiOpOffset];
        vOpcodeAttrs(spAtt, spOp, spAttrs);

        // complete this rule's attributes
        spRuleAttrs->bLeft = spAttrs->bLeft;
        spRuleAttrs->bNested = spAttrs->bNested;
        spRuleAttrs->bRight = spAttrs->bRight;
        spRuleAttrs->bEmpty = spAttrs->bEmpty;
        spRuleAttrs->bFinite = spAttrs->bFinite;
        spRuleAttrs->bCyclic = spAttrs->bCyclic;
        spRuleAttrs->bIsOpen = APG_FALSE;
        spRuleAttrs->bIsComplete = APG_TRUE;
        break;
    }
    TRACE_RULE_CLOSE(spAtt, spRuleAttrs->cpRuleName, spAttrs);
}

static void vOpcodeAttrs(attrs_ctx* spAtt, api_op* spOp, api_attr_w* spAttrs) {
    TRACE_OPCODE_OPEN(spAtt, spOp);
    vAttrsInit(spAttrs);
    switch (spOp->uiId) {
    case ID_ALT:
        vAltAttrs(spAtt, spOp, spAttrs);
        break;
    case ID_CAT:
        vCatAttrs(spAtt, spOp, spAttrs);
        break;
    case ID_REP:
        vOpcodeAttrs(spAtt, (spOp + 1), spAttrs);
        if(spOp->luiMin == 0){
            spAttrs->bEmpty = APG_TRUE;
            spAttrs->bFinite = APG_TRUE;
        }
        break;
    case ID_RNM:
        vRuleAttrs(spAtt, spOp->uiIndex, spAttrs);
        break;
    case ID_BKR:
        vBkrAttrs(spAtt, spOp, spAttrs);
        break;
    case ID_NOT:
    case ID_AND:
    case ID_BKA:
    case ID_BKN:
        vOpcodeAttrs(spAtt, (spOp + 1), spAttrs);
        spAttrs->bEmpty = APG_TRUE;
        break;
    case ID_TLS:
        spAttrs->bEmpty = spOp->uiAcharLength ? APG_FALSE : APG_TRUE;
        spAttrs->bFinite = APG_TRUE;
        spAttrs->bCyclic = APG_FALSE;
        break;
    case ID_TRG:
    case ID_TBS:
        spAttrs->bEmpty = APG_FALSE;
        spAttrs->bFinite = APG_TRUE;
        spAttrs->bCyclic = APG_FALSE;
        break;
    case ID_UDT:
        spAttrs->bEmpty = spOp->uiEmpty ? APG_TRUE : APG_FALSE;
        spAttrs->bFinite = APG_TRUE;
        spAttrs->bCyclic = APG_FALSE;
        break;
    case ID_ABG:
    case ID_AEN:
        spAttrs->bEmpty = APG_TRUE;
        spAttrs->bFinite = APG_TRUE;
        spAttrs->bCyclic = APG_FALSE;
        break;
    default:
        XTHROW(spAtt->spException, "unknown opcode id encountered");
    }
    TRACE_OPCODE_CLOSE(spAtt, spOp, spAttrs);
}

static void vAltAttrs(attrs_ctx* spAtt, api_op* spOp, api_attr_w* spAttrs) {
    aint ui;
    aint uiCount = spOp->uiChildCount;
    api_attr_w saChildren[uiCount];
    api_op* spChildOp;
    for (ui = 0; ui < uiCount; ui++) {
        spChildOp = &spAtt->spApi->spOpcodes[spOp->uipChildIndex[ui]];
        vOpcodeAttrs(spAtt, spChildOp, &saChildren[ui]);
    }
    // if any attribute is true for any ALT child, then it is true for the ALT node
    spAttrs->bLeft = APG_FALSE;
    spAttrs->bNested = APG_FALSE;
    spAttrs->bRight = APG_FALSE;
    spAttrs->bEmpty = APG_FALSE;
    spAttrs->bFinite = APG_FALSE;
    spAttrs->bCyclic = APG_FALSE;
    for (ui = 0; ui < uiCount; ui++) {
        if (saChildren[ui].bEmpty == APG_TRUE) {
            spAttrs->bEmpty = APG_TRUE;
        }
        if (saChildren[ui].bFinite == APG_TRUE) {
            spAttrs->bFinite = APG_TRUE;
        }
        if (saChildren[ui].bLeft == APG_TRUE) {
            spAttrs->bLeft = APG_TRUE;
        }
        if (saChildren[ui].bNested == APG_TRUE) {
            spAttrs->bNested = APG_TRUE;
        }
        if (saChildren[ui].bRight == APG_TRUE) {
            spAttrs->bRight = APG_TRUE;
        }
        if (saChildren[ui].bCyclic == APG_TRUE) {
            spAttrs->bCyclic = APG_TRUE;
        }
    }
}
static void vCatAttrs(attrs_ctx* spAtt, api_op* spOp, api_attr_w* spAttrs) {
    aint ui;
    aint uiCount = spOp->uiChildCount;
    api_attr_w saChildAttrs[uiCount];
    api_op* spChildOp;
    api_attr_w* spChild = &saChildAttrs[0];
    for (ui = 0; ui < uiCount; ui++, spChild++) {
        spChildOp = &spAtt->spApi->spOpcodes[spOp->uipChildIndex[ui]];
        vOpcodeAttrs(spAtt, spChildOp, spChild);
    }
    spChild = &saChildAttrs[0];
    spAttrs->bCyclic = bIsCatCyclic(spChild, uiCount);
    spAttrs->bLeft = bIsCatLeft(spChild, uiCount);
    spAttrs->bNested = bIsCatNested(spChild, uiCount);
    spAttrs->bRight = bIsCatRight(spChild, uiCount);
    spAttrs->bEmpty = bIsCatEmpty(spChild, uiCount);
    spAttrs->bFinite = bIsCatFinite(spChild, uiCount);
}
static abool bIsCatCyclic(api_attr_w* spChild, aint uiCount){
    // if all children are cyclic, CAT is cyclic (i.e. if any child is NOT cyclic, CAT is not cyclic)
    abool bReturn = APG_TRUE;
    aint ui = 0;
    for (; ui < uiCount; ui++) {
        if (spChild[ui].bCyclic == APG_FALSE) {
            bReturn = APG_FALSE;
            break;
        }
    }
    return bReturn;
}
static abool bIsCatEmpty(api_attr_w* spChild, aint uiCount){
    // if any child is not empty, CAT is not empty
    abool bReturn = APG_TRUE;
    aint ui = 0;
    for (; ui < uiCount; ui++) {
        if (spChild[ui].bEmpty != APG_TRUE) {
            bReturn = APG_FALSE;
            break;
        }
    }
    return bReturn;
}
static abool bIsCatFinite(api_attr_w* spChild, aint uiCount){
    // if any child is not finite, CAT is not finite
    abool bReturn = APG_TRUE;
    aint ui = 0;
    for (; ui < uiCount; ui++) {
        if (spChild[ui].bFinite != APG_TRUE) {
            bReturn = APG_FALSE;
            break;
        }
    }
    return bReturn;
}
static abool bIsCatLeft(api_attr_w* spChild, aint uiCount) {
    abool bReturn = APG_FALSE;
    aint ui = 0;
    for (; ui < uiCount; ui++) {
        if (spChild[ui].bLeft == APG_TRUE) {
            // if left-most, non-empty child is left or leaf, CAT is left
            bReturn = APG_TRUE;
            break;
        } else if (!spChild[ui].bEmpty) {
            // first non-empty child is not left or leaf, CAT is not left
            bReturn = APG_FALSE;
            break;
        }// else keep looking
    }
    return bReturn;
}
static abool bIsCatRight(api_attr_w* spChild, aint uiCount) {
    abool bReturn = APG_FALSE;
    aint ui, uii;
    for (ui = uiCount; ui > 0; ui--) {
        uii = ui -1;
        if (spChild[uii].bRight == APG_TRUE) {
            // if right-most child is right or leaf, CAT is right
            bReturn = APG_TRUE;
            break;
        } else if (!spChild[uii].bEmpty) {
            // first non-empty child is not right or leaf, CAT is not right
            bReturn = APG_FALSE;
            break;
        }// else keep looking
    }
    return bReturn;
}
static abool bIsRecursive(api_attr_w* spAttrs) {
    if (spAttrs->bLeft) {
        return APG_TRUE;
    }
    if (spAttrs->bNested) {
        return APG_TRUE;
    }
    if (spAttrs->bRight) {
        return APG_TRUE;
    }
    if (spAttrs->bCyclic) {
        return APG_TRUE;
    }
    return APG_FALSE;
}
static abool bIsCatNested(api_attr_w* spChild, aint uiCount) {
    aint ui, uj, uk;
    api_attr_w* spAi;
    api_attr_w* spAj;
    api_attr_w* spAk;
    while (APG_TRUE) {
        // 1.) if any child is nested, CAT is nested
        for (ui = 0; ui < uiCount; ui++) {
            spAi = &spChild[ui];
            if (spAi->bNested == APG_TRUE) {
                goto nested;
            }
        }

        // 2.) the left-most right recursive child is followed by at least one non-empty child
        for (ui = 0; ui < uiCount; ui++) {
            spAi = &spChild[ui];
            if (!spAi->bEmpty && (spAi->bRight && !spAi->bLeft)) {
                for (uj = ui + 1; uj < uiCount; uj++) {
                    spAj = &spChild[uj];
                    if (!spAj->bEmpty) {
                        goto nested;
                    }
                }
            }
        }

        // 3.) the right-most left-recursive child is preceded by at least one non-empty child
        for (ui = uiCount; ui > 0; ui--) {
            spAi = &spChild[ui - 1];
            if (!spAi->bEmpty && (spAi->bLeft && !spAi->bRight)) {
                for (uj = ui; uj > 0; uj--) {
                    spAj = &spChild[uj - 1];
                    if (!spAj->bEmpty) {
                        goto nested;
                    }
                }
            }
        }

        // 4.) there is at least one recursive child between the left-most and right-most non-recursive children
        for (ui = 0; ui < uiCount; ui++) {
            spAi = &spChild[ui];
            if (!spAi->bEmpty) {
                for (uj = ui + 1; uj < uiCount; uj++) {
                    spAj = &spChild[uj];
                    if (bIsRecursive(spAj)) {
                        for (uk = uj + 1; uk < uiCount; uk++) {
                            spAk = &spChild[uk];
                            if (!spAk->bEmpty) {
                                goto nested;
                            }
                        }
                    }
                }
            }
        }
        break;
    }
    return APG_FALSE;
    nested: return APG_TRUE;
}

static void vBkrAttrs(attrs_ctx* spAtt, api_op* spOp, api_attr_w* spAttrs) {
    aint uiRuleCount = spAtt->spApi->uiRuleCount;
    if (spOp->uiBkrIndex >= uiRuleCount) {
        // use UDT values
        spAttrs->bEmpty = spAtt->spApi->spUdts[spOp->uiBkrIndex - uiRuleCount].uiEmpty ? APG_TRUE : APG_FALSE;
        spAttrs->bFinite = APG_TRUE;
    } else {
        // use the rule empty and finite values
        vRuleAttrs(spAtt, spOp->uiBkrIndex, spAttrs);

        // however, this is a terminal node, like TLS except the string is not known in advance
        spAttrs->bLeft = APG_FALSE;
        spAttrs->bNested = APG_FALSE;
        spAttrs->bRight = APG_FALSE;
        spAttrs->bCyclic = APG_FALSE;
    }
}

#ifdef TRACE_ATTRIBUTES
static const char* s_cpTrue = "yes";
static const char* s_cpFalse = "no";
static const char* s_cpUndef = "undef";
static const char * cpBool(abool aTF) {
    if (aTF == APG_TRUE) {
        return s_cpTrue;
    }
    if (aTF == APG_FALSE) {
        return s_cpFalse;
    }
    return s_cpUndef;
}
static void vRuleOpen(attrs_ctx* spAtt, const char* cpRuleName) {
    vIndent(s_uiTreeDepth);
    printf("%s: open\n", cpRuleName);
    s_uiTreeDepth++;
}
static void vRuleClose(attrs_ctx* spAtt, const char* cpRuleName, api_attr_w* spRuleAttr) {
    s_uiTreeDepth--;
    vIndent(s_uiTreeDepth);
    printf("%s: (l:%s, n:%s, r:%s, e:%s, f:%s, cyclic:%s)\n", cpRuleName,
            cpBool(spRuleAttr->bLeft),cpBool(spRuleAttr->bNested), cpBool(spRuleAttr->bRight),
            cpBool(spRuleAttr->bEmpty),cpBool(spRuleAttr->bFinite), cpBool(spRuleAttr->bCyclic));
}
static void vOpcodeOpen(attrs_ctx* spAtt, api_op* spOp) {
    vIndent(s_uiTreeDepth);
    printf("%s: open\n", cpUtilOpName(spOp->uiId));
    s_uiTreeDepth++;
}
static void vOpcodeClose(attrs_ctx* spAtt, api_op* spOp, api_attr_w* spOpAttr) {
    s_uiTreeDepth--;
    vIndent(s_uiTreeDepth);
    printf("%s: (l:%s, n:%s, r:%s, e:%s, f:%s, cyclic:%s)\n", cpUtilOpName(spOp->uiId),
            cpBool(spOpAttr->bLeft),cpBool(spOpAttr->bNested), cpBool(spOpAttr->bRight),
            cpBool(spOpAttr->bEmpty),cpBool(spOpAttr->bFinite), cpBool(spOpAttr->bCyclic));
}
static void vIndent(aint uiIndent) {
    while (uiIndent--) {
        printf(".");
    }
}
#endif /* TRACE_ATTRIBUTES */
