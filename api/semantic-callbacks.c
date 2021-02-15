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
/** \file semantic-callbacks.c
 * \brief These are the callback functions which translate the AST from the syntax phase.
 *
 * All of the hard work of determining the parser's opcodes is done here.
 */

#include "./api.h"
#include "./apip.h"
#include "../library/parserp.h"
#include "./semantics.h"
#include "sabnf-grammar.h"

static const char* s_cpNoTab = "tab (\\t, 0x09) found. Not allowed in TLS strings (char-val RFC 5234).";

static void vSemPushError(semantic_data* spData, aint uiCharIndex, const char* cpMsg) {
    spData->uiErrorsFound++;
    vLineError(spData->spApi, uiCharIndex, "semantic", cpMsg);
    XTHROW(spData->spApi->spException, cpMsg);
}

static aint uiRuleLookup(ast_data* spAstData){
    semantic_data* spData = (semantic_data*)spAstData->vpUserData;
    if(spAstData->uiState == ID_AST_PRE){
        spData->uiIncAlt = APG_FALSE;
    }else{
        // look up the rule name
        char caName[RULENAME_MAX];
        size_t iBufSize = 2 * RULENAME_MAX;
        char caBuf[iBufSize];
        semantic_rule* spRules = (semantic_rule*)vpVecFirst(spData->vpVecRules);
        aint uiRuleCount = uiVecLen(spData->vpVecRules);
        aint uiFound = uiFindRule(spRules, uiRuleCount, spData->cpName, spData->uiNameLength);
        if(uiFound == APG_UNDEFINED){
            if(spData->uiIncAlt){
                // name not found, but incremental alternative specified
                aint uiLen = spData->uiNameLength + 1;
                if(uiLen > RULENAME_MAX){
                    uiLen = RULENAME_MAX;
                }
                snprintf(caName, uiLen, "%s", spData->cpName);
                snprintf(caBuf, iBufSize,
                        "incremental alternative rule name \"%s\" used without previous definition", caName);
                vSemPushError(spData, spAstData->uiPhraseOffset, caBuf);
            }else{
                // name not found, push a new rule_op
                semantic_rule sRule;
                sRule.cpName = spData->cpName;
                sRule.uiNameLength = spData->uiNameLength;

                // set up the ALT stack to remember which ALT op is current
                sRule.uiCurrentAlt = APG_UNDEFINED;
                sRule.vpVecAltStack = vpVecCtor(spData->vpMem, sizeof(aint), 100);
                vpVecPush(sRule.vpVecAltStack, &sRule.uiCurrentAlt);

                // set up the CAT stack to remember which CAT op is current
                sRule.uiCurrentCat = APG_UNDEFINED;
                sRule.vpVecCatStack = vpVecCtor(spData->vpMem, sizeof(aint), 100);
                vpVecPush(sRule.vpVecCatStack, &sRule.uiCurrentCat);

                // set up the vector of opcodes for this rule
                sRule.vpVecOps = vpVecCtor(spData->vpMem, sizeof(semantic_op), 500);

                // push the new rule on the rule vector
                sRule.uiIndex = spData->uiRuleIndex++;
                spData->spCurrentRule = (semantic_rule*)vpVecPush(spData->vpVecRules, (void*)&sRule);
            }
        }else{
            if(spData->uiIncAlt){
                // name found && IncAlt, reset current rule
                spData->spCurrentRule = (semantic_rule*)vpVecAt(spData->vpVecRules, uiFound);
                if(!spData->spCurrentRule){
                    XTHROW(spData->spApi->spException, "rule index out of range");
                }
            }else{
                // if name found && not incAlt, error, rule defined multiple times
                aint uiLen = spData->uiNameLength + 1;
                if(uiLen > RULENAME_MAX){
                    uiLen = RULENAME_MAX;
                }
                snprintf(caName, uiLen, "%s", spData->cpName);
                snprintf(caBuf, iBufSize, "rule name \"%s\" previously defined", caName);
                vSemPushError(spData, spAstData->uiPhraseOffset, caBuf);
            }
        }
    }
    return ID_AST_OK;
}

static aint uiRuleName(ast_data* spAstData){
    semantic_data* spData = (semantic_data*)spAstData->vpUserData;
    if(spAstData->uiState == ID_AST_PRE){
        spData->cpName = &spData->spApi->cpInput[spAstData->uiPhraseOffset];
        spData->uiNameLength = spAstData->uiPhraseLength;
    }
    return ID_AST_OK;
}

static aint uiIncAlt(ast_data* spAstData){
    semantic_data* spData = (semantic_data*)spAstData->vpUserData;
    if(spAstData->uiState == ID_AST_PRE){
        spData->uiIncAlt = APG_TRUE;
    }
    return ID_AST_OK;
}

static aint uiAlternation(ast_data* spAstData){
    semantic_data* spData = (semantic_data*)spAstData->vpUserData;
    semantic_rule* spRule = spData->spCurrentRule;
    if(spAstData->uiState == ID_AST_PRE){
        // set up a new ALT opcode
        semantic_op sOp = {ID_ALT};
        sOp.vpVecChildList = vpVecCtor(spData->vpMem, sizeof(aint), 10);
        spRule->uiCurrentAlt = uiVecLen(spRule->vpVecOps);
        vpVecPush(spRule->vpVecOps, &sOp);

        // push the new, current ALT opcode index on the ALT stack
        vpVecPush(spRule->vpVecAltStack, &spRule->uiCurrentAlt);
    }else{
        // reset the current ALT opcode index
        vpVecPop(spRule->vpVecAltStack);
        aint* uip = (aint*)vpVecLast(spRule->vpVecAltStack);
        if(!uip){
            XTHROW(spData->spApi->spException, "the ALT stack should never be empty");
        }
        spRule->uiCurrentAlt = *uip;
    }
    return ID_AST_OK;
}

static aint uiConcatenation(ast_data* spAstData){
    semantic_data* spData = (semantic_data*)spAstData->vpUserData;
    semantic_rule* spRule = spData->spCurrentRule;
    if(spAstData->uiState == ID_AST_PRE){
        // set up a new CAT opcode
        semantic_op sOp = {ID_CAT};
        spRule->uiCurrentCat = uiVecLen(spRule->vpVecOps);
        sOp.vpVecChildList = vpVecCtor(spData->vpMem, sizeof(aint), 10);
        vpVecPush(spRule->vpVecOps, &sOp);

        // report this CAT opcode as a child of the immediate Alt parent
        semantic_op* spAlt = (semantic_op*)vpVecAt(spRule->vpVecOps, spRule->uiCurrentAlt);
        if(!spAlt){
            XTHROW(spData->spApi->spException, "the ALT stack should never be empty");
        }
        vpVecPush(spAlt->vpVecChildList, &spRule->uiCurrentCat);

        // push the new, current ALT opcode index on the ALT stack
        vpVecPush(spRule->vpVecCatStack, &spRule->uiCurrentCat);
    }else{
        // reset the current CAT opcode index
        vpVecPop(spRule->vpVecCatStack);
        aint* uip = (aint*)vpVecLast(spRule->vpVecCatStack);
        if(!uip){
            XTHROW(spData->spApi->spException, "the CAT stack should never be empty");
        }
        spRule->uiCurrentCat = *uip;
    }
    return ID_AST_OK;
}

static aint uiRepetition(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op sOp = {ID_REP};
        sOp.luiMin = (luint)1;
        sOp.luiMax = (luint)1;
        aint uiIndex = uiVecLen(spRule->vpVecOps);
        spRule->spCurrentOp = (semantic_op*)vpVecPush(spRule->vpVecOps, &sOp);
        semantic_op* spCat = (semantic_op*)vpVecAt(spRule->vpVecOps, spRule->uiCurrentCat);
        if(!spCat){
            XTHROW(spData->spApi->spException, "the CAT stack should never be empty");
        }
        vpVecPush(spCat->vpVecChildList, &uiIndex);
    }
    return ID_AST_OK;
}

static aint uiBkaOp(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op sOp = {ID_BKA};
        vpVecPush(spRule->vpVecOps, &sOp);
    }
    return ID_AST_OK;
}

static aint uiBknOp(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op sOp = {ID_BKN};
        vpVecPush(spRule->vpVecOps, &sOp);
    }
    return ID_AST_OK;
}

static aint uiAndOp(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op sOp = {ID_AND};
        vpVecPush(spRule->vpVecOps, &sOp);
    }
    return ID_AST_OK;
}

static aint uiNotOp(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op sOp = {ID_NOT};
        vpVecPush(spRule->vpVecOps, &sOp);
    }
    return ID_AST_OK;
}

static aint uiRepOp(ast_data* spAstData){
    semantic_data* spData = (semantic_data*)spAstData->vpUserData;
    semantic_rule* spRule = spData->spCurrentRule;
    if(spAstData->uiState == ID_AST_PRE){
        semantic_op sOp = {ID_REP};
        sOp.luiMin = 0;
        sOp.luiMax = (luint)-1;
        spRule->spCurrentOp = (semantic_op*)vpVecPush(spRule->vpVecOps, &sOp);
    }else{
        semantic_op* spRep = (semantic_op*)vpVecLast(spRule->vpVecOps);
        if(spRep->luiMin > spRep->luiMax){
            char caBuf[128];
            snprintf(caBuf, 128, "REP: minimum(%"PRIuMAX") must be <= maximum (%"PRIuMAX")",
                    spRule->spCurrentOp->luiMin, spRule->spCurrentOp->luiMax);
            vSemPushError(spData, spAstData->uiPhraseOffset, caBuf);
        }
    }
    return ID_AST_OK;
}

static aint uiRepMin(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op* spRep = (semantic_op*)vpVecLast(spRule->vpVecOps);
        aint ui;
        luint luiTest, luiDigit;
        spRep->luiMin = 0;
        for(ui = 0; ui < spAstData->uiPhraseLength; ui++){
            if(!bMultiplyLong(10, spRep->luiMin, &luiTest)){
                goto fail;
            }
            luiDigit = (luint)(spAstData->acpString[spAstData->uiPhraseOffset + ui] - 48);
            if(!bSumLong(luiTest, luiDigit, &spRep->luiMin)){
                goto fail;
            }
        }
        return ID_AST_OK;
        fail:{
            char caString[spAstData->uiPhraseLength + 1];
            char caBuf[128];
            memcpy((void*)caString, (void*)&spAstData->acpString[spAstData->uiPhraseOffset], spAstData->uiPhraseLength);
            caString[spAstData->uiPhraseLength] = 0;
            snprintf(caBuf, 128,
                    "REP: n*m: n out of range: %s", caString);
            vSemPushError(spData, spAstData->uiPhraseOffset, caBuf);
        }
    }
    return ID_AST_OK;
}

static aint uiRepMax(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op* spRep = (semantic_op*)vpVecLast(spRule->vpVecOps);
        aint ui;
        luint luiTest, luiDigit;
        spRep->luiMax = 0;
        for(ui = 0; ui < spAstData->uiPhraseLength; ui++){
            if(!bMultiplyLong(10, spRep->luiMax, &luiTest)){
                goto fail;
            }
            luiDigit = (luint)(spAstData->acpString[spAstData->uiPhraseOffset + ui] - 48);
            if(!bSumLong(luiTest, luiDigit, &spRep->luiMax)){
                goto fail;
            }
        }
        return ID_AST_OK;
        fail:{
            char caString[spAstData->uiPhraseLength + 1];
            char caBuf[128];
            memcpy((void*)caString, (void*)&spAstData->acpString[spAstData->uiPhraseOffset], spAstData->uiPhraseLength);
            caString[spAstData->uiPhraseLength] = 0;
            snprintf(caBuf, 128,
                    "REP: n*m: m out of range: %s", caString);
            vSemPushError(spData, spAstData->uiPhraseOffset, caBuf);
        }
    }
    return ID_AST_OK;
}

static aint uiRepMinMax(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op* spRep = (semantic_op*)vpVecLast(spRule->vpVecOps);
        aint ui;
        luint luiTest, luiDigit;
        spRep->luiMin = 0;
        for(ui = 0; ui < spAstData->uiPhraseLength; ui++){
            if(!bMultiplyLong(10, spRep->luiMin, &luiTest)){
                goto fail;
            }
            luiDigit = (luint)(spAstData->acpString[spAstData->uiPhraseOffset + ui] - 48);
            if(!bSumLong(luiTest, luiDigit, &spRep->luiMin)){
                goto fail;
            }
        }
        spRep->luiMax = spRep->luiMin;
        return ID_AST_OK;
        fail:{
            char caString[spAstData->uiPhraseLength + 1];
            char caBuf[128];
            memcpy((void*)caString, (void*)&spAstData->acpString[spAstData->uiPhraseOffset], spAstData->uiPhraseLength);
            caString[spAstData->uiPhraseLength] = 0;
            snprintf(caBuf, 128,
                    "REP: n or n*n: n out of range: %s", caString);
            vSemPushError(spData, spAstData->uiPhraseOffset, caBuf);
        }
    }
    return ID_AST_OK;
}

static aint uiOptionOpen(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op sOp = {ID_REP};
        sOp.luiMin = 0;
        sOp.luiMax = 1;
        vpVecPush(spRule->vpVecOps, &sOp);
    }
    return ID_AST_OK;
}

static aint uiRnmOp(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op sOp = {ID_RNM};
//        sOp.cpName = (char*)&spAstData->acpString[spAstData->uiPhraseOffset];
        sOp.cpName = &spData->spApi->cpInput[spAstData->uiPhraseOffset];
        sOp.uiNameLength = spAstData->uiPhraseLength;
        vpVecPush(spRule->vpVecOps, &sOp);
    }
    return ID_AST_OK;
}

static aint uiUdtOp(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op sOp = {ID_UDT};
        sOp.uiEmpty = APG_FALSE;
        sOp.cpName = &spData->spApi->cpInput[spAstData->uiPhraseOffset];
        sOp.uiNameLength = spAstData->uiPhraseLength;
        spRule->spCurrentOp = vpVecPush(spRule->vpVecOps, &sOp);
    }
    return ID_AST_OK;
}

static aint uiUdtEmpty(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        spRule->spCurrentOp->uiEmpty = APG_TRUE;
    }
    return ID_AST_OK;
}

static aint uiUdtNonEmpty(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        spRule->spCurrentOp->uiEmpty = APG_FALSE;
    }
    return ID_AST_OK;
}

static aint uiBkrOp(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op sOp = {ID_BKR};
        sOp.uiCase = ID_BKR_CASE_I;
        sOp.uiMode = ID_BKR_MODE_U;
        spRule->spCurrentOp = (semantic_op*)vpVecPush(spRule->vpVecOps, &sOp);
    }
    return ID_AST_OK;
}

static aint uiCs(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        spRule->spCurrentOp->uiCase = ID_BKR_CASE_S;
    }
    return ID_AST_OK;
}

static aint uiCi(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        spRule->spCurrentOp->uiCase = ID_BKR_CASE_I;
    }
    return ID_AST_OK;
}

static aint uiBkrUm(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        spRule->spCurrentOp->uiMode = ID_BKR_MODE_U;
    }
    return ID_AST_OK;
}

static aint uiBkrPm(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        spRule->spCurrentOp->uiMode = ID_BKR_MODE_P;
    }
    return ID_AST_OK;
}

static aint uiBkrName(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
//        spRule->spCurrentOp->cpName = (char*)&spAstData->acpString[spAstData->uiPhraseOffset];
        spRule->spCurrentOp->cpName = &spData->spApi->cpInput[spAstData->uiPhraseOffset];
        spRule->spCurrentOp->uiNameLength = spAstData->uiPhraseLength;
    }
    return ID_AST_OK;
}

static aint uiAbgOp(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op sOp = {ID_ABG};
        vpVecPush(spRule->vpVecOps, &sOp);
    }
    return ID_AST_OK;
}

static aint uiAenOp(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op sOp = {ID_AEN};
        vpVecPush(spRule->vpVecOps, &sOp);
    }
    return ID_AST_OK;
}

static aint uiClsOp(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        semantic_op sOp = {ID_TBS};
        spRule->spCurrentOp = (semantic_op*)vpVecPush(spRule->vpVecOps, &sOp);
    }
    return ID_AST_OK;
}

static aint uiClsString(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        spRule->spCurrentOp->uiStringIndex = uiVecLen(spData->vpVecAcharsTable);
        spRule->spCurrentOp->uiStringLength = spAstData->uiPhraseLength;
        if(!spAstData->uiPhraseLength){
            vSemPushError(spData, spAstData->uiPhraseOffset,
                    "case-sensitive string may not be empty - use case-insensitive string (\"\") to represent an empty string");
        }else{
            // push the actual string into the character table for case sensitive compare
            aint ui = 0;
            for(; ui < spAstData->uiPhraseLength; ui++){
                luint luiChar = (luint)spAstData->acpString[spAstData->uiPhraseOffset + ui];
                vpVecPush(spData->vpVecAcharsTable, (void*)&luiChar);
            }
        }
    }
    return ID_AST_OK;
}

static aint uiTlsOp(ast_data* spAstData){
    semantic_data* spData = (semantic_data*)spAstData->vpUserData;
    semantic_rule* spRule = spData->spCurrentRule;
    if(spAstData->uiState == ID_AST_PRE){
        semantic_op sOp = {ID_TLS};
        sOp.uiCase = ID_BKR_CASE_I;
        spRule->spCurrentOp = (semantic_op*)vpVecPush(spRule->vpVecOps, &sOp);
    }else{
        luint luiChar;
        aint ui = 0;
        aint uiIndex = spRule->spCurrentOp->uiStringIndex;
        if(spRule->spCurrentOp->uiCase == ID_BKR_CASE_I){
            // push lower case string for case insensitive compare
            luint* luipChars = (luint*)vpVecAt(spData->vpVecAcharsTable, uiIndex);
            for(; ui < spRule->spCurrentOp->uiStringLength; ui++, luipChars++){
                luiChar = *luipChars;
                if((luiChar >= 65) && (luiChar <= 90)){
                    *luipChars += 32;
                }
            }

        }else{
            // push actual string for case sensitive compare
            spRule->spCurrentOp->uiId = ID_TBS;
        }
    }
    return ID_AST_OK;
}

static aint uiTbsOp(ast_data* spAstData){
    semantic_data* spData = (semantic_data*)spAstData->vpUserData;
    semantic_rule* spRule = spData->spCurrentRule;
    if(spAstData->uiState == ID_AST_PRE){
        semantic_op sOp = {ID_TBS};
        sOp.uiStringIndex = uiVecLen(spData->vpVecAcharsTable);
        spRule->spCurrentOp = (semantic_op*)vpVecPush(spRule->vpVecOps, &sOp);
    }else{
        spRule->spCurrentOp->uiStringLength = uiVecLen(spData->vpVecAcharsTable) - spRule->spCurrentOp->uiStringIndex;
    }
    return ID_AST_OK;
}

static aint uiDstring(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_POST){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        vpVecPush(spData->vpVecAcharsTable, (void*)&spData->luiNum);
    }
    return ID_AST_OK;
}

static aint uiTrgOp(ast_data* spAstData){
    semantic_data* spData = (semantic_data*)spAstData->vpUserData;
    semantic_rule* spRule = spData->spCurrentRule;
    if(spAstData->uiState == ID_AST_PRE){
        semantic_op sOp = {ID_TRG};
        spRule->spCurrentOp = (semantic_op*)vpVecPush(spRule->vpVecOps, &sOp);
    }else{
        if(spRule->spCurrentOp->luiMin > spRule->spCurrentOp->luiMax){
            char caBuf[128];
            snprintf(caBuf, 128, "TRG: minimum character (%"PRIuMAX") must be <= maximum character (%"PRIuMAX")",
                    spRule->spCurrentOp->luiMin, spRule->spCurrentOp->luiMax);
            vSemPushError(spData, spAstData->uiPhraseOffset, caBuf);
        }
    }
    return ID_AST_OK;
}

static aint uiTlsString(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        semantic_rule* spRule = spData->spCurrentRule;
        spRule->spCurrentOp->uiStringIndex = uiVecLen(spData->vpVecAcharsTable);
        spRule->spCurrentOp->uiStringLength = spAstData->uiPhraseLength;

        // push the string into the achar table
        aint ui = 0;
        for(; ui < spAstData->uiPhraseLength; ui++){
            luint luiChar = (luint)spAstData->acpString[spAstData->uiPhraseOffset + ui];
            vpVecPush(spData->vpVecAcharsTable, (void*)&luiChar);
        }
    }
    return ID_AST_OK;
}

static aint uiStringTab(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        vSemPushError(spData, spAstData->uiPhraseOffset, s_cpNoTab);
    }
    return ID_AST_OK;
}

static aint uiDnum(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        spData->luiNum = (luint)(spAstData->acpString[spAstData->uiPhraseOffset] - 48);
        aint ui = 1;
        luint luiTest, luiDigit;
        spData->luiNum = 0;
        for(ui = 0; ui < spAstData->uiPhraseLength; ui++){
            if(!bMultiplyLong(10, spData->luiNum, &luiTest)){
                goto fail;
            }
            luiDigit = (luint)(spAstData->acpString[spAstData->uiPhraseOffset + ui] - 48);
            if(!bSumLong(luiTest, luiDigit, &spData->luiNum)){
                goto fail;
            }
        }
        return ID_AST_OK;
        fail:{
            char caString[spAstData->uiPhraseLength + 1];
            char caBuf[128];
            memcpy((void*)caString, (void*)&spAstData->acpString[spAstData->uiPhraseOffset], spAstData->uiPhraseLength);
            caString[spAstData->uiPhraseLength] = 0;
            snprintf(caBuf, 128,
                    "decimal number out of range: %s", caString);
            vSemPushError(spData, spAstData->uiPhraseOffset, caBuf);
        }
    }
    return ID_AST_OK;
}

static aint uiBnum(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        spData->luiNum = (luint)(spAstData->acpString[spAstData->uiPhraseOffset] - 48);
        aint ui;
        luint luiTest, luiDigit;
        spData->luiNum = 0;
        for(ui = 0; ui < spAstData->uiPhraseLength; ui++){
            if(!bMultiplyLong(2, spData->luiNum, &luiTest)){
                goto fail;
            }
            luiDigit = (luint)(spAstData->acpString[spAstData->uiPhraseOffset + ui] - 48);
            if(!bSumLong(luiTest, luiDigit, &spData->luiNum)){
                goto fail;
            }
        }
        return ID_AST_OK;
        fail:{
            char caString[spAstData->uiPhraseLength + 1];
            char caBuf[128];
            memcpy((void*)caString, (void*)&spAstData->acpString[spAstData->uiPhraseOffset], spAstData->uiPhraseLength);
            caString[spAstData->uiPhraseLength] = 0;
            snprintf(caBuf, 128,
                    "binary number out of range: %s", caString);
            vSemPushError(spData, spAstData->uiPhraseOffset, caBuf);
        }
}
    return ID_AST_OK;
}

static luint luiGetNum(luint luiChar){
    luint luiNum;
    if(luiChar >= 48 && luiChar <= 57){
        luiNum = luiChar - 48;
    }else if(luiChar >= 65 && luiChar <= 70){
        luiNum = luiChar - 55;
    }else{
        luiNum = luiChar - 87;
    }
    return luiNum;
}
static aint uiXnum(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_PRE){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        spData->luiNum = luiGetNum((luint)spAstData->acpString[spAstData->uiPhraseOffset]);
        aint ui;
        luint luiTest, luiDigit;
        spData->luiNum = 0;
        for(ui = 0; ui < spAstData->uiPhraseLength; ui++){
            if(!bMultiplyLong(16, spData->luiNum, &luiTest)){
                goto fail;
            }
            luiDigit = luiGetNum((luint)spAstData->acpString[spAstData->uiPhraseOffset + ui]);
            if(!bSumLong(luiTest, luiDigit, &spData->luiNum)){
                goto fail;
            }
        }
        return ID_AST_OK;
        fail:{
            char caString[spAstData->uiPhraseLength + 1];
            char caBuf[128];
            memcpy((void*)caString, (void*)&spAstData->acpString[spAstData->uiPhraseOffset], spAstData->uiPhraseLength);
            caString[spAstData->uiPhraseLength] = 0;
            snprintf(caBuf, 128,
                    "hexidecimal number out of range: %s", caString);
            vSemPushError(spData, spAstData->uiPhraseOffset, caBuf);
        }
    }
    return ID_AST_OK;
}

static aint uiDmin(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_POST){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        spData->spCurrentRule->spCurrentOp->luiMin = spData->luiNum;
    }
    return ID_AST_OK;
}

static aint uiDmax(ast_data* spAstData){
    if(spAstData->uiState == ID_AST_POST){
        semantic_data* spData = (semantic_data*)spAstData->vpUserData;
        spData->spCurrentRule->spCurrentOp->luiMax = spData->luiNum;
    }
    return ID_AST_OK;
}

/** \brief Set the callback functions for the AST translation of the semantic phase parse to opcodes
 * \param vpAstCtx Pointer to an AST context.
 */
void vSabnfGrammarAstCallbacks(void* vpAstCtx) {
    aint ui;
    ast_callback cb[RULE_COUNT_SABNF_GRAMMAR];
    cb[SABNF_GRAMMAR_ABGOP] = uiAbgOp;
    cb[SABNF_GRAMMAR_AENOP] = uiAenOp;
    cb[SABNF_GRAMMAR_ALPHANUM] = NULL;
    cb[SABNF_GRAMMAR_ALTERNATION] = uiAlternation;
    cb[SABNF_GRAMMAR_ALTOP] = NULL;
    cb[SABNF_GRAMMAR_ANDOP] = uiAndOp;
    cb[SABNF_GRAMMAR_BASICELEMENT] = NULL;
    cb[SABNF_GRAMMAR_BASICELEMENTERR] = NULL;
    cb[SABNF_GRAMMAR_BIN] = NULL;
    cb[SABNF_GRAMMAR_BKAOP] = uiBkaOp;
    cb[SABNF_GRAMMAR_BKNOP] = uiBknOp;
    cb[SABNF_GRAMMAR_BKR_NAME] = uiBkrName;
    cb[SABNF_GRAMMAR_BKRMODIFIER] = NULL;
    cb[SABNF_GRAMMAR_BKROP] = uiBkrOp;
    cb[SABNF_GRAMMAR_BLANKLINE] = NULL;
    cb[SABNF_GRAMMAR_BMAX] = uiDmax;
    cb[SABNF_GRAMMAR_BMIN] = uiDmin;
    cb[SABNF_GRAMMAR_BNUM] = uiBnum;
    cb[SABNF_GRAMMAR_BSTRING] = uiDstring;
    cb[SABNF_GRAMMAR_CATOP] = NULL;
    cb[SABNF_GRAMMAR_CI] = uiCi;
    cb[SABNF_GRAMMAR_CLSCLOSE] = NULL;
    cb[SABNF_GRAMMAR_CLSOP] = uiClsOp;
    cb[SABNF_GRAMMAR_CLSOPEN] = NULL;
    cb[SABNF_GRAMMAR_CLSSTRING] = uiClsString;
    cb[SABNF_GRAMMAR_COMMENT] = NULL;
    cb[SABNF_GRAMMAR_CONCATENATION] = uiConcatenation;
    cb[SABNF_GRAMMAR_CS] = uiCs;
    cb[SABNF_GRAMMAR_DEC] = NULL;
    cb[SABNF_GRAMMAR_DEFINED] = NULL;
    cb[SABNF_GRAMMAR_DEFINEDAS] = NULL;
    cb[SABNF_GRAMMAR_DEFINEDASERROR] = NULL;
    cb[SABNF_GRAMMAR_DEFINEDASTEST] = NULL;
    cb[SABNF_GRAMMAR_DMAX] = uiDmax;
    cb[SABNF_GRAMMAR_DMIN] = uiDmin;
    cb[SABNF_GRAMMAR_DNUM] = uiDnum;
    cb[SABNF_GRAMMAR_DSTRING] = uiDstring;
    cb[SABNF_GRAMMAR_ENAME] = NULL;
    cb[SABNF_GRAMMAR_FILE] = NULL;
    cb[SABNF_GRAMMAR_GROUP] = NULL;
    cb[SABNF_GRAMMAR_GROUPCLOSE] = NULL;
    cb[SABNF_GRAMMAR_GROUPERROR] = NULL;
    cb[SABNF_GRAMMAR_GROUPOPEN] = NULL;
    cb[SABNF_GRAMMAR_HEX] = NULL;
    cb[SABNF_GRAMMAR_INCALT] = uiIncAlt;
    cb[SABNF_GRAMMAR_LINECONTINUE] = NULL;
    cb[SABNF_GRAMMAR_LINEEND] = NULL;
    cb[SABNF_GRAMMAR_LINEENDERROR] = NULL;
    cb[SABNF_GRAMMAR_MODIFIER] = NULL;
    cb[SABNF_GRAMMAR_NOTOP] = uiNotOp;
    cb[SABNF_GRAMMAR_OPTION] = NULL;
    cb[SABNF_GRAMMAR_OPTIONCLOSE] = NULL;
    cb[SABNF_GRAMMAR_OPTIONERROR] = NULL;
    cb[SABNF_GRAMMAR_OPTIONOPEN] = uiOptionOpen;
    cb[SABNF_GRAMMAR_OWSP] = NULL;
    cb[SABNF_GRAMMAR_PM] = uiBkrPm;
    cb[SABNF_GRAMMAR_PREDICATE] = NULL;
    cb[SABNF_GRAMMAR_PROSVAL] = NULL;
    cb[SABNF_GRAMMAR_PROSVALCLOSE] = NULL;
    cb[SABNF_GRAMMAR_PROSVALOPEN] = NULL;
    cb[SABNF_GRAMMAR_PROSVALSTRING] = NULL;
    cb[SABNF_GRAMMAR_REP_MAX] = uiRepMax;
    cb[SABNF_GRAMMAR_REP_MIN] = uiRepMin;
    cb[SABNF_GRAMMAR_REP_MIN_MAX] = uiRepMinMax;
    cb[SABNF_GRAMMAR_REP_NUM] = NULL;
    cb[SABNF_GRAMMAR_REPETITION] = uiRepetition;
    cb[SABNF_GRAMMAR_REPOP] = uiRepOp;
    cb[SABNF_GRAMMAR_RNAME] = NULL;
    cb[SABNF_GRAMMAR_RNMOP] = uiRnmOp;
    cb[SABNF_GRAMMAR_RULE] = NULL;
    cb[SABNF_GRAMMAR_RULEERROR] = NULL;
    cb[SABNF_GRAMMAR_RULELOOKUP] = uiRuleLookup;
    cb[SABNF_GRAMMAR_RULENAME] = uiRuleName;
    cb[SABNF_GRAMMAR_RULENAMEERROR] = NULL;
    cb[SABNF_GRAMMAR_RULENAMETEST] = NULL;
    cb[SABNF_GRAMMAR_SPACE] = NULL;
    cb[SABNF_GRAMMAR_STRINGTAB] = uiStringTab;
    cb[SABNF_GRAMMAR_TBSOP] = uiTbsOp;
    cb[SABNF_GRAMMAR_TLSCASE] = NULL;
    cb[SABNF_GRAMMAR_TLSCLOSE] = NULL;
    cb[SABNF_GRAMMAR_TLSOP] = uiTlsOp;
    cb[SABNF_GRAMMAR_TLSOPEN] = NULL;
    cb[SABNF_GRAMMAR_TLSSTRING] = uiTlsString;
    cb[SABNF_GRAMMAR_TRGOP] = uiTrgOp;
    cb[SABNF_GRAMMAR_UDT_EMPTY] = uiUdtEmpty;
    cb[SABNF_GRAMMAR_UDT_NON_EMPTY] = uiUdtNonEmpty;
    cb[SABNF_GRAMMAR_UDTOP] = uiUdtOp;
    cb[SABNF_GRAMMAR_UM] = uiBkrUm;
    cb[SABNF_GRAMMAR_UNAME] = NULL;
    cb[SABNF_GRAMMAR_WSP] = NULL;
    cb[SABNF_GRAMMAR_XMAX] = uiDmax;
    cb[SABNF_GRAMMAR_XMIN] = uiDmin;
    cb[SABNF_GRAMMAR_XNUM] = uiXnum;
    cb[SABNF_GRAMMAR_XSTRING] = uiDstring;
    for (ui = 0; ui < (aint) RULE_COUNT_SABNF_GRAMMAR; ui += 1) {
        vAstSetRuleCallback(vpAstCtx, ui, cb[ui]);
    }
}
