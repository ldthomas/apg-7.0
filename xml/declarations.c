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
/** \file xml/declarations.c
 * \brief This file has all of the APG callback functions for the XML parser.
 */
#include "callbacks.h"

static const char* s_cpUtf8 = "UTF-8";
static const char* s_cpUtf8Default = "UTF-8 (default value)";
static const char* s_cpVersionDefault = "1.0 (default value)";
static const char* s_cpNoDefault= "no (default value)";
static const char* s_cpUtf16 = "UTF-16";
static const char* s_cpVersion = "1.0";
static const char* s_cpYes = "yes";
static const char* s_cpNo = "no";
static const char* s_cpStandaloneError= "standalone declaration error: must be \"yes\" or \"no\"";
static const char* s_cpCData = "CDATA";
static const uint32_t s_uiAmp = 38;
static const uint32_t s_uiSemi = 59;

/*********************************************************
 * DOCUMENT ++++++++++++++++++++++++++++++++++++++++++++++
 ********************************************************/
void vDocument(callback_data* spData){
    xml* spXml = (xml*)spData->vpUserData;
    if(spData->uiParserState == ID_ACTIVE){
    }else if(spData->uiParserState == ID_MATCH){
        if(spData->uiParserPhraseLength != spData->uiStringLength){
            XML_THROW("Syntax error. A syntactically correct document was found but followed by extraneous characters.");
        }
    }else if(spData->uiParserState == ID_NOMATCH){
        XML_THROW("Syntax error. Document not matched.");
    }
}
/*********************************************************
 * DOCUMENT ----------------------------------------------
 ********************************************************/

/*********************************************************
 * XML DECLARATION +++++++++++++++++++++++++++++++++++++++
 ********************************************************/
void vXmlDeclOpen(callback_data* spData){
    xml* spXml = (xml*)spData->vpUserData;
    if(spData->uiParserState == ID_MATCH){
        vVecClear(spXml->vpVec8);
        vVecClear(spXml->vpVecString);
    }else if(spData->uiParserState == ID_NOMATCH){
        if(spXml->pfnXmlDeclCallback){
            // default info
            xmldecl_info sInfo;
            sInfo.cpExists = s_cpNo;
            sInfo.cpVersion = s_cpVersionDefault;
            sInfo.cpEncoding = s_cpUtf8Default;
            sInfo.cpStandalone = s_cpNoDefault;
            spXml->pfnXmlDeclCallback(&sInfo, spXml->vpXmlDeclData);
        }
    }
}
void vXmlDeclClose(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        // check semantics
        xml* spXml = (xml*)spData->vpUserData;
        char caBuf[CABUF_LEN];
        const char* cpEncoding = s_cpUtf8;
        const char* cpStandalone;
        const char* cpTrueType = cpUtilUtfTypeName(spXml->uiTrueType);
        if(!spXml->sXmlDecl.cpVersion){
            XML_THROW("XML declaration version number not declared");
        }
        if(strcmp(spXml->sXmlDecl.cpVersion, s_cpVersion) != 0){
            XML_THROW("XML declaration version number is \"%s\". Must be \"1.0\"");
        }
        if(!spXml->sXmlDecl.cpEncoding){
            cpEncoding = s_cpUtf8Default;
        }else{
            if(strcmp(spXml->sXmlDecl.cpEncoding, s_cpUtf8) == 0){
                cpEncoding = s_cpUtf8;
                if(spXml->uiTrueType != UTF_8){
                    snprintf(caBuf, CABUF_LEN,
                            "XML declaration encoding is %s but data has type %s",
                            spXml->sXmlDecl.cpEncoding, cpTrueType);
                    XML_THROW(caBuf);
                }
            }else if(strcmp(spXml->sXmlDecl.cpEncoding, s_cpUtf16) == 0){
                cpEncoding =s_cpUtf16;
                if(!(spXml->uiTrueType == UTF_16 || spXml->uiTrueType == UTF_16BE || spXml->uiTrueType == UTF_16LE)){
                    snprintf(caBuf, CABUF_LEN,
                            "XML declaration encoding is %s but data has type %s",
                            spXml->sXmlDecl.cpEncoding, cpTrueType);
                    XML_THROW(caBuf);
                }
            }else{
                snprintf(caBuf, CABUF_LEN,
                        "XML declaration encoding is \"%s\": Must be \"UTF-8\" or \"UTF-16\"",
                        spXml->sXmlDecl.cpEncoding);
                XML_THROW(caBuf);
            }
        }
        if(spXml->sXmlDecl.cpStandalone){
            cpStandalone = spXml->sXmlDecl.cpStandalone;
            if(strcmp(spXml->sXmlDecl.cpStandalone, s_cpStandaloneError) == 0){
                XML_THROW(s_cpStandaloneError);
            }
        }else{
            cpStandalone = s_cpNoDefault;
        }
        if(spXml->pfnXmlDeclCallback){
            // NOTE: all strings are static and are valid for the duration of the callback
            spXml->sXmlDecl.cpExists = s_cpYes;
            spXml->sXmlDecl.cpVersion = s_cpVersion;
            spXml->sXmlDecl.cpEncoding = cpEncoding;
            spXml->sXmlDecl.cpStandalone = cpStandalone;
            spXml->pfnXmlDeclCallback(&spXml->sXmlDecl, spXml->vpXmlDeclData);

            // clear the work vectors
            vVecClear(spXml->vpVecString);
            vVecClear(spXml->vpVecCData);
            vVecClear(spXml->vpVec8);
        }
    }else if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("XML declaration syntax error.");
    }
}
void vVersionInfo(callback_data* spData){
    if (spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("version information is malformed");
    }
}
void vVersionNum(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        char* cpNum = (char*)vpVecPushn(spXml->vpVecString, NULL, (spData->uiParserPhraseLength + 1));
        aint ui = 0;
        for(; ui < spData->uiParserPhraseLength; ui++){
            cpNum[ui] = (char)spData->acpString[spData->uiParserOffset + ui];
        }
        cpNum[ui] = 0;
        spXml->sXmlDecl.cpVersion = cpNum;
    }else if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("XML declaration syntax error. Version number not of form \"1.123...\"");
    }
}

void vEncDef(callback_data* spData){
    if (spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("XML declaration syntax error. Malformed encoding definition.");
    }
}
void vEncName(callback_data* spData){
    xml* spXml = (xml*)spData->vpUserData;
    if(spData->uiParserState == ID_MATCH){
        char* cpBuf = (char*)vpVecPushn(spXml->vpVecString, NULL, (spData->uiParserOffset + 1));
        aint ui;
        for(ui = 0; ui < spData->uiParserPhraseLength; ui++){
            cpBuf[ui] = (char)spData->acpString[spData->uiParserOffset + ui];
        }
        cpBuf[ui] = 0;
        spXml->sXmlDecl.cpEncoding = cpBuf;
    }else if (spData->uiParserState == ID_NOMATCH){
        XML_THROW("XML declaration syntax error. Malformed encoding name.");
    }
}
void vSDeclOther(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("XML declaration syntax error. standalone must be either \"yes\" or \"no\".");
    }
}
void vSDeclYes(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->sXmlDecl.cpStandalone = s_cpYes;
        spXml->bStandalone = APG_TRUE;
    }
}
void vSDeclNo(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->sXmlDecl.cpStandalone = s_cpNo;
        spXml->bStandalone = APG_FALSE;
    }
}
/*********************************************************
 * XML DECLARATION ---------------------------------------
 ********************************************************/

/*********************************************************
 * DOCUMENT TYPE DECLARATION (DTD) +++++++++++++++++++++++
 ********************************************************/
void vDtdOpen(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->uiDTDOffset = spData->uiParserOffset;
    }else if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        if(spXml->pfnDTDCallback){
            dtd_info sDtd = {};
            spXml->pfnDTDCallback(&sDtd, spXml->vpDTDData);
        }
    }
}
void vDtdName(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->sDtdName.uiOffset = uiVecLen(spXml->vpVec32);
        spXml->sDtdName.uiLength = (uint32_t)uiVecLen(spXml->vpVecName);
        vpVecPushn(spXml->vpVec32, vpVecFirst(spXml->vpVecName), spXml->sDtdName.uiLength);
    }
}
void vDtdClose(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        if(spXml->pfnDTDCallback){
            aint ui, uiIndex, uiCount;
            u32_phrase* spTempData;
            entity_decl* spNamedValues;
            dtd_info sDtd = {};
            vVecClear(spXml->vpVecCData);
            spTempData = (u32_phrase*)vpVecPush(spXml->vpVecCData, NULL);
            vMakeCDataDisplay(spXml, &spXml->sDtdName, spTempData, spData->uiParserOffset);
            sDtd.bExists = APG_TRUE;
            sDtd.bStandalone = spXml->bStandalone;
            sDtd.bExtSubset = spXml->bExtSubset;
            sDtd.uiExternalIds = spXml->uiExternalIds;
            sDtd.uiPEDecls= spXml->uiPEDecls;
            sDtd.uiPERefs = spXml->uiPERefs;
            sDtd.uiAttListsDeclared = spXml->uiAttListsDeclared;
            sDtd.uiElementDecls = spXml->uiElementDecls;
            sDtd.spName = spTempData;
            sDtd.uiNotationDecls = uiVecLen(spXml->vpVecNotationDecls);
            sDtd.uiGEDeclsDeclared = spXml->uiGEDeclsTotal;
            sDtd.uiGEDeclsNotProcessed = spXml->uiGEDeclsNotProcessed;
            uiCount = uiVecLen(spXml->vpVecGEDefs);
            sDtd.uiGEDeclsUnique = uiCount;
            if(uiVecLen(spXml->vpVecGEDefs)){
                uiIndex = uiVecLen(spXml->vpVecCData);
                spNamedValues = (entity_decl*)vpVecFirst(spXml->vpVecGEDefs);
                for(ui = 0; ui < uiCount; ui++, spNamedValues++){
                    spTempData = (u32_phrase*)vpVecPush(spXml->vpVecCData, NULL);
                    vMakeCDataDisplay(spXml, &spNamedValues->sName, spTempData, spData->uiParserOffset);
                }
                sDtd.spGENames = (u32_phrase*)vpVecAt(spXml->vpVecCData, uiIndex);

                // make the list of GE values data
                uiIndex = uiVecLen(spXml->vpVecCData);
                spNamedValues = (entity_decl*)vpVecFirst(spXml->vpVecGEDefs);
                for(ui = 0; ui < uiCount; ui++, spNamedValues++){
                    spTempData = (u32_phrase*)vpVecPush(spXml->vpVecCData, NULL);
                    vMakeCDataDisplay(spXml, &spNamedValues->sValue, spTempData, spData->uiParserOffset);
                }
                sDtd.spGEValues = (u32_phrase*)vpVecAt(spXml->vpVecCData, uiIndex);
            }
            sDtd.uiAttListsDeclared = spXml->uiAttListsDeclared;
            sDtd.uiAttListsNotProcessed = spXml->uiAttListsNotProcessed;
            uiCount = uiVecLen(spXml->vpVecAttDecls);
            sDtd.uiAttListsUnique = uiCount;
            if(uiCount){
                // make the list of attribute list element names data
                uiIndex = uiVecLen(spXml->vpVecCData);
                att_decl* spAttList = (att_decl*)vpVecFirst(spXml->vpVecAttDecls);
                for(ui = 0; ui < uiCount; ui++, spAttList++){
                    spTempData = (u32_phrase*)vpVecPush(spXml->vpVecCData, NULL);
                    vMakeCDataDisplay(spXml, &spAttList->sElementName, spTempData, spData->uiParserOffset);
                }
                sDtd.spAttElementNames = (u32_phrase*)vpVecAt(spXml->vpVecCData, uiIndex);

                // make the list of attribute list attribute names data
                uiIndex = uiVecLen(spXml->vpVecCData);
                spAttList = (att_decl*)vpVecFirst(spXml->vpVecAttDecls);
                for(ui = 0; ui < uiCount; ui++, spAttList++){
                    spTempData = (u32_phrase*)vpVecPush(spXml->vpVecCData, NULL);
                    vMakeCDataDisplay(spXml, &spAttList->sAttName, spTempData, spData->uiParserOffset);
                }
                sDtd.spAttNames = (u32_phrase*)vpVecAt(spXml->vpVecCData, uiIndex);

                // make the list of attribute types
                uiIndex = uiVecLen(spXml->vpVecCData);
                spAttList = (att_decl*)vpVecFirst(spXml->vpVecAttDecls);
                for(ui = 0; ui < uiCount; ui++, spAttList++){
                    spTempData = (u32_phrase*)vpVecPush(spXml->vpVecCData, NULL);
                    vMakeCDataDisplay(spXml, &spAttList->sAttType, spTempData, spData->uiParserOffset);
                }
                sDtd.spAttTypes = (u32_phrase*)vpVecAt(spXml->vpVecCData, uiIndex);

                // make the list of attribute values
                uiIndex = uiVecLen(spXml->vpVecCData);
                spAttList = (att_decl*)vpVecFirst(spXml->vpVecAttDecls);
                for(ui = 0; ui < uiCount; ui++, spAttList++){
                    spTempData = (u32_phrase*)vpVecPush(spXml->vpVecCData, NULL);
                    vMakeCDataDisplay(spXml, &spAttList->sAttValue, spTempData, spData->uiParserOffset);
                }
                sDtd.spAttValues = (u32_phrase*)vpVecAt(spXml->vpVecCData, uiIndex);
            }
            if(sDtd.uiNotationDecls){
                // make the list of GE names data
                uiIndex = uiVecLen(spXml->vpVecCData);
                spNamedValues = (entity_decl*)vpVecFirst(spXml->vpVecNotationDecls);
                for(ui = 0; ui < sDtd.uiNotationDecls; ui++, spNamedValues++){
                    spTempData = (u32_phrase*)vpVecPush(spXml->vpVecCData, NULL);
                    vMakeCDataDisplay(spXml, &spNamedValues->sName, spTempData, spData->uiParserOffset);
                }
                sDtd.spNotationNames = (u32_phrase*)vpVecAt(spXml->vpVecCData, uiIndex);

                // make the list of GE values data
                uiIndex = uiVecLen(spXml->vpVecCData);
                spNamedValues = (entity_decl*)vpVecFirst(spXml->vpVecNotationDecls);
                for(ui = 0; ui < sDtd.uiNotationDecls; ui++, spNamedValues++){
                    spTempData = (u32_phrase*)vpVecPush(spXml->vpVecCData, NULL);
                    vMakeCDataDisplay(spXml, &spNamedValues->sValue, spTempData, spData->uiParserOffset);
                }
                sDtd.spNotationValues = (u32_phrase*)vpVecAt(spXml->vpVecCData, uiIndex);
            }

            // call the user's callback function
            spXml->pfnDTDCallback(&sDtd, spXml->vpDTDData);

        }
        if(uiMsgsCount(spXml->vpMsgs)){
//                printf("XML non-validating parser exit on DTD errors.\n");
//                vUtilPrintMsgs(spXml->vpMsgs);
//                printf("\n");
            vThrowError(spXml, "Document Type Declaration bad content",
                    spXml->uiDTDOffset, __LINE__, __FILE__, __func__);
        }
        // clear the work vectors
        vVecClear(spXml->vpVecString);
        vVecClear(spXml->vpVecCData);
        vVecClear(spXml->vpVec8);
        vMsgsClear(spXml->vpMsgs);
    }else if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("Document Type Declaration syntax error");
    }
}

void vExtSubset(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->bExtSubset = APG_TRUE;
        vLogMsg(spXml, spData->uiParserOffset, "External Subset");
    }
}
/****************** EXTERNAL ID ******************************/
void vExternalID(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->uiExternalIds++;
        vLogMsg(spXml, spData->uiParserOffset, "External ID");
    }
}
void vNExternalID(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->uiExternalIds++;
    }
}
/****************** EXTERNAL ID ******************************/
/****************** PARAMETER ENTITY *************************/
void vPEDeclOpen(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->uiPEDecls++;
    }
}
void vPEDeclClose(callback_data* spData){
    if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("Parameter Entity Declaration syntax error. Expected closure not found");
    }
}
void vPEReference(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->uiPERefs++;
    }
}
void vPERefError(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("Well-formedness constraint: PEs in Internal Subset\n"
                "In the internal DTD subset, parameter-entity references MUST NOT occur within markup declarations...");
    }
}
/****************** PARAMETER ENTITY *************************/
/****************** GENERAL ENTITY ***************************/
void vGEDeclName(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        uint32_t* uipName = (uint32_t*)vpVecFirst(spXml->vpVecName);
        aint uiNameLen = uiVecLen(spXml->vpVecName);
        if(!uipName || !uiNameLen){
            XML_THROW("General Entity Declaration has no name.");
        }

        // initialize the name General Entity named value
        memset((void*)&spXml->sCurrentEntity, 0, sizeof(entity_decl));
        // save the name
        spXml->sCurrentEntity.sName.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
        spXml->sCurrentEntity.sName.uiLength = uiNameLen;
        vpVecPushn(spXml->vpVec32, uipName, uiNameLen);

        // set up for the named value
        spXml->sCurrentEntity.sValue.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
        spXml->sCurrentEntity.sValue.uiLength = 0;
        spXml->sCurrentEntity.uiInputOffset = spData->uiParserOffset;
        spXml->uiSavedOffset = spData->uiParserOffset;
    }
}
void vGEPERef(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->sCurrentEntity.bGEPERef = APG_TRUE;
        vLogMsg(spXml, spData->uiParserOffset, "General Entity declaration contains unread Parameter Entity");
    }
}
void vGEDefEx(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->sCurrentEntity.bGEDefEx = APG_TRUE;
        vLogMsg(spXml, spData->uiParserOffset, "General Entity has an external definition");
    }
}
void vGEDeclClose(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->uiGEDeclsTotal++;
        while(APG_TRUE){
            if((spXml->uiPERefs == 0) || spXml->bStandalone){
                // OK to process this GE
                uint32_t* uipChars = (uint32_t*)vpVecFirst(spXml->vpVec32);
                aint uiLen32 = uiVecLen(spXml->vpVec32);
                if(uiLen32 < spXml->sCurrentEntity.sValue.uiOffset){
                    XML_THROW("General Entity Declaration syntax error. No value data.");
                }
                spXml->sCurrentEntity.sValue.uiLength = uiLen32 - spXml->sCurrentEntity.sValue.uiOffset;

                // lookup
                uint32_t* uipName = uipChars + spXml->sCurrentEntity.sName.uiOffset;
                uint32_t uiLen = spXml->sCurrentEntity.sName.uiLength;
                entity_decl* spFound = spEntityNameLookup(spXml, spData->uiParserOffset, uipName, uiLen);
                if(!spFound){
                    // not found - add it and sort
                    spXml->sCurrentEntity.spXml = spXml;
                    vpVecPush(spXml->vpVecGEDefs, &spXml->sCurrentEntity);
                    qsort(vpVecFirst(spXml->vpVecGEDefs), (size_t)uiVecLen(spXml->vpVecGEDefs),
                            sizeof(entity_decl), iEntityComp);
                }
                break;
            }
            spXml->uiGEDeclsNotProcessed++;
            if(spXml->sCurrentEntity.bGEPERef){
                // don't process this GE due to parameter entity in definition
                vLogMsg(spXml, spXml->uiSavedOffset, "General Entity not processed (contains parameter entity)");
                break;
            }
            if(spXml->sCurrentEntity.bGEDefEx){
                // don't process this GE due to external definition
                vLogMsg(spXml, spXml->uiSavedOffset, "General Entity not processed (contains external definition)");
                break;
            }
            // don't process this GE due to conditions
            vLogMsg(spXml, spXml->uiSavedOffset, "General Entity not processed (preceded by parameter entity)");
            break;
        }
    }else if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("General Entity Declaration syntax error. Expected closure not found");
    }
}
void vCloseQuote(callback_data* spData){
    if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("Expected closing quotation mark (single or double) not found");
    }
}
void vEntityChar(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        vpVecPush(spXml->vpVec32, (void*)&spXml->uiChar);
    }
}
void vGERef(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        uint32_t* uipRefName = (uint32_t*)vpVecFirst(spXml->vpVecName);
        uint32_t uiRefLen = (uint32_t)uiVecLen(spXml->vpVecName);
        uint32_t* uipThisName = (uint32_t*)vpVecFirst(spXml->vpVec32) + spXml->sCurrentEntity.sName.uiOffset;
        uint32_t uiThisLen = spXml->sCurrentEntity.sName.uiLength;
        if(bCompNames(uipRefName, uiRefLen, uipThisName, uiThisLen)){
            vLogMsg(spXml, spXml->uiSavedOffset,
                    "Well-formedness constraint: No Recursion\n"
                    "A parsed entity MUST NOT contain a recursive reference to itself, either directly or indirectly.");
            spXml->sCurrentEntity.bEntityDeclaredError = APG_TRUE;
            return;
        }
        // it's a valid name
        vpVecPush(spXml->vpVec32, (void*)&s_uiAmp);
        vpVecPushn(spXml->vpVec32, vpVecFirst(spXml->vpVecName), uiVecLen(spXml->vpVecName));
        vpVecPush(spXml->vpVec32, (void*)&s_uiSemi);
    }
}
/****************** GENERAL ENTITY ***************************/
/****************** ATTRIBUTES *******************************/
void vAttlistOpen(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->uiAttListsDeclared++;
        if((spXml->uiPERefs == 0) || spXml->bStandalone){
            // OK to process this attribute
            uint32_t* uipName = (uint32_t*)vpVecFirst(spXml->vpVecName);
            aint uiNameLen = uiVecLen(spXml->vpVecName);
            if(!uipName || !uiNameLen){
                XML_THROW("Attribute List Declaration element has no name.");
            }

            // push an att value & save the element name
            memset((void*)&spXml->sCurrentAttList, 0, sizeof(spXml->sCurrentAttList));
            spXml->sCurrentAttList.spXml = spXml;
            spXml->sCurrentAttList.sElementName.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
            spXml->sCurrentAttList.sElementName.uiLength = (uint32_t)uiNameLen;
            vpVecPushn(spXml->vpVec32, uipName, uiNameLen);
            spXml->uiSavedOffset = spData->uiParserOffset;
        }
    }
}
void vAttName(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        if((spXml->uiPERefs == 0) || spXml->bStandalone){
            // OK to process this attribute
            uint32_t* uipName = (uint32_t*)vpVecFirst(spXml->vpVecName);
            aint uiNameLen = uiVecLen(spXml->vpVecName);
            if(!uipName || !uiNameLen){
                XML_THROW("Attribute List Declaration attribute has no name.");
            }

            // get the current attribute value & save the attribute name
            spXml->sCurrentAttList.sAttName.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
            spXml->sCurrentAttList.sAttName.uiLength = (uint32_t)uiNameLen;
            vpVecPushn(spXml->vpVec32, uipName, uiNameLen);
        }// else{ don't process this GE due to conditions
    }
}
void vAttType(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        if((spXml->uiPERefs == 0) || spXml->bStandalone){
            vConvertParsedData(spXml, &spData->acpString[spData->uiParserOffset], spData->uiParserPhraseLength,
                    &spXml->sCurrentAttList.sAttType.uiOffset, &spXml->sCurrentAttList.sAttType.uiLength);
            aint ui;
            uint32_t* uipData = (uint32_t*)vpVecFirst(spXml->vpVec32) + spXml->sCurrentAttList.sAttType.uiOffset;
            spXml->sCurrentAttList.bIsCDATA = APG_FALSE;
            if(spXml->sCurrentAttList.sAttType.uiLength == 5){
                for(ui = 0; ui < 5; ui++){
                    if(s_cpCData[ui] != (char)uipData[ui]){
                        goto notfound;
                    }
                }
                spXml->sCurrentAttList.bIsCDATA = APG_TRUE;
            }
            notfound:;

            // initialize the value entry
            spXml->sCurrentAttList.sAttValue.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
            spXml->sCurrentAttList.sAttValue.uiLength = 0;
        }// else{ don't process this GE due to conditions
    }
}
void vAttlistValue(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        if((spXml->uiPERefs == 0) || spXml->bStandalone){
            // get the current attribute value & save the attribute value
            spXml->sCurrentAttList.bHasData = APG_TRUE;
            uint32_t* uipAttValue = (uint32_t*)vpVecAt(spXml->vpVec32, spXml->sCurrentAttList.sAttValue.uiOffset);
            spXml->sCurrentAttList.sAttValue.uiLength = (uint32_t)uiVecLen(spXml->vpVec32) -
                    spXml->sCurrentAttList.sAttValue.uiOffset;
            spXml->sCurrentAttList.sAttValue = sNormalizeAttributeValue(spXml, spData->uiParserOffset, uipAttValue,
                    spXml->sCurrentAttList.sAttValue.uiLength, spXml->sCurrentAttList.bIsCDATA);
        }// else{ don't process this GE due to conditions
    }
}
void vAttDef(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        if(spXml->sCurrentAttList.uiAttCount){
            spXml->uiAttListsDeclared++;
        }
        if((spXml->uiPERefs == 0) || spXml->bStandalone){
            // get the current attribute value & save the attribute value
            if(spXml->sCurrentAttList.bHasData || !spXml->sCurrentAttList.bInvalidValue){
                // look up the element/attribute name pair - ignore this declaration if it is a duplicate
                att_decl* spFound = spLeftMostElement(spXml, &spXml->sCurrentAttList);
                if(spFound){
                    aint ui;
                    if(spFound->uiAttCount){
                        // see if the attribute name is unique for this element
                        uint32_t* uipChars = (uint32_t*)vpVecFirst(spXml->vpVec32);
                        uint32_t* uipLName = uipChars + spXml->sCurrentAttList.sAttName.uiOffset;
                        uint32_t uiLLen = spXml->sCurrentAttList.sAttName.uiLength;
                        uint32_t* uipRName;
                        uint32_t uiRLen;
                        for(ui = 0; ui < spFound->uiAttCount; ui++){
                            uipRName = uipChars + spFound[ui].sAttName.uiOffset;
                            uiRLen = spFound[ui].sAttName.uiLength;
                            if(bCompNames(uipLName, uiLLen, uipRName, uiRLen)){
                                // duplicate attribute name for this element -ignore it
                                return;
                            }
                        }
                    }
                    // if the attribute name is unique
                    for(ui = 0; ui < spFound->uiAttCount; ui++){
                        spFound[ui].uiAttCount++;
                    }
                    spXml->sCurrentAttList.uiAttCount = spFound->uiAttCount;
                    vpVecPush(spXml->vpVecAttDecls, &spXml->sCurrentAttList);
                    qsort(vpVecFirst(spXml->vpVecAttDecls), (size_t)uiVecLen(spXml->vpVecAttDecls), sizeof(att_decl), iAttComp);

                }else{
                    // first element by this name - push it on the list and sort
                    spXml->sCurrentAttList.uiAttCount = 1;
                    vpVecPush(spXml->vpVecAttDecls, &spXml->sCurrentAttList);
                    qsort(vpVecFirst(spXml->vpVecAttDecls), (size_t)uiVecLen(spXml->vpVecAttDecls), sizeof(att_decl), iAttComp);
                }
            }
        }else{
            // don't process this GE due to conditions
            vLogMsg(spXml, spData->uiParserOffset,
                    "Attribute List declaration not processed due to PE references found and standalone=\"no\".");
            spXml->uiAttListsNotProcessed++;
        }
    }
}
void vAttlistClose(callback_data* spData){
    if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("Expected close of attribute list declaration not found");
    }
}

/****************** ATTRIBUTES *******************************/
/****************** NOTATIONAL REFERENCE *********************/
void vNotationOpen(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        uint32_t* uipName = (uint32_t*)vpVecFirst(spXml->vpVecName);
        aint uiNameLen = uiVecLen(spXml->vpVecName);
        if(!uipName || !uiNameLen){
            XML_THROW("Notation Declaration has no name.");
        }

        // initialize the Notation Declaration named value
        entity_decl* spNotation = (entity_decl*)vpVecPush(spXml->vpVecNotationDecls, NULL);
        memset(spNotation, 0, sizeof(entity_decl));

        // push the name cdata_id on the save vec
        spNotation->sName.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
        spNotation->sName.uiLength = (uint32_t)uiNameLen;
        vpVecPushn(spXml->vpVec32, uipName, uiNameLen);

        // set up for the named value
        spXml->uiSavedOffset = spData->uiParserOffset;
    }
}
void vNotationDef(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        // convert the definition characters and save
        entity_decl* spNotation = (entity_decl*)vpVecLast(spXml->vpVecNotationDecls);
        if(!spNotation){
            XML_THROW("Notation Declaration syntax error. Name value of Notation should not be empty.");
        }
        // convert parsed data to UTF-32
        vConvertParsedData(spXml, &spData->acpString[spData->uiParserOffset], spData->uiParserPhraseLength,
                &spNotation->sValue.uiOffset, &spNotation->sValue.uiLength);
    }
}
void vNotationClose(callback_data* spData){
    if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("Notation Declaration syntax error. Expected closure not found");
    }
}
/****************** NOTATIONAL REFERENCE *********************/
void vElementOpen(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        xml* spXml = (xml*)spData->vpUserData;
        spXml->uiSavedOffset = spData->uiParserOffset;
        spXml->uiElementDecls++;
    }
}
void vElementClose(callback_data* spData){
    if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        vLogMsg(spXml, spXml->uiSavedOffset, "Malformed element declaration.");
        XML_THROW("Element declaration expected closure not found");
    }
}
/*********************************************************
 * DOCUMENT TYPE DECLARATION (DTD) +++++++++++++++++++++++
 ********************************************************/
/*********************************************************
 * ENTITY REFERENCES +++++++++++++++++++++++++++++++++++++
 ********************************************************/
void vRefClose(callback_data* spData){
    if(spData->uiParserState == ID_NOMATCH){
        xml* spXml = (xml*)spData->vpUserData;
        XML_THROW("mal formed reference, expected ; not found");
    }
}
/*********************************************************
 * ENTITY REFERENCES -------------------------------------
 ********************************************************/
