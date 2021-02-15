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
/** \file xml/elements.c
 * \brief Callback functions for the element component rule names.
 */

#include "callbacks.h"

typedef struct {
    u32_phrase sName;
    u32_phrase* spAttNames;
    u32_phrase* spAttValues;
} att_cdata;
static att_cdata sMakeAtts(xml*spXml, element_frame* spF, aint uiOffset);
static abool bAttNameLookup(xml* spXml, uint32_t* uipName, uint32_t uiLen);
static att_decl* spElNameLookup(xml* spXml, uint32_t* uipEName, uint32_t uiELen);

static uint8_t s_ucRightBracket = 93;

/*********************************************************
 * ELEMENTS ++++++++++++++++++++++++++++++++++++++++++++++
 ********************************************************/
void vEStart(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        // check for default attributes
        element_frame* spF = spXml->spCurrentFrame; // convenience
        uint32_t* uipChars = (uint32_t*)vpVecFirst(spXml->vpVec32);
        uint32_t* uipName = uipChars + spF->sSName.uiOffset;
        att_decl* spDecl = spElNameLookup(spXml, uipName, spF->sSName.uiLength);
        if(spDecl){
            // this element has default attribute values
            att_decl* spEnd = spDecl + spDecl->uiAttCount;
            for(; spDecl < spEnd; spDecl++){
                uipName = uipChars + spDecl->sAttName.uiOffset;
                if(!bAttNameLookup(spXml, uipName, spDecl->sAttName.uiLength)){
                    // no attribute of this default name exists - add the attribute with its default value
                    named_value* spNew = (named_value*)vpVecPush(spXml->vpVecAttList, NULL);
                    spNew->sName = spDecl->sAttName;
                    spNew->sValue = spDecl->sAttValue;
                    spF->uiAttCount++;
                }
            }
        }
    }
}
void vEOpen(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        aint uiLen = uiVecLen(spXml->vpVecName);
        vPushFrame(spData);
        spXml->spCurrentFrame->uiElementOffset = spData->uiParserOffset;
        spXml->spCurrentFrame->sSName.uiOffset = uiVecLen(spXml->vpVec32);
        spXml->spCurrentFrame->sSName.uiLength = (uint32_t)uiLen;
        vpVecPushn(spXml->vpVec32, vpVecFirst(spXml->vpVecName), uiLen);
    }
}
void vEReserved(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("Tag names beginning with \"xml:\" are reserved - Extensible Markup Language (XML) 1.0 (Fifth Edition) errata\n"
                "https://www.w3.org/XML/xml-V10-5e-errata");
    }
}
void vEmptyClose(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        if(spXml->pfnEmptyTagCallback){
            att_cdata sAttData = sMakeAtts(spXml, spXml->spCurrentFrame, spData->uiParserOffset);
            spXml->pfnEmptyTagCallback(&sAttData.sName, sAttData.spAttNames, sAttData.spAttValues,
                    spXml->spCurrentFrame->uiAttCount, spXml->vpEmptyTagData);
        }
        vPopFrame(spData);
    }
}
void vSTagClose(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        if(spXml->pfnStartTagCallback){
            att_cdata sAttData = sMakeAtts(spXml, spXml->spCurrentFrame, spData->uiParserOffset);
            spXml->pfnStartTagCallback(&sAttData.sName, sAttData.spAttNames, sAttData.spAttValues,
                    spXml->spCurrentFrame->uiAttCount, spXml->vpStartTagData);
        }

        // initialize content
        spXml->spCurrentFrame->sContent.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
        spXml->spCurrentFrame->sContent.uiLength = 0;
    }else if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("malformed start tag");
    }
}
void vETagClose(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        element_frame* spF = spXml->spCurrentFrame; // convenience
        uint32_t uiELen = (uint32_t)uiVecLen(spXml->vpVecName);
        uint32_t* uipEName = (uint32_t*)vpVecFirst(spXml->vpVecName);
        uint32_t* uipSName = (uint32_t*)vpVecAt(spXml->vpVec32, spF->sSName.uiOffset);
        uint32_t uiSLen = spF->sSName.uiLength;
        if(!bCompNames(uipSName, uiSLen, uipEName, uiELen)){
            XML_THROW("Well-formedness constraint: Element Type Match\n"
                    "The Name in an element's end-tag MUST match the element type in the start-tag.");
        }
        spF->sContent.uiLength = (uint32_t)uiVecLen(spXml->vpVec32) - spF->sContent.uiOffset;

        // copy name to end tag name and call the user's call back function, if any
        spF->sEName.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
        spF->sEName.uiLength = uiELen;
        vpVecPushn(spXml->vpVec32, vpVecFirst(spXml->vpVecName), (aint)uiELen);
        if(spXml->pfnEndTagCallback){
            u32_phrase sName;
            u32_phrase sContent;
            vMakeCDataDisplay(spXml, &spF->sEName, &sName, spData->uiParserOffset);
            vMakeCDataDisplay(spXml, &spF->sContent, &sContent, spData->uiParserOffset);
            spXml->pfnEndTagCallback(&sName, &sContent, spXml->vpEndTagData);
        }
        vPopFrame(spData);
    }else if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("malformed end tag");
    }
}
/*********************************************************
 * ELEMENTS ----------------------------------------------
 ********************************************************/
/*********************************************************
 * ATTRIBUTES ++++++++++++++++++++++++++++++++++++++++++++
 ********************************************************/
void vElAttName(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;

        // validate the name - make sure it is not a duplicate
        aint uiNameLen = uiVecLen(spXml->vpVecName);
        if(bAttNameLookup(spXml, (uint32_t*)vpVecFirst(spXml->vpVecName), (uint32_t)uiNameLen)){
            XML_THROW("Well-formedness constraint: Unique Att Spec\n"
                    "An attribute name MUST NOT appear more than once in the same start-tag or empty-element tag.");
        }

        // push the name on the 32 data
        named_value* spNv = (named_value*)vpVecPush(spXml->vpVecAttList, NULL);
        spNv->sName.uiOffset = uiVecLen(spXml->vpVec32);
        spNv->sName.uiLength = uiVecLen(spXml->vpVecName);
        vpVecPushn(spXml->vpVec32, vpVecFirst(spXml->vpVecName), uiNameLen);
        spNv->sValue.uiOffset = uiVecLen(spXml->vpVec32);
    }
}
void vAttValue(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        named_value* spNv = (named_value*)vpVecLast(spXml->vpVecAttList);
        spNv->sValue.uiLength = (uint32_t)(uiVecLen(spXml->vpVec32) - spNv->sValue.uiOffset);
        spXml->spCurrentFrame->uiAttCount++;

        // validate the value
        uint32_t* uipData = (uint32_t*)vpVecFirst(spXml->vpVec32) + spNv->sValue.uiOffset;
        uint32_t* uipDataEnd = uipData + spNv->sValue.uiLength;
        for(; uipData < uipDataEnd; uipData++){
            if(*uipData == 60){
                XML_THROW("Well-formedness constraint: No < in Attribute Values\n"
                        "The replacement text of any entity referred to directly or indirectly in an attribute value MUST NOT contain a <.");
            }
        }
    }
}
void vDvalue(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        vpVecPush(spXml->vpVec32, &spXml->uiChar);
    }
}
void vDChar(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        // normalize the attribute value here
        if((spXml->uiChar == 9) || (spXml->uiChar == 10) || (spXml->uiChar == 13)){
            spXml->uiChar = 32;
        }
    }
}
void vEntityRef(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        // look up the entity name
        uint32_t* uipName = (uint32_t*)vpVecFirst(spXml->vpVecName);
        uint32_t uiNameLen = (uint32_t)uiVecLen(spXml->vpVecName);
        entity_decl* spEntity = spEntityNameLookup(spXml, spXml->spCurrentFrame->uiElementOffset, uipName, uiNameLen);
        if(!spEntity){
            XML_THROW("Well-formedness constraint: Entity Declared\n"
                    "The replacement text of any entity referred to directly or indirectly in an attribute value MUST NOT contain a <.");
        }
        if(!spEntity->bExpanded){
            vExpandEntity(spXml, spData->uiParserOffset, spEntity);
        }

        // push the entity value on vpVec32
        uint32_t* uipValue = (uint32_t*)vpVecFirst(spXml->vpVec32) + spEntity->sValue.uiOffset;
        vpVecPushn(spXml->vpVec32, uipValue, spEntity->sValue.uiLength);
    }
}
void vDecValue(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        aint ui;
        const achar* acpChar = &spData->acpString[spData->uiParserOffset];
        uint32_t uiSum, uiDigit;
        uiSum = 0;
        for(ui = 0; ui < spData->uiParserPhraseLength; ui++, acpChar++){
            uiDigit = *acpChar - 48;
            if(!bMultiply32(uiSum, 10, &uiSum)){
                XML_THROW("decimal value in Reference is too large: causes uint32_t overflow");
            }
            if(!bSum32(uiSum, uiDigit, &uiSum)){
                XML_THROW("decimal value in Reference is too large: causes uint32_t overflow");
            }
        }
        if(!bValidateChar(uiSum)){
            char caBuf[CABUF_LEN];
            snprintf(caBuf, CABUF_LEN, "Well-formedness Constraint: Legal Character\n"
                    "Characters referred to using character references MUST match the production for Char\n"
                    "https://www.w3.org/TR/REC-xml/#sec-references\n"
                    "decimal character: %u", uiSum);
            XML_THROW(caBuf);
        }
        spXml->uiChar = uiSum;
    }else if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("decimal character reference error");
    }
}
void vHexValue(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        aint ui;
        char caBuf[CABUF_LEN];
        const achar* acpChar = &spData->acpString[spData->uiParserOffset];
        uint32_t uiSum = 0;
        uint32_t uiDigit = 0;
        for(ui = 0; ui < spData->uiParserPhraseLength; ui++, acpChar++){
            if(*acpChar >= 48 && *acpChar <= 57){
                uiDigit = *acpChar - 48;
            }else if(*acpChar >= 65 && *acpChar <= 70){
                uiDigit = *acpChar - 55;
            }else if(*acpChar >= 97 && *acpChar <= 102){
                uiDigit = *acpChar - 87;
            }else{
                snprintf(caBuf, CABUF_LEN, "illegal hex digit in Reference: %c", *(char*)acpChar);
                XML_THROW(caBuf);
            }
            if(!bMultiply32(uiSum, 16, &uiSum)){
                XML_THROW("hex value in Reference is too large: causes 32-bit overflow");
            }
            if(!bSum32(uiSum, uiDigit, &uiSum)){
                XML_THROW("hex value in Reference is too large: causes 32-bit overflow");
            }
        }
        if(!bValidateChar(uiSum)){
            char caBuf[CABUF_LEN];
            snprintf(caBuf, CABUF_LEN, "Well-formedness Constraint: Legal Character\n"
                    "Characters referred to using character references MUST match the production for Char\n"
                    "https://www.w3.org/TR/REC-xml/#sec-references\n"
                    "hex character: 0x%X", uiSum);
            XML_THROW(caBuf);
        }
        spXml->uiChar = uiSum;
    }else if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("hex character reference error");
    }
}
/*********************************************************
 * ATTRIBUTES --------------------------------------------
 ********************************************************/
/** Compares the given name to all of the names in the attribute list.
 * That is, the list of attributes found in the element tag.
 * Used to compare declared, default attribute names to names found in the element tags.
 */
static abool bAttNameLookup(xml* spXml, uint32_t* uipName, uint32_t uiLen){
    if(spXml->spCurrentFrame->uiAttCount){
        uint32_t* uipChars = (uint32_t*)vpVecFirst(spXml->vpVec32);
        uint32_t* uipLName;
        uint32_t uiLLen;
        named_value* spValue = (named_value*)vpVecFirst(spXml->vpVecAttList);
        named_value* spEnd = spValue + spXml->spCurrentFrame->uiAttCount;
        for(; spValue < spEnd; spValue++){
            uipLName = uipChars + spValue->sName.uiOffset;
            uiLLen = spValue->sName.uiLength;
            if(bCompNames(uipLName, uiLLen, uipName, uiLen)){
                return APG_TRUE;
            }
        }
    }
    return APG_FALSE;
}
static att_decl* spElNameLookup(xml* spXml, uint32_t* uipEName, uint32_t uiELen){
    aint uiCount = uiVecLen(spXml->vpVecAttDecls);
    if(uiCount){
        uint32_t* uipChars = (uint32_t*)vpVecFirst(spXml->vpVec32);
        att_decl* spDecl = (att_decl*)vpVecFirst(spXml->vpVecAttDecls);
        att_decl* spEnd = spDecl + uiCount;
        for(; spDecl < spEnd; spDecl++){
            if(bCompNames((uipChars + spDecl->sElementName.uiOffset), spDecl->sElementName.uiLength, uipEName, uiELen)){
                return spDecl;
            }
        }
    }
    return NULL;
}

/*********************************************************
 * CONTENT --------------------------------------------
 ********************************************************/
void vCharData(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        vpVecPush(spXml->vpVec32, &spXml->uiChar);
    }
}
void vCDSectEnd(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("\"]]>\" not allowed in content character data");
    }
}

static att_cdata sMakeAtts(xml*spXml, element_frame* spF, aint uiOffset){
    att_cdata sReturn;
    vVecClear(spXml->vpVecString);
    vVecClear(spXml->vpVecCData);
    vMakeCDataDisplay(spXml, &spF->sSName, &sReturn.sName, uiOffset);
    if(spF->uiAttCount){
        sReturn.spAttNames = (u32_phrase*)vpVecPushn(spXml->vpVecCData, NULL, (2 * spF->uiAttCount));
        sReturn.spAttValues = sReturn.spAttNames + spF->uiAttCount;
        uint32_t ui = 0;
        named_value* spAtt = (named_value*)vpVecFirst(spXml->vpVecAttList) + spF->uiBaseAtt;
        for(; ui < spF->uiAttCount; ui++){
            vMakeCDataDisplay(spXml, &spAtt[ui].sName, &sReturn.spAttNames[ui], uiOffset);
            vMakeCDataDisplay(spXml, &spAtt[ui].sValue, &sReturn.spAttValues[ui], uiOffset);
        }
    }
    return sReturn;
}

/*********************************************************
 * CDATA SECTIONS-----------------------------------------
 ********************************************************/
void vCDEnd(callback_data* spData){
    if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("expected end of CDATA section ']]>' not found");
    }
}
void vCDRb(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        vpVecPush(spXml->vpVec32, &s_ucRightBracket);
        vpVecPush(spXml->vpVec32, &spXml->uiChar);
    }
}
void vCD2Rb(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        vpVecPush(spXml->vpVec32, &s_ucRightBracket);
        vpVecPush(spXml->vpVec32, &s_ucRightBracket);
        vpVecPush(spXml->vpVec32, &spXml->uiChar);
    }
}
