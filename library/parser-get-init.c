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
/** \file library/parser-get-init.c
 * \brief Private parser utility functions. Never called directly by user.
 *
 * This set of functions is called by the parser's constructor to convert
 * the initialization data into the internal memory representation required by the parser.
 */

#include "./lib.h"
#include "./parserp.h"

/** \brief Extract the alphabet character table from the initialization data.
 *
 */
aint uiGetAcharTable(parser_init* spParserInit, achar* acpAcharTable){
    aint ui;
    aint uiLen = spParserInit->uiAcharTableLength;
    if(spParserInit->uiSizeofAchar == 1){
        uint8_t* ucpTable = (uint8_t*)spParserInit->vpAcharTable;
        for(ui = 0; ui < uiLen; ui += 1){
            acpAcharTable[ui] = (achar)ucpTable[ui];
        }
        return APG_SUCCESS;
    }
    if(spParserInit->uiSizeofAchar == 2){
        uint16_t* usipTable = (uint16_t*)spParserInit->vpAcharTable;
        for(ui = 0; ui < uiLen; ui += 1){
            acpAcharTable[ui] = (achar)usipTable[ui];
        }
        return APG_SUCCESS;
    }
    if(spParserInit->uiSizeofAchar == 4){
        uint32_t* uipTable = (uint32_t*)spParserInit->vpAcharTable;
        for(ui = 0; ui < uiLen; ui += 1){
            acpAcharTable[ui] = (achar)uipTable[ui];
        }
        return APG_SUCCESS;
    }
    if(spParserInit->uiSizeofAchar == 8){
        uint64_t* ulipTable = (uint64_t*)spParserInit->vpAcharTable;
        for(ui = 0; ui < uiLen; ui += 1){
            acpAcharTable[ui] = (achar)ulipTable[ui];
        }
        return APG_SUCCESS;
    }
    return APG_FAILURE;
}

/** \brief Re-size the initialization data to the required integer size.
 *
 */
abool bGetParserInitData(parser_init* spParserInit, luint* luipParserInit){
    aint ui;
    aint uiLen = spParserInit->uiParserInitLength;
    const void* vpData = spParserInit->vpParserInit;
    if(spParserInit->uiSizeofUint == 1){
        uint8_t* ucpTable = (uint8_t*)vpData;
        for(ui = 0; ui < uiLen; ui += 1){
            if((signed char)ucpTable[ui] == -1){
                luipParserInit[ui] = (luint)-1;
            }else{
                luipParserInit[ui] = (luint)ucpTable[ui];
            }
        }
        return APG_SUCCESS;
    }
    if(spParserInit->uiSizeofUint == 2){
        uint16_t* usipTable = (uint16_t*)vpData;
        for(ui = 0; ui < uiLen; ui += 1){
            if((signed short int)usipTable[ui] == -1){
                luipParserInit[ui] = (luint)-1;
            }else{
                luipParserInit[ui] = (luint)usipTable[ui];
            }
        }
        return APG_SUCCESS;
    }
    if(spParserInit->uiSizeofUint == 4){
        uint32_t* uipTable = (uint32_t*)vpData;
        for(ui = 0; ui < uiLen; ui += 1){
            if((signed int)uipTable[ui] == -1){
                luipParserInit[ui] = (luint)-1;
            }else{
                luipParserInit[ui] = (luint)uipTable[ui];
            }
        }
        return APG_SUCCESS;
    }
    if(spParserInit->uiSizeofUint == 8){
        uint64_t* ulipTable = (uint64_t*)vpData;
        for(ui = 0; ui < uiLen; ui += 1){
            luipParserInit[ui] = ulipTable[ui];
        }
        return APG_SUCCESS;
    }
    return APG_FAILURE;
}

/** \brief Extract the child index list from the initialization data.
 *
 */
void vGetChildListTable(init_hdr* spInitHdr, aint* uipList) {
    aint ui;
    luint* uipTable = (luint*) spInitHdr + spInitHdr->uiChildListOffset;
    for (ui = 0; ui < spInitHdr->uiChildListLength; ui += 1) {
        uipList[ui] = (aint) uipTable[ui];
    }
}
