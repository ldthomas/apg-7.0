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
#ifndef CALLBACKS_H_
#define CALLBACKS_H_

/** \file xml/callbacks.h
 * \brief Declaration for all of the XML Component parser callback functions.
 *
 */

#include <limits.h>
#include "../utilities/utilities.h"
#include "xmlgrammar.h"
#include "xml.h"
#include "xmlp.h"

#define STATE_BEGIN 1
#define STATE_TEXT  2
#define STATE_WSP   3
#define ATT_AMP     38
#define ATT_HASH    35
#define ATT_X       120
#define ATT_SEMI    59
#define CABUF_LEN   256

#define XML_THROW(msg) vThrowError(spXml, (msg), spData->uiParserOffset, __LINE__, __FILE__, __func__)

// helpers
void vThrowError(xml* spXml, const char* cpMsg, aint uiOffset, unsigned int uiLine, const char* cpFile, const char* cpFunc);
void vLogMsg(xml* spXml, aint uiOffset, char* cpTitle);
void vPushFrame(callback_data* spData);
void vPopFrame(callback_data* spData);
uint32_t ui2byte(const achar* acpBytes);
uint32_t ui3byte(const achar* acpBytes);
uint32_t ui4byte(const achar* acpBytes);
abool bValidateChar(uint32_t uiChar);
void vMakeCDataDisplay(xml* spXml, cdata_id* spDataId, u32_phrase* spCData, aint uiOffset);
void vMakeCDataIdFromInput(xml* spXml, const achar* acpInput, aint uiLen, aint uiOffset, cdata_id* spCDataId);
cdata_id sCapturePhrase(xml* spXml, achar* acpPhrase, aint uiPhraseLength, aint uiOffset);
int iAttComp(const void* vpL, const void* vpR);
att_decl* spLeftMostElement(xml* spXml, att_decl* spAttList);
int iEntityComp(const void* vpL, const void* vpR);
void vHexValue(callback_data* spData);
void vDecValue(callback_data* spData);
uint32_t uiDecValue32(xml* spXml, aint uiOffset, uint32_t* uipChars, uint32_t uiCount);
uint32_t uiHexValue32(xml* spXml, aint uiOffset, uint32_t* uipChars, uint32_t uiCount);
abool bHasEntity(uint32_t* uipChars, uint32_t uiLen, uint32_t* uipEntityOffset, uint32_t* uipEntityLen);
cdata_id sNormalizeAttributeValue(xml* spXml, aint uiOffset, uint32_t* uipAttValue, uint32_t uiLength, abool bIsCDATA);
void vExpandEntity(xml* spXml, aint uiOffset, entity_decl* spValuei);
int iSortCompNames(const uint32_t* uipLName, uint32_t uiLLen, const uint32_t* uipRName, uint32_t uiRLen);
abool bCompNames(const uint32_t* uipLName, uint32_t uiLLen, const uint32_t* uipRName, uint32_t uiRLen);
entity_decl* spEntityNameLookup(xml* spXml, aint uiOffset, uint32_t* uipName, uint32_t uiNameLen);
void vConvertParsedData(xml* spXml, const achar* acpData, aint uiDataLen, uint32_t* uipOffset, uint32_t* uipLength);
int iCompNames(const uint32_t* uipLName, uint32_t uiLLen, const uint32_t* uipRName, uint32_t uiRLen);

// callbacks
void vDocument(callback_data* spData);
void vXmlDeclOpen(callback_data* spData);
void vXmlDeclClose(callback_data* spData);
void vEncDef(callback_data* spData);
void vVersionNum(callback_data* spData);
void vVersionInfo(callback_data* spData);
void vVersionNum(callback_data* spData);
void vEncDef(callback_data* spData);
void vEncName(callback_data* spData);
void vSDeclOther(callback_data* spData);
void vSDeclYes(callback_data* spData);
void vSDeclNo(callback_data* spData);
void vDtdOpen(callback_data* spData);
void vDtdName(callback_data* spData);
void vDtdClose(callback_data* spData);
void vExtSubset(callback_data* spData);
void vExternalID(callback_data* spData);
void vNExternalID(callback_data* spData);
void vPEDeclOpen(callback_data* spData);
void vPEDeclClose(callback_data* spData);
void vPEReference(callback_data* spData);
void vPERefError(callback_data* spData);
void vGEDeclName(callback_data* spData);
void vGEPERef(callback_data* spData);
void vGEDefEx(callback_data* spData);
void vGEDeclClose(callback_data* spData);
void vCloseQuote(callback_data* spData);
void vEntityChar(callback_data* spData);
void vGERef(callback_data* spData);
void vAttlistOpen(callback_data* spData);
void vAttName(callback_data* spData);
void vAttType(callback_data* spData);
void vAttlistValue(callback_data* spData);
void vAttDef(callback_data* spData);
void vAttlistClose(callback_data* spData);
void vNotationOpen(callback_data* spData);
void vNotationDef(callback_data* spData);
void vNotationClose(callback_data* spData);
void vDvalue(callback_data* spData);
void vDChar(callback_data* spData);
void vEntityRef(callback_data* spData);
void vElementOpen(callback_data* spData);
void vElementClose(callback_data* spData);
void vETagClose(callback_data* spData);


void vEOpen(callback_data* spData);
void vEStart(callback_data* spData);
void vEReserved(callback_data* spData);
void vEmptyClose(callback_data* spData);
void vSTagClose(callback_data* spData);
void vAttValue(callback_data* spData);
void vElAttName(callback_data* spData);
void vCharData(callback_data* spData);
void vCDSectEnd(callback_data* spData);
void vCDEnd(callback_data* spData);
void vCDRb(callback_data* spData);
void vCD2Rb(callback_data* spData);
void vRefClose(callback_data* spData);
//void u_vCommentClose(callback_data* spData);
void vComment(callback_data* spData);
void vDoubleh(callback_data* spData);
void vPIClose(callback_data* spData);
void vPITarget(callback_data* spData);
void vPIInfo(callback_data* spData);
void vPIInfoq(callback_data* spData);
void vPIInfoa(callback_data* spData);
void vPIForbidden(callback_data* spData);
void vPIReserved(callback_data* spData);
void vAscii(callback_data* spData);
void vUtf82(callback_data* spData);
void vUtf83(callback_data* spData);
void vUtf84(callback_data* spData);
//void vAmp(callback_data* spData);
//void vLt(callback_data* spData);
//void vGt(callback_data* spData);
//void vApos(callback_data* spData);
//void vQuot(callback_data* spData);
void vName(callback_data* spData);
void vNameStartChar(callback_data* spData);
void vNameOtherChar(callback_data* spData);

#endif /* CALLBACKS_H_ */
