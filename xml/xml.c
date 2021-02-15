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
/** \file xml/xml.c
 * \brief The APG XML parser.
 *
 * This is meant to be a [standards-compliant](https://www.w3.org/TR/REC-xml/), non-validating XML parser.
 * XML input may be UTF-8, UTF-16BE or UTF-16LE.
 * If UTF-16, a Byte Order Mark (BOM) must be present and UTF-16 must be specified in the XML declaration
 * which must be present. UTF-16 format is transcoded to UTF-8 prior to parsing.
 * All line ends are converted to 0x0A (\\n or LF) and all characters are checked for XML character validity.
 *
 * The Document Type Declaration (DTD) internal subset, if present, is parsed and entity declarations
 * and default attribute values are tabulated and used when parsing the document body.
 *
 * All applicable well-formedness constraints are checked.
 - 1.) Well-formedness constraint: PEs in Internal Subset<br>
In the internal DTD subset, parameter-entity references MUST NOT occur within markup declarations; they may occur where markup declarations can occur. (This does not apply to references that occur in external parameter entities or to the external subset.)

 - 2.) Well-formedness constraint: External Subset<br>
The external subset, if any, MUST match the production for extSubset.<br>
  <i>(Not Applicable - External subset not parsed.)</i>

 - 3.) Well-formedness constraint: PE Between Declarations<br>
The replacement text of a parameter entity reference in a DeclSep MUST match the production extSubsetDecl.<br>
 <i>(Not Applicable - Parameter Entities not read.)</i>

 - 4.) Well-formedness constraint: Element Type Match<br>
The Name in an element's end-tag MUST match the element type in the start-tag.

 - 5.0 Well-formedness constraint: Unique Att Spec<br>
An attribute name MUST NOT appear more than once in the same start-tag or empty-element tag.

 - 6.) Well-formedness constraint: No External Entity References<br>
Attribute values MUST NOT contain direct or indirect entity references to external entities.<br>
 <i>(Not Applicable - No external entities are read.)</i>

 - 7.) Well-formedness constraint: No < in Attribute Values<br>
The replacement text of any entity referred to directly or indirectly in an attribute value MUST NOT contain a <.

 - 8.) Well-formedness constraint: Legal Character<br>
Characters referred to using character references MUST match the production for Char.

 - 9.) Well-formedness constraint: Entity Declared<br>
In a document without any DTD, a document with only an internal DTD subset which contains no parameter entity references, or a document with " standalone='yes' ", for an entity reference that does not occur within the external subset or a parameter entity, the Name given in the entity reference MUST match that in an entity declaration that does not occur within the external subset or a parameter entity, except that well-formed documents need not declare any of the following entities: amp, lt, gt, apos, quot. The declaration of a general entity MUST precede any reference to it which appears in a default value in an attribute-list declaration.


 - 10.) Well-formedness constraint: Parsed Entity<br>
An entity reference MUST NOT contain the name of an unparsed entity.
Unparsed entities may be referred to only in attribute values declared to be of type ENTITY or ENTITIES.<br>
 <i>(Not Applicable - No unparsed entities are read.)</i>

 - 11.) Well-formedness constraint: No Recursion<br>
A parsed entity MUST NOT contain a recursive reference to itself, either directly or indirectly.

 - 12.) Well-formedness constraint: In DTD<br>
Parameter-entity references MUST NOT appear outside the DTD.<br>
 <i>(The ABNF grammar prevents this.)</i>
     */

#include "xml.h"

//#include "../utilities/format.h"
#include "xmlgrammar.h"
#include "xmlp.h"

/** @name Misc. Constants
 * Some misc. constants used by the parser.
 */
/**@{*/
#define TAB                 9
#define LF                  10
#define CR                  13
#define LINE_LEN            16
#define LINE_LEN4           4
#define LINE_LEN8           8
#define LINE_LEN12          12
#define LT			        0x3C
#define CHARS_LINE_LEN      8
#define CHAR_BUF_LEN        256
/**@}*/

/** \struct input_info
 * \brief Information about the input data.
 */
typedef struct{
   aint uiType; ///< \brief The data type as determined from an examination of the first few byters.
   abool bBom; ///< \brief Specifies whether a BOM is present (required for UTF-16).
   abool bValid; ///< \brief True if this is a valid XML file, false otherwise.
} input_info;

static const void* s_vpMagicNumber = (const void*)"xml";

static void vClear(xml* spXml);
//static void vDisplayParserState(xml* spXml, parser_state* spState);
static void vDisplayXml(xml* spXml, abool bShowLines, void* vpVecChars);
static abool bIsUtf8(uint8_t* ucpData);
static abool bIsUtf16be(uint8_t* ucpData);
static abool bIsUtf16le(uint8_t* ucpData);
static abool bUtfType(uint8_t* ucpData, aint* uipStartByte, aint* uipTrueType, input_info* spInfo);
static void vEmptyTagDisplay(u32_phrase* spName, u32_phrase* spAttrNames, u32_phrase* spAttrValues,
        uint32_t uiAttrCount, void* vpUserData);
static void vStartTagDisplay(u32_phrase* spName, u32_phrase* spAttrNames, u32_phrase* spAttrValues,
        uint32_t uiAttrCount, void* vpUserData);
static void vEndTagDisplay(u32_phrase* spName, u32_phrase* spContent, void* vpUserData);
static void vPIDisplay(u32_phrase* spTarget, u32_phrase* spInfo, void* vpUserData);
static void vXmlDeclDisplay(xmldecl_info* spInfo, void* vpUserData);
static void vDTDDisplay(dtd_info* spInfo, void* vpUserData);
static void vCommentDisplay(u32_phrase* spComment, void* vpUserData);
static void vDisplayUnicode(void* vpFmt, const uint32_t* uipChars, uint32_t uiLength);
static void vGetData(xml* spXml, uint8_t* ucpData, aint uiDataLen, char* cpBuf, size_t uiBufBeg, size_t uiBufLen);
static void vDisplayCData(char* cpName, u32_phrase* spData, void* vpFmt);

/**
 * \brief The XML Parser constructor.
 *
 * Allocates memory for the XML Parser component and constructs all of the required working memory vectors.
 * All display is to `stdout` and the default values of all callback function pointers are NULL.
 * To set the callback functions see:<br>
  \ref vXmlSetXmlDeclCallback<br>
  \ref vXmlSetDTDCallback<br>
  \ref vXmlSetEmptyTagCallback<br>
  \ref vXmlSetStartTagCallback<br>
  \ref vXmlSetEndTagCallback<br>
  \ref vXmlSetPICallback<br>
  \ref vXmlSetCommentCallback<br>
 * Note that this parser assumes that input is always in the form of 8-bit byte streams
 * and that the type "char" is also 8-bits.
 * For string functions, it is always assumed that conversions from uint8_t to char is OK.
 * \param spEx Pointer to a valid exception structure.
 * \return Pointer to the parser's context. An exception is thrown on memory allocation errors.
 */
void* vpXmlCtor(exception* spEx){
    xml* spXml = NULL;
    if(!bExValidate(spEx)){
        vExContext();
    }
    if(CHAR_BIT != 8){
        // make sure a char is 8 bits for this processor and compiler
        char caBuf[CHAR_BUF_LEN];
        snprintf(caBuf, CHAR_BUF_LEN,
                "On this system, length of char is %d bits. This XML processor requires length of char to be 8 bits.",
                (int)CHAR_BIT);
        XTHROW(spEx, caBuf);
    }
    void* vpMem = vpMemCtor(spEx);
    spXml = (xml*)vpMemAlloc(vpMem, sizeof(xml));
    memset((void*)spXml, 0, sizeof(xml));
    spXml->vpMem = vpMem;
    spXml->spException = spEx;
    spXml->vpConv = vpConvCtor(spEx);
    spXml->vpFmt = vpFmtCtor(spEx);
    spXml->vpMsgs = vpMsgsCtor(spEx);
    spXml->vpVecChars = vpVecCtor(vpMem, sizeof(uint8_t), 4096);
    spXml->vpVec32 = vpVecCtor(vpMem, sizeof(uint32_t), 4096);
    spXml->vpVec8 = vpVecCtor(vpMem, sizeof(uint8_t), 4096);
    spXml->vpVecString = vpVecCtor(vpMem, sizeof(char), 4096);
    spXml->vpVecName = vpVecCtor(vpMem, sizeof(uint32_t), 4096);
    spXml->vpVecCData = vpVecCtor(vpMem, sizeof(u32_phrase), 512);
    spXml->vpVecGEDefs= vpVecCtor(vpMem, sizeof(entity_decl), 64);
    spXml->vpVecEntityFrames = vpVecCtor(vpMem, sizeof(entity_frame), 512);
    spXml->vpVecNotationDecls= vpVecCtor(vpMem, sizeof(entity_decl), 64);
    spXml->vpVecAttDecls = vpVecCtor(vpMem, sizeof(att_decl), 64);
    spXml->vpVecAttWork = vpVecCtor(vpMem, sizeof(uint32_t), 4096);
    spXml->vpVecAttList = vpVecCtor(vpMem, sizeof(named_value), 64);
    spXml->vpVecFrame = vpVecCtor(vpMem, sizeof(element_frame), 4096);
    spXml->vpValidate = s_vpMagicNumber;
    return (void*)spXml;
}

static void vClear(xml* spXml){
    vMsgsClear(spXml->vpMsgs);
    vLinesDtor(spXml->vpLines);
    spXml->vpLines = NULL;
    vParserDtor(spXml->vpParser);
    spXml->vpParser = NULL;
    if(spXml->ucpData){
        vMemFree(spXml->vpMem, spXml->ucpData);
        spXml->ucpData = NULL;
    }
    if(spXml->acpChars){
        vMemFree(spXml->vpMem, spXml->acpChars);
        spXml->acpChars = NULL;
    }
    spXml->bStandalone = APG_FALSE;
    spXml->bExtSubset = APG_FALSE;
    spXml->uiExternalIds = 0;
    spXml->uiPEDecls = 0;
    spXml->uiPERefs = 0;
    spXml->uiGEDeclsNotProcessed = 0;
    spXml->uiAttListsNotProcessed = 0;
    vVecClear(spXml->vpVecChars);
    vVecClear(spXml->vpVecName);
    vVecClear(spXml->vpVec32);
    vVecClear(spXml->vpVec8);
    vVecClear(spXml->vpVecString);
    vVecClear(spXml->vpVecCData);
    vVecClear(spXml->vpVecGEDefs);
    vVecClear(spXml->vpVecEntityFrames);
    vVecClear(spXml->vpVecNotationDecls);
    vVecClear(spXml->vpVecAttDecls);
    vVecClear(spXml->vpVecAttWork);
    vVecClear(spXml->vpVecFrame);
    vVecClear(spXml->vpVecAttList);
}


/**
 * \brief The XML Parser component destructor.
 *
 * Releases all heap memory and
 * clears the context to prevent accidental use after release.
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlDtor(void* vpCtx){
    xml* spXml = (xml*)vpCtx;
    if(vpCtx && (spXml->vpValidate == s_vpMagicNumber)){
        void* vpMem;
        vConvDtor(spXml->vpConv);
        vFmtDtor(spXml->vpFmt);
        vMsgsDtor(spXml->vpMsgs);
        vLinesDtor(spXml->vpLines);
        vParserDtor(spXml->vpParser);
        vpMem = spXml->vpMem;
        memset((void*)spXml, 0, sizeof(xml));
        vMemDtor(vpMem);
    }
}

/** \brief Validate an XML context pointer.
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \return True if the pointer is a valid XML context pointer. False otherwise.
 */
abool bXmlValidate(void* vpCtx){
    xml* spXml = (xml*)vpCtx;
    if(vpCtx && (spXml->vpValidate == s_vpMagicNumber)){
        return APG_TRUE;
    }
    return APG_FALSE;
}

/** \brief Gets the XML byte stream from a file.
 *
 * This function just serves to get the file into a memory array.
 * \ref vXmlGetArray is then called to do the actual work.
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \param[in] cpFileName - The name of the XML file.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlGetFile(void* vpCtx, const char* cpFileName){
    xml* spXml = (xml*)vpCtx;
    if(vpCtx && (spXml->vpValidate == s_vpMagicNumber)){
        aint uiLen = 0;
        size_t uiBufSize = PATH_MAX + CHAR_BUF_LEN;
        size_t uiBufLen, uiBufBeg;
        char caBuf[PATH_MAX + CHAR_BUF_LEN];
        vUtilFileRead(spXml->vpMem, cpFileName, NULL, &uiLen);
        vClear(spXml);
        spXml->ucpData = (uint8_t*)vpMemAlloc(spXml->vpMem, uiLen);
        vUtilFileRead(spXml->vpMem, cpFileName, spXml->ucpData, &uiLen);
        uiBufBeg = (size_t)snprintf(caBuf, uiBufSize, "file: %s: ", cpFileName);
        if(uiBufBeg >= uiBufSize){
            // should never get here
            snprintf(caBuf, uiBufSize, "file name \"%s\" too long", cpFileName);
            XTHROW(spXml->spException, caBuf);
        }
        uiBufLen = uiBufSize - uiBufBeg;
        vGetData(spXml, spXml->ucpData, uiLen, caBuf, uiBufBeg, uiBufLen);
        vMemFree(spXml->vpMem, spXml->ucpData);
        spXml->ucpData = NULL;
    }else{
        vExContext();
    }
}

static void vGetData(xml* spXml, uint8_t* ucpData, aint uiDataLen, char* cpBuf, size_t uiBufBeg, size_t uiBufLen){
    aint uiStartByte;
    aint uiTrueType;
    input_info sInfo;
    vVecClear(spXml->vpVecChars);
    vLinesDtor(spXml->vpLines);
    spXml->vpLines = NULL;
    if(uiDataLen < 3){
        snprintf(cpBuf, uiBufLen, "input error: data has too few bytes (< 3)");
        XTHROW(spXml->spException, cpBuf);
    }

    // determine the actual data type
    if(!bUtfType(ucpData, &uiStartByte, &uiTrueType, &sInfo)){
        if(sInfo.bBom){
            const char* cpType = cpUtilUtfTypeName(sInfo.uiType);
            snprintf(&cpBuf[uiBufBeg], uiBufLen,
                    "input error: data begins with %s encoding type BOM but invalid XML characters follows", cpType);
            XTHROW(spXml->spException, cpBuf);
        }
        switch(sInfo.uiType){
        case UTF_8:
            // should never get here, but just in case
            snprintf(&cpBuf[uiBufBeg], uiBufLen,
                    "unexpected input error: type is UTF-8 an no errors are expected at this stage");
            break;
        case UTF_16BE:
            snprintf(&cpBuf[uiBufBeg], uiBufLen,
                    "input error: encoding type appears to be UTF-16BE but required BOM not present");
            break;
        case UTF_16LE:
            snprintf(&cpBuf[uiBufBeg], uiBufLen,
                    "input error: encoding type appears to be UTF-16LE but required BOM not present");
            break;
        default:
            snprintf(&cpBuf[uiBufBeg], uiBufLen,
                "input error: unrecognized encoding type  - invalid XML document");
            break;
        }
        if(cpBuf[uiBufBeg]){
            XTHROW(spXml->spException, cpBuf);
        }
    }
    spXml->uiTrueType = uiTrueType;
    ucpData += uiStartByte;
    uiDataLen -= uiStartByte;
    conv_src sSrc = {uiTrueType, ucpData, uiDataLen};

    // translate the input to 32-bit code points
    // NOTE: this process validates input characters are valid Unicode code points
    vConvDecode(spXml->vpConv, &sSrc);

    // validate control characters and convert line ends
    uint32_t uiCodeLen = 0;
    uint32_t* uipCode, *uipTrans;
    vConvGetCodePoints(spXml->vpConv, NULL, &uiCodeLen);
    vVecClear(spXml->vpVec32);
    uipCode = (uint32_t*)vpVecPushn(spXml->vpVec32, NULL, (aint)((2 * uiCodeLen) + 2));
    uipTrans = uipCode + uiCodeLen + 2;
    vConvGetCodePoints(spXml->vpConv, uipCode, &uiCodeLen);
    uipCode[uiCodeLen] = 0; // this insures the test for CRLF doesn't overrun & fail at the end of data
    uint32_t ui = 0;
    uint32_t uiCount = 0;
    uint32_t uiPoint;
    for(; ui < uiCodeLen; ui++){
        uiPoint = uipCode[ui];
        // check for invalid control characters
        if((uiPoint < 9) || (uiPoint == 11) || (uiPoint ==12) || ((uiPoint > 13) && (uiPoint < 32))){
            snprintf(&cpBuf[uiBufBeg], uiBufLen,
                    "code point 0x%02X at offset %"PRIuMAX" is disallowed control character",
                    uiPoint, (luint)ui);
            XTHROW(spXml->spException, cpBuf);
        }
        if((uiPoint == 0xFFFE) || (uiPoint == 0xFFFF)){
            snprintf(&cpBuf[uiBufBeg], uiBufLen,
                    "code point 0x%X at offset %"PRIuMAX" is disallowed (characters 0xFFFE & 0xFFFF are forbidden)",
                    uiPoint, (luint)ui);
            XTHROW(spXml->spException, cpBuf);
        }
        // convert line ends
        if(uiPoint == 13){
            uipTrans[uiCount++] = 10;
            if(uipCode[ui + 1] == 10){
                ui++;
            }
        }else{
            uipTrans[uiCount++] = uiPoint;
        }
    }

    // convert to UTF-8
    vConvUseCodePoints(spXml->vpConv, uipTrans, uiCount);
    conv_dst sDst = {UTF_8, NOBOM, NULL, 0};
    vConvEncode(spXml->vpConv, &sDst);
    vVecClear(spXml->vpVec32);
    vpVecPushn(spXml->vpVecChars, sDst.ucpData, sDst.uiDataLen);
    spXml->vpLines = vpLinesCtor(spXml->spException, vpVecFirst(spXml->vpVecChars), uiVecLen(spXml->vpVecChars));
}

/** \brief Gets the XML byte stream from a byte array.
 *
 * The first four characters are examined to determine the encoding scheme.
 * The data is the normalized as follows:
 *  - If UTF-16(BE or LE), the data is converted to UTF-8.
 *  - All line ends are converted, if necessary, to a single line feed (0x0A)
 *  - The all characters are examined for XML validity.
 *      - no control characters other than 0x09, 0x0A, 0x0D (tab, line feed, carriage return)
 *      - any Unicode character, excluding the surrogate blocks, FFFE, and FFFF
 *  - An exception is thrown if any characters are not valid.

 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \param[in] ucpData - The input byte stream as an array of unsigned characters.
 * \param[in] uiDataLen - The number of bytes in the stream.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlGetArray(void* vpCtx, uint8_t* ucpData, aint uiDataLen){
    xml* spXml = (xml*)vpCtx;
    if(!vpCtx || (spXml->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    char caBuf[CHAR_BUF_LEN];
    caBuf[0] = 0;
    vClear(spXml);
    vGetData(spXml, ucpData, uiDataLen, caBuf, 0, CHAR_BUF_LEN);
}

// Add the pre-defined entity definitions to the entity list
static void vPreDefinedEntities(xml* spXml){
    static uint32_t uiAmp = 38;
    static uint32_t uiApos = 39;
    static uint32_t uiGt = 62;
    static uint32_t uiLt = 60;
    static uint32_t uiQuot = 34;
    static uint32_t uiAmpName[] = {97,109,112};
    static uint32_t uiAposName[] = {97,112,111,115};
    static uint32_t uiGtName[] = {103,116};
    static uint32_t uiLtName[] = {108,116};
    static uint32_t uiQuotName[] = {113,117,111,116};
    entity_decl* spValue;
    // amp
    spValue = (entity_decl*)vpVecPush(spXml->vpVecGEDefs, NULL);
    memset(spValue, 0, sizeof(entity_decl));
    spValue->spXml = spXml;
    spValue->sName.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
    spValue->sName.uiLength = 3;
    vpVecPushn(spXml->vpVec32, uiAmpName, 3);
    spValue->sValue.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
    spValue->sValue.uiLength = 1;
    vpVecPush(spXml->vpVec32, &uiAmp);
    // apos
    spValue = (entity_decl*)vpVecPush(spXml->vpVecGEDefs, NULL);
    memset(spValue, 0, sizeof(entity_decl));
    spValue->spXml = spXml;
    spValue->sName.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
    spValue->sName.uiLength = 4;
    vpVecPushn(spXml->vpVec32, uiAposName, 4);
    spValue->sValue.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
    spValue->sValue.uiLength = 1;
    vpVecPush(spXml->vpVec32, &uiApos);
    // gt
    spValue = (entity_decl*)vpVecPush(spXml->vpVecGEDefs, NULL);
    memset(spValue, 0, sizeof(entity_decl));
    spValue->spXml = spXml;
    spValue->sName.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
    spValue->sName.uiLength = 2;
    vpVecPushn(spXml->vpVec32, uiGtName, 2);
    spValue->sValue.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
    spValue->sValue.uiLength = 1;
    vpVecPush(spXml->vpVec32, &uiGt);
    // lt
    spValue = (entity_decl*)vpVecPush(spXml->vpVecGEDefs, NULL);
    memset(spValue, 0, sizeof(entity_decl));
    spValue->spXml = spXml;
    spValue->sName.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
    spValue->sName.uiLength = 2;
    vpVecPushn(spXml->vpVec32, uiLtName, 2);
    spValue->sValue.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
    spValue->sValue.uiLength = 1;
    vpVecPush(spXml->vpVec32, &uiLt);
    // quot
    spValue = (entity_decl*)vpVecPush(spXml->vpVecGEDefs, NULL);
    memset(spValue, 0, sizeof(entity_decl));
    spValue->spXml = spXml;
    spValue->sName.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
    spValue->sName.uiLength = 4;
    vpVecPushn(spXml->vpVec32, uiQuotName, 4);
    spValue->sValue.uiOffset = (uint32_t)uiVecLen(spXml->vpVec32);
    spValue->sValue.uiLength = 1;
    vpVecPush(spXml->vpVec32, &uiQuot);
    spXml->uiGEDeclsTotal = 5;
}

/** \brief Parse the XML data from \ref vXmlGetFile or \ref vXmlGetArray.
 *
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlParse(void* vpCtx){
    xml* spXml = (xml*)vpCtx;
    if(!vpCtx || (spXml->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    parser_state sState;
    parser_config sInput;
    aint ui;
    aint uiCharCount = uiVecLen(spXml->vpVecChars);
    uint8_t* uipInputChars = (uint8_t*)vpVecFirst(spXml->vpVecChars);
    if(!uipInputChars || !uiCharCount){
        XTHROW(spXml->spException, "no XML input");
    }

    // initialize the pre-defined general entities
    vPreDefinedEntities(spXml);

    // construct the parser
    spXml->vpParser = vpParserCtor(spXml->spException, vpXmlgrammarInit);

    // set the rule callback functions
    vXmlgrammarRuleCallbacks(spXml->vpParser);

    // if achar is not 8-bit character, convert the input characters
    achar* acpInput = (achar*)uipInputChars;
    if(sizeof(achar) != sizeof(uint8_t)){
        if(spXml->acpChars){
            vMemFree(spXml->vpMem, spXml->acpChars);
        }
        spXml->acpChars = (achar*)vpMemAlloc(spXml->vpMem, (sizeof(achar) * uiCharCount));
        for(ui = 0; ui < uiCharCount; ui++){
            spXml->acpChars[ui] = (achar)uipInputChars[ui];
        }
        acpInput = spXml->acpChars;
    }

    // configure the parser
    memset((void*)&sInput, 0, sizeof(sInput));
    sInput.acpInput = acpInput;
    sInput.uiInputLength = uiCharCount;
    sInput.uiStartRule = XMLGRAMMAR_DOCUMENT;
    sInput.vpUserData = (void*)spXml;
    sInput.bParseSubString = APG_FALSE;

    // parse the input
    vParserParse(spXml->vpParser, &sInput, &sState);

    if(!sState.uiSuccess){
        XTHROW(spXml->spException, "XML parser failed: invalid XML input");
    }
    if(spXml->acpChars){
        vMemFree(spXml->vpMem, spXml->acpChars);
        spXml->acpChars = NULL;
    }
    vClear(spXml);
}

/** \brief Display the parser's messages on stdout, if any.
 *
 * Nothing is displayed if there are no messages.
 * Messages are collected as the parser detects errors in the DTD.
 * Other than the DTD, fatal errors result in a thrown exception.
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlDisplayMsgs(void* vpCtx){
    xml* spXml = (xml*)vpCtx;
    if(!vpCtx || (spXml->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    if(uiMsgsCount(spXml->vpMsgs)){
        vUtilPrintMsgs(spXml->vpMsgs);
    }
}

/** \brief Give the user a handle to the message log.
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \return Pointer to the parser's message log context.
 */
void* vpXmlGetMsgs(void* vpCtx){
    xml* spXml = (xml*)vpCtx;
    if(!vpCtx || (spXml->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    return spXml->vpMsgs;
}

/** \brief Set the user's callback function for the start tags (`<name attr="10">`).
 *
 * This function is called when a start tag has been found.
 * See \ref pfnStartTagCallback for the declaration.<br>
 *  Note that the utility function vStartTagDisplay will make a simple display of the start tag name and attributes.
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \param[in] pfnCallback Pointer to the callback function to use.
 * Use `DEFAULT_CALLBACK` to invoke a pre-defined callback function which will simply display the data to `stdout`.
 * In this case, the vpUserData argument is ignored and my be NULL.
 * \param[in] vpUserData This pointer will be passed through to the user's callback function.
 * It is never dereferenced by the XML parser and may be NULL.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlSetStartTagCallback(void* vpCtx, pfnStartTagCallback pfnCallback, void* vpUserData){
    xml* spXml = (xml*)vpCtx;
    if(vpCtx && (spXml->vpValidate == s_vpMagicNumber)){
        if(pfnCallback == DEFAULT_CALLBACK){
            spXml->pfnStartTagCallback = vStartTagDisplay;
            spXml->vpStartTagData = (void*)spXml;
        }else{
            spXml->pfnStartTagCallback = pfnCallback;
            spXml->vpStartTagData = vpUserData;
        }
    }else{
        vExContext();
    }
}

/** \brief Set the user's callback function for the empty tags (`<name attr="10"/>`).
 *
 * This function is called when an empty tag has been found.
 * See \ref pfnEmptyTagCallback for the declaration.<br>
 * Note that this function presents the callback function with exactly the same information that a start tag callback will get.
 * The user may assign the same callback function to both.
 * The option to have separate callback functions for empty and start tags
 * is simply a convenience in case a distinction needs to be made.<br>
 * Note that the utility function vStartTagDisplay will make a simple display of the empty tag name and attributes.
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \param[in] pfnCallback Pointer to the callback function to use.
 * Use `DEFAULT_CALLBACK` to invoke a pre-defined callback function which will simply display the data to `stdout`.
 * In this case, the vpUserData argument is ignored and my be NULL.
 * \param[in] vpUserData This pointer will be passed through to the user's callback function.
 * It is never dereferenced by the XML parser and may be NULL.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlSetEmptyTagCallback(void* vpCtx, pfnEmptyTagCallback pfnCallback, void* vpUserData){
    xml* spXml = (xml*)vpCtx;
    if(vpCtx && (spXml->vpValidate == s_vpMagicNumber)){
        if(pfnCallback == DEFAULT_CALLBACK){
            spXml->pfnEmptyTagCallback = vEmptyTagDisplay;
            spXml->vpEmptyTagData = (void*)spXml;
        }else{
            spXml->pfnEmptyTagCallback = pfnCallback;
            spXml->vpEmptyTagData = vpUserData;
        }
    }
}
/** \brief Set the user's callback function for the end tags (`</name>`).
 *
 * This function is called when an end tag has been found.
 * See \ref pfnEndTagCallback for the declaration.
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \param[in] pfnCallback Pointer to the callback function to use.
 * Use `DEFAULT_CALLBACK` to invoke a pre-defined callback function which will simply display the data to `stdout`.
 * In this case, the vpUserData argument is ignored and my be NULL.
 * \param[in] vpUserData This pointer will be passed through to the user's callback function.
 * It is never dereferenced by the XML parser and may be NULL.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlSetEndTagCallback(void* vpCtx, pfnEndTagCallback pfnCallback, void* vpUserData){
    xml* spXml = (xml*)vpCtx;
    if(vpCtx && (spXml->vpValidate == s_vpMagicNumber)){
        if(pfnCallback == DEFAULT_CALLBACK){
            spXml->pfnEndTagCallback = vEndTagDisplay;
            spXml->vpEndTagData = (void*)spXml;
        }else{
            spXml->pfnEndTagCallback = pfnCallback;
            spXml->vpEndTagData = vpUserData;
        }
    }
}

/** \brief Set the user's callback function for the Processing Instruction tags(`<?target instructions?>`).
 *
 * This function is called when a Processing Instruction tag has been found.
 * See \ref pfnPICallback for the declaration.
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \param[in] pfnCallback Pointer to the callback function to use.
 * Use `DEFAULT_CALLBACK` to invoke a pre-defined callback function which will simply display the data to `stdout`.
 * In this case, the vpUserData argument is ignored and my be NULL.
 * \param[in] vpUserData This pointer will be passed through to the user's callback function.
 * It is never dereferenced by the XML parser and may be NULL.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlSetPICallback(void* vpCtx, pfnPICallback pfnCallback, void* vpUserData){
    xml* spXml = (xml*)vpCtx;
    if(vpCtx && (spXml->vpValidate == s_vpMagicNumber)){
        if(pfnCallback == DEFAULT_CALLBACK){
            spXml->pfnPICallback = vPIDisplay;
            spXml->vpPIData = (void*)spXml;
        }else{
            spXml->pfnPICallback = pfnCallback;
            spXml->vpPIData = vpUserData;
        }
    }
}

/** \brief Set the user's callback function for the XML declaration.
 *
 * This function is called when parsing the XML declaration has finished.
 * See \ref pfnXmlDeclCallback for the declaration.
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \param[in] pfnCallback Pointer to the callback function to use.
 * Use `DEFAULT_CALLBACK` to invoke a pre-defined callback function which will simply display the data to `stdout`.
 * In this case, the vpUserData argument is ignored and my be NULL.
 * \param[in] vpUserData This pointer will be passed through to the user's callback function.
 * It is never dereferenced by the XML parser and may be NULL.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlSetXmlDeclCallback(void* vpCtx, pfnXmlDeclCallback pfnCallback, void* vpUserData){
    xml* spXml = (xml*)vpCtx;
    if(vpCtx && (spXml->vpValidate == s_vpMagicNumber)){
        if(pfnCallback == DEFAULT_CALLBACK){
            spXml->pfnXmlDeclCallback = vXmlDeclDisplay;
            spXml->vpXmlDeclData = (void*)spXml;
        }else{
            spXml->pfnXmlDeclCallback = pfnCallback;
            spXml->vpXmlDeclData = vpUserData;
        }
    }
}

/** \brief Set the user's callback function for the Processing Instruction tags(`<?target instructions?>`).
 *
 * This function is called when a Processing Instruction tag has been found.
 * See \ref pfnDTDCallback for the declaration.
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \param[in] pfnCallback Pointer to the callback function to use.
 * Use `DEFAULT_CALLBACK` to invoke a pre-defined callback function which will simply display the data to `stdout`.
 * In this case, the vpUserData argument is ignored and my be NULL.
 * \param[in] vpUserData This pointer will be passed through to the user's callback function.
 * It is never dereferenced by the XML parser and may be NULL.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlSetDTDCallback(void* vpCtx, pfnDTDCallback pfnCallback, void* vpUserData){
    xml* spXml = (xml*)vpCtx;
    if(vpCtx && (spXml->vpValidate == s_vpMagicNumber)){
        if(pfnCallback == DEFAULT_CALLBACK){
            spXml->pfnDTDCallback = vDTDDisplay;
            spXml->vpDTDData = (void*)spXml;
        }else{
            spXml->pfnDTDCallback = pfnCallback;
            spXml->vpDTDData = vpUserData;
        }
    }
}

/** \brief Set the user's callback function for comments.
 *
 * This function is called when a comment has been found.
 * See \ref pfnCommentCallback for the declaration.
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \param[in] pfnCallback Pointer to the callback function to use.
 * Use `DEFAULT_CALLBACK` to invoke a pre-defined callback function which will simply display the data to `stdout`.
 * In this case, the vpUserData argument is ignored and my be NULL.
 * \param[in] vpUserData This pointer will be passed through to the user's callback function.
 * It is never dereferenced by the XML parser and may be NULL.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlSetCommentCallback(void* vpCtx, pfnCommentCallback pfnCallback, void* vpUserData){
    xml* spXml = (xml*)vpCtx;
    if(vpCtx && (spXml->vpValidate == s_vpMagicNumber)){
        if(pfnCallback == DEFAULT_CALLBACK){
            spXml->pfnCommentCallback = vCommentDisplay;
            spXml->vpCommentData = (void*)spXml;
        }else{
            spXml->pfnCommentCallback = pfnCallback;
            spXml->vpCommentData = vpUserData;
        }
    }
}

/** \brief Display input file.
 *
 * This utility function will display the (converted<sup>*</sup>) input byte stream in hexdump format,
 * displaying the data as both hex bytes and ASCII for printing characters.<br>
 * <sup>*</sup>
 *   - if type is UTF-16 (BE or LE) it is converted to UTF-8 before display and parsing
 *   - the BOM, if present, is stripped
 *   - to display the original, as-is, file, use a hexdump utility external to this XML processor
 * \param[in] vpCtx pointer to the parser's context - previously returned from \ref vpXmlCtor.
 * \param[in] bShowLines - if true (!= 0) data is displayed by line number. Otherwise, just a dump blob.
 * \return void - an exception is thrown on any fatal error.
 */
void vXmlDisplayInput(void* vpCtx, abool bShowLines){
    xml* spXml = (xml*)vpCtx;
    if(vpCtx && (spXml->vpValidate == s_vpMagicNumber)){
        vDisplayXml(spXml, bShowLines, spXml->vpVecChars);
    }
}

/****************************************************************
 * STATIC FUNCTIONS
 ****************************************************************/
static void vDisplayCData(char* cpName, u32_phrase* spData, void* vpUserData){
    xml* spXml = (xml*)vpUserData;
    if(bIsPhrase32Ascii(spData)){
        vVecClear(spXml->vpVecString);
        char* cpStr = (char*)vpVecPushn(spXml->vpVecString, NULL, (spData->uiLength + 1));
        cpPhrase32ToStr(spData, cpStr);
        printf("%10s: '%s'\n", cpName, cpStr);
        vVecClear(spXml->vpVecString);
    }else{
        printf("%10s: (some or all characters non-ASCII)\n", cpName);
        vDisplayUnicode(spXml->vpFmt, spData->uipPhrase, spData->uiLength);
    }
}

static void vDisplayUnicode(void* vpFmt, const uint32_t* uipChars, uint32_t uiLength){
    const char* cpNext = cpFmtFirstUnicode(vpFmt, uipChars, uiLength, 0, 0);
    while(cpNext){
        printf("%s", cpNext);
        cpNext = cpFmtNext(vpFmt);
    }
}
/** \brief Simple start tag callback function which displays the tag information on `stdout`.
 *
 * See \ref pfnStartTagCallback.
 * Displays the element name and attributes.
 * See \ref u32_phrase for the format of the information.
 * Data is presented as a 32-bit integer array or a null-terminated string if all are printing characters.
 * \param spName The element name.
 * \param spAttNames An array of attribute names.
 * \param spAttValues An array of attribute values
 * \param spAttCount The number of attributes.
 * \param A pointer to the user's data passed the parser via \ref bXmlParse..
 * \return void
 */
static void vStartTagDisplay(u32_phrase* spName, u32_phrase* spAttNames, u32_phrase* spAttValues,
        uint32_t uiAttCount, void* vpUserData){
    printf("Start Tag\n");
    vDisplayCData("name", spName, vpUserData);
    printf("Attributes (%u)\n", uiAttCount);
    uint32_t ui = 0;
    for(; ui < uiAttCount; ui++){
        vDisplayCData("name", &spAttNames[ui], vpUserData);
        vDisplayCData("value", &spAttValues[ui], vpUserData);
    }
    printf("\n");
}

/** \brief Simple end tag callback function which displays the tag information on `stdout`.
 *
 * See \ref pfnEndTagCallback.
 * Displays the element name and content.
 * See \ref u32_phrase for the format of the information.
 * Data is presented as a 32-bit integer array or a null-terminated string if all are printing characters.
 * \param spName The element name.
 * \param spContent The element's character data content.
 * \param A pointer to a format object
 */
static void vEndTagDisplay(u32_phrase* spName, u32_phrase* spContent, void* vpUserData){
    printf("End Tag\n");
    vDisplayCData("name", spName, vpUserData);
    vDisplayCData("content", spContent, vpUserData);
    printf("\n");
}

/** \brief Simple processing instruction callback function which displays the instruction information on `stdout`.
 *
 * See \ref pfnEndTagCallback.
 * Displays the element name and content.
 * See \ref u32_phrase for the format of the information.
 * Data is presented as a 32-bit integer array or a null-terminated string if all are printing characters.
 * \param spTarget The Processing Instruction target or name.
 * \param spInfo The processing instructions.
 * \param A pointer to the user's data passed the parser, \ref bXmlParse..
 * \return void
 */
static void vPIDisplay(u32_phrase* spTarget, u32_phrase* spInfo, void* vpUserData){
    printf("Processing Instruction\n");
    vDisplayCData("target", spTarget, vpUserData);
    vDisplayCData("info", spInfo, vpUserData);
    printf("\n");
}

static void vXmlDeclDisplay(xmldecl_info* spInfo, void* vpUserData){
    printf("INFORMATION: XML DECLARATION\n");
    static char* cpFormat = ""
            "exists     = %s\n"
            "version    = %s\n"
            "encoding   = %s\n"
            "standalone = %s\n";
    printf(cpFormat, spInfo->cpExists, spInfo->cpVersion, spInfo->cpEncoding, spInfo->cpStandalone);
    printf("\n");
}
static void vCommentDisplay(u32_phrase* spComment, void* vpUserData){
    vDisplayCData("comment", spComment, vpUserData);
    printf("\n");
}

static void vDTDDisplay(dtd_info* spInfo, void* vpUserData){
    static char* cpYes = "yes";
    static char* cpNo = "no";
    printf("INFORMATION: DOCUMENT TYPE DECLARATION (DTD)\n");
    if(spInfo->bExists){
        printf("%3s: %s\n", cpYes, "DTD exists");
        printf("%3s: %s\n", (spInfo->bStandalone ? cpYes : cpNo), "Document is standalone");
        printf("%3s: %s\n", (spInfo->bExtSubset ? cpYes : cpNo), "DTD has external subset");
        printf("%3d: %s\n", (int)spInfo->uiExternalIds, "external ids");
        printf("%3d: %s\n", (int)spInfo->uiPEDecls, "Parameter Entity declarations");
        printf("%3d: %s\n", (int)spInfo->uiPERefs, "Parameter Entity references");
        printf("%3d: %s\n", (int)spInfo->uiGEDeclsDeclared,
                "General Entity declarations: all declarations (includes pre-defined & not processed)");
        printf("%3d: %s\n", (int)spInfo->uiGEDeclsNotProcessed, "General Entity declarations: not processed");
        printf("%3d: %s\n", (int)spInfo->uiGEDeclsUnique, "General Entity declarations: unique processed (includes pre-defined)");
        printf("%3d: %s\n", (int)spInfo->uiAttListsDeclared, "Attribute List declarations: all declarations");
        printf("%3d: %s\n", (int)spInfo->uiAttListsUnique, "Attribute List declarations: unique element/attribute name combinations");
        printf("%3d: %s\n", (int)spInfo->uiAttListsNotProcessed, "Attribute List declarations: not processed");
        printf("%3d: %s\n", (int)spInfo->uiElementDecls, "Element declarations");
        printf("%3d: %s\n", (int)spInfo->uiNotationDecls, "Notation declarations");

        printf("\n");
        vDisplayCData("document name", spInfo->spName, vpUserData);
        if(spInfo->uiGEDeclsUnique){
            printf("\n");
            printf("General Entity names and values\n");
            aint ui = 0;
            for(; ui < spInfo->uiGEDeclsUnique; ui++){
                vDisplayCData("entity  name", &spInfo->spGENames[ui], vpUserData);
                vDisplayCData("entity value", &spInfo->spGEValues[ui], vpUserData);
            }
        }
        if(spInfo->uiAttListsUnique){
            printf("\n");
            printf("Attribute List element names, attribute names and attribute values\n");
            aint ui = 0;
            for(; ui < spInfo->uiAttListsUnique; ui++){
                vDisplayCData("element    name", &spInfo->spAttElementNames[ui], vpUserData);
                vDisplayCData("attribute  name", &spInfo->spAttNames[ui], vpUserData);
                vDisplayCData("attribute  type", &spInfo->spAttTypes[ui], vpUserData);
                vDisplayCData("attribute value", &spInfo->spAttValues[ui], vpUserData);
            }
        }
        if(spInfo->uiNotationDecls){
            printf("\n");
            printf("Notation names and values\n");
            aint ui = 0;
            for(; ui < spInfo->uiNotationDecls; ui++){
                vDisplayCData("notation name", &spInfo->spNotationNames[ui], vpUserData);
                vDisplayCData("notation value", &spInfo->spNotationValues[ui], vpUserData);
            }
        }
    }else{
        printf("%3s: %s\n", cpNo, "DTD exists");
    }
    printf("\n");
}
/** \brief Simple empty tag callback function which displays the tag information on `stdout`.
 *
 * See \ref pfnStartTagCallback.
 * Displays the element name and attributes.
 * See \ref u32_phrase for the format of the information.
 * Data is presented as a 32-bit integer array or a null-terminated string if all are printing characters.
 * \param spName The element name.
 * \param spAttNames An array of attribute names.
 * \param spAttValues An array of attribute values
 * \param spAttCount The number of attributes.
 * \param A pointer to the user's data passed the parser, \ref bXmlParse..
 */
static void vEmptyTagDisplay(u32_phrase* spName, u32_phrase* spAttNames, u32_phrase* spAttValues,
        uint32_t uiAttCount, void* vpUserData){
    printf("Empty Tag\n");
    vDisplayCData("name", spName, vpUserData);
    printf("Attributes (%u)\n", uiAttCount);
    uint32_t ui = 0;
    for(; ui < uiAttCount; ui++){
        vDisplayCData("name", &spAttNames[ui], vpUserData);
        vDisplayCData("value", &spAttValues[ui], vpUserData);
    }
    printf("\n");
}

// display the parser's state on the current output stream
//static void vDisplayParserState(xml* spXml, parser_state* spState) {
//    aint uiState;
//    printf("PARSER STATE:\n");
//    printf("       success: ");
//    printf("%s", cpUtilTrueFalse(spState->uiSuccess));
//    printf("\n");
//    printf("         state: ");
//    uiState = spState->uiState;
//    if ((uiState == ID_MATCH) && (spState->uiPhraseLength == 0)) {
//        uiState = ID_EMPTY;
//    }
//    if(uiState == ID_MATCH){
//        printf("MATCH\n");
//    }else if(uiState == ID_NOMATCH){
//        printf("NO MATCH\n");
//    }else if(uiState == ID_EMPTY){
//        printf("EMPTY\n");
//    }
//    printf(" phrase length: %"PRIuMAX"\n", (luint) spState->uiPhraseLength);
//    printf("  input length: %"PRIuMAX"\n", (luint) spState->uiStringLength);
//    printf("max tree depth: %"PRIuMAX"\n", (luint) spState->uiMaxTreeDepth);
//    printf("     hit count: %"PRIuMAX"\n", (luint) spState->uiHitCount);
//}

static aint uiCountDigits(aint uiValue){
    aint uiReturn = 1;
    uiValue = uiValue/10;
    while(uiValue){
        uiReturn++;
        uiValue = uiValue/10;
    }
    return uiReturn;
}
static void vDisplayXml(xml* spXml, abool bShowLines, void* vpVecChars){
    const char* cpNext;
    uint8_t* ucpChars = (uint8_t*)vpVecFirst(vpVecChars);
    while(APG_TRUE){
        static char* cpFmt1 = "%d: %s";
        static char* cpFmt2 = "%02d: %s";
        static char* cpFmt3 = "%03d: %s";
        static char* cpFmt4 = "%04d: %s";
        static char* cpAlt1 = " : %s";
        static char* cpAlt2 = "  : %s";;
        static char* cpAlt3 = "   : %s";;
        static char* cpAlt4 = "    : %s";;
        char* cpFmt, *cpAlt;
        printf("   true type: %s\n", cpUtilUtfTypeName(spXml->uiTrueType));
        printf("display type: %s\n", cpUtilUtfTypeName(UTF_8));
        if(!ucpChars){
            printf("00000000 no data\n");
            break;
        }
        if(bShowLines){
            line* spLine;
            int iLineNo, iPartial;
            if(!uiLinesCount(spXml->vpLines)){
                printf("00000000 no lines\n");
                break;
            }
            // count the lines
            spLine = spLinesFirst(spXml->vpLines);
            iLineNo = 0;
            while(spLine) {
                spLine = spLinesNext(spXml->vpLines);
                iLineNo++;
            }
            // select the format
            aint uiDigits = uiCountDigits((aint)iLineNo);
            switch(uiDigits){
            case 1:
                cpFmt = cpFmt1;
                cpAlt = cpAlt1;
                break;
            case 2:
                cpFmt = cpFmt2;
                cpAlt = cpAlt2;
                break;
            case 3:
                cpFmt = cpFmt3;
                cpAlt = cpAlt3;
                break;
            default:
                cpFmt = cpFmt4;
                cpAlt = cpAlt4;
                break;
            }

            spLine = spLinesFirst(spXml->vpLines);
            iLineNo = 0;
            while(spLine) {
                cpNext = cpFmtFirstBytes(spXml->vpFmt, &ucpChars[spLine->uiCharIndex], spLine->uiLineLength,
                        FMT_CANONICAL, 0, 0);
                iPartial = 0;
                while(cpNext){
                    if(iPartial == 0){
                        printf(cpFmt, iLineNo, cpNext);
                    }else{
                        printf(cpAlt, cpNext);
                    }
                    cpNext = cpFmtNext(spXml->vpFmt);
                    iPartial++;
                }
                spLine = spLinesNext(spXml->vpLines);
                iLineNo++;
            }
        }else{
            cpNext = cpFmtFirstBytes(spXml->vpFmt, ucpChars, uiVecLen(vpVecChars), FMT_CANONICAL, 0, 0);
            while(cpNext){
                printf("%s", cpNext);
                cpNext = cpFmtNext(spXml->vpFmt);
            }
        }

        // success
        break;
    }
}
static abool bIsUtf8(uint8_t* ucpData){
    abool bReturn = APG_TRUE;
    while(APG_TRUE){
        if(ucpData[0] == 0x3C &&
                ucpData[1] == 0x3F &&
                ucpData[1] == 0x78 &&
                ucpData[1] == 0x6D &&
                ucpData[1] == 0x6C){
            // begins with UTF-8 declaration "<?xml" (no white space allowed)
            break;
        }
        if((ucpData[0] == 0x3C) && (ucpData[1] != 0)){
            // begins with tag character "<"
            break;
        }
        if((ucpData[0] == 0x20) && (ucpData[1] != 0)){
            // begins with UTF-8 white space (OK when no declaration is present)
            break;
        }
        if((ucpData[0] == 0x09) && (ucpData[1] != 0)){
            // begins with UTF-8 white space (OK when no declaration is present)
            break;
        }
        if((ucpData[0] == 0x0A) && (ucpData[1] != 0)){
            // begins with UTF-8 white space (OK when no declaration is present)
            break;
        }
        if((ucpData[0] == 0x0D) && (ucpData[1] != 0)){
            // begins with UTF-8 white space (OK when no declaration is present)
            break;
        }
        bReturn = APG_FALSE;
        break;
    }
    return bReturn;
}
static abool bIsUtf16be(uint8_t* ucpData){
    abool bReturn = APG_TRUE;
    while(APG_TRUE){
        if(ucpData[0] == 0x00 && ucpData[1] == 0x3C &&
                ucpData[2] == 0x00 && ucpData[3] == 0x3F &&
                ucpData[4] == 0x00 && ucpData[5] == 0x78 &&
                ucpData[6] == 0x00 && ucpData[7] == 0x6D &&
                ucpData[8] == 0x00 && ucpData[9] == 0x6C){
            // begins with UTF-16BE declaration "<?xml" (no white space allowed)
            break;
        }
        if(ucpData[0] == 0x00 && ucpData[1] == 0x3C){
            // begins with UTF-16BE tag character "<"
            break;
        }
        if(ucpData[0] == 0x00 && ucpData[1] == 0x20){
            // begins with UTF-16BE white space (OK when no declaration)
            break;
        }
        if(ucpData[0] == 0x00 && ucpData[1] == 0x09){
            // begins with UTF-16BE white space (OK when no declaration)
            break;
        }
        if(ucpData[0] == 0x00 && ucpData[1] == 0x0A){
            // begins with UTF-16BE white space (OK when no declaration)
            break;
        }
        if(ucpData[0] == 0x00 && ucpData[1] == 0x0D){
            // begins with UTF-16BE white space (OK when no declaration)
            break;
        }
        bReturn = APG_FALSE;
        break;
    }
    return bReturn;
}
static abool bIsUtf16le(uint8_t* ucpData){
    abool bReturn = APG_TRUE;
    while(APG_TRUE){
        if(ucpData[0] == 0x3C && ucpData[1] == 0x00 &&
                ucpData[2] == 0x3F && ucpData[3] == 0x00 &&
                ucpData[4] == 0x78 && ucpData[5] == 0x00 &&
                ucpData[6] == 0x6D && ucpData[7] == 0x00 &&
                ucpData[8] == 0x6C && ucpData[9] == 0x00){
            // begins with UTF-16LE declaration "<?xml" (no white space allowed)
            break;
        }
        if(ucpData[0] == 0x3C && ucpData[1] == 0x00){
            // begins with UTF-16LE tag character "<"
            break;
        }
        if(ucpData[0] == 0x20 && ucpData[1] == 0x00){
            // begins with UTF-16LE white space (OK when no declaration)
            break;
        }
        if(ucpData[0] == 0x09 && ucpData[1] == 0x00){
            // begins with UTF-16LE white space (OK when no declaration)
            break;
        }
        if(ucpData[0] == 0x0A && ucpData[1] == 0x00){
            // begins with UTF-16LE white space (OK when no declaration)
            break;
        }
        if(ucpData[0] == 0x0D && ucpData[1] == 0x00){
            // begins with UTF-16LE white space (OK when no declaration)
            break;
        }
        bReturn = APG_FALSE;
        break;
    }
    return bReturn;
}
static abool bUtfType(uint8_t* ucpData, aint* uipStartByte, aint* uipTrueType, input_info* spInfo){
    spInfo->bValid = APG_FALSE;
    *uipStartByte = 0;
    *uipTrueType = BINARY;
    while(APG_TRUE){
        if(ucpData[0] == 0xEF && ucpData[1] == 0xBB && ucpData[2] == 0xBF){
            // UTF-8 with BOM
            *uipStartByte = 3;
            *uipTrueType = UTF_8;
            spInfo->uiType = UTF_8;
            spInfo->bBom = APG_TRUE;
            spInfo->bValid = bIsUtf8(&ucpData[3]);
            break;
        }
        if((ucpData[0] == 0xFE && ucpData[1] == 0xFF) && !(ucpData[2] == 0 && ucpData[3] == 0)){
            // UTF-16BE with BOM
            *uipStartByte = 2;
            *uipTrueType = UTF_16BE;
            spInfo->uiType = UTF_16BE;
            spInfo->bBom = APG_TRUE;
            spInfo->bValid = bIsUtf16be(&ucpData[2]);
            break;
        }
        if((ucpData[0] == 0xFF && ucpData[1] == 0xFE) && !(ucpData[2] == 0 && ucpData[3] == 0)){
            // UTF-16LE with BOM
            *uipStartByte = 2;
            *uipTrueType = UTF_16LE;
            spInfo->uiType = UTF_16LE;
            spInfo->bBom = APG_TRUE;
            spInfo->bValid = bIsUtf16le(&ucpData[2]);
            break;
        }
        // test for encodings without BOM
        // Note: must begin with "<?xml" or
        //       first byte must be '<' or white space (allowed if no XML declaration is present)
        if(bIsUtf8(ucpData)){
            // UTF-8 without BOM
            *uipTrueType = UTF_8;
            spInfo->uiType = UTF_8;
            spInfo->bBom = APG_FALSE;
            spInfo->bValid = APG_TRUE;
            break;
        }
        if(bIsUtf16be(ucpData)){
            // UTF-16BE without BOM
            *uipTrueType = UTF_16BE;
            spInfo->uiType = UTF_16BE;
            spInfo->bBom = APG_FALSE;
            spInfo->bValid = APG_FALSE;
            break;
        }
        if(bIsUtf16le(ucpData)){
            // UTF-16LE without BOM
            *uipTrueType = UTF_16LE;
            spInfo->uiType = UTF_16LE;
            spInfo->bBom = APG_FALSE;
            spInfo->bValid = APG_FALSE;
            break;
        }

        // unrecognized type
        spInfo->uiType = UTF_UNKNOWN;
        spInfo->bBom = APG_FALSE;
        spInfo->bValid = APG_FALSE;
        break;
    }
    return spInfo->bValid;
}

