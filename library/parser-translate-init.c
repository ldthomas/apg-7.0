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
/** \file library/parser-translate-init.c
 * \brief Private parser utility functions. Never called directly by user.
 *
 * This set of functions is called by the parser's constructor to translate
 * the initialization data into the rules, UDTs and opcodes.
 */

#include "./lib.h"
#include "./parserp.h"

/** \brief Translate the initialization data for the rules into the internal rules format.
 *
 */
void vTranslateRules(parser* spCtx, rule* spRules, opcode* spOpcodes, luint* luipData) {
    aint ui;
    aint uj = 0;
    rule* spRule = spRules;
    for (ui = 0; ui < spCtx->uiRuleCount; ui++, spRule++) {
        spRule->uiRuleIndex = (aint)luipData[uj++];
        spRule->ucpPpptMap = spCtx->ucpMaps + luipData[uj++];
        spRule->cpRuleName = spCtx->cpStringTable + luipData[uj++];
        spRule->spOp = spOpcodes + luipData[uj++];
        spRule->uiOpcodeCount = (aint)luipData[uj++];
        spRule->uiEmpty = (aint)luipData[uj++];
    }
}

/** \brief Translate the initialization data for the UDTs into the internal UDT format.
 *
 */
void vTranslateUdts(parser* spCtx, udt* spUdts, luint* luipData) {
    if (spCtx->uiUdtCount) {
        aint ui;
        aint uj = 0;
        udt* spUdt = spUdts;
        for (ui = 0; ui < spCtx->uiUdtCount; ui++, spUdt++) {
            spUdt->uiUdtIndex = (aint)luipData[uj++];
            spUdt->cpUdtName = spCtx->cpStringTable + luipData[uj++];
            spUdt->uiEmpty = (aint)luipData[uj++];
        }
    }
}

/** \brief Translate the initialization data for the opcodes into the internal opcode format.
 *
 */
void vTranslateOpcodes(parser* spCtx, rule* spRules, udt* spUdts, opcode* spOpcodes, luint* luipData) {
    aint ui;
    aint uj = 0;
    opcode* spOp = spOpcodes;
    for (ui = 0; ui < spCtx->uiOpcodeCount; ui++, spOp++) {
        spOp->sGen.uiId = (aint)luipData[uj++];
        switch (spOp->sGen.uiId) {
        case ID_ALT:
            spOp->sGen.ucpPpptMap = spCtx->ucpMaps + luipData[uj++];
            spOp->sAlt.uipChildList = spCtx->uipChildList + luipData[uj++];
            spOp->sAlt.uiChildCount = (aint)luipData[uj++];
            break;
        case ID_CAT:
            spOp->sGen.ucpPpptMap = spCtx->ucpMaps + luipData[uj++];
            spOp->sCat.uipChildList = spCtx->uipChildList + luipData[uj++];
            spOp->sCat.uiChildCount = (aint)luipData[uj++];
            break;
        case ID_REP:
            spOp->sGen.ucpPpptMap = spCtx->ucpMaps + luipData[uj++];
            spOp->sRep.uiMin= (aint)luipData[uj++];
            if(luipData[uj] == (luint)-1){
                spOp->sRep.uiMax= (aint)-1;
            }else{
                spOp->sRep.uiMax= (aint)luipData[uj];
            }
            uj++;
            break;
        case ID_RNM:
            spOp->sGen.ucpPpptMap = spCtx->ucpMaps + luipData[uj++];
            spOp->sRnm.spRule = spRules + luipData[uj++];
            break;
        case ID_TRG:
            spOp->sGen.ucpPpptMap = spCtx->ucpMaps + luipData[uj++];
            spOp->sTrg.acMin = (achar)luipData[uj++];
            spOp->sTrg.acMax = (achar)luipData[uj++];
            break;
        case ID_TLS:
            spOp->sGen.ucpPpptMap = spCtx->ucpMaps + luipData[uj++];
            spOp->sTls.acpStrTbl = spCtx->acpAcharTable + luipData[uj++];
            spOp->sTls.uiStrLen = (aint)luipData[uj++];
            break;
        case ID_TBS:
            spOp->sGen.ucpPpptMap = spCtx->ucpMaps + luipData[uj++];
            spOp->sTbs.acpStrTbl = spCtx->acpAcharTable + luipData[uj++];
            spOp->sTbs.uiStrLen = (aint)luipData[uj++];
            break;
        case ID_UDT:
            spOp->sGen.ucpPpptMap = NULL;
            spOp->sUdt.spUdt = spUdts + luipData[uj++];
            spOp->sUdt.uiEmpty = (aint)luipData[uj++];
            break;
        case ID_BKR:
            spOp->sGen.ucpPpptMap = NULL;
            spOp->sBkr.uiRuleIndex = (aint)luipData[uj++];
            spOp->sBkr.uiCase = (aint)luipData[uj++];
            spOp->sBkr.uiMode = (aint)luipData[uj++];
            break;
        case ID_AND:
        case ID_NOT:
            spOp->sGen.ucpPpptMap = spCtx->ucpMaps + luipData[uj++];
            break;
            // these opcodes have no PPPT map
        case ID_BKA:
        case ID_BKN:
        case ID_ABG:
        case ID_AEN:
            spOp->sGen.ucpPpptMap = NULL;
            break;
        default:
            XTHROW(spCtx->spException, "unrecognized opcode found in initialization data");
            break;
        }
//        vPrintOneOpcode(spCtx, spOp, APG_FALSE, stdout);
    }
}

