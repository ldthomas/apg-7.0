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
/** \file xml/basics.c
 * \brief Callback functions for basic rules common to all component parsers.
 */

#include "callbacks.h"

#define CLOSE_BRACKET   62
#define HYPHEN          45

static const uint32_t s_uiAmp = 38;
static const uint32_t s_uiSemi = 59;
static const char* s_cpLineFoundFmt = "line: %2"PRIuMAX" offset: %2"PRIuMAX"(0x%02"PRIXMAX"): %s";
static const char* s_cpLineNotFoundFmt = "line: %2"PRIuMAX" offset: %2"PRIuMAX"(0x%02"PRIXMAX")(EOF): %s";

/*********************************************************
 * HELPER FUNCTIONS ++++++++++++++++++++++++++++++++++++++
 ********************************************************/
void vThrowError(xml* spXml, const char* cpMsg, aint uiOffset, unsigned int uiLine, const char* cpFile, const char* cpFunc){
    aint uiXmlLine, uiRelOffset;
    char caBuf[CABUF_LEN];
    if(!bLinesFindLine(spXml->vpLines, uiOffset, &uiXmlLine, & uiRelOffset)){
        uiXmlLine = uiLinesCount(spXml->vpLines);
        uiRelOffset = uiLinesLength(spXml->vpLines);
        snprintf(caBuf, CABUF_LEN, s_cpLineNotFoundFmt,
                (luint)uiXmlLine, (luint)uiRelOffset, (luint)uiRelOffset, cpMsg);
    }else{
        snprintf(caBuf, CABUF_LEN, s_cpLineFoundFmt,
                (luint)uiXmlLine, (luint)uiRelOffset, (luint)uiRelOffset, cpMsg);
    }
    vExThrow(spXml->spException, caBuf, uiLine, cpFile, cpFunc);
}
void vLogMsg(xml* spXml, aint uiOffset, char* cpTitle){
    // log an error message noting its location in the document
    aint uiXmlLine, uiRelOffset;
    char caBuf[PATH_MAX + CABUF_LEN];
    size_t uiSize = PATH_MAX + CABUF_LEN;
    if(bLinesFindLine(spXml->vpLines, uiOffset, &uiXmlLine, &uiRelOffset)){
        snprintf(caBuf, uiSize, s_cpLineFoundFmt, (luint)uiXmlLine, (luint)uiRelOffset, (luint)uiRelOffset, cpTitle);
    }else{
        uiXmlLine = uiLinesCount(spXml->vpLines);
        uiRelOffset = uiLinesLength(spXml->vpLines);
        snprintf(caBuf, CABUF_LEN, s_cpLineNotFoundFmt,
                (luint)uiXmlLine, (luint)uiRelOffset, (luint)uiRelOffset, cpTitle);
    }
    vMsgsLog(spXml->vpMsgs, caBuf);
}
// push an element frame on the stack for each new element opened
void vPushFrame(callback_data* spData){
    xml* spXml = (xml*)spData->vpUserData;
    spXml->spCurrentFrame = (element_frame*)vpVecPush(spXml->vpVecFrame, NULL);
    memset((void*)spXml->spCurrentFrame, 0, sizeof(element_frame));
    spXml->spCurrentFrame->uiBase32 = uiVecLen(spXml->vpVec32);
    spXml->spCurrentFrame->uiBaseAtt = uiVecLen(spXml->vpVecAttList);
}

// return remaining number of frames on the stack
void vPopFrame(callback_data* spData){
    xml* spXml = (xml*)spData->vpUserData;
    if(spXml->spCurrentFrame){
        vpVecPopi(spXml->vpVec32, spXml->spCurrentFrame->uiBase32);
        vpVecPopi(spXml->vpVecAttList, spXml->spCurrentFrame->uiBaseAtt);
        element_frame* spFrame = (element_frame*)vpVecPop(spXml->vpVecFrame);
        // sanity check
        if(spFrame != spXml->spCurrentFrame){
            XML_THROW("popped frame not same as current frame");
        }
        spXml->spCurrentFrame = (element_frame*)vpVecLast(spXml->vpVecFrame);
    }
}

// convert two UTF-8 bytes to one UTF-32 code point
uint32_t ui2byte(const achar* acpBytes){
    uint32_t uiChar = 0;
    uiChar = ((uint32_t)acpBytes[0] & 0x1f) << 6;
    uiChar += (uint32_t)acpBytes[1] & 0x3f;
    return uiChar;
}
// convert three UTF-8 bytes to one UTF-32 code point
uint32_t ui3byte(const achar* acpBytes){
    uint32_t uiChar = 0;
    uiChar = ((uint32_t)acpBytes[0] & 0xf) << 12;
    uiChar += ((uint32_t)acpBytes[1] & 0x3f) << 6;
    uiChar += (uint32_t)acpBytes[2] & 0x3f;
    return uiChar;
}
// convert four UTF-8 bytes to one UTF-32 code point
uint32_t ui4byte(const achar* acpBytes){
    uint32_t uiChar = 0;
    uiChar = ((uint32_t)acpBytes[0] & 0x7) << 18;
    uiChar += ((uint32_t)acpBytes[1] & 0x3f) << 12;
    uiChar += ((uint32_t)acpBytes[2] & 0x3f) << 6;
    uiChar += (uint32_t)acpBytes[3] & 0x3f;
    return uiChar;
}
abool bValidateChar(uint32_t uiChar){
    abool bReturn = APG_FALSE;
    while(APG_TRUE){
        if(uiChar >= 0 && uiChar < 9){
            break; // disallowed ASCII control characters
        }
        if(uiChar > 10 && uiChar < 13){
            break; // disallowed ASCII control characters
        }
        if(uiChar > 13 && uiChar < 32){
            break; // disallowed ASCII control characters
        }
        if(uiChar >= 0xD800 && uiChar < 0xE000){
            break; // surrogate pairs block
        }
        if(uiChar == 0xFFFE || uiChar == 0xFFFF){
            break; // Unicode Standard D14
                   // Noncharacter: A code point that is permanently reserved for internal use.
        }
        if(uiChar > 0x10FFFF){
            break; // beyond Unicode range
        }
        bReturn = APG_TRUE;
        break;
    }
    return bReturn;
}

// convert an array of 32-bit code points (XML Chars) to a u32_phrase struct for user consumption
// assumes the code points are in the vector vpVeUint
// places ASCII strings in the vector vpVecString
// string data is transient
void vMakeCDataDisplay(xml* spXml, cdata_id* spDataId, u32_phrase* spCData, aint uiOffset){
    spCData->uiLength = spDataId->uiLength;
    spCData->uipPhrase = NULL;
    if(spDataId->uiLength){
        spCData->uipPhrase = (uint32_t*)vpVecAt(spXml->vpVec32, spDataId->uiOffset);
        if(!spCData->uipPhrase){
            vThrowError(spXml, "vector index unexpectedly out of range",
                    uiOffset, __LINE__, __FILE__, __func__);
        }
    }
}
void vMakeCDataIdFromInput(xml* spXml, const achar* acpInput, aint uiLen, aint uiOffset, cdata_id* spCDataId){
    // get some work space
    aint uiCheckPoint = uiVecLen(spXml->vpVec8);
    uint8_t* ucpData = (uint8_t*)vpVecPushn(spXml->vpVec8, NULL, uiLen);

    // convert from achar to uint8_t
    aint ui;
    for(ui = 0; ui < uiLen; ui++){
        ucpData[ui] = (uint8_t)acpInput[ui];
    }

    // decode UTF-8
    conv_src sSrc = {UTF_8, ucpData, uiLen};
    vConvDecode(spXml->vpConv, &sSrc);

    // get a copy of the UTF-32 code points
    spCDataId->uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
    vConvGetCodePoints(spXml->vpConv, NULL, &spCDataId->uiLength);
    uint32_t* uip32 = (uint32_t*)vpVecPushn(spXml->vpVec32, NULL, spCDataId->uiLength);
    vConvGetCodePoints(spXml->vpConv, uip32, &spCDataId->uiLength);

    // restore the work space
    vpVecPopi(spXml->vpVec8, uiCheckPoint);
}
// capture a parsed phrase and save it as UTF-32 code points
cdata_id sCapturePhrase(xml* spXml, achar* acpPhrase, aint uiPhraseLength, aint uiOffset){
    cdata_id sId = {uiVecLen(spXml->vpVec32), 0};
    if(uiPhraseLength){

        // allocate temporary work space on vpVecUnit8
        aint uiCheckPoint = uiVecLen(spXml->vpVec8);
        uint8_t* ucpBuf = (uint8_t*)vpVecPushn(spXml->vpVec8, NULL, uiPhraseLength);

        // intermediate conversion - convert parsed phrase from achar (the parser's alphabet characters)
        // to a byte stream.
        // NOTE: achar may vary from app to app, but the input characters will always be < 256;
        aint ui;
        for(ui = 0; ui < uiPhraseLength; ui++){
            ucpBuf[ui] = (uint8_t)acpPhrase[ui];
        }

        // convert the parsed byte stream to UTF-32 code points
        conv_src sSrc = {UTF_8, ucpBuf, uiPhraseLength};
        vConvDecode(spXml->vpConv, &sSrc);

        // allocate space to save the code points in vpVec32
        vConvGetCodePoints(spXml->vpConv, NULL, &sId.uiLength);
        uint32_t* uipCode = (uint32_t*)vpVecPushn(spXml->vpVec32, NULL, sId.uiLength);

        // copy the code points to vpVec32
        vConvGetCodePoints(spXml->vpConv, uipCode, &sId.uiLength);

        // release the work space on vpVec8
        vpVecPopi(spXml->vpVec8, uiCheckPoint);
    }
    return sId;
}

/** \brief Converts parsed UTF-8 data to UTF-32 code points.
 *
 * At times content is needed and it is necessary to convert the raw UTF-8 input data to internal 32-bit code points.
 * \param spXml An XML context pointer
 * \param acpData Pointer to the UTF-8 data to convert. Note that achar may not necessarily be 8-bit characters.
 * \param uiDataLen The number of input characters to convert.
 * \param uipOffset pointer to receive the offset into vpVec32 of the first converted code point
 * \param uipLength Pointer to receive the number of 32-bit code points.
 */
void vConvertParsedData(xml* spXml, const achar* acpData, aint uiDataLen, uint32_t* uipOffset, uint32_t* uipLength){
    if(!acpData || !uiDataLen){
        *uipOffset = uiVecLen(spXml->vpVec32);
        *uipLength = 0;
        return;
    }
    aint ui;
    conv_src sSrc;
    uint32_t* uipCodePoints;
    uint8_t* ucpData = (uint8_t*)acpData;
    if(sizeof(achar) != sizeof(uint8_t)){
        vVecClear(spXml->vpVec8);
        ucpData = (uint8_t*)vpVecPushn(spXml->vpVec8, NULL, uiDataLen);
        for(ui = 0; ui < uiDataLen; ui++){
            ucpData[ui] = (uint8_t)acpData[ui];
        }
    }
    sSrc.uiDataType = UTF_8;
    sSrc.ucpData = ucpData;
    sSrc.uiDataLen = uiDataLen;
    vConvDecode(spXml->vpConv, &sSrc);
    vConvGetCodePoints(spXml->vpConv, NULL, uipLength);
    *uipOffset = uiVecLen(spXml->vpVec32);
    uipCodePoints = (uint32_t*)vpVecPushn(spXml->vpVec32, NULL, *uipLength);
    vConvGetCodePoints(spXml->vpConv, uipCodePoints, uipLength);
}
/*********************************************************
 * HELPER FUNCTIONS --------------------------------------
 ********************************************************/

/*********************************************************
 * COMMENTS ++++++++++++++++++++++++++++++++++++++++++++++
 ********************************************************/
void vDoubleh(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("double hyphens ('--' or '--->') not allowed in comments");
    }
}
void vComment(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        if(spXml->pfnCommentCallback){
            uint8_t* ucpComment = (uint8_t*)spData->acpString + spData->uiParserOffset;
            if(sizeof(achar) != sizeof(uint8_t)){
                // convert the alphabet characters to uint8_t bytes
                vVecClear(spXml->vpVec8);
                ucpComment = (uint8_t*)vpVecPushn(spXml->vpVec8, NULL, spData->uiParserPhraseLength);
                aint ui = 0;
                aint uj = spData->uiParserOffset;
                for(; ui < spData->uiParserPhraseLength; ui++, uj++){
                    ucpComment[ui] = (uint8_t)spData->acpString[uj];
                }
            }

            // convert the UTF-8 comment to UTF-32
            conv_src sSrc;
            sSrc.uiDataType = UTF_8;
            sSrc.ucpData = ucpComment;
            sSrc.uiDataLen = spData->uiParserPhraseLength;
            vConvDecode(spXml->vpConv, &sSrc);
            aint uiIndex = uiVecLen(spXml->vpVec32);
            uint32_t uiComLen;
            vConvGetCodePoints(spXml->vpConv, NULL, &uiComLen);
            uint32_t* uipCom = (uint32_t*)vpVecPushn(spXml->vpVec32, NULL, uiComLen);
            vConvGetCodePoints(spXml->vpConv, uipCom, &uiComLen);

            // make display data and call the user's callback function
            u32_phrase sComData;
            cdata_id sComId = {uiIndex, (aint)uiComLen};
            vMakeCDataDisplay(spXml, &sComId, &sComData, spData->uiParserOffset);
            spXml->pfnCommentCallback(&sComData, spXml->vpCommentData);

            // clean up
            vpVecPopi(spXml->vpVec32, uiIndex);
            vVecClear(spXml->vpVec8);

            spData->uiCallbackState = ID_MATCH;
            spData->uiCallbackPhraseLength = spData->uiParserPhraseLength;
        }
    }
}
// UDT - handwritten comment callback function
/*********************************************************
 * COMMENTS ----------------------------------------------
 ********************************************************/

/*********************************************************
 * PROCESSING INSTRUCTIONS +++++++++++++++++++++++++++++++
 ********************************************************/
void vPIOpen(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        // NOTE: Processing Instructions use an element frame.
        // The PI target is stored in the frame start tage name.
        // the PI information is store in the frame end tag name
        vPushFrame(spData);
    }
}
void vPIClose(callback_data* spData){
    xml* spXml = (xml*)spData->vpUserData;
    if(spData->uiParserState == ID_MATCH){
        if(spXml->pfnPICallback){
            u32_phrase sTarget, sInfo;
            vMakeCDataDisplay(spXml, &spXml->spCurrentFrame->sSName, &sTarget, spData->uiParserOffset);
            vMakeCDataDisplay(spXml, &spXml->spCurrentFrame->sEName, &sInfo, spData->uiParserOffset);
            spXml->pfnPICallback(&sTarget, &sInfo, spXml->vpPIData);
        }
        vPopFrame(spData);
    }else if(spData->uiParserState == ID_NOMATCH){
        XML_THROW("expected close of processing instruction not found");
    }
}
void vPITarget(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        // swap sName and sSName;
        aint uiLen = uiVecLen(spXml->vpVecName);
        uint32_t* uipName = (uint32_t*)vpVecFirst(spXml->vpVecName);
        spXml->spCurrentFrame->sSName.uiOffset = uiVecLen(spXml->vpVec32);
        spXml->spCurrentFrame->sSName.uiLength = uiLen;
        vpVecPushn(spXml->vpVec32, (void*)uipName, uiLen);
    }else if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("processing instruction target is invalid");
    }
}
void vPIInfo(callback_data* spData){
    if(spData->uiParserState == ID_ACTIVE){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->spCurrentFrame->sEName.uiOffset = uiVecLen(spXml->vpVec32);
        spXml->spCurrentFrame->sEName.uiLength = 0;
    }
}
void vPIInfoq(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        uint32_t uiChars[2] = {63, spXml->uiChar};
        vpVecPushn(spXml->vpVec32, uiChars, 2);
        spXml->spCurrentFrame->sEName.uiLength += 2;
    }
}
//static void vPIInfo8(callback_data* spData){} same as vPIInfoa
void vPIInfoa(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        vpVecPush(spXml->vpVec32, &spXml->uiChar);
        spXml->spCurrentFrame->sEName.uiLength += 1;
    }
}
void vPIForbidden(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("Processing Instruction name \"xml\" is forbidden - see https://www.w3.org/XML/xml-V10-5e-errata");
    }
}
void vPIReserved(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("Processing Instruction names beginning with \"xml-\" are reserved - see https://www.w3.org/XML/xml-V10-5e-errata");
    }
}
/*********************************************************
 * PROCESSING INSTRUCTIONS -------------------------------
 ********************************************************/

/*********************************************************
 * BASICS ++++++++++++++++++++++++++++++++++++++++++++++++
 ********************************************************/
void vAscii(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->uiChar = (uint32_t)spData->acpString[spData->uiParserOffset];
    }
}
void vUtf82(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->uiChar = ui2byte(&spData->acpString[spData->uiParserOffset]);
    }
}
void vUtf83(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->uiChar = ui3byte(&spData->acpString[spData->uiParserOffset]);
    }
}
void vUtf84(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->uiChar = ui4byte(&spData->acpString[spData->uiParserOffset]);
    }
}
void vName(callback_data* spData){
    xml* spXml = (xml*)spData->vpUserData;
    if(spData->uiParserState == ID_ACTIVE){
        vVecClear(spXml->vpVecName);
    }
}
void vNameStartChar(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;

        // validate - must be in range
        // https://www.w3.org/TR/REC-xml/
        // Extensible Markup Language (XML) 1.0 (Fifth Edition)
        // W3C Recommendation 26 November 2008
        // [4] NameStartChar
        // ":" | [A-Z] | "_" | [a-z] | [#xC0-#xD6] | [#xD8-#xF6] | [#xF8-#x2FF] |
        //[#x370-#x37D] | [#x37F-#x1FFF] | [#x200C-#x200D] | [#x2070-#x218F] | [#x2C00-#x2FEF] | [#x3001-#xD7FF] |
        //[#xF900-#xFDCF] | [#xFDF0-#xFFFD] | [#x10000-#xEFFFF]
        aint uiFound = APG_TRUE;
        uint32_t ui = spXml->uiChar;
        while(APG_TRUE){
            if(ui >= 65 && ui <= 90){
                break;
            }
            if(ui >= 97 && ui <= 122){
                break;
            }
            if(ui == 58 || ui == 95){
                break;
            }
            if(ui >= 0xC0 && ui <= 0xD6){
                break;
            }
            if(ui >= 0xD8 && ui <= 0xF6){
                break;
            }
            if(ui >= 0xF8 && ui <= 0x2FF){
                break;
            }
            if(ui >= 0x370 && ui <= 0x37D){
                break;
            }
            if(ui >= 0x37F && ui <= 0x1FFF){
                break;
            }
            if(ui >= 0x200C && ui <= 0x200D){
                break;
            }
            if(ui >= 0x2070 && ui <= 0x218F){
                break;
            }
            if(ui >= 0x2C00 && ui <= 0x2FEF){
                break;
            }
            if(ui >= 0x3001 && ui <= 0xD7FF){
                break;
            }
            if(ui >= 0xF900 && ui <= 0xFDCF){
                break;
            }
            if(ui >= 0xFDF0 && ui <= 0xFFFD){
                break;
            }
            if(ui >= 0x10000 && ui <= 0xEFFFF){
                break;
            }
            uiFound = APG_FALSE;
            break;
        }
        if(uiFound){
            vpVecPush(spXml->vpVecName, &spXml->uiChar);
        }else{
            // Name fails because first character is not a NameStartChar
            spData->uiCallbackState = ID_NOMATCH;
        }
    }
}
void vNameOtherChar(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;

        // validate
        // | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] | [#x203F-#x2040]
        // https://www.w3.org/TR/REC-xml/
        // Extensible Markup Language (XML) 1.0 (Fifth Edition)
        // W3C Recommendation 26 November 2008
        // [4a] NameChar
        aint uiFound = APG_TRUE;
        uint32_t ui = spXml->uiChar;
        while(APG_TRUE){
            if(ui >= 48 && ui <= 57){
                break;
            }
            if(ui == 45 || ui == 46){
                break;
            }

            if(ui == 0xB7){
                break;
            }
            if(ui >= 0x300 && ui <= 0x36F){
                break;
            }
            if(ui >= 0x203F && ui <= 0x2040){
                break;
            }
            uiFound = APG_FALSE;
            break;
        }
        if(uiFound){
            vpVecPush(spXml->vpVecName, &spXml->uiChar);
        }else{
            // note a valid name char
            spData->uiCallbackState = ID_NOMATCH;
        }
    }
}

uint32_t uiHexValue32(xml* spXml, aint uiOffset, uint32_t* uipChars, uint32_t uiCount){
    uint32_t ui;
    uint32_t uiSum = 0;
    uint32_t uiDigit = 0;
    char caBuf[CABUF_LEN];
    for(ui = 0; ui < uiCount; ui++){
        if(uipChars[ui] >= 48 && uipChars[ui] <= 57){
            uiDigit = uipChars[ui] - 48;
        }else if(uipChars[ui] >= 65 && uipChars[ui] <= 70){
            uiDigit = uipChars[ui] - 55;
        }else if(uipChars[ui] >= 97 && uipChars[ui] <= 102){
            uiDigit = uipChars[ui] - 87;
        }else{
            snprintf(caBuf, CABUF_LEN, "illegal hex digit in Reference: %c", (char)uipChars[ui]);
            vThrowError(spXml, caBuf,
                    uiOffset, __LINE__, __FILE__, __func__);
        }
        if(!bMultiply32(uiSum, 16, &uiSum)){
            vThrowError(spXml, "decimal value in Reference is too large: causes uint32_t overflow",
                    uiOffset, __LINE__, __FILE__, __func__);
        }
        if(!bSum32(uiSum, uiDigit, &uiSum)){
            vThrowError(spXml, "decimal value in Reference is too large: causes uint32_t overflow",
                    uiOffset, __LINE__, __FILE__, __func__);
        }
    }
    if(!bValidateChar(uiSum)){
        char caBuf[CABUF_LEN];
        snprintf(caBuf, CABUF_LEN, "Well-formedness Constraint: Legal Character\n"
                "Characters referred to using character references MUST match the production for Char\n"
                "https://www.w3.org/TR/REC-xml/#sec-references\n"
                "hex character: 0x%X", uiSum);
        vThrowError(spXml, caBuf,
                uiOffset, __LINE__, __FILE__, __func__);
    }
    return uiSum;
}
uint32_t uiDecValue32(xml* spXml, aint uiOffset, uint32_t* uipChars, uint32_t uiCount){
    uint32_t ui, uiSum, uiDigit;
    uiSum = 0;
    for(ui = 0; ui < uiCount; ui++){
        uiDigit = uipChars[ui] - 48;
        if(!bMultiply32(uiSum, 10, &uiSum)){
            vThrowError(spXml, "decimal value in Reference is too large: causes uint32_t overflow",
                    uiOffset, __LINE__, __FILE__, __func__);
        }
        if(!bSum32(uiSum, uiDigit, &uiSum)){
            vThrowError(spXml, "decimal value in Reference is too large: causes uint32_t overflow",
                    uiOffset, __LINE__, __FILE__, __func__);
        }
    }
    if(!bValidateChar(uiSum)){
        char caBuf[CABUF_LEN];
        snprintf(caBuf, CABUF_LEN, "Well-formedness Constraint: Legal Character\n"
                "Characters referred to using character references MUST match the production for Char\n"
                "https://www.w3.org/TR/REC-xml/#sec-references\n"
                "decimal character: %u", uiSum);
        vThrowError(spXml, caBuf,
                uiOffset, __LINE__, __FILE__, __func__);
    }
    return uiSum;
}
att_decl* spLeftMostElement(xml* spXml, att_decl* spAttList){
    att_decl* spReturn = NULL;
    att_decl* spNamed = (att_decl*)vpVecFirst(spXml->vpVecAttDecls);
    aint uiCount = uiVecLen(spXml->vpVecAttDecls);
    att_decl* spAm;
    aint uiL, uiR, uiM;
    int i;
    if(spNamed && uiCount){
        uint32_t* uipChar32 = (uint32_t*)vpVecFirst(spXml->vpVec32);
        uint32_t* uipName = uipChar32 + spAttList->sElementName.uiOffset;
        uint32_t uiNameLen = spAttList->sElementName.uiLength;
        uiL = 0;
        uiR = uiCount;
        while(uiL < uiR){
            uiM = uiL + (uiR - uiL)/2;
            spAm = &spNamed[uiM];
            i = iCompNames(&uipChar32[spAm->sElementName.uiOffset], spAm->sElementName.uiLength, uipName, uiNameLen);
            if(i < 0){
                uiL = uiM + 1;
            }else{
                uiR = uiM;
            }
        }
        if((uiL < uiCount)){
            spAm = &spNamed[uiL];
            i = iCompNames(&uipChar32[spAm->sElementName.uiOffset], spAm->sElementName.uiLength, uipName, uiNameLen);
            if( i == 0){
                spReturn = spAm;
            }
        }
    }
    return spReturn;
}
/** \brief Find the left-most occurrence of the given entity name.
 *
 * If the list of entity names are alphabetical this binary algorithm will find the given name.
 * If there is more than one identical name in the list, this algorithm will find the left-most occurrence of that name.
 * https://en.wikipedia.org/wiki/Binary_search_algorithm#Procedure_for_finding_the_leftmost_element
 */
entity_decl* spEntityNameLookup(xml* spXml, aint uiOffset, uint32_t* uipName, uint32_t uiNameLen){
    entity_decl* spReturn = NULL;
    entity_decl* spNamed = (entity_decl*)vpVecFirst(spXml->vpVecGEDefs);
    aint uiCount = uiVecLen(spXml->vpVecGEDefs);
    entity_decl* spAm;
    aint uiL, uiR, uiM;
    int i;
    if(spNamed && uiCount){
        uint32_t* uipChar32 = (uint32_t*)vpVecFirst(spXml->vpVec32);
        uiL = 0;
        uiR = uiCount;
        while(uiL < uiR){
            uiM = uiL + (uiR - uiL)/2;
            spAm = &spNamed[uiM];
            i = iCompNames(&uipChar32[spAm->sName.uiOffset], spAm->sName.uiLength, uipName, uiNameLen);
            if(i < 0){
                uiL = uiM + 1;
            }else{
                uiR = uiM;
            }
        }
        if((uiL < uiCount)){
            spAm = &spNamed[uiL];
            i = iCompNames(&uipChar32[spAm->sName.uiOffset], spAm->sName.uiLength, uipName, uiNameLen);
            if( i == 0){
                spReturn = spAm;
            }
        }
    }
    return spReturn;
}
abool bCompNames(const uint32_t* uipLName, uint32_t uiLLen, const uint32_t* uipRName, uint32_t uiRLen){
    if(0 == iCompNames(uipLName, uiLLen, uipRName, uiRLen)){
        return APG_TRUE;
    }
    return APG_FALSE;
}
cdata_id sNormalizeAttributeValue(xml* spXml, aint uiOffset, uint32_t* uipAttValue, uint32_t uiLength, abool bIsCDATA){
    cdata_id sReturn = {};
    uint32_t* uipData, *uipDataEnd;;
    vVecClear(spXml->vpVecAttWork);
    uint32_t ui, uiInc, uiChar, uiLastChar, uiRef;
    uint32_t uiSp = 32;
    ui = 0;
    uiInc = 0;
    while(ui < uiLength){
        if(uipAttValue[ui] == ATT_AMP){
            for(uiInc = ui + 1; uiInc < uiLength; uiInc++){
                if(uipAttValue[uiInc] == ATT_SEMI){
                    goto found;
                }                    }
            vThrowError(spXml, "attribute value has & (begins character or entity reference) with no closing ;",
                    uiOffset,  __LINE__, __FILE__, __func__);
            found:;
            if(uipAttValue[ui+1] == ATT_HASH){
                if(uipAttValue[ui+2] == ATT_X){
                    // handle hex ref
                    uiRef = ui + 3;
                    uiChar = uiHexValue32(spXml, uiOffset, &uipAttValue[uiRef], uiInc-uiRef);
                }else{
                    //handle dec ref
                    uiRef = ui + 2;
                    uiChar = uiDecValue32(spXml, uiOffset, &uipAttValue[uiRef], uiInc-uiRef);
                }
                vpVecPush(spXml->vpVecAttWork, &uiChar);
            }else{
                // handle entity ref
                uiRef = ui + 1;
                entity_decl* spEntity = spEntityNameLookup(spXml, uiOffset, &uipAttValue[uiRef], uiInc-uiRef);
                if(!spEntity){
                    vThrowError(spXml, "undeclared entity name in attribute list value",
                            uiOffset,  __LINE__, __FILE__, __func__);
                }
                if(!spEntity->bExpanded){
                    vExpandEntity(spXml, uiOffset, spEntity);
                }
                uipData = (uint32_t*)vpVecPushn(spXml->vpVecAttWork, vpVecAt(spXml->vpVec32, spEntity->sValue.uiOffset),
                        spEntity->sValue.uiLength);
                uipDataEnd = uipData + spEntity->sValue.uiLength;
                for(; uipData < uipDataEnd; uipData++){
                    if(*uipData == 9 || *uipData == 10 || *uipData == 13){
                        *uipData = 32;
                    }
                }
            }
            ui = uiInc + 1;
        }else{
            // handle char
            uiChar = uipAttValue[ui++];
            if(uiChar == 9 || uiChar == 10 || uiChar == 13){
                vpVecPush(spXml->vpVecAttWork, &uiSp);
            }else{
                vpVecPush(spXml->vpVecAttWork, &uiChar);
            }
        }
    }
    // check for <
    uipData = vpVecFirst(spXml->vpVecAttWork);
    uiInc = (uint32_t)uiVecLen(spXml->vpVecAttWork);
    for(ui = 0; ui < uiInc; ui ++){
        if(uipData[ui] == 60){
            vLogMsg(spXml, uiOffset, "Well-formedness constraint: No \"<\" in Attribute Values\n"
                    "The replacement text of any entity referred to directly or indirectly in an attribute value MUST NOT contain a <.");
            break;
        }
    }
    //replace attribute value with normalized value in work vector
    if(bIsCDATA){
        // copy as is
        sReturn.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
        sReturn.uiLength = (uint32_t)uiVecLen(spXml->vpVecAttWork);
        vpVecPushn(spXml->vpVec32, vpVecFirst(spXml->vpVecAttWork), uiVecLen(spXml->vpVecAttWork));
        return sReturn;
    }
    // Not CDATA: therefore remove leading, trailing and multiple interior whitespace
    // (set white space to remove to zero (0 is not a valid XML character), then skip zeros when copying
    uint32_t* uipChar = (uint32_t*)vpVecFirst(spXml->vpVecAttWork);
    uint32_t* uipEnd = uipChar + (uint32_t)uiVecLen(spXml->vpVecAttWork);
    uint32_t uiState = STATE_BEGIN;
    for(; uipChar < uipEnd; uipChar++){
        if(uiState == STATE_BEGIN){
            if(*uipChar == 32){
                *uipChar = 0;
            }else{
                uiState = STATE_TEXT;
            }
        }else if(uiState == STATE_TEXT){
            if(*uipChar == 32){
                uiState = STATE_WSP;
            }
        }else if(uiState == STATE_WSP){
            if(*uipChar == 32){
                *uipChar = 0;
            }else{
                uiState = STATE_TEXT;
            }
        }else{
            // error
            vThrowError(spXml, "attribute value normalization: should never get here",
                    uiOffset,  __LINE__, __FILE__, __func__);
        }
    }

    // remove excess white space (leading, trailing and multiple occurrences in between)
    uipChar = (uint32_t*)vpVecFirst(spXml->vpVecAttWork);
    uiLength = (uint32_t)uiVecLen(spXml->vpVecAttWork);
    sReturn.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
    sReturn.uiLength = 0;
    uiLastChar = 0;
    for(ui = 0; ui < uiLength; ui++){
        uiChar = uipChar[ui];
        if(uiChar != 0){
            vpVecPush(spXml->vpVec32, &uiChar);
            sReturn.uiLength++;
            uiLastChar = uiChar;
        }
    }
    if(uiLastChar == 32){
        vpVecPop(spXml->vpVec32);
        sReturn.uiLength--;
    }
    return sReturn;
}
int iAttComp(const void* vpL, const void* vpR){
    const att_decl* spLeft = (att_decl*)vpL;
    const att_decl* spRight = (att_decl*)vpR;
    uint32_t uiLen = spLeft->sElementName.uiLength < spRight->sElementName.uiLength ?
            spLeft->sElementName.uiLength : spRight->sElementName.uiLength;
    uint32_t ui = 0;
    uint32_t *uipLeft = (uint32_t*)vpVecFirst(spLeft->spXml->vpVec32) + spLeft->sElementName.uiOffset;
    uint32_t *uipRight = (uint32_t*)vpVecFirst(spLeft->spXml->vpVec32) + spRight->sElementName.uiOffset;
    for(; ui < uiLen; ui++){
        if(uipLeft[ui] < uipRight[ui]){
            return -1;
        }
        if(uipLeft[ui] > uipRight[ui]){
            return 1;
        }
    }
    if(spLeft->sElementName.uiLength < spRight->sElementName.uiLength){
        return -1;
    }
    if(spLeft->sElementName.uiLength > spRight->sElementName.uiLength){
        return 1;
    }
    return 0;
}
int iEntityComp(const void* vpL, const void* vpR){
    const entity_decl* spLeft = (entity_decl*)vpL;
    const entity_decl* spRight = (entity_decl*)vpR;
    uint32_t uiLen = spLeft->sName.uiLength < spRight->sName.uiLength ? spLeft->sName.uiLength : spRight->sName.uiLength;
    uint32_t ui = 0;
    uint32_t *uipLeft = (uint32_t*)vpVecFirst(spLeft->spXml->vpVec32) + spLeft->sName.uiOffset;
    uint32_t *uipRight = (uint32_t*)vpVecFirst(spLeft->spXml->vpVec32) + spRight->sName.uiOffset;
    for(; ui < uiLen; ui++){
        if(uipLeft[ui] < uipRight[ui]){
            return -1;
        }
        if(uipLeft[ui] > uipRight[ui]){
            return 1;
        }
    }
    if(spLeft->sName.uiLength < spRight->sName.uiLength){
        return -1;
    }
    if(spLeft->sName.uiLength > spRight->sName.uiLength){
        return 1;
    }
    return 0;
}
void vExpandEntity(xml* spXml, aint uiOffset, entity_decl* spThisEntity){
    if(!spThisEntity->bExpanded){
        uint32_t* uipChars32, *uipTemp;
        entity_frame* spFrame = NULL;
        uint32_t uiEntityOffset, uiEntityLength;
        uint32_t uiFromOffset, uiRemainingChars, uiCopyChars;
        uint32_t ui;
        aint uiFrames, uiReplacementBegin;
        entity_decl* spEntityFound;
        uiFromOffset = spThisEntity->sValue.uiOffset;
        uiRemainingChars = spThisEntity->sValue.uiLength;
        uipChars32 = (uint32_t*)vpVecFirst(spXml->vpVec32);
        while(bHasEntity(&uipChars32[uiFromOffset], uiRemainingChars, &uiEntityOffset, &uiEntityLength)){
            // look up the name and make sure it has been declared
            spEntityFound = spEntityNameLookup(spXml, uiOffset,
                    &uipChars32[uiFromOffset + uiEntityOffset + 1], (uiEntityLength - 2));
            if(!spEntityFound){
                vThrowError(spXml, "General Entity refers to undeclared entity",
                        spThisEntity->uiInputOffset, __LINE__, __FILE__, __func__);
            }
            // see if it is referring to itself indirectly
            uiFrames = uiVecLen(spXml->vpVecEntityFrames);
            if(uiFrames > 1){
                entity_frame* spParent = (entity_frame*)vpVecFirst(spXml->vpVecEntityFrames);
                entity_frame* spEnd = spParent + uiFrames - 1;
                for(; spParent < spEnd; spParent++){
                    if(spParent->uiNameOffset == spThisEntity->sName.uiOffset){
                        vThrowError(spXml, "General Entity refers to itself indirectly",
                                spThisEntity->uiInputOffset, __LINE__, __FILE__, __func__);
                    }
                }
            }
            if(!spFrame){
                // push a frame on the stack for this entity expansion
                spFrame = (entity_frame*)vpVecPush(spXml->vpVecEntityFrames, NULL);
                spFrame->uiNameOffset = spThisEntity->sName.uiOffset;
                uiReplacementBegin = uiVecLen(spXml->vpVec32);
            }
            // copy the prefix
            if(uiEntityOffset){
                uipTemp = (uint32_t*)vpVecPushn(spXml->vpVec32, NULL, uiEntityOffset);
                uipChars32 = (uint32_t*)vpVecFirst(spXml->vpVec32);
                for(ui = 0; ui < uiEntityOffset; ui++){
                    uipTemp[ui] = uipChars32[uiFromOffset + ui];
                }
            }

            // update the "copy from" info to just past the found entity - ready to search for another
            uiCopyChars = uiEntityOffset + uiEntityLength;
            uiFromOffset += uiCopyChars;
            uiRemainingChars = (uiCopyChars < uiRemainingChars) ? (uiRemainingChars - uiCopyChars) : 0;
            if(spEntityFound->bExpanded){
                // it has been expanded, just use it
                uiCopyChars = spEntityFound->sValue.uiLength;
                uipTemp = (uint32_t*)vpVecPushn(spXml->vpVec32, NULL, uiCopyChars);
                uipChars32 = (uint32_t*)vpVecFirst(spXml->vpVec32);
                for(ui = 0; ui < uiCopyChars; ui++){
                    uipTemp[ui] = uipChars32[spEntityFound->sValue.uiOffset + ui];
                }
            }else if(bHasEntity(&uipChars32[spEntityFound->sValue.uiOffset],
                    spEntityFound->sValue.uiLength, &uiEntityOffset, &uiEntityLength)){
                // expand the found entity (note that its expanded value will be put at the end of our work so far)
                vExpandEntity(spXml, uiOffset, spEntityFound);
            }else{
                // just copy the entity from its original place to here
                spEntityFound->bExpanded = APG_TRUE;
                uiEntityOffset = spEntityFound->sValue.uiOffset;
                uiEntityLength = spEntityFound->sValue.uiLength;
                uipTemp = (uint32_t*)vpVecPushn(spXml->vpVec32, NULL, uiEntityLength);
                uipChars32 = (uint32_t*)vpVecFirst(spXml->vpVec32);
                for(ui = 0; ui < uiEntityLength; ui++){
                    uipTemp[ui] = uipChars32[uiEntityOffset + ui];
                }
            }
        }
        if(spFrame){
            // copy the tail
            if(uiRemainingChars){
                uipTemp = (uint32_t*)vpVecPushn(spXml->vpVec32, NULL, uiRemainingChars);
                uipChars32 = (uint32_t*)vpVecFirst(spXml->vpVec32);
                for(ui = 0; ui < uiRemainingChars; ui++){
                    uipTemp[ui] = uipChars32[uiFromOffset + ui];
                }
            }
            spThisEntity->sValue.uiOffset = uiReplacementBegin;
            spThisEntity->sValue.uiLength = (uint32_t)uiVecLen(spXml->vpVec32) - uiReplacementBegin;
            vpVecPop(spXml->vpVecEntityFrames);
        }
        spThisEntity->bExpanded = APG_TRUE;
    }
}
abool bHasEntity(uint32_t* uipChars, uint32_t uiLen, uint32_t* uipEntityOffset, uint32_t* uipEntityLen){
    abool bReturn = APG_FALSE;
    uint32_t ui = 0;
    uint32_t uiFoundLen = 0;
    *uipEntityOffset = 0;
    for(; ui < uiLen; ui++){
        if(uipChars[ui] == s_uiAmp){
            *uipEntityOffset = ui;
            uiFoundLen = 0;
        }
        if(uipChars[ui] == s_uiSemi){
            bReturn = APG_TRUE;
            uiFoundLen++;
            goto found;
        }
        uiFoundLen++;
    }
    uiFoundLen = 0;
    found:;
    *uipEntityLen = uiFoundLen;
    return bReturn;

}
int iCompNames(const uint32_t* uipLName, uint32_t uiLLen, const uint32_t* uipRName, uint32_t uiRLen){
    uint32_t uiLen = (uiLLen < uiRLen) ? uiLLen : uiRLen;
    uint32_t ui = 0;
    for(; ui < uiLen; ui++){
        if(uipLName[ui] < uipRName[ui]){
            return -1;
        }
        if(uipLName[ui] > uipRName[ui]){
            return 1;
        }
    }
    if(uiLLen < uiRLen){
        return -1;
    }
    if(uiLLen > uiRLen){
        return 1;
    }
    return 0;
}

/*********************************************************
 * BASICS ------------------------------------------------
 ********************************************************/

void vXmlgrammarRuleCallbacks(void* vpParserCtx){
    aint ui;
    parser_callback cb[RULE_COUNT_XMLGRAMMAR];
    memset((void*)cb, 0, sizeof(cb));
    cb[XMLGRAMMAR_ANOTLAQ] = vAscii;
    cb[XMLGRAMMAR_ANOTLAA] = vAscii;
    cb[XMLGRAMMAR_ANOTGT] = vAscii;
    cb[XMLGRAMMAR_ANOTLA] = vAscii;
    cb[XMLGRAMMAR_ANOTQ] = vAscii;
    cb[XMLGRAMMAR_ASCII] = vAscii;
    cb[XMLGRAMMAR_ANOTPAQ] = vAscii;
    cb[XMLGRAMMAR_ANOTPAA] = vAscii;
    cb[XMLGRAMMAR_ANOTQUOT] = vAscii;
    cb[XMLGRAMMAR_ANOTRB] = vAscii;
    cb[XMLGRAMMAR_ANOTAPOS] = vAscii;
    cb[XMLGRAMMAR_ATTCHARA] = vEntityChar;
    cb[XMLGRAMMAR_ATTCHARD] = vEntityChar;
    cb[XMLGRAMMAR_ATTLISTOPEN] = vAttlistOpen;
    cb[XMLGRAMMAR_ATTNAME] = vAttName;
    cb[XMLGRAMMAR_ATTLISTVALUE] = vAttlistValue;
    cb[XMLGRAMMAR_ATTLISTCLOSE] = vAttlistClose;
    cb[XMLGRAMMAR_ATTDEF] = vAttDef;
    cb[XMLGRAMMAR_ATTVALUE] = vAttValue;
    cb[XMLGRAMMAR_ELATTNAME] = vElAttName;
    cb[XMLGRAMMAR_ATTTYPE] = vAttType;
    cb[XMLGRAMMAR_CDCHAR] = vCharData;
    cb[XMLGRAMMAR_CDEND] = vCDEnd;
    cb[XMLGRAMMAR_CDRB] = vCDRb;
    cb[XMLGRAMMAR_CD2RB] = vCD2Rb;
    cb[XMLGRAMMAR_CHARDATA] = vCharData;
    cb[XMLGRAMMAR_CONTENTREF] = vCharData;
    cb[XMLGRAMMAR_COMMENT] = vComment;
    cb[XMLGRAMMAR_CLOSEQUOT] = vCloseQuote;
    cb[XMLGRAMMAR_CLOSEAPOS] = vCloseQuote;
    cb[XMLGRAMMAR_CDSECTEND] = vCDSectEnd;
    cb[XMLGRAMMAR_DECVALUE] = vDecValue;
    cb[XMLGRAMMAR_DOCUMENT] = vDocument;
    cb[XMLGRAMMAR_DOCNAME] = vDtdName;
    cb[XMLGRAMMAR_DOCOPEN] = vDtdOpen;
    cb[XMLGRAMMAR_DOCCLOSE] = vDtdClose;
    cb[XMLGRAMMAR_DOUBLEH] = vDoubleh;
    cb[XMLGRAMMAR_DCHAR] = vDChar;
    cb[XMLGRAMMAR_DVALUE] = vDvalue;
    cb[XMLGRAMMAR_ELEMENTCLOSE] = vElementClose;
    cb[XMLGRAMMAR_ELEMENTOPEN] = vElementOpen;
    cb[XMLGRAMMAR_EMPTYCLOSE] = vEmptyClose;
    cb[XMLGRAMMAR_ENCDEF] = vEncDef;
    cb[XMLGRAMMAR_ENCNAME] = vEncName;
    cb[XMLGRAMMAR_EOPEN] = vEOpen;
    cb[XMLGRAMMAR_ERESERVED] = vEReserved;
    cb[XMLGRAMMAR_ESTART] = vEStart;
    cb[XMLGRAMMAR_ETAGCLOSE] = vETagClose;
    cb[XMLGRAMMAR_EXTERNALID] = vExternalID;
    cb[XMLGRAMMAR_NEXTERNALID] = vNExternalID;
    cb[XMLGRAMMAR_ENTITYCHARA] = vEntityChar;
    cb[XMLGRAMMAR_ENTITYCHARD] = vEntityChar;
    cb[XMLGRAMMAR_ENTITYREF] = vEntityRef;
    cb[XMLGRAMMAR_EXTSUBSET] = vExtSubset;
    cb[XMLGRAMMAR_GEDEFEX] = vGEDefEx;
    cb[XMLGRAMMAR_GEPEREF] = vGEPERef;
    cb[XMLGRAMMAR_GEREF] = vGERef;
    cb[XMLGRAMMAR_GEDECLCLOSE] = vGEDeclClose;
    cb[XMLGRAMMAR_GEDECLNAME] = vGEDeclName;
    cb[XMLGRAMMAR_HEXVALUE] = vHexValue;
    cb[XMLGRAMMAR_NAME] = vName;
    cb[XMLGRAMMAR_NAMEOTHERCHAR] = vNameOtherChar;
    cb[XMLGRAMMAR_NAMESTARTCHAR] = vNameStartChar;
    cb[XMLGRAMMAR_NOTATIONCLOSE] = vNotationClose;
    cb[XMLGRAMMAR_NOTATIONDEF] = vNotationDef;
    cb[XMLGRAMMAR_NOTATIONOPEN] = vNotationOpen;
    cb[XMLGRAMMAR_PEDECLOPEN] = vPEDeclOpen;
    cb[XMLGRAMMAR_PEDEF] = NULL;
    cb[XMLGRAMMAR_PEDECLCLOSE] = vPEDeclClose;
    cb[XMLGRAMMAR_PEREFERENCE] = vPEReference;
    cb[XMLGRAMMAR_PEREFERROR] = vPERefError;
    cb[XMLGRAMMAR_PICLOSE] = vPIClose;
    cb[XMLGRAMMAR_PIFORBIDDEN] = vPIForbidden;
    cb[XMLGRAMMAR_PIRESERVED] = vPIReserved;
    cb[XMLGRAMMAR_PIINFO] = vPIInfo;
    cb[XMLGRAMMAR_PIINFOA] = vPIInfoa;
    cb[XMLGRAMMAR_PIINFOCHAR] = NULL;
    cb[XMLGRAMMAR_PIINFOQ] = vPIInfoq;
    cb[XMLGRAMMAR_PIOPEN] = vPIOpen;
    cb[XMLGRAMMAR_PITARGET] = vPITarget;
    cb[XMLGRAMMAR_REFCLOSE] = vRefClose;
    cb[XMLGRAMMAR_SDECLOTHER] = vSDeclOther;
    cb[XMLGRAMMAR_SDECLNO] = vSDeclNo;
    cb[XMLGRAMMAR_SDECLYES] = vSDeclYes;
    cb[XMLGRAMMAR_STAGCLOSE] = vSTagClose;
    cb[XMLGRAMMAR_SCHAR] = vDChar;
    cb[XMLGRAMMAR_SVALUE] = vDvalue;
    cb[XMLGRAMMAR_UTF82] = vUtf82;
    cb[XMLGRAMMAR_UTF83] = vUtf83;
    cb[XMLGRAMMAR_UTF84] = vUtf84;
    cb[XMLGRAMMAR_VERSIONINFO] = vVersionInfo;
    cb[XMLGRAMMAR_VERSIONNUM] = vVersionNum;
    cb[XMLGRAMMAR_XMLDECLCLOSE] = vXmlDeclClose;
    cb[XMLGRAMMAR_XMLDECLOPEN] = vXmlDeclOpen;

    for(ui = 0; ui < (aint)RULE_COUNT_XMLGRAMMAR; ui++){
        vParserSetRuleCallback(vpParserCtx, ui, cb[ui]);
    }
}
