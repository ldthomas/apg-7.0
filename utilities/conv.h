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
/** \file conv.h
 * \brief Header for the Unicode UTF encoding module.
 */
#ifndef CONV_H_
#define CONV_H_

/** @name Base64 Bits and Masks
 */
///@{
/** \brief The base64 bit. Or (|) with data type for base64 encoding/decoding.
 *
 * - When or'ed with a data source type identifier, base64 decoding will precede the type decoding.
 * - When or'ed with a data destination type identifier, base64 encoding with follow the type encoding.
 */
#define BASE64      0x8000
/** \brief The base64 mask. And (&) with the type to extract the base64 bit, if any.
 *
 * e.g.<br>
 *  - uiType & BASE64_MASK > 0, base64 bit is set
 *  - uiType & BASE64_MASK == 0, base64 bit is not set
 */
#define BASE64_MASK 0xFF00
/** \brief The type mask. And (&) with the type to extract the type, without the base64 bit.
 *
 * e.g.<br>
 *  - uiType & TYPE_MASK is the encode/decode type
 */

#define TYPE_MASK   0xFF
///@}

/** @name The Encoding Type Identifiers */
///@{
/** \brief Alias for ISO_8895_1. */
#define BINARY      4
/** \brief Alias for ISO_8895_1. */
#define LATIN1      4
/** \brief All 8-bit, single-byte characters. Unicode characters ranging from U+0000 to U+00FF.
 *
 * For a discussion and mapping of the characters, see the
 * [Wikipedia page](https://en.wikipedia.org/wiki/ISO%2FIEC_8859-1).
 *
 * Note: Care must be taken using this as the destination type.
 * Unicode code points > 0xFF will result in an error.
 * */
#define ISO_8859_1  4
/** \brief Data type macro for UTF-8 encoding/decoding. */
#define UTF_8       8
/** \brief Data type macro for UTF-16 encoding/decoding. */
#define UTF_16      16
/** \brief Data type macro for UTF-16BE encoding/decoding. */
#define UTF_16BE    17
/** \brief Data type macro for UTF-16LE encoding/decoding. */
#define UTF_16LE    18
/** \brief Data type macro for UTF-32 encoding/decoding. */
#define UTF_32      32
/** \brief Data type macro for UTF-32BE encoding/decoding. */
#define UTF_32BE    33
/** \brief Data type macro for UTF-32LE encoding/decoding. */
#define UTF_32LE    34
///@}

/** @name Miscellaneous */
///@{
/** \brief Data type macro for unknown encoding type. */
#define UTF_UNKNOWN 40
/** \brief The "true" macro for destination BOM flag. */
#define BOM         1
/** \brief The "false" macro for destination BOM flag. */
#define NOBOM       0
/** \brief For configuring base64 destinations, indicates line breaks as line feed (\\n, 0x0A). */
#define BASE64_LF   10
/** \brief For configuring base64 destinations, indicates line breaks as carriage return + line feed (\\r\\n, 0x0D0A). */
#define BASE64_CRLF 13
///@}

/** \struct conv_src
 * \brief Defines the input data type, location and length.
 */
typedef struct {
    aint uiDataType; /**< \brief One of the encoding type identifiers, \ref UTF_8, etc.
    If or'ed (|) with \ref BASE64, the source data stream is assumed to be base64 encoded and
    base64 decoding precedes the type decoding.*/
    uint8_t* ucpData; /**< \brief Pointer to the byte stream to decode, */
    aint uiDataLen; /**< \brief Number of bytes in the byte stream. */
} conv_src;

/** \struct conv_dst
 * \brief Defines the output data type, location, length and whether or not to preface with a Byte Order Mark (BOM).
 */
typedef struct{
    aint uiDataType; /**< \brief [in] One of the encoding type identifiers, \ref UTF_8, etc.*/
    abool bBOM; /**< \brief [in] If true(\ref BOM) prepend a Byte Order Mark, if false(\ref NOBOM) no Byte Order Mark prepended. */
    uint8_t* ucpData; /**< \brief [out] Pointer to the output byte stream. Valid until another function call on the context handle. */
    aint uiDataLen; /**< \brief [out] Number of bytes in the byte stream. */
} conv_dst;

void* vpConvCtor(exception* spEx);
void  vConvDtor(void* vpCtx);
void vConvDecode(void* vpCtx, conv_src* spSrc);
void vConvEncode(void* vpCtx, conv_dst* spDst);
void vConvConvert(void* vpCtx, conv_src* spSrc, conv_dst* spDst);
void vConvGetCodePoints(void* vpCtx, uint32_t* uipData, uint32_t* uipDataLen);
void vConvUseCodePoints(void* vpCtx, uint32_t* uipSrc, aint uiSrcLen);
void vConvConfigureBase64(void* vpCtx, aint uiLineLen, aint uiLineEnd);

#endif /* CONV_H_ */
