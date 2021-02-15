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
/** \file parser-callbacks.c
 * \brief For internal object use only. The JSON parser grammar call back functions.
 *
 * These are the functions that interact with the parse tree nodes to translate the ABNF rules into usable data.
 * Applications should never have need of this functions directly.
 */
//#include <stdio.h>
#include <limits.h>
#include "./json.h"
#include "./jsonp.h"
#include "./json-grammar.h"

/** \def THROW_ERROR(msg, off)
 * \brief A specialized exception thrower for the callback functions.
 */
#define THROW_ERROR(msg, off) vThrowError(spJson, (msg), (off), __FILE__, __func__, __LINE__)

/*********************************************************
 * helper functions
 *********************************************************/
/** @name Private Functions
 * Not static because referenced across multiple files. */
///@{
uint32_t uiUtf8_2byte(char* cpBytes){
    uint32_t uiChar = 0;
    uiChar = ((uint32_t)cpBytes[0] & 0x1f) << 6;
    uiChar += (uint32_t)cpBytes[1] & 0x3f;
    return uiChar;
}
uint32_t uiUtf8_3byte(char* cpBytes){
    uint32_t uiChar = 0;
    uiChar = ((uint32_t)cpBytes[0] & 0xf) << 12;
    uiChar += ((uint32_t)cpBytes[1] & 0x3f) << 6;
    uiChar += (uint32_t)cpBytes[2] & 0x3f;
    return uiChar;
}
uint32_t uiUtf8_4byte(char* cpBytes){
    uint32_t uiChar = 0;
    uiChar = ((uint32_t)cpBytes[0] & 0x7) << 18;
    uiChar += ((uint32_t)cpBytes[1] & 0x3f) << 12;
    uiChar += ((uint32_t)cpBytes[2] & 0x3f) << 6;
    uiChar += (uint32_t)cpBytes[3] & 0x3f;
    return uiChar;
}
aint uiUtf16_1(char* cpHex, uint32_t* uipChar){
    char* cpPtr;
    *uipChar = (uint32_t)strtol(cpHex, &cpPtr, 16);
    if(*uipChar >= 0xD800 && *uipChar < 0xE000){
        return JSON_UTF16_BAD_LOW;
    }
    return JSON_UTF16_MATCH;
}
aint uiUtf16_2(char* cpHex, uint32_t* uipChar){
    char* cpPtr;
    uint32_t uiHigh = (uint32_t)strtol(cpHex, &cpPtr, 16);
    if(uiHigh < 0xD800 || uiHigh >= 0xE000){
        // not a surrogate pair
        return JSON_UTF16_NOMATCH;
    }
    if(uiHigh >= 0xDC00){
        return JSON_UTF16_BAD_HIGH;
    }
    uint32_t uiLow = (uint32_t)strtol(&cpHex[5], &cpPtr, 16);
    if(!(uiLow >= 0xDC00 && uiLow < 0xE000)){
        return JSON_UTF16_BAD_LOW;
    }
    // decode the surrogate pairs
    uiHigh = (uiHigh - 0xd800) << 10;
    uiLow -= 0xdc00;
    *uipChar = uiHigh + uiLow + 0x10000;
    return JSON_UTF16_MATCH;
}
///@}

static void vThrowError(json* spJson, char* cpMsg, aint uiOffset,
        const char* cpFile, const char* cpFunc, unsigned int uiCodeLine){
    aint uiLine, uiRelOffset;
    char caBuf[1024];
    if(bLinesFindLine(spJson->vpLines, uiOffset, &uiLine, &uiRelOffset)){
        snprintf(caBuf, 1024,
                "%s: near: line: %"PRIuMAX": character: %"PRIuMAX" (0x%"PRIXMAX")",
                cpMsg, (luint)uiLine, (luint)uiRelOffset, (luint)uiRelOffset);
    }else{
        snprintf(caBuf, 1024,
                "%s: character offset out of range: %"PRIuMAX" (0x%"PRIXMAX")",
                cpMsg, (luint)uiOffset, (luint)uiOffset);
    }
    vExThrow(spJson->spException, caBuf, uiCodeLine, cpFile, cpFunc);
}
static void vPushFrameAndValue(callback_data* spData){
    json* spJson = (json*)spData->vpUserData;
    frame* spPrev = spJson->spCurrentFrame;
    frame* spCurrent = (frame*)vpVecPush(spJson->vpVecFrames, NULL);
    memset((void*)spCurrent, 0, sizeof(frame));
    spCurrent->uiNextKey = APG_UNDEFINED;
    spCurrent->vpVecIndexes = vpVecCtor(spJson->vpMem, sizeof(aint), 128);
    spCurrent->uiValue = uiVecLen(spJson->vpVecValuesr);
    value_r* spValuer = (value_r*)vpVecPush(spJson->vpVecValuesr, NULL);
    if(spPrev){
        spValuer->uiKey = spPrev->uiNextKey;
    }else{
        spValuer->uiKey = APG_UNDEFINED;
    }
    spJson->spCurrentFrame = spCurrent;
}

// return remaining number of frames on the stack
static void vPopFrame(callback_data* spData){
    json* spJson = (json*)spData->vpUserData;
    if(spJson->spCurrentFrame){
        frame* spFrame = (frame*)vpVecPop(spJson->vpVecFrames);
        // sanity check
        if(spFrame != spJson->spCurrentFrame){
            THROW_ERROR("popped frame not same as current frame", spData->uiParserOffset);
        }
        vVecDtor(spFrame->vpVecIndexes);
        spJson->spCurrentFrame = (frame*)vpVecLast(spJson->vpVecFrames);
    }
}

static value_r* spFrameValue(json* spJson, frame* spFrame, aint uiOffset){
    value_r* spValuer = (value_r*)vpVecAt(spJson->vpVecValuesr, spFrame->uiValue);
    if(!spValuer){
        THROW_ERROR("vector index out of range", uiOffset);
    }
    return spValuer;
}
static abool bMultiplyUint(uint64_t uiL, uint64_t uiR, uint64_t* uipA){
    if(uiL == 0 || uiR == 0){
        *uipA = 0;
        return APG_SUCCESS;
    }
    uint64_t uiTest = uiL * uiR;
    uint64_t uiCheck = uiTest / uiR;
    if(uiCheck != uiL){
        return APG_FAILURE;
    }
    *uipA = uiTest;
    return APG_SUCCESS;
}

static abool bSumUint(uint64_t uiL, uint64_t uiR, uint64_t* uipA){
    uint64_t uiTest = uiL + uiR;
    if(uiTest < uiR){
        return APG_FAILURE;
    }
    uint64_t uiCheck = uiTest - uiR;
    if(uiCheck != uiL){
        return APG_FAILURE;
    }
    *uipA = uiTest;
    return APG_SUCCESS;
}
static abool bMultiplyInt(int64_t iL, int64_t iR, int64_t * ipA){
    if(iL == 0 || iR == 0){
        *ipA = 0;
        return APG_SUCCESS;
    }
    int64_t iTest = iL * iR;
    if(iTest < 0){
        return APG_FAILURE;
    }
    *ipA = iTest;
    return APG_SUCCESS;
}
static abool bSumInt(int64_t iL, int64_t iR, int64_t* ipA){
    int64_t iTest = iL + iR;
    if(iTest < 0){
        return APG_FAILURE;
    }
    *ipA = iTest;
    return APG_SUCCESS;
}
static abool bStringToInt(char* cpString, int64_t* ipInt){
    abool bReturn = APG_FAILURE;
    *ipInt = 0;
    while(APG_TRUE){
        char* cpEnd = cpString + strlen(cpString);
        int64_t iVal = *cpString++ - 48;
        while(cpString < cpEnd){
            if(!bMultiplyInt(iVal, 10LL, &iVal)){
                goto fail;
            }
            if(!bSumInt(iVal, (int64_t)(*cpString++ - 48), &iVal)){
                goto fail;
            }
        }
        *ipInt = iVal;
        bReturn = APG_SUCCESS;
        break;
    }
    fail:;
    return bReturn;
}
static abool bStringToUint(char* cpString, uint64_t* uipInt){
    abool bReturn = APG_FAILURE;
    *uipInt = 0;
    while(APG_TRUE){
        char* cpEnd = cpString + strlen(cpString);
        uint64_t uiVal = *cpString++ - 48;
        while(cpString < cpEnd){
            if(!bMultiplyUint(uiVal, 10LL, &uiVal)){
                goto fail;
            }
            if(!bSumUint(uiVal, (int64_t)(*cpString++ - 48), &uiVal)){
                goto fail;
            }
        }
        *uipInt = uiVal;
        bReturn = APG_SUCCESS;
        break;
    }
    fail:;
    return bReturn;
}
/*********************************************************
 * helper functions
 *********************************************************/

/*********************************************************
 * call back functions
 *********************************************************/
static void vJsonText(callback_data* spData){
    json* spJson = (json*)spData->vpUserData;
    if(spData->uiParserState == ID_ACTIVE){
        // clear all memory from previous parse, if any
        vVecClear(spJson->vpVecStringsr);
        vVecClear(spJson->vpVecChildIndexes);
        vVecClear(spJson->vpVecChars);
        vVecClear(spJson->vpVecAscii);
        vVecClear(spJson->vpVecValuesr);
        vVecClear(spJson->vpVecNumbers);
        vVecClear(spJson->vpVecFrames);
        vVecClear(spJson->vpVecValues);
        vVecClear(spJson->vpVecStrings);
        vVecClear(spJson->vpVecChildPointers);
    }else if(spData->uiParserState == ID_MATCH){
        if(spData->uiParserPhraseLength < spData->uiStringLength){
            THROW_ERROR("parser did not match the entire document", spData->uiParserOffset);
        }
        char caBuf[64];
        // convert relative values to absolute values
        uint32_t* uipChars = (uint32_t*)vpVecFirst(spJson->vpVecChars);
        string_r* spStringsr = (string_r*)vpVecFirst(spJson->vpVecStringsr);
        spJson->uiStringCount = uiVecLen(spJson->vpVecStringsr);
        string_r* spStringrBeg = spStringsr;
        string_r* spStringrEnd = spStringrBeg  + spJson->uiStringCount;
        json_number* spNumber = (json_number*)vpVecFirst(spJson->vpVecNumbers);
        value_r* spValuesr = (value_r*)vpVecFirst(spJson->vpVecValuesr);
        spJson->uiValueCount = uiVecLen(spJson->vpVecValuesr);
        value_r* spValuerBeg = spValuesr;
        value_r* spValuerEnd = spValuerBeg + spJson->uiValueCount;
        aint* uipChildIndexes = (aint*)vpVecFirst(spJson->vpVecChildIndexes);
        aint uiChildCount = uiVecLen(spJson->vpVecChildIndexes);
        aint* uipChildList;
        aint ui;

        // allocate the child value pointer list
        spJson->sppChildPointers = (struct json_value_tag**)vpVecPushn(spJson->vpVecChildPointers, NULL, uiChildCount);

        // allocate the strings and convert all relative strings to absolute
        spJson->spStrings = (u32_phrase*)vpVecPushn(spJson->vpVecStrings, NULL, spJson->uiStringCount);
        u32_phrase* spString = spJson->spStrings;
        for(; spStringrBeg < spStringrEnd; spStringrBeg++, spString++){
            spString->uiLength = spStringrBeg->uiLength;
            spString->uipPhrase = uipChars + spStringrBeg->uiCharsOffset;
        }

        // allocate the values and convert relative indexes to absolute pointers
        spJson->spValues = (json_value*)vpVecPushn(spJson->vpVecValues, NULL, spJson->uiValueCount);
        memset((void*)spJson->spValues, 0, (sizeof(json_value) * spJson->uiValueCount));
        json_value* spValue = spJson->spValues;
        aint uiChildPointerTop = 0;
        for(; spValuerBeg < spValuerEnd; spValuerBeg++, spValue++){
            spValue->uiId = spValuerBeg->uiId;
            if(spValuerBeg->uiKey == APG_UNDEFINED){
                spValue->spKey = NULL;
            }else{
                spValue->spKey = &spJson->spStrings[spValuerBeg->uiKey];
            }
            spValue->sppChildren = NULL;
            spValue->uiChildCount = 0;
            switch(spValuerBeg->uiId){
            case JSON_ID_STRING:
                spValue->spString = &spJson->spStrings[spValuerBeg->uiString];
                break;
            case JSON_ID_TRUE:
            case JSON_ID_FALSE:
            case JSON_ID_NULL:
                break;
            case JSON_ID_NUMBER:
                spValue->spNumber = &spNumber[spValuerBeg->uiNumber];
                break;
            case JSON_ID_OBJECT:
            case JSON_ID_ARRAY:
                // convert the array of child indexes to an array of absolute pointers
                uipChildList = &uipChildIndexes[spValuerBeg->uiChildListOffset];
                spValue->uiChildCount = spValuerBeg->uiChildCount;
                spValue->sppChildren = &spJson->sppChildPointers[uiChildPointerTop];
                for(ui = 0; ui < spValuerBeg->uiChildCount; ui++, uipChildList++){
                    spJson->sppChildPointers[uiChildPointerTop++] = (struct json_value_tag*)(spJson->spValues + *uipChildList);
                }
                break;
            default:
                snprintf(caBuf, 64, "unrecognized value type: %"PRIuMAX"", (luint)spValuerBeg->uiId);
                THROW_ERROR(caBuf, spData->uiParserOffset);
                break;
            }
        }
    }else if(spData->uiParserState == ID_NOMATCH){
        THROW_ERROR("JSON-text not matched but no identifiable errors found", spData->uiParserOffset);
    }
}
static void vValue(callback_data* spData){
    json* spJson = (json*)spData->vpUserData;
    if(spData->uiParserState == ID_ACTIVE){
        // push value and frame
        vPushFrameAndValue(spData);
    }else if(spData->uiParserState == ID_MATCH){
        // complete the value
        frame* spFrame = spJson->spCurrentFrame;
        aint uiValueIndex = spFrame->uiValue;
        value_r* spValue = spFrameValue(spJson, spFrame, spData->uiParserOffset);
        spValue->uiChildCount = uiVecLen(spFrame->vpVecIndexes);
        if(spValue->uiChildCount){
            // copy the vector of indexes
            aint* uipChildIndex = (aint*)vpVecFirst(spFrame->vpVecIndexes);
            spValue->uiChildListOffset = uiVecLen(spJson->vpVecChildIndexes);
            aint* uipIndex = (aint*)vpVecPushn(spJson->vpVecChildIndexes, NULL, spValue->uiChildCount);
            aint* uipEnd = uipIndex + spValue->uiChildCount;
            while(uipIndex < uipEnd){
                *uipIndex++ = *uipChildIndex++;
            }
        }
        // pop the frame
        vPopFrame(spData);
        if(spJson->spCurrentFrame){
            // report this value as a child of the parent value
            vpVecPush(spJson->spCurrentFrame->vpVecIndexes, &uiValueIndex);
        }
    }else if(spData->uiParserState == ID_NOMATCH){
        // pop value and frame
        vPopFrame(spData);
        vpVecPop(spJson->vpVecValuesr);
    }
}
static void vEndMemberSep(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        THROW_ERROR("Trailing comma not allowed in objects (REF8259)", spData->uiParserOffset);
    }
}
static void vEndValueSep(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        THROW_ERROR("Trailing comma not allowed in arrays (REF8259)", spData->uiParserOffset);
    }
}
static void vObjectBegin(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        frame* spFrame = spJson->spCurrentFrame;
        value_r* spValue = spFrameValue(spJson, spFrame, spData->uiParserOffset);
        spValue->uiId = JSON_ID_OBJECT;
    }
}
static void vObjectEnd(callback_data* spData){
    if(spData->uiParserState == ID_NOMATCH){
        json* spJson = (json*)spData->vpUserData;
        THROW_ERROR("Expected closing object bracket '}' not found.", spData->uiParserOffset);
    }
}
static void vArrayBegin(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        frame* spFrame = spJson->spCurrentFrame;
        value_r* spValue = spFrameValue(spJson, spFrame, spData->uiParserOffset);
        spValue->uiId = JSON_ID_ARRAY;
    }
}
static void vArrayEnd(callback_data* spData){
    if(spData->uiParserState == ID_NOMATCH){
        json* spJson = (json*)spData->vpUserData;
        THROW_ERROR("Expected closing array bracket ']' not found.", spData->uiParserOffset);
    }
}
static void vFalse(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        frame* spFrame = spJson->spCurrentFrame;
        value_r* spValue = spFrameValue(spJson, spFrame, spData->uiParserOffset);
        spValue->uiId = JSON_ID_FALSE;
    }
}
static void vTrue(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        frame* spFrame = spJson->spCurrentFrame;
        value_r* spValue = spFrameValue(spJson, spFrame, spData->uiParserOffset);
        spValue->uiId = JSON_ID_TRUE;
    }
}
static void vNull(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        frame* spFrame = spJson->spCurrentFrame;
        value_r* spValue = spFrameValue(spJson, spFrame, spData->uiParserOffset);
        spValue->uiId = JSON_ID_NULL;
    }
}
static void vKeyBegin(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        spJson->spCurrentFrame->uiNextKey = uiVecLen(spJson->vpVecStringsr);

        // push a new string and initialize it
        string_r* spString = (string_r*)vpVecPush(spJson->vpVecStringsr, NULL);
        spString->uiCharsOffset = uiVecLen(spJson->vpVecChars);
    }
}
static void vStringBegin(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        frame* spFrame = spJson->spCurrentFrame;
        value_r* spValue = spFrameValue(spJson, spFrame, spData->uiParserOffset);
        spValue->uiId = JSON_ID_STRING;
        spValue->uiString = uiVecLen(spJson->vpVecStringsr);

        // push a new string and initialize it
        string_r* spString = (string_r*)vpVecPush(spJson->vpVecStringsr, NULL);
        spString->uiCharsOffset = uiVecLen(spJson->vpVecChars);
    }
}
static void vStringEnd(callback_data* spData){
    if (spData->uiParserState == ID_NOMATCH) {
        json *spJson = (json*) spData->vpUserData;
        THROW_ERROR("Expected close of string not found.", spData->uiParserOffset);
    }
}
static void vStringContent(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        string_r* spString = (string_r*)vpVecLast(spJson->vpVecStringsr);
        spString->uiLength = uiVecLen(spJson->vpVecChars) - spString->uiCharsOffset;
    }
}
static void vChar(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        // push the character on the character string vector
        vpVecPush(spJson->vpVecChars, &spJson->uiChar);
    }else if(spData->uiParserState == ID_NOMATCH){
        json* spJson = (json*)spData->vpUserData;
        if((char)spData->acpString[spData->uiParserOffset] != 34){
            THROW_ERROR("invalid character detected - probably mal-formed UTF-8", spData->uiParserOffset);
        }
    }
}
static void vAscii(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        spJson->uiChar = (uint32_t)spData->acpString[spData->uiParserOffset];
    }
}
static void vUtf82(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        char caHex[] = {
                (char)spData->acpString[spData->uiParserOffset],
                (char)spData->acpString[spData->uiParserOffset+1],
        };
        spJson->uiChar = uiUtf8_2byte(caHex);
    }
}
static void vUtf83(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        char caHex[] = {
                (char)spData->acpString[spData->uiParserOffset],
                (char)spData->acpString[spData->uiParserOffset+1],
                (char)spData->acpString[spData->uiParserOffset+2],
        };
        spJson->uiChar = uiUtf8_3byte(caHex);
    }
}
static void vUtf84(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        char caHex[] = {
                (char)spData->acpString[spData->uiParserOffset],
                (char)spData->acpString[spData->uiParserOffset+1],
                (char)spData->acpString[spData->uiParserOffset+2],
                (char)spData->acpString[spData->uiParserOffset+3],
        };
        spJson->uiChar = uiUtf8_4byte(caHex);
    }
}
static void vRSolidus(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        spJson->uiChar = 0x5C;
    }
}
static void vSolidus(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        spJson->uiChar = 0x2F;
    }
}
static void vQuote(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        spJson->uiChar = 0x22;
    }
}
static void vBackSpace(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        spJson->uiChar = 0x08;
    }
}
static void vFormFeed(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        spJson->uiChar = 0x0C;
    }
}
static void vLineFeed(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        spJson->uiChar = 0x0A;
    }
}
static void vCr(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        spJson->uiChar = 0x0D;
    }
}
static void vTab(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        spJson->uiChar = 0x09;
    }
}
static void vUtf161(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        const achar* acpDigits = &spData->acpString[spData->uiParserOffset];
        char caHex[5];
        caHex[0] = (char)acpDigits[2];
        caHex[1] = (char)acpDigits[3];
        caHex[2] = (char)acpDigits[4];
        caHex[3] = (char)acpDigits[5];
        caHex[4] = 0;
        if(uiUtf16_1(caHex, &spJson->uiChar) != JSON_UTF16_MATCH){
            THROW_ERROR("UTF-16 encoding error - surrogate pair range not allowed", spData->uiParserOffset);
        }
    }
}
static void vUtf162(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        const achar* acpDigits = &spData->acpString[spData->uiParserOffset];
        char caHex[10];
        caHex[0] = (char)acpDigits[2];
        caHex[1] = (char)acpDigits[3];
        caHex[2] = (char)acpDigits[4];
        caHex[3] = (char)acpDigits[5];
        caHex[4] = 0;
        caHex[5] = (char)acpDigits[8];
        caHex[6] = (char)acpDigits[9];
        caHex[7] = (char)acpDigits[10];
        caHex[8] = (char)acpDigits[11];
        caHex[9] = 0;
        aint uiRet = uiUtf16_2(caHex, &spJson->uiChar);
        switch(uiRet){
        case JSON_UTF16_MATCH:
            return;
        case JSON_UTF16_NOMATCH:
            spData->uiCallbackState = ID_NOMATCH;
            return;
        case JSON_UTF16_BAD_HIGH:
            THROW_ERROR("UTF-16 encoding error - low surrogate not preceded by high surrogate", spData->uiParserOffset);
            return;
        case JSON_UTF16_BAD_LOW:
            THROW_ERROR("UTF-16 encoding error - high surrogate not followed by low surrogate", spData->uiParserOffset);
            return;
        }
    }
}
static void vNumber(callback_data* spData){
    if(spData->uiParserState == ID_ACTIVE){
        json* spJson = (json*)spData->vpUserData;
        spJson->bHasFrac = APG_FALSE;
        spJson->bHasMinus = APG_FALSE;
    }else if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        char caBuf[128];
        value_r* spValue = spFrameValue(spJson, spJson->spCurrentFrame, spData->uiParserOffset);
        spValue->uiId = JSON_ID_NUMBER;
        spValue->uiNumber = uiVecLen(spJson->vpVecNumbers);
        json_number* spNumber = (json_number*)vpVecPush(spJson->vpVecNumbers, NULL);
        // use the top of vpVecAscii as a working string
        aint uiTop = uiVecLen(spJson->vpVecAscii);
        char* cpNumber = (char*)vpVecPushn(spJson->vpVecAscii, NULL, (spData->uiParserPhraseLength + 1));
        aint ui = 0;
        char* cpPtr;
        const achar* acpParsed = &spData->acpString[spData->uiParserOffset];
        for(;ui < spData->uiParserPhraseLength; ui++){
            cpNumber[ui] = (char)acpParsed[ui];
        }
        cpNumber[ui] = 0;
        while(APG_TRUE){
            if(spJson->bHasFrac){
                spNumber->dFloat = strtod(cpNumber, &cpPtr);
                if(cpPtr > cpNumber){
                    spNumber->uiType = JSON_ID_FLOAT;
                    break;
                }
                snprintf(caBuf, 128, "Unable to convert floating point string with \"strtod()\": %s", cpNumber);
                THROW_ERROR(caBuf, spData->uiParserOffset);
                break;
            }
            if(spJson->bHasMinus){
                if(!bStringToInt(&cpNumber[1], &spNumber->iSigned)){
                    snprintf(caBuf, 128, "Integer value too large to convert to int: %s", cpNumber);
                    THROW_ERROR(caBuf, spData->uiParserOffset);
                }
                spNumber->iSigned = -spNumber->iSigned;
                spNumber->uiType = JSON_ID_SIGNED;
                break;
            }
            if(!bStringToUint(cpNumber, &spNumber->uiUnsigned)){
                snprintf(caBuf, 128, "Integer value too large to convert to unsigned int: %s", cpNumber);
                THROW_ERROR(caBuf, spData->uiParserOffset);
            }
            spNumber->uiType = JSON_ID_UNSIGNED;
            break;
        }
        // pop off the working string
        vpVecPopi(spJson->vpVecAscii, uiTop);
    }
}
static void vFracOnly(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        THROW_ERROR("Fraction found with no leading integer. Not allowed by RFC 8259.", spData->uiParserOffset);
    }
}
static void vFrac(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        spJson->bHasFrac= APG_TRUE;
    }
}
static void vFracDigits(callback_data* spData){
    if(spData->uiParserState == ID_NOMATCH){
        json* spJson = (json*)spData->vpUserData;
        THROW_ERROR("A decimal point must be followed by one or more digits (REF8259)", spData->uiParserOffset);
    }
}
static void vMinus(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        spJson->bHasMinus = APG_TRUE;
    }
}
static void vPlus(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        json* spJson = (json*)spData->vpUserData;
        THROW_ERROR("Leading plus (+) sign not allowed for decimal portion of floating point number (REF8259)", spData->uiParserOffset);
    }
}
static void vNameSeparator(callback_data* spData){
    if(spData->uiParserState == ID_NOMATCH){
        json* spJson = (json*)spData->vpUserData;
        THROW_ERROR("Expected key/value name separator (:) not found", spData->uiParserOffset);
    }
}

/*********************************************************
 * call back functions
 *********************************************************/
/** @name Another Private Function
 * Define the call back funtions to the parser.
 * Not static because referenced across multiple files. */
///@{
void vJsonGrammarRuleCallbacks(void* vpParserCtx){
    aint ui;
    parser_callback cb[RULE_COUNT_JSON_GRAMMAR];
    memset((void*)cb, 0, sizeof(cb));
    cb[JSON_GRAMMAR_ASCII] = vAscii;
    cb[JSON_GRAMMAR_BACKSPACE] = vBackSpace;
    cb[JSON_GRAMMAR_BEGIN_ARRAY] = vArrayBegin;
    cb[JSON_GRAMMAR_BEGIN_OBJECT] = vObjectBegin;
    cb[JSON_GRAMMAR_CHAR] = vChar;
    cb[JSON_GRAMMAR_CR] = vCr;
    cb[JSON_GRAMMAR_END_ARRAY] = vArrayEnd;
    cb[JSON_GRAMMAR_END_MEMBER_SEPARATOR] = vEndMemberSep;
    cb[JSON_GRAMMAR_END_OBJECT] = vObjectEnd;
    cb[JSON_GRAMMAR_END_VALUE_SEPARATOR] = vEndValueSep;
    cb[JSON_GRAMMAR_FALSE] = vFalse;
    cb[JSON_GRAMMAR_FORM_FEED] = vFormFeed;
    cb[JSON_GRAMMAR_FRAC] = vFrac;
    cb[JSON_GRAMMAR_FRAC_DIGITS] = vFracDigits;
    cb[JSON_GRAMMAR_FRAC_ONLY] = vFracOnly;
    cb[JSON_GRAMMAR_JSON_TEXT] = vJsonText;
    cb[JSON_GRAMMAR_KEY_BEGIN] = vKeyBegin;
    cb[JSON_GRAMMAR_LINE_FEED] = vLineFeed;
    cb[JSON_GRAMMAR_MINUS] = vMinus;
    cb[JSON_GRAMMAR_NAME_SEPARATOR] = vNameSeparator;
    cb[JSON_GRAMMAR_NULL] = vNull;
    cb[JSON_GRAMMAR_NUMBER] = vNumber;
    cb[JSON_GRAMMAR_PLUS] = vPlus;
    cb[JSON_GRAMMAR_QUOTE] = vQuote;
    cb[JSON_GRAMMAR_R_SOLIDUS] = vRSolidus;
    cb[JSON_GRAMMAR_SOLIDUS] = vSolidus;
    cb[JSON_GRAMMAR_STRING_BEGIN] = vStringBegin;
    cb[JSON_GRAMMAR_STRING_CONTENT] = vStringContent;
    cb[JSON_GRAMMAR_STRING_END] = vStringEnd;
    cb[JSON_GRAMMAR_TAB] = vTab;
    cb[JSON_GRAMMAR_TRUE] = vTrue;
    cb[JSON_GRAMMAR_UTF16_1] = vUtf161;
    cb[JSON_GRAMMAR_UTF16_2] = vUtf162;
    cb[JSON_GRAMMAR_UTF8_2] = vUtf82;
    cb[JSON_GRAMMAR_UTF8_2] = vUtf82;
    cb[JSON_GRAMMAR_UTF8_3] = vUtf83;
    cb[JSON_GRAMMAR_UTF8_4] = vUtf84;
    cb[JSON_GRAMMAR_VALUE] = vValue;

    for(ui = 0; ui < (aint)RULE_COUNT_JSON_GRAMMAR; ui++){
        vParserSetRuleCallback(vpParserCtx, ui, cb[ui]);
    }
}
///@}

