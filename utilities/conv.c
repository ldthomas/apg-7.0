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
/** \file conv.c
 * \brief A Unicode encoding/decoding object.
 *
 * This object provides functions for encoding and decoding data represented in the UTF-8, UTF-16 and UTF-32
 * [Unicode formats](https://www.unicode.org/versions/Unicode9.0.0/ch03.pdf#G7404)
 * as well as the [ISO 8859-1](https://en.wikipedia.org/wiki/ISO%2FIEC_8859-1) format.
 * Care has been taken to observe most (hopefully all) of the Unicode restrictions.
 * Decoding or encoding of values outside the Unicode range is forbidden.
 * That is characters 0xD800 to 0xDFFF and characters greater than 0x10FFFF are forbidden.
 * Over long UTF-8 encodings are also forbidden.
 * For example, encoding 0x7FF in three bytes instead of two is forbidden.
 *
 *
 * Conversions are done in a two-step process.
 * A source data byte stream is first decoded into an array of 32-bit Unicode code points.
 * This persistent array is then encoded into the destination data byte stream.
 * Functions are available to access the intermediate
 * 32-bit code point data and to insert 32-bit code points directly.
 *
 * Source and destination byte streams may be base64 encoded.
 */
#include "../library/lib.h"
#include "./conv.h"

/** @name Private Macros &ndash; used only by the conv object */
///@{
#define BASE64_LINE_LEN 76
#define TAIL_CHAR   61
#define NON_BYTE_MASK 0xFFFFFF00
#define BYTE_MASK 0xFF
///@}

static const void* s_vpMagicNumber = (void*)"conv";
static uint8_t s_caBOM8[] = {0xEF, 0xBB, 0xBF};
static uint8_t s_caBOM16BE[] = {0xFE, 0xFF};
static uint8_t s_caBOM16LE[] = {0xFF, 0xFE};
static uint8_t s_caBOM32BE[] = {0, 0, 0xFE, 0xFF};
static uint8_t s_caBOM32LE[] = {0xFF, 0xFE, 0, 0};
static uint8_t s_ucaCRLF[] = {13,10};

/** \struct conv_error
 * \brief Defines the value, location and error message when a value is in error and a fatal error is issued.
 */
typedef struct {
    uint32_t uiValue; ///< \brief The value that is in error.
    uint32_t uiOffset; ///< \brief Offset to the value that is in error.
    const char* cpMsg; ///< \brief Pointer to the error message.
    abool bHasError; ///< \brief True if the data represents a valid error. False if no error has been detected.
} conv_error;

/** \struct conv
 * \brief The conv object context.
 */
typedef struct {
    const void* vpValidate; ///< \brief Must be the "magic number" to be a valid context.
    exception* spException; ///< \brief Pointer to the exception structure
                            /// for reporting errors to the application catch block.
    void* vpMem; ///< \brief Pointer to a memory object used for all memory allocations.
    void* vpVecInput; ///< \brief Pointer to a vector which holds a copy of the input byte stream.
    void* vpVecOutput; ///< \brief Pointer to a vector which holds the output byte stream.
    void* vpVec32bit; ///< \brief Pointer to a vector which holds the 32-bit decoded data.
    aint uiTail; ///< \brief Used in for base64.
    aint uiBase64LineLen; ///< \brief Used in for base64.
    aint uiBase64LineEnd; ///< \brief Used in for base64.
    conv_error sError;
} conv;

static abool bIsBOM8(uint8_t* ucpStream, aint uiLen);
static abool bIsBOM16BE(uint8_t* ucpStream, aint uiLen);
static abool bIsBOM16LE(uint8_t* ucpStream, aint uiLen);
static abool bIsBOM32BE(uint8_t* ucpStream, aint uiLen);
static abool bIsBOM32LE(uint8_t* ucpStream, aint uiLen);
static void vBase64Encode(conv* spConv, conv_dst* spDst);
static void vBase64Decode(conv* spConv, uint8_t* ucpSrc, aint uiSrcLen);
static void vBase64Validate(conv* spConv, uint8_t* ucpSrc, aint uiSrcLen);
static void vBinaryEncode(conv* spConv);
static void vBinaryDecode(conv* spConv);
static void vUtf32BEDecode(conv* spConv, uint8_t* ucpData, aint uiDataLen);
static void vUtf32LEDecode(conv* spConv, uint8_t* ucpData, aint uiDataLen);
static void vUtf32BEEncode(conv* spConv, conv_dst* spDst);
static void vUtf32LEEncode(conv* spConv, conv_dst* spDst);
static void vUtf16BEDecode(conv* spConv, uint8_t* ucpData, aint uiDataLen);
static void vUtf16LEDecode(conv* spConv, uint8_t* ucpData, aint uiDataLen);
static void vUtf16BEEncode(conv* spConv, conv_dst* spDst);
static void vUtf16LEEncode(conv* spConv, conv_dst* spDst);
static void vUtf8Encode(conv* spConv, conv_dst* spDst);
static void vUtf8Decode(conv* spConv, uint8_t* ucpData, aint uiDataLen);
static void vSetError(conv* spConv, uint32_t uiValue, uint32_t uiOffset, const char* cpMsg);

static uint8_t ucaBase64Chars[] = {
        65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,
        97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,
        48,49,50,51,52,53,54,55,56,57,
        43,47,61
};
static uint32_t uiEncode64Mask = 0x0000003F;
static uint32_t uiDecode64Mask = 0x000000FF;

/** \brief The data conversion object constructor.
 * \param spEx Pointer to a valid exception structure initialized with \ref XCTOR().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Pointer to the object context on success.
 * An exception is thrown on failure, which can only be a memory allocation failure.
 */
void* vpConvCtor(exception* spEx){
    conv* spConv = NULL;
    if(bExValidate(spEx)){
        void* vpMem = vpMemCtor(spEx);
        spConv = (conv*)vpMemAlloc(vpMem, sizeof(conv));
        memset(spConv, 0, sizeof(conv));
        spConv->vpMem = vpMem;
        spConv->spException = spEx;
        aint uiBufSize = 128 * 1024;
        spConv->vpVecInput = (uint8_t*)vpVecCtor(vpMem, sizeof(uint8_t), sizeof(uint8_t) * uiBufSize);
        spConv->vpVecOutput = (uint8_t*)vpVecCtor(vpMem, sizeof(uint8_t), sizeof(uint8_t) * uiBufSize);
        spConv->vpVec32bit = (uint32_t*)vpVecCtor(vpMem, sizeof(uint32_t), sizeof(uint32_t) * uiBufSize);
        spConv->uiBase64LineLen = BASE64_LINE_LEN;
        spConv->uiBase64LineEnd = BASE64_LF;
        spConv->vpValidate = s_vpMagicNumber;
        return (void*)spConv;
    }
    vExContext();
    return NULL;
}

/** \brief Conversion object destructor.
 * \param vpCtx A pointer to a valid context previously return from \ref vpConvCtor().
 * Silently ignored if NULL.
 * However, if non-NULL it must be a valid context pointer.
 * The application will silently exit with \ref BAD_CONTEXT exit code if vpCtx is invalid.
 */
void vConvDtor(void* vpCtx){
    conv* spCtx = (conv*)vpCtx;
    if(vpCtx){
        if(spCtx->vpValidate == s_vpMagicNumber){
            void* vpMem = spCtx->vpMem;
            memset(vpCtx, 0, sizeof(conv));
            vMemDtor(vpMem);
        }else{
            vExContext();
        }
    }
}

/** \brief Configures base64 output format.
 *
 * The default base64 output has LF (\\n) line breaks at 76 characters.
 * This function will modify that output format if called prior to \ref vConvEncode().
 * \param vpCtx A pointer to a valid context previously return from \ref vpConvCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param uiLineLen Break output lines at every uiLineLen characters. If 0 (zero), no line breaks will be added.
 * \param uiLineEnd Must be one of:
 *  - \ref BASE64_LF for \\n line ends
 *  - \ref BASE64_CRLF for \\r\\n line breaks.
 *
 */
void vConvConfigureBase64(void* vpCtx, aint uiLineLen, aint uiLineEnd){
    conv* spConv = (conv*)vpCtx;
    if(spConv && spConv->vpValidate == s_vpMagicNumber){
        if(uiLineEnd == BASE64_LF || uiLineEnd == BASE64_CRLF){
            spConv->uiBase64LineEnd = uiLineEnd;
        }else{
            XTHROW(spConv->spException, "uiLineEnd must be one of BASE64_LF or BASE64_CRLF");
        }
        spConv->uiBase64LineLen = uiLineLen;
    }else{
        vExContext();
    }
}

/** \brief Decode a source byte stream to 32-bit Unicode code points.
 * \param vpCtx A pointer to a valid context previously return from \ref vpConvCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spSrc Pointer to the source byte stream definition.
 */
void vConvDecode(void* vpCtx, conv_src* spSrc){
    conv* spConv = (conv*)vpCtx;
    if(spConv && spConv->vpValidate == s_vpMagicNumber){
        vVecClear(spConv->vpVecInput);
        vVecClear(spConv->vpVecOutput);
        vVecClear(spConv->vpVec32bit);
        spConv->sError.bHasError = APG_FALSE;
        uint8_t* ucpData;
        aint uiDataLen;
        if(!spSrc || !spSrc->ucpData || !spSrc->uiDataLen){
            XTHROW(spConv->spException, "source cannot be NULL or empty");
            return;
        }
        if(spSrc->uiDataType & BASE64_MASK){
            // preprocess with base64 conversion
            vBase64Decode(spConv, spSrc->ucpData, spSrc->uiDataLen);
        }else{
            // make a permanent copy of user's input data
            vpVecPushn(spConv->vpVecInput, spSrc->ucpData, spSrc->uiDataLen);
        }
        ucpData = (uint8_t*)vpVecFirst(spConv->vpVecInput);
        uiDataLen = uiVecLen(spConv->vpVecInput);
        if(!ucpData || !uiDataLen){
            XTHROW(spConv->spException, "internal error processing input");
            return;
        }
        switch(spSrc->uiDataType & TYPE_MASK){
        case BINARY:
            vBinaryDecode(spConv);
            break;
        case UTF_8:
            if(bIsBOM8(ucpData, uiDataLen)){
                vUtf8Decode(spConv, (ucpData + 3), (uiDataLen - 3));
            }else{
                vUtf8Decode(spConv, ucpData, uiDataLen);
            }
            break;
        case UTF_16:
            if(bIsBOM16BE(ucpData, uiDataLen)){
                vUtf16BEDecode(spConv, (ucpData + 2), (uiDataLen - 2));
            }else if(bIsBOM16LE(ucpData, uiDataLen)){
                vUtf16LEDecode(spConv, (ucpData + 2), (uiDataLen - 2));
            }else{
                vUtf16BEDecode(spConv, ucpData, uiDataLen);
            }
            break;
        case UTF_16BE:
            if(bIsBOM16BE(ucpData, uiDataLen)){
                vUtf16BEDecode(spConv, (ucpData + 2), (uiDataLen - 2));
            }else{
                vUtf16BEDecode(spConv, ucpData, uiDataLen);
            }
            break;
        case UTF_16LE:
            if(bIsBOM16LE(ucpData, uiDataLen)){
                vUtf16LEDecode(spConv, (ucpData + 2), (uiDataLen - 2));
            }else{
                vUtf16LEDecode(spConv, ucpData, uiDataLen);
            }
            break;
        case UTF_32:
            if(bIsBOM32BE(ucpData, uiDataLen)){
                vUtf32BEDecode(spConv, (ucpData + 4), (uiDataLen - 4));
            }else if(bIsBOM32LE(ucpData, uiDataLen)){
                vUtf32LEDecode(spConv, (ucpData + 4), (uiDataLen - 4));
            }else{
                vUtf32BEDecode(spConv, ucpData, uiDataLen);
            }
            break;
        case UTF_32BE:
            if(bIsBOM32BE(ucpData, uiDataLen)){
                vUtf32BEDecode(spConv, (ucpData + 4), (uiDataLen - 4));
            }else{
                vUtf32BEDecode(spConv, ucpData, uiDataLen);
            }
            break;
        case UTF_32LE:
            if(bIsBOM32LE(ucpData, uiDataLen)){
                vUtf32LEDecode(spConv, (ucpData + 4), (uiDataLen - 4));
            }else{
                vUtf32LEDecode(spConv, ucpData, uiDataLen);
            }
            break;
        default:
            XTHROW(spConv->spException, "unrecognized encoding type");
            break;
        }
    }else{
        vExContext();
    }
}
/** \brief Encode the 32-bit Unicode code points to a byte stream.
 * \param vpCtx A pointer to a valid context previously return from \ref vpConvCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spDst Pointer to the destination byte stream definition.
 * Byte stream data remains valid until another call on the context handle vpCtx.
 */
void vConvEncode(void* vpCtx, conv_dst* spDst){
    conv* spConv = (conv*)vpCtx;
    if(spConv && spConv->vpValidate == s_vpMagicNumber){
        vVecClear(spConv->vpVecInput);
        vVecClear(spConv->vpVecOutput);
        spConv->sError.bHasError = APG_FALSE;
        if(!uiVecLen(spConv->vpVec32bit)){
            XTHROW(spConv->spException, "no 32-bit data to encode");
            return;
        }
        switch(spDst->uiDataType & TYPE_MASK){
        case BINARY:
            vBinaryEncode(spConv);
            break;
        case UTF_8:
            vUtf8Encode(spConv, spDst);
            break;
        case UTF_16:
        case UTF_16BE:
            vUtf16BEEncode(spConv, spDst);
            break;
        case UTF_16LE:
            vUtf16LEEncode(spConv, spDst);
            break;
        case UTF_32:
        case UTF_32BE:
            vUtf32BEEncode(spConv, spDst);
            break;
        case UTF_32LE:
            vUtf32LEEncode(spConv, spDst);
            break;
        default:
            XTHROW(spConv->spException, "unrecognized encoding type");
            break;
        }
        if(spDst->uiDataType & BASE64_MASK){
            // final base64 conversion of translated byte stream
            vBase64Encode(spConv, spDst);
        }
        spDst->ucpData = (uint8_t*)vpVecFirst(spConv->vpVecOutput);
        spDst->uiDataLen = uiVecLen(spConv->vpVecOutput);
    }else{
        vExContext();
    }
}
/** \brief Access the intermediate 32-bit data following a call to \ref vConvDecode() or \ref vConvUseCodePoints().
 *
 * Copies the code points into the caller's array.
 *  - if uipData == NULL, the number of code points is returned in *uipDataLen
 *  - if uipData != NULL, *uipDataLen is taken as the array length and up to *uipDataLen code points are copied to uipData
 *    - on return if *uipDataLen > than its original value, not all data was copied. Try again with a larger array buffer.
 *    - on return if *uipDataLen <= than its original value all data was copied

 * \param vpCtx A pointer to a valid context previously return from \ref vpConvCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param uipData Pointer to the array to receive the data.
 * If NULL no data is returned but the number of code points is set.
 * \param uipDataLen Pointer to an unsigned integer to receive the number of code points.
 */
void vConvGetCodePoints(void* vpCtx, uint32_t* uipData, uint32_t* uipDataLen){
    conv* spConv = (conv*)vpCtx;
    if(spConv && spConv->vpValidate == s_vpMagicNumber){
        if(!uipDataLen){
            XTHROW(spConv->spException, "data length pointer cannot be NULL");
            return;
        }
        uint32_t uiOriginalLen = *uipDataLen;
        *uipDataLen = uiVecLen(spConv->vpVec32bit);
        uint32_t* uip32 = (uint32_t*)vpVecFirst(spConv->vpVec32bit);
        if(!*uipDataLen || !uip32){
            XTHROW(spConv->spException, "no 32-bit data to copy");
            return;
        }
        if(uipData && uiOriginalLen){
            aint ui = 0;
            aint uiLen = *uipDataLen >= uiOriginalLen ? uiOriginalLen : *uipDataLen;
            for(; ui < uiLen; ui++){
                uipData[ui] = uip32[ui];
            }
        }
    }else{
        vExContext();
    }
}

/** \brief Insert a stream of 32-bit Unicode code points as the intermediate data.
 *
 * This data provided will be encoded with a call to \ref vConvEncode().
 * \param vpCtx A pointer to a valid context previously return from \ref vpConvCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param uipSrc- pointer to an array of 32-bit Unicode code points.
 * \param uiSrcLen - the number of code points.
 */
void vConvUseCodePoints(void* vpCtx, uint32_t* uipSrc, aint uiSrcLen){
    conv* spConv = (conv*)vpCtx;
    if(spConv && spConv->vpValidate == s_vpMagicNumber){
        aint ui;
        uint32_t* uip32bit;
        if(!uipSrc || !uiSrcLen){
            XTHROW(spConv->spException, "source cannot be NULL or empty");
            return;
        }
        vVecClear(spConv->vpVec32bit);
        uip32bit = (uint32_t*)vpVecPushn(spConv->vpVec32bit, NULL, uiSrcLen);
        for(ui = 0; ui < uiSrcLen; ui++){
            uip32bit[ui] = uipSrc[ui];
        }
    }else{
        vExContext();
    }
}

/** \brief Decodes and encodes in a single functions call.
 *
 * Equivalent to calling \ref vConvDecode() and \ref vConvEncode() in succession.
 * \param vpCtx A pointer to a valid context previously return from \ref vpConvCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spSrc Pointer to the source byte stream definition.
 * \param[in,out] spDst Pointer to the destination byte stream definition.
 */
void vConvConvert(void* vpCtx, conv_src* spSrc, conv_dst* spDst){
    conv* spConv = (conv*)vpCtx;
    if(spConv && spConv->vpValidate == s_vpMagicNumber){
        if(!spSrc || !spDst){
            XTHROW(spConv->spException, "source and destination must be non-NULL");
            return;
        }
        vConvDecode(vpCtx, spSrc);
        vConvEncode(vpCtx, spDst);
    }else{
        vExContext();
    }
}

static void vBase64Encode(conv* spConv, conv_dst* spDst){
    aint uiTail, uiUnits, u3, u4, uu;
    uint8_t* ucpTrans = NULL;
    uint32_t ui32;
    uint8_t* ucpSrc;
    aint uiSrcLen;
    ucpSrc = (uint8_t*)vpVecFirst(spConv->vpVecOutput);
    uiSrcLen = uiVecLen(spConv->vpVecOutput);
    if(!ucpSrc){
        XTHROW(spConv->spException, "internal error - vBase64Encode called with no source");
    }
    uiUnits = uiSrcLen / 3;
    uiTail = 3 - uiSrcLen % 3;
    if(uiTail == 3){
        ucpTrans = (uint8_t*)vpMemAlloc(spConv->vpMem, (aint)sizeof(uint32_t) * uiUnits * 4);
    }else{
        ucpTrans = (uint8_t*)vpMemAlloc(spConv->vpMem, (aint)sizeof(uint32_t) * (uiUnits + 1) * 4);
    }
    u3 = 0;
    u4 = 0;
    uu = 0;
    for(; uu < uiUnits; uu++){
        ui32 = ucpSrc[u3++] << 16;
        ui32 += ucpSrc[u3++] << 8;
        ui32 += ucpSrc[u3++];
        ucpTrans[u4++] = ucaBase64Chars[(ui32 >> 18) & uiEncode64Mask];
        ucpTrans[u4++] = ucaBase64Chars[(ui32 >> 12) & uiEncode64Mask];
        ucpTrans[u4++] = ucaBase64Chars[(ui32 >> 6) & uiEncode64Mask];
        ucpTrans[u4++] = ucaBase64Chars[ui32 & uiEncode64Mask];
    }
    if(uiTail == 1){
        ui32 = ucpSrc[u3++] << 16;
        ui32 += ucpSrc[u3] << 8;
        ucpTrans[u4++] = ucaBase64Chars[(ui32 >> 18) & uiEncode64Mask];
        ucpTrans[u4++] = ucaBase64Chars[(ui32 >> 12) & uiEncode64Mask];
        ucpTrans[u4++] = ucaBase64Chars[(ui32 >> 6) & uiEncode64Mask];
        ucpTrans[u4++] = ucaBase64Chars[64];
    }else if(uiTail == 2){
        ui32 = ucpSrc[u3] << 16;
        ucpTrans[u4++] = ucaBase64Chars[(ui32 >> 18) & uiEncode64Mask];
        ucpTrans[u4++] = ucaBase64Chars[(ui32 >> 12) & uiEncode64Mask];
        ucpTrans[u4++] = ucaBase64Chars[64];
        ucpTrans[u4++] = ucaBase64Chars[64];
    }
    vVecClear(spConv->vpVecOutput);
    if(spConv->uiBase64LineLen){
        aint ui = 0;
//        uint8_t ucaCRLF[] = {13,10};
        for(; ui < u4; ui++){
            if((ui % spConv->uiBase64LineLen == 0) && ui){
                if(spConv->uiBase64LineEnd == BASE64_LF){
                    vpVecPush(spConv->vpVecOutput, &s_ucaCRLF[1]);
                }else{
                    vpVecPushn(spConv->vpVecOutput, s_ucaCRLF, 2);
                }
            }
            vpVecPush(spConv->vpVecOutput, &ucpTrans[ui]);
        }
        if(spConv->uiBase64LineEnd == BASE64_LF){
            vpVecPush(spConv->vpVecOutput, &s_ucaCRLF[1]);
        }else{
            vpVecPushn(spConv->vpVecOutput, s_ucaCRLF, 2);
        }
    }else{
        vpVecPushn(spConv->vpVecOutput, ucpTrans, u4);
    }
    vMemFree(spConv->vpMem, ucpTrans);
}
static void vBase64Validate(conv* spConv, uint8_t* ucpSrc, aint uiSrcLen){
    uint8_t* ucpValues = NULL;
    spConv->uiTail = 0;
    uint8_t ucChar;
    uint8_t* ucpChar = ucpSrc;
    uint8_t* ucpEnd = ucpSrc + uiSrcLen - spConv->uiTail;
    ucpValues = (uint8_t*)vpMemAlloc(spConv->vpMem, (uiSrcLen + 32));
    aint uiValuesLen = 0;
    aint uiOffset = 0;
    for(; ucpChar < ucpEnd; ucpChar++, uiOffset++){
        ucChar = *ucpChar;
        while(APG_TRUE){
            if(ucChar == 10 || ucChar == 13 || ucChar == 9 || ucChar == 32){
                // ignore white space
                break;
            }
            if(ucChar >= 65 && ucChar <= 90){
                ucpValues[uiValuesLen++] = ucChar - 65;
                break;
            }
            if(ucChar >= 97 && ucChar <= 122){
                ucpValues[uiValuesLen++] = ucChar - 71;
                break;
            }
            if(ucChar >= 48 && ucChar <= 57){
                ucpValues[uiValuesLen++] = ucChar + 4;
                break;
            }
            if(ucChar == 43){
                ucpValues[uiValuesLen++] = 62;
                break;
            }
            if(ucChar == 47){
                ucpValues[uiValuesLen++] = 63;
                break;
            }
            if(ucChar == TAIL_CHAR){
                spConv->uiTail++;
                ucpValues[uiValuesLen++] = 64;
                break;
            }
            vSetError(spConv, (uint32_t)ucChar, (uint32_t)uiOffset, "invalid base64 character");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            return;
        }
    }
    if(spConv->uiTail > 2){
        XTHROW(spConv->spException, "too many base64 tail characters");
    }
    if(spConv->uiTail == 2){
        if(!(ucpValues[uiValuesLen - 1] == 64 && ucpValues[uiValuesLen - 2] == 64)){
            XTHROW(spConv->spException, "bad base64 tail characters");
        }
    }else if(spConv->uiTail == 1){
        if(ucpValues[uiValuesLen - 1] != 64){
            XTHROW(spConv->spException, "bad base64 tail characters");
        }
    }
    if((uiValuesLen % 4) != 0){
        XTHROW(spConv->spException, "number of base 64 characters not multiple of 4");
    }
    vpVecPushn(spConv->vpVecInput, ucpValues, uiValuesLen);
    vMemFree(spConv->vpMem, ucpValues);
}
static void vBase64Decode(conv* spConv, uint8_t* ucpSrc, aint uiSrcLen){
    uint8_t* ucpOut = NULL;
    vBase64Validate(spConv, ucpSrc, uiSrcLen);
    uint8_t* ucpBase = (uint8_t*)vpVecFirst(spConv->vpVecInput);
    aint uiBaseLen = uiVecLen(spConv->vpVecInput);
    aint uiUnits = uiBaseLen / 4;
    ucpOut = (uint8_t*)vpMemAlloc(spConv->vpMem, (uiUnits * 3));
    if(spConv->uiTail){
        uiUnits--;
    }
    aint u3 = 0;
    aint u4 = 0;
    aint uu = 0;
    uint32_t ui32bits = 0;
    for(; uu < uiUnits; uu++){
        ui32bits = ((uint32_t)ucpBase[u4++]) << 18;
        ui32bits += ((uint32_t)ucpBase[u4++]) << 12;
        ui32bits += ((uint32_t)ucpBase[u4++]) << 6;
        ui32bits += (uint32_t)ucpBase[u4++];
        ucpOut[u3++] = (uint8_t)((ui32bits >> 16) & uiDecode64Mask);
        ucpOut[u3++] = (uint8_t)((ui32bits >> 8) & uiDecode64Mask);
        ucpOut[u3++] = (uint8_t)(ui32bits & uiDecode64Mask);
    }
    if(spConv->uiTail == 1){
        ui32bits = ((uint32_t)ucpBase[u4++]) << 18;
        ui32bits += ((uint32_t)ucpBase[u4++]) << 12;
        ui32bits += ((uint32_t)ucpBase[u4++]) << 6;
        ucpOut[u3++] = (uint8_t)((ui32bits >> 16) & uiDecode64Mask);
        ucpOut[u3++] = (uint8_t)((ui32bits >> 8) & uiDecode64Mask);
    }else if(spConv->uiTail == 2){
        ui32bits = ((uint32_t)ucpBase[u4++]) << 18;
        ui32bits += ((uint32_t)ucpBase[u4++]) << 12;
        ucpOut[u3++] = (uint8_t)((ui32bits >> 16) & uiDecode64Mask);
    }
    vVecClear(spConv->vpVecInput);
    vpVecPushn(spConv->vpVecInput, ucpOut, u3);
    vMemFree(spConv->vpMem, ucpOut);
}

static void vBinaryDecode(conv* spConv){
    uint32_t* uip32;
    uint32_t* uipEnd;
    uint8_t* ucpIn;
    aint uiLen;
    ucpIn = (uint8_t*)vpVecFirst(spConv->vpVecInput);
    uiLen = uiVecLen(spConv->vpVecInput);
    if(!ucpIn || !uiLen){
        XTHROW(spConv->spException, "internal error - function called without necessary data");
        return;
    }
    uip32 = (uint32_t*)vpVecPushn(spConv->vpVec32bit, NULL, uiLen);
    uipEnd = uip32 + uiLen;
    while(uip32 < uipEnd){
        *uip32++ = (uint32_t)*ucpIn++;
    }
}
static void vBinaryEncode(conv* spConv){
    uint32_t* uip32;
    uint32_t* uipEnd;
    uint8_t* ucpOut;
    aint uiLen;
    uip32 = (uint32_t*)vpVecFirst(spConv->vpVec32bit);
    uiLen = uiVecLen(spConv->vpVec32bit);
    if(!uip32 || !uiLen){
        XTHROW(spConv->spException, "internal error - function called without necessary data");
        return;
    }
    ucpOut = (uint8_t*)vpVecPushn(spConv->vpVecOutput, NULL, uiLen);
    uipEnd = uip32 + uiLen;
    uint32_t uiOffset = 0;
    while(uip32 < uipEnd){
        if((*uip32 & NON_BYTE_MASK) != 0){
            vSetError(spConv, *uip32, uiOffset, "can't binary encode values > 0xFF");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            return;
        }
        uiOffset++;
        *ucpOut++ = (uint8_t)*uip32++;
    }
}

static void vUtf32BEDecode(conv* spConv, uint8_t* ucpData, aint uiDataLen){
    uint32_t uc = 0;
    uint32_t uiWord;
    if(!ucpData || !uiDataLen){
        XTHROW(spConv->spException, "internal error - function called without necessary data");
        return;
    }
    if(uiDataLen %4 != 0){
        XTHROW(spConv->spException, "UTF-32BE data cannot have an odd number of bytes");
        return;
    }
    while(uc < uiDataLen){
        uiWord  = ((uint32_t)ucpData[uc++]) << 24;
        uiWord += ((uint32_t)ucpData[uc++]) << 16;
        uiWord += ((uint32_t)ucpData[uc++]) << 8;
        uiWord += (uint32_t)ucpData[uc++];
        if(uiWord >= 0xd800 && uiWord < 0xe000){
            vSetError(spConv, uiWord, uc, "UTF-32BE value in surrogate pair range");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            break;
        }
        if(uiWord > 0x10ffff){
            vSetError(spConv, uiWord, uc, "UTF-32BE value out of range (> 0x10FFFF)");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            break;
        }
        vpVecPush(spConv->vpVec32bit, &uiWord);
    }
}
static void vUtf32LEDecode(conv* spConv, uint8_t* ucpData, aint uiDataLen){
    uint32_t uc = 0;
    uint32_t uiWord;
    if(!ucpData || !uiDataLen){
        XTHROW(spConv->spException, "internal error - function called without necessary data");
        return;
    }
    if(uiDataLen %4 != 0){
        XTHROW(spConv->spException, "UTF-32LE data cannot have odd number of bytes");
        return;
    }
    while(uc < uiDataLen){
        uiWord  = (uint32_t)ucpData[uc++];
        uiWord += ((uint32_t)ucpData[uc++]) << 8;
        uiWord += ((uint32_t)ucpData[uc++]) << 16;
        uiWord += ((uint32_t)ucpData[uc++]) << 24;
        if(uiWord >= 0xd800 && uiWord < 0xe000){
            vSetError(spConv, uiWord, uc, "UTF-32LE value in surrogate pair range");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            break;
        }
        if(uiWord > 0x10ffff){
            vSetError(spConv, uiWord, uc, "UTF-32LE value out of range (> 0x10FFFF)");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            break;
        }
        vpVecPush(spConv->vpVec32bit, &uiWord);
    }
}
static void vUtf32BEEncode(conv* spConv, conv_dst* spDst){
    uint32_t ui;
    uint8_t ucaBuf[4];
    uint32_t uiWord;
    uint32_t* uipWords = (uint32_t*)vpVecFirst(spConv->vpVec32bit);
    aint uiWordCount = uiVecLen(spConv->vpVec32bit);
    if(!uipWords || !uiWordCount){
        XTHROW(spConv->spException, "internal error - function called without necessary data");
        return;
    }
    vVecClear(spConv->vpVecOutput);
    if(spDst->bBOM){
        ucaBuf[0] = s_caBOM32BE[0];
        ucaBuf[1] = s_caBOM32BE[1];
        ucaBuf[2] = s_caBOM32BE[2];
        ucaBuf[3] = s_caBOM32BE[3];
        vpVecPushn(spConv->vpVecOutput, ucaBuf, 4);
    }
    for(ui = 0; ui < uiWordCount; ui++){
        uiWord = uipWords[ui];
        if(uiWord >= 0xd800 && uiWord < 0xe000){
            vSetError(spConv, uiWord, ui, "UTF-32BE value in surrogate pair range");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            break;
        }
        if(uiWord > 0x10ffff){
            vSetError(spConv, uiWord, ui, "UTF-32BE value out of range (> 0x10FFFF)");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            break;
        }
        ucaBuf[0] = (uint8_t)(uiWord >> 24);
        ucaBuf[1] = (uint8_t)((uiWord >> 16) & BYTE_MASK);
        ucaBuf[2] = (uint8_t)((uiWord >> 8) & BYTE_MASK);
        ucaBuf[3] = (uint8_t)(uiWord & BYTE_MASK);
        vpVecPushn(spConv->vpVecOutput, ucaBuf, 4);
    }
    spDst->ucpData = (uint8_t*)vpVecFirst(spConv->vpVecOutput);
    spDst->uiDataLen = uiVecLen(spConv->vpVecOutput);
}
static void vUtf32LEEncode(conv* spConv, conv_dst* spDst){
    uint32_t ui;
    uint8_t ucaBuf[4];
    uint32_t uiWord;
    uint32_t* uipWords = (uint32_t*)vpVecFirst(spConv->vpVec32bit);
    aint uiWordCount = uiVecLen(spConv->vpVec32bit);
    if(!uipWords || !uiWordCount){
        XTHROW(spConv->spException, "internal error - function called without necessary data");
        return;
    }
    vVecClear(spConv->vpVecOutput);
    if(spDst->bBOM){
        ucaBuf[0] = s_caBOM32LE[0];
        ucaBuf[1] = s_caBOM32LE[1];
        ucaBuf[2] = s_caBOM32LE[2];
        ucaBuf[3] = s_caBOM32LE[3];
        vpVecPushn(spConv->vpVecOutput, ucaBuf, 4);
    }
    for(ui = 0; ui < uiWordCount; ui++){
        uiWord = uipWords[ui];
        if(uiWord >= 0xd800 && uiWord < 0xe000){
            vSetError(spConv, uiWord, ui, "UTF-32LE value in surrogate pair range");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            break;
        }
        if(uiWord > 0x10ffff){
            vSetError(spConv, uiWord, ui, "UTF-32LE value out of range (> 0x10FFFF)");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            break;
        }
        ucaBuf[3] = (uint8_t)(uiWord >> 24);
        ucaBuf[2] = (uint8_t)((uiWord >> 16) & BYTE_MASK);
        ucaBuf[1] = (uint8_t)((uiWord >> 8) & BYTE_MASK);
        ucaBuf[0] = (uint8_t)(uiWord & BYTE_MASK);
        vpVecPushn(spConv->vpVecOutput, ucaBuf, 4);
    }
    spDst->ucpData = (uint8_t*)vpVecFirst(spConv->vpVecOutput);
    spDst->uiDataLen = uiVecLen(spConv->vpVecOutput);
}
static abool bIsBOM8(uint8_t* ucpStream, aint uiLen){
    abool bReturn = APG_FALSE;
    if(uiLen > 3){
        if(ucpStream[0] == s_caBOM8[0] &&
                ucpStream[1] == s_caBOM8[1] &&
                ucpStream[2] == s_caBOM8[2]){
            bReturn = APG_TRUE;
        }
    }
    return bReturn;
}
static abool bIsBOM16BE(uint8_t* ucpStream, aint uiLen){
    abool bReturn = APG_FALSE;
    if(uiLen > 2){
        if(ucpStream[0] == s_caBOM16BE[0] &&
                ucpStream[1] == s_caBOM16BE[1]){
            bReturn = APG_TRUE;
        }
    }
    return bReturn;
}
static abool bIsBOM16LE(uint8_t* ucpStream, aint uiLen){
    abool bReturn = APG_FALSE;
    if(uiLen > 2){
        if(ucpStream[0] == s_caBOM16LE[0] &&
                ucpStream[1] == s_caBOM16LE[1]){
            bReturn = APG_TRUE;
        }
    }
    return bReturn;
}
static abool bIsBOM32BE(uint8_t* ucpStream, aint uiLen){
    abool bReturn = APG_FALSE;
    if(uiLen > 4){
        if(ucpStream[0] == s_caBOM32BE[0] &&
                ucpStream[1] == s_caBOM32BE[1] &&
                ucpStream[2] == s_caBOM32BE[2] &&
                ucpStream[3] == s_caBOM32BE[3] ){
            bReturn = APG_TRUE;
        }
    }
    return bReturn;
}
static abool bIsBOM32LE(uint8_t* ucpStream, aint uiLen){
    abool bReturn = APG_FALSE;
    if(uiLen > 4){
        if(ucpStream[0] == s_caBOM32LE[0] &&
                ucpStream[1] == s_caBOM32LE[1] &&
                ucpStream[2] == s_caBOM32LE[2] &&
                ucpStream[3] == s_caBOM32LE[3] ){
            bReturn = APG_TRUE;
        }
    }
    return bReturn;
}
static void vUtf8Encode(conv* spConv, conv_dst* spDst){
    uint32_t ui;
    uint8_t ucaBuf[4];
    uint32_t uiWord;
    uint32_t* uipWords = (uint32_t*)vpVecFirst(spConv->vpVec32bit);
    aint uiWordCount = uiVecLen(spConv->vpVec32bit);
    spDst->ucpData = NULL;
    spDst->uiDataLen = 0;
    if(!uipWords || !uiWordCount){
        XTHROW(spConv->spException, "internal error - function called without necessary data");
        return;
    }
    vVecClear(spConv->vpVecOutput);
    if(spDst->bBOM){
        ucaBuf[0] = s_caBOM8[0];
        ucaBuf[1] = s_caBOM8[1];
        ucaBuf[2] = s_caBOM8[2];
        vpVecPushn(spConv->vpVecOutput, ucaBuf, 3);
    }
    for(ui = 0; ui < uiWordCount; ui++){
        uiWord = uipWords[ui];
        if(uiWord < 0x80){
            // one byte
            ucaBuf[0] = (uint8_t)uiWord;
            vpVecPush(spConv->vpVecOutput, &ucaBuf[0]);
        }else if(uiWord < 0x800){
            // 2 bytes
            ucaBuf[0] = 0xc0 + (uint8_t)((uiWord & 0x7c0) >> 6);
            ucaBuf[1] = 0x80 + (uint8_t)(uiWord & 0x3f);
            vpVecPushn(spConv->vpVecOutput, &ucaBuf[0], 2);
        }else if(uiWord < 0xd800){
            // 3 bytes
            ucaBuf[0] = 0xe0 + (uint8_t)((uiWord & 0xf000) >> 12);
            ucaBuf[1] = 0x80 + (uint8_t)((uiWord & 0xfc0) >> 6);
            ucaBuf[2] = 0x80 + (uint8_t)(uiWord & 0x3f);
            vpVecPushn(spConv->vpVecOutput, &ucaBuf[0], 3);
        }else if(uiWord < 0xe000){
            // 3-byte error, Unicode UTF-16 surrogate pairs
            vSetError(spConv, uiWord, ui, "UTF-8 value in surrogate pair");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            break;
        }else if(uiWord < 0x10000){
            // 3 bytes
            ucaBuf[0] = 0xe0 + (uint8_t)((uiWord & 0xf000) >> 12);
            ucaBuf[1] = 0x80 + (uint8_t)((uiWord & 0xfc0) >> 6);
            ucaBuf[2] = 0x80 + (uint8_t)(uiWord & 0x3f);
            vpVecPushn(spConv->vpVecOutput, &ucaBuf[0], 3);
        }else if(uiWord <= 0x10ffff){
            // 4 bytes
            ucaBuf[0] = 0xf0 + (uint8_t)((uiWord & 0x1c0000) >> 18);
            ucaBuf[1] = 0x80 + (uint8_t)((uiWord & 0x3f000) >> 12);
            ucaBuf[2] = 0x80 + (uint8_t)((uiWord & 0xfc0) >> 6);
            ucaBuf[3] = 0x80 + (uint8_t)(uiWord & 0x3f);
            vpVecPushn(spConv->vpVecOutput, &ucaBuf[0], 4);
        }else{
            // out of Unicode range
            vSetError(spConv, uiWord, ui, "UTF-8 value out of range (> 0x10FFFF)");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            break;
        }
    }
    spDst->ucpData = (uint8_t*)vpVecFirst(spConv->vpVecOutput);
    spDst->uiDataLen = uiVecLen(spConv->vpVecOutput);
}
static void vUtf8Decode(conv* spConv, uint8_t* ucpData, aint uiDataLen){
    if(!ucpData || !uiDataLen){
        XTHROW(spConv->spException, "internal error - function called without necessary data");
        return;
    }
    uint8_t ucChar;
    uint32_t uiWord;
    uint32_t uc = 0;
    aint uiRemainder = uiDataLen - 1;
    while(uc < uiDataLen){
        ucChar = ucpData[uc];
        if(ucChar == 0xc0 || ucChar == 0xc1 || ucChar >= 0xf5){
            vSetError(spConv, (uint32_t)ucChar, uc, "invalid UTF-8 value");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            return;
        }
        if(ucChar < 0x80){
            // one byte
            uiWord = (uint32_t)ucChar;
            uc++;
            uiRemainder--;
        }else if(ucChar  < 0xe0){
            // 2 byte (starts at 0x80)
            if(uiRemainder < 1){
                vSetError(spConv, (uint32_t)ucChar, uc, "UTF-8 data has too few trailing bytes");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
            uiWord = (uint32_t)(ucChar & 0x1f) << 6;
            uiWord += (uint32_t)(ucpData[++uc] & 0x3f);
            uc++;
            uiRemainder -= 2;
        }else if(ucChar < 0xf0){
            // 3byte (starts at 0xe0)
            if(uiRemainder < 2){
                vSetError(spConv, (uint32_t)ucChar, uc, "UTF-8 data has too few trailing bytes");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
            uiWord = (uint32_t)(ucChar & 0xf) << 12;
            uiWord += (uint32_t)(ucpData[++uc] & 0x3f) << 6;
            uiWord += (uint32_t)(ucpData[++uc] & 0x3f);
            uc++;
            uiRemainder -= 3;
            if(uiWord >= 0xd800 && uiWord <= 0xdfff){
                vSetError(spConv, (uint32_t)ucChar, (uc - 3), "UTF-8 value in surrogate pair range (0xD800 - 0xDFFF)");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
            if(uiWord < 0x800){
                vSetError(spConv, (uint32_t)ucChar, (uc - 3), "UTF-8 value has over-long encoding");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
        }else if(ucChar < 0xf5){
            // 4 byte (starts with at 0xf0)
            if(uiRemainder < 3){
                vSetError(spConv, (uint32_t)ucChar, uc, "UTF-8 data has too few trailing bytes");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
            // 3byte
            uiWord = (uint32_t)(ucChar & 0x7) << 18;
            uiWord += (uint32_t)(ucpData[++uc] & 0x3f) << 12;
            uiWord += (uint32_t)(ucpData[++uc] & 0x3f) << 6;
            uiWord += (uint32_t)(ucpData[++uc] & 0x3f);
            uc++;
            uiRemainder -= 4;
            if(uiWord < 0x10000){
                vSetError(spConv, (uint32_t)ucChar, (uc - 4), "UTF-8 value has over-long encoding");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
            if(uiWord > 0x10ffff){
                vSetError(spConv, (uint32_t)ucChar, (uc - 4), "UTF-8 value out of range (> 0x10FFFF)");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
        }
        vpVecPush(spConv->vpVec32bit, &uiWord);
    }
}
static void vUtf16BEDecode(conv* spConv, uint8_t* ucpData, aint uiDataLen){
    if(!ucpData || !uiDataLen){
        XTHROW(spConv->spException, "internal error - function called without necessary data");
        return;
    }
    if(uiDataLen % 2 != 0){
        XTHROW(spConv->spException, "supposed UTF-16BE data has odd number of byte");
        return;
    }
    uint32_t uiLow, uiHigh;
    uint32_t uc = 0;
    aint uiRemainder = uiDataLen;
    while(uc < uiDataLen){
        uiHigh = (((uint32_t)ucpData[uc]) << 8);
        uc++;
        uiHigh += (((uint32_t)ucpData[uc]) & 0xff);
        uc++;
        uiRemainder -= 2;
        if(uiHigh >= 0xd800 && uiHigh < 0xdc00){
            // handle surrogate pairs
            if(uiRemainder == 0){
                vSetError(spConv, uiHigh, (uc - 2), "UTF-16BE data has missing low surrogate value");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
            uiLow = (((uint32_t)ucpData[uc]) << 8);
            uc++;
            uiLow += (((uint32_t)ucpData[uc]) & 0xff);
            uc++;
            uiRemainder -= 2;
            if(uiLow >= 0xdc00 && uiLow < 0xe000){
                uiHigh = (uiHigh - 0xd800) << 10;
                uiLow -= 0xdc00;
                uiHigh += uiLow + 0x10000;
                vpVecPush(spConv->vpVec32bit, &uiHigh);
            }else{
                vSetError(spConv, uiLow, (uc - 2), "UTF-16BE data has missing low surrogate value");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
        }else{
            if(uiHigh >= 0xdc00 && uiHigh < 0xe000){
                vSetError(spConv, uiHigh, (uc - 2), "UTF-16BE data has high surrogate out of order");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
            // high surrogate is complete word
           vpVecPush(spConv->vpVec32bit, &uiHigh);
        }
    }
}
static void vUtf16LEDecode(conv* spConv, uint8_t* ucpData, aint uiDataLen){
    if(!ucpData || !uiDataLen){
        XTHROW(spConv->spException, "internal error - function called without necessary data");
        return;
    }
    if(uiDataLen % 2 != 0){
        XTHROW(spConv->spException, "supposed UTF-16LE data has odd number of byte");
        return;
    }
    uint32_t uiLow, uiHigh;
    uint32_t uc = 0;
    aint uiRemainder = uiDataLen;
    while(uc < uiDataLen){
        uiHigh = (((uint32_t)ucpData[uc]) & 0xff);
        uc++;
        uiHigh += (((uint32_t)ucpData[uc]) << 8);
        uc++;
        uiRemainder -= 2;
        if(uiHigh >= 0xd800 && uiHigh < 0xdc00){
            // handle surrogate pairs
            if(uiRemainder == 0){
                vSetError(spConv, uiHigh, (uc - 2), "UTF-16LE data has missing low surrogate value");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
            uiLow = (((uint32_t)ucpData[uc]) & 0xff);
            uc++;
            uiLow += (((uint32_t)ucpData[uc]) << 8);
            uc++;
            uiRemainder -= 2;
            if(uiLow >= 0xdc00 && uiLow < 0xe000){
                uiHigh = (uiHigh - 0xd800) << 10;
                uiLow -= 0xdc00;
                uiHigh += uiLow + 0x10000;
                vpVecPush(spConv->vpVec32bit, &uiHigh);
            }else{
                vSetError(spConv, uiLow, (uc - 2), "UTF-16LE data has missing low surrogate value");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
        }else{
            if(uiHigh >= 0xdc00 && uiHigh < 0xe000){
                vSetError(spConv, uiHigh, (uc - 2), "UTF-16LE data has high surrogate out of order");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
            // high surrogate is complete word
            vpVecPush(spConv->vpVec32bit, &uiHigh);
        }
    }
}
static void vUtf16BEEncode(conv* spConv, conv_dst* spDst){
    uint32_t ui;
    uint8_t ucaBuf[4];
    uint32_t uiWord;
    uint32_t* uipWords = (uint32_t*)vpVecFirst(spConv->vpVec32bit);
    aint uiWordCount = uiVecLen(spConv->vpVec32bit);
    spDst->ucpData = NULL;
    spDst->uiDataLen = 0;
    if(!uipWords || !uiWordCount){
        XTHROW(spConv->spException, "internal error - function called without necessary data");
        return;
    }
    vVecClear(spConv->vpVecOutput);
    if(spDst->bBOM){
        ucaBuf[0] = s_caBOM16BE[0];
        ucaBuf[1] = s_caBOM16BE[1];
        vpVecPushn(spConv->vpVecOutput, ucaBuf, 2);
    }
    for(ui = 0; ui < uiWordCount; ui++){
        uiWord = uipWords[ui];
        if(uiWord < 0x10000){
            // 2 bytes
            if(uiWord >= 0xd800 && uiWord < 0xe000){
                vSetError(spConv, uiWord, ui, "UTF-16BE has value in surrogate pair range (0xD800-0xDFFF)");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
            ucaBuf[0] = (uint8_t)(uiWord >> 8);
            ucaBuf[1] = (uint8_t)(uiWord & 0xFF);
            vpVecPushn(spConv->vpVecOutput, &ucaBuf[0], 2);
        }else if(uiWord <= 0x10FFFF){
            // 4 bytes
            uiWord -= 0x10000;
            uint32_t uiHigh = 0xd800 + (uiWord >> 10);
            uint32_t uiLow = 0xdc00 + (uiWord & 0x3FF);
            ucaBuf[0] = (uint8_t)(uiHigh >> 8);
            ucaBuf[1] = (uint8_t)(uiHigh & 0xFF);
            ucaBuf[2] = (uint8_t)(uiLow >> 8);
            ucaBuf[3] = (uint8_t)(uiLow & 0xFF);
            vpVecPushn(spConv->vpVecOutput, &ucaBuf[0], 4);
        }else{
            // out of Unicode range
            vSetError(spConv, uiWord, ui, "UTF-16BE has value out of range (> 0x10FFFF)");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            return;
        }
    }
    spDst->ucpData = (uint8_t*)vpVecFirst(spConv->vpVecOutput);
    spDst->uiDataLen = uiVecLen(spConv->vpVecOutput);
}
static void vUtf16LEEncode(conv* spConv, conv_dst* spDst){
    uint32_t ui;
    uint8_t ucaBuf[4];
    uint32_t uiWord;
    uint32_t* uipWords = (uint32_t*)vpVecFirst(spConv->vpVec32bit);
    aint uiWordCount = uiVecLen(spConv->vpVec32bit);
    spDst->ucpData = NULL;
    spDst->uiDataLen = 0;
    if(!uipWords || !uiWordCount){
        XTHROW(spConv->spException, "internal error - function called without necessary data");
        return;
    }
    vVecClear(spConv->vpVecOutput);
    if(spDst->bBOM){
        ucaBuf[0] = s_caBOM16LE[0];
        ucaBuf[1] = s_caBOM16LE[1];
        vpVecPushn(spConv->vpVecOutput, ucaBuf, 2);
    }
    for(ui = 0; ui < uiWordCount; ui++){
        uiWord = uipWords[ui];
        if(uiWord < 0x10000){
            // 2 bytes
            if(uiWord >= 0xd800 && uiWord < 0xe000){
                vSetError(spConv, uiWord, ui, "UTF-16LE has value in surrogate pair range (0xD800-0xDFFF)");
                XTHROW(spConv->spException, spConv->sError.cpMsg);
                return;
            }
            ucaBuf[1] = (uint8_t)(uiWord >> 8);
            ucaBuf[0] = (uint8_t)(uiWord & 0xFF);
            vpVecPushn(spConv->vpVecOutput, &ucaBuf[0], 2);
        }else if(uiWord <= 0x10FFFF){
            // 4 bytes
            uiWord -= 0x10000;
            uint32_t uiHigh = 0xd800 + (uiWord >> 10);
            uint32_t uiLow = 0xdc00 + (uiWord & 0x3FF);
            ucaBuf[1] = (uint8_t)(uiHigh >> 8);
            ucaBuf[0] = (uint8_t)(uiHigh & 0xFF);
            ucaBuf[3] = (uint8_t)(uiLow >> 8);
            ucaBuf[2] = (uint8_t)(uiLow & 0xFF);
            vpVecPushn(spConv->vpVecOutput, &ucaBuf[0], 4);
        }else{
            // out of Unicode range
            vSetError(spConv, uiWord, ui, "UTF-16LE has value out of range (> 0x10FFFF)");
            XTHROW(spConv->spException, spConv->sError.cpMsg);
            return;
        }
    }
    spDst->ucpData = (uint8_t*)vpVecFirst(spConv->vpVecOutput);
    spDst->uiDataLen = uiVecLen(spConv->vpVecOutput);
}
static void vSetError(conv* spConv, uint32_t uiValue, uint32_t uiOffset, const char* cpMsg){
    spConv->sError.bHasError = APG_TRUE;
    spConv->sError.uiValue = uiValue;
    spConv->sError.uiOffset = uiOffset;
    spConv->sError.cpMsg = cpMsg;
}
