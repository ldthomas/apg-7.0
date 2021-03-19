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
/** \file json/builder.c
 * \brief The JSON builder object.
 *
 * A suite of functions for building a tree of JSON values.
 */

#include "json.h"
#include "jsonp.h"
#include "./json-grammar.h"

static const void* s_vpMagicNumber = (const void*)"builder";

/** \struct counts
 * \brief Internal workings only. Don't worry about it.
 */
typedef struct{
    aint uiValues;
    aint uiStrings;
    aint uiNumbers;
    aint uiChildren;
    aint uiAsciis;
    aint uiLists;
} counts;

/** \struct nexts
 * \brief Internal workings only. Don't worry about it.
 */
typedef struct{
    json_value** sppList;
    json_value* spValue;
    u32_phrase* spString;
    json_number* spNumber;
} nexts;

/** \struct bvalue
 * \brief A builder value. Internal workings only. Don't worry about it.
 */
typedef struct{
    aint uiId;
    union{
        aint uiStringOffset;
        aint uiChildOffset;
        double dNumber;
        uint64_t uiNumber;
        int64_t iNumber;
    };
    union{
        aint uiNumberId;
        aint uiStringLength;
        aint uiChildCount;
    };
    aint uiNext;
    aint uiKeyOffset;
    aint uiKeyLength;
    abool bKey;
} bvalue;

/** \struct build
 * \brief The builder object context.
 */
typedef struct{
    const void* vpValidate; ///< \brief "magic number" for object validation
    exception* spException; ///< \brief Pointer to the exception structure
                            /// for reporting errors to the application catch block.
    aint uiContextIndex; ///< \brief Index of the saved context pointer in the JSON context;
    void* vpMem; ///< \brief  pointer to a memory object context used only for this builder object
    void* vpVec32; ///< \brief vector of 32-bit Unicode code points
    void* vpVecAchars; ///< \brief Vector for temporary achar representation of a string.
    void* vpVecb; ///< \brief vector of `bvalue` structs holding the user values from the `Make` & `Add` functions
    json* spJson; ///< \brief JSON object context pointer. Used only for string conversions.
    uint32_t* uipChars; ///< \brief Points to the list of 32-bit characters.
    bvalue* spBValues; ///< \brief Points to the first `bvalue`.
    u32_phrase* spStrings; ///< \brief An array of all the referenced strings.
    json_number* spNumbers; ///< \brief An array of all the referenced numbers.
    char* cpAscii; ///< \brief A buffer to hold all ASCII strings referenced.
    json_value* spValues; ///< \brief An array of the final value tree.
    json_value** sppChildList; ///< \brief Pointer to a array of pointers to the children values.
    aint  uiRoot; ///< \brief Index of the root node build value (bvalue).
    void* vpUserData; ///< \brief Pointer to user data. Available to the error handling routine.
} build;

static aint uiMakeSimple(build* spBld, aint uiType);
static u32_phrase* spMakeString(build* spBld, nexts* spNexts, aint uiOffset, aint uiLength);
static json_number* spMakeNumber(build* spBld, nexts* spNexts, bvalue* spBValue);
static json_value* spBuildWalk(build* spBld, nexts* spNexts, bvalue* spBValue);
static void vCountWalk(build* spBld, counts* spCounts, bvalue* spRoot);
static void vJsonBuilderCallbacks(void* vpParserCtx);

/** \brief The builder object constructor.
 *
 * \param vpJsonCtx - Pointer to a valid JSON object previously created with \ref vpJsonCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \return Pointer to the buider object context. If NULL, a memory allocation error has occurred.
 */
void* vpJsonBuildCtor(void* vpJsonCtx){
    json* spJson = (json*) vpJsonCtx;
    if(!bJsonValidate(vpJsonCtx)){
        vExContext();
    }
    void* vpMem = spJson->vpMem;
    build* spBld = (build*)vpMemAlloc(vpMem, sizeof(build));
    memset((void*)spBld, 0, sizeof(build));
    spBld->vpVec32 = vpVecCtor(vpMem, sizeof(uint32_t), 4096);
    spBld->vpVecAchars = vpVecCtor(vpMem, sizeof(achar), 256);
    spBld->vpVecb = vpVecCtor(vpMem, sizeof(bvalue), 1024);
    // push a single, dummy value so that the user never gets a `0` index
    bvalue sB = {};
    vpVecPush(spBld->vpVecb, &sB);
    spBld->vpMem = vpMem;
    spBld->spException = spMemException(vpMem);
    spBld->spJson = spJson;
    spBld->uiContextIndex = uiVecLen(spJson->vpVecBuilders);
    vpVecPush(spJson->vpVecBuilders, &spBld);
    spBld->vpValidate = s_vpMagicNumber;
    return (void*)spBld;
}

/** \brief The builder object destructor.
 *
 * Frees all memory allocations associated with this object.
 * Note however, that \ref vJsonDtor(), the parent object destructor, will free all memory associated with this builder object,
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 */
void vJsonBuildDtor(void* vpBuildCtx){
    build* spBld = (build*)vpBuildCtx;
    if(vpBuildCtx){
        if(spBld->vpValidate == s_vpMagicNumber){
            void* vpMem = spBld->vpMem;
            void** vppContext = (void**)vpVecFirst(spBld->spJson->vpVecBuilders);
            vppContext[spBld->uiContextIndex] = NULL;
            vVecDtor(spBld->vpVec32);
            vVecDtor(spBld->vpVecAchars);
            vVecDtor(spBld->vpVecb);
            vMemFree(vpMem, spBld->spValues);
            vMemFree(vpMem, spBld->sppChildList);
            vMemFree(vpMem, spBld->spStrings);
            vMemFree(vpMem, spBld->cpAscii);
            vMemFree(vpMem, spBld->spNumbers);
            memset((void*)spBld, 0, sizeof(build));
            vMemFree(vpMem, spBld);
        }else{
            vExContext();
        }
    }
}

/** \brief Clears all memory associated with this builder object.
 *
 * Reset this object for building a new value tree.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 */
void vJsonBuildClear(void* vpBuildCtx){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    // clear previous data, if any (vMemFree ok with NULL pointer)
    vMemFree(spBld->vpMem, spBld->spValues);
    vMemFree(spBld->vpMem, spBld->spNumbers);
    vMemFree(spBld->vpMem, spBld->spStrings);
    vMemFree(spBld->vpMem, spBld->cpAscii);
    vMemFree(spBld->vpMem, spBld->sppChildList);
    spBld->spValues = NULL;
    spBld->spNumbers = NULL;
    spBld->spStrings = NULL;
    spBld->cpAscii = NULL;
    spBld->sppChildList = NULL;
    vVecClear(spBld->vpVec32);
    vVecClear(spBld->vpVecAchars);
    vVecClear(spBld->vpVecb);
    spBld->uipChars = NULL;
    spBld->spBValues = NULL;
    spBld->uiRoot = 0;
    // push a single, dummy value so that the user never gets a `0` index
    bvalue sB = {};
    vpVecPush(spBld->vpVecb, &sB);
}

/** \brief Make a string value from UTF-32 code points.
 *
 * Note that the input is raw UTF-32, 32-bit code points.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \param uipData Pointer to the array of Unicode UTF-32 code points.
 * May be NULL for empty array (uiLength ignored.)
 * \param uiLength The number of code points in the array. May be zero.
 * \return Returns an index reference to the created value.
 * Exception thrown on fatal error.
 */
aint uiJsonBuildMakeStringU(void* vpBuildCtx, const uint32_t* uipData, aint uiLength){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    if(uipData == NULL){
        uiLength = 0;
    }
    aint ui;
    char caBuf[128];
    aint uiOffset = uiVecLen(spBld->vpVec32);
    aint uiNext = uiVecLen(spBld->vpVecb);
    bvalue* spValue = (bvalue*)vpVecPush(spBld->vpVecb, NULL);
    memset(spValue, 0, sizeof(bvalue));
    if(uiLength){
        uint32_t* uipChars = (uint32_t*)vpVecPushn(spBld->vpVec32, NULL, uiLength);
        // validate a save the characters
        for(ui = 0; ui < uiLength; ui++){
            if(uipData[ui] >= 0xd800 && uipData[ui] <= 0xdbff){
                snprintf(caBuf, 128, "code point uipData[%"PRIuMAX"]=0x%04"PRIXMAX" is in surrogate pair range, [0xD800 - 0xDBFF]",
                        (luint)ui, (luint)uipData[ui]);
                XTHROW(spBld->spException, caBuf);
            }
            if(uipData[ui] > 0x10FFFF){
                snprintf(caBuf, 128, "code point uipData[%"PRIuMAX"]=0x%04"PRIXMAX" is out of range (> 0x10FFFF)",
                        (luint)ui, (luint)uipData[ui]);
                XTHROW(spBld->spException, caBuf);
            }
            uipChars[ui] = uipData[ui];
        }
    }
    spValue->uiId = JSON_ID_STRING;
    spValue->uiStringOffset = uiOffset;
    spValue->uiStringLength = uiLength;
    return uiNext;
}

/** \brief Make a string value from a null-terminated ASCII string.
 *
 * Note that the input is raw UTF-32, 32-bit code points.
 *
 * Note that the input must be a valid [RFC8259](https://tools.ietf.org/html/rfc8259)-compliant string.
 * Be especially careful since the input argument is a C-language string which must result
 * in a RFC8259-compliant string. In particular, this means the the reverse solidus, "\",
 * must always be escaped. For example:
 *  - intended JSON string:    "I want a line feed, \n, a quote, \", and the Unicode code point \u00FF."
 *  - C-language string input: "I want a line feed, \\n, a quote, \\", and the Unicode code point \\u00FF."
 *
 *  Also note that this means the characters 0x00-0x1F must be escaped, e.g. \u0000 or \u00FF, and that characters 0x80-0xFF must be
 *  either escaped or UTF-8 encoded. Example, the character value 0xFF can be input with either of these C-language strings.
 *   - "\\u00FF" or
 *   - "\xC3\xBF"
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \param cpString Pointer to the string to make into a value.
 * String may be empty but not NULL.
 * \return Returns an index reference to the created value.
 * Exception thrown on fatal error.
 */
aint uiJsonBuildMakeStringA(void *vpBuildCtx, const char *cpString) {
    build *spBld = (build*) vpBuildCtx;
    if (!spBld || (spBld->vpValidate != s_vpMagicNumber)) {
        vExContext();
    }
    if (cpString == NULL) {
        XTHROW(spBld->spException, "input string cpString may not be NULL");
    }
    aint uiOffset = uiVecLen(spBld->vpVec32);
    aint uiNext = uiVecLen(spBld->vpVecb);
    bvalue *spValue = (bvalue*) vpVecPush(spBld->vpVecb, NULL);
    memset(spValue, 0, sizeof(bvalue));
    aint uiLength = (aint) strlen(cpString);
    if (uiLength) {
        // parse JSON string to 32-bit UTF-32 code points
        parser_config sConfig = {};
        parser_state sState;
        if(sizeof(achar) == sizeof(char)){
            sConfig.acpInput = (const achar*)cpString;
        }else{
            achar* acpTemp = (achar*)vpVecPushn(spBld->vpVecAchars, NULL, uiLength);
            aint ui = 0;
            for(; ui < uiLength; ui++){
                acpTemp[ui] = (achar)((uint8_t)cpString[ui]);
            }
            sConfig.acpInput = (const achar*)acpTemp;
        }
        sConfig.uiInputLength = uiLength;
        sConfig.uiStartRule = JSON_GRAMMAR_STRING_CONTENT;
        sConfig.vpUserData = (void*)spBld;
        void* vpParser = vpParserCtor(spBld->spException, vpJsonGrammarInit);
        vJsonBuilderCallbacks(vpParser);
        vParserParse(vpParser, &sConfig, &sState);
        if(!sState.uiSuccess){
            XTHROW(spBld->spException, "unable to parse given JSON string");
        }
        vParserDtor(vpParser);
    }
    aint uiTest = uiVecLen(spBld->vpVec32);
    uiTest -= uiOffset;
    spValue->uiId = JSON_ID_STRING;
    spValue->uiStringOffset = uiOffset;
    spValue->uiStringLength = uiVecLen(spBld->vpVec32) - uiOffset;
    return uiNext;
}

/** \brief Make a JSON floating point number value.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \param dNumber The floating point number to make a JSON value from.
 * \return Returns an index reference to the created value.
 */
aint uiJsonBuildMakeNumberF(void* vpBuildCtx, double dNumber){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    aint uiNext = uiVecLen(spBld->vpVecb);
    bvalue* spValue = (bvalue*)vpVecPush(spBld->vpVecb, NULL);
    memset(spValue, 0, sizeof(bvalue));
    spValue->uiId = JSON_ID_NUMBER;
    spValue->uiNumberId = JSON_ID_FLOAT;
    spValue->dNumber = dNumber;
    return uiNext;
}

/** \brief Make a JSON signed integer number value.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \param iNumber The signed integer to make a JSON value from.
 * If iNumber >= 0, an unsigned integer value will be created.
 * \return Returns an index reference to the created value.
 */
aint uiJsonBuildMakeNumberS(void* vpBuildCtx, int64_t iNumber){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    aint uiNext = uiVecLen(spBld->vpVecb);
    bvalue* spValue = (bvalue*)vpVecPush(spBld->vpVecb, NULL);
    memset(spValue, 0, sizeof(bvalue));
    spValue->uiId = JSON_ID_NUMBER;
    if(iNumber < 0){
        spValue->uiNumberId = JSON_ID_SIGNED;
        spValue->iNumber = iNumber;
    }else{
        spValue->uiNumberId = JSON_ID_UNSIGNED;
        spValue->uiNumber = (uint64_t)iNumber;
    }
    return uiNext;
}
/** \brief Make a JSON unsigned integer number value.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \param uiNumber The unsigned integer to make a JSON value from.
 * \return Returns an index reference to the created value.
 */
aint uiJsonBuildMakeNumberU(void* vpBuildCtx, uint64_t uiNumber){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    aint uiNext = uiVecLen(spBld->vpVecb);
    bvalue* spValue = (bvalue*)vpVecPush(spBld->vpVecb, NULL);
    memset(spValue, 0, sizeof(bvalue));
    spValue->uiId = JSON_ID_NUMBER;
    spValue->uiNumberId = JSON_ID_UNSIGNED;
    spValue->uiNumber = uiNumber;
    return uiNext;
}

/** \brief Make a JSON true value.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \return Returns an index reference to the created value.
 */
aint uiJsonBuildMakeTrue(void* vpBuildCtx){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    return uiMakeSimple(spBld, JSON_ID_TRUE);
}

/** \brief Make a JSON false value.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtof();
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \return Returns an index reference to the created value.
 */
aint uiJsonBuildMakeFalse(void* vpBuildCtx){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    return uiMakeSimple(spBld, JSON_ID_FALSE);
}

/** \brief Make a JSON null value.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtof();
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \return Returns an index reference to the created value.
 */
aint uiJsonBuildMakeNull(void* vpBuildCtx){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    return uiMakeSimple(spBld, JSON_ID_NULL);
}

/** \brief Make a JSON object value.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \return Returns an index reference to the created value.
 */
aint uiJsonBuildMakeObject(void* vpBuildCtx){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    return uiMakeSimple(spBld, JSON_ID_OBJECT);
}

/** \brief Makea JSON array value.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtof();
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \return Returns an index reference to the created value.
 */
aint uiJsonBuildMakeArray(void* vpBuildCtx){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    return uiMakeSimple(spBld, JSON_ID_ARRAY);
}

/** \brief Add a child value to a parent object value.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \param uiObject The parent object value to add a child value to.
 * The return index from uiJsonBuildMakeObject().
 * \param uiKey Must the index of a string object defining the object's member key.<br>
 * `e.g. uiKey = uiJsonBuilderStringA(vpBuildCtx, "my key");`
 * \param uiAdd The index of the value to add as a child of uiParent.
 * \return Returns an index reference to the created value.
 * Exception thrown on fatal error.
 */
aint uiJsonBuildAddToObject(void* vpBuildCtx, aint uiObject, aint uiKey, aint uiAdd){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    if(uiObject == 0){
        XTHROW(spBld->spException, "parent object (uiObject) cannot be zero");
    }
    bvalue* spParent = (bvalue*)vpVecAt(spBld->vpVecb, uiObject);
    if(!spParent){
        XTHROW(spBld->spException, "parent object (uiObject) out of range - does not exist");
    }
    if(spParent->uiId != JSON_ID_OBJECT){
        XTHROW(spBld->spException, "parent (uiObject) not of type JSON_ID_OBJECT");
    }
    if(uiAdd == 0){
        XTHROW(spBld->spException, "value to add (uiAdd) cannot be zero");
    }
    bvalue* spAdd = (bvalue*)vpVecAt(spBld->vpVecb, uiAdd);
    if(!spAdd){
        XTHROW(spBld->spException, "value to add out of range - does not exist");
    }
    if(uiKey == 0){
        XTHROW(spBld->spException, "object key (uiKey) cannot be zero");
    }
    bvalue* spKey = (bvalue*)vpVecAt(spBld->vpVecb, uiKey);
    if(!spKey){
        XTHROW(spBld->spException, "key value out of range - does not exist");
    }
    if(spKey->uiId != JSON_ID_STRING){
        XTHROW(spBld->spException, "key value must type JSON_ID_STRING");
    }
    aint uiThis = uiVecLen(spBld->vpVecb);
    bvalue* spThis = (bvalue*)vpVecPush(spBld->vpVecb, NULL);
    memset(spThis, 0, sizeof(bvalue));

    // refresh the pointers
    spParent = (bvalue*)vpVecAt(spBld->vpVecb, uiObject);
    spAdd = (bvalue*)vpVecAt(spBld->vpVecb, uiAdd);
    spKey = (bvalue*)vpVecAt(spBld->vpVecb, uiKey);
    if(spParent->uiChildOffset == 0){
        // adding the first child to the parent object
        spParent->uiChildOffset = uiThis;
    }else{
        // link the previous child to this one
        bvalue* spPrev = (bvalue*)vpVecAt(spBld->vpVecb, spParent->uiNext);
        if(!spPrev){
            XTHROW(spBld->spException, "parent object has invalid offset to the last child value");
        }
        spPrev->uiNext = uiThis;
    }
    spParent->uiNext = uiThis;
    spParent->uiChildCount++;
    *spThis = *spAdd;
    spThis->uiNext = 0;
    spThis->bKey = APG_TRUE;
    spThis->uiKeyOffset = spKey->uiStringOffset;
    spThis->uiKeyLength = spKey->uiStringLength;
    return uiThis;
}

/** \brief Add a child value to a parent array value.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \param uiArray The parent array value to add a child value to.
 * The return index from uiJsonBuildMakeArray().
 * \param uiAdd The index of the value to add as a child of uiParent.
 * \return Returns an index reference to the created value.
 * Exception thrown on fatal error.
 */
aint uiJsonBuildAddToArray(void* vpBuildCtx, aint uiArray, aint uiAdd){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    if(uiArray == 0){
        XTHROW(spBld->spException, "parent array (uiArray) cannot be zero");
    }
    bvalue* spParent = (bvalue*)vpVecAt(spBld->vpVecb, uiArray);
    if(!spParent){
        XTHROW(spBld->spException, "parent array (uiArray) out of range - does not exist");
    }
    if(spParent->uiId != JSON_ID_ARRAY){
        XTHROW(spBld->spException, "parent (uiArray) not of type JSON_ID_ARRAY");
    }
    if(uiAdd == 0){
        XTHROW(spBld->spException, "value to add (uiAdd) cannot be zero");
    }
    bvalue* spAdd = (bvalue*)vpVecAt(spBld->vpVecb, uiAdd);
    if(!spAdd){
        XTHROW(spBld->spException, "value to add out of range - does not exist");
    }
    aint uiThis = uiVecLen(spBld->vpVecb);
    bvalue* spThis = (bvalue*)vpVecPush(spBld->vpVecb, NULL);
    memset(spThis, 0, sizeof(bvalue));
    if(spParent->uiChildOffset == 0){
        // adding the first child to the parent object
        spParent->uiChildOffset = uiThis;
    }else{
        // link the previous child to this one
        bvalue* spPrev = (bvalue*)vpVecAt(spBld->vpVecb, spParent->uiNext);
        if(!spPrev){
            XTHROW(spBld->spException, "parent object has invalid offset to the last child value");
        }
        spPrev->uiNext = uiThis;
    }
    spParent->uiNext = uiThis;
    spParent->uiChildCount++;
    *spThis = *spAdd;
    spThis->bKey = APG_FALSE;
    return uiThis;
}

/** \brief Build the JSON object.
 *
 * Build a tree of values, from the collection of values "added" with uiJsonBuilderAdd() calls.
 *
 * \param vpBuildCtx A context pointer returned from a previous call to vpJsonBuildCtor().
 * Silently exits the application with exit code \ref BAD_CONTEXT if pointer is invalid.
 * \param uiRoot Index of the value to use as the tree root.
 * Must be a return value from one of the uiJsonBuildMake...() functions.
 * \return Pointer to an iterator over the built tree of values.
 */
void* vpJsonBuild(void* vpBuildCtx, aint uiRoot){
    build* spBld = (build*)vpBuildCtx;
    if(!spBld || (spBld->vpValidate != s_vpMagicNumber)){
        vExContext();
    }

    if(uiRoot){
        bvalue* spRoot = (bvalue*)vpVecAt(spBld->vpVecb, uiRoot);
        if(spRoot){
            spRoot->uiNext = 0;
            spBld->uiRoot = uiRoot;
        }else{
            XTHROW(spBld->spException, "root value index out of range");
        }
    }else{
        XTHROW(spBld->spException, "root value index may not be zero");
    }

    // calculate the space to allocate for each
    aint uiBValues = uiVecLen(spBld->vpVecb);
    if(uiBValues <= 1){
        XTHROW(spBld->spException, "no added values to build");
    }
    spBld->uipChars = vpVecFirst(spBld->vpVec32);
    spBld->spBValues = (bvalue*)vpVecFirst(spBld->vpVecb);
    bvalue* spRoot = &spBld->spBValues[spBld->uiRoot]; // skip over the dummy build value
    counts sCounts = {};
    vCountWalk(spBld, &sCounts, spRoot);

    // allocate space
    vMemFree(spBld->vpMem, spBld->spValues);
    vMemFree(spBld->vpMem, spBld->spNumbers);
    vMemFree(spBld->vpMem, spBld->spStrings);
    vMemFree(spBld->vpMem, spBld->cpAscii);
    vMemFree(spBld->vpMem, spBld->sppChildList);
    spBld->spValues = NULL;
    spBld->spNumbers = NULL;
    spBld->spStrings = NULL;
    spBld->cpAscii = NULL;
    spBld->sppChildList = NULL;
    spBld->spValues = (json_value*)vpMemAlloc(spBld->vpMem, ((aint)sizeof(json_value) * sCounts.uiValues));
    memset(spBld->spValues, 0, ((aint)sizeof(json_value) * sCounts.uiValues));
    if(sCounts.uiChildren){
        spBld->sppChildList = (json_value**)vpMemAlloc(spBld->vpMem, ((aint)sizeof(json_value*) * sCounts.uiChildren));
    }
    if(sCounts.uiStrings){
        spBld->spStrings = (u32_phrase*)vpMemAlloc(spBld->vpMem, ((aint)sizeof(u32_phrase) * sCounts.uiStrings));
        // max number of ASCII characters needed is the number of 32-bit characters + 1 null terminator for each `string`
        spBld->cpAscii = (char*)vpMemAlloc(spBld->vpMem, ((aint)sizeof(char) * sCounts.uiAsciis));
    }
    if(sCounts.uiNumbers){
        spBld->spNumbers = (json_number*)vpMemAlloc(spBld->vpMem, ((aint)sizeof(json_number) * sCounts.uiNumbers));
    }

    // construct the value tree
    nexts sNexts;
    sNexts.spNumber = spBld->spNumbers;
    sNexts.spString = spBld->spStrings;
    sNexts.spValue = spBld->spValues;
    sNexts.sppList = spBld->sppChildList;
    json_value* spValues = spBuildWalk(spBld, &sNexts, spRoot);

    // make the tree iterator
    json_iterator* spIt = spJsonIteratorCtor(spBld->spJson);
    spIt->sppValues = (json_value**)vpVecPushn(spIt->vpVec, NULL, sCounts.uiValues);
    aint ui = 0;
    for(; ui < sCounts.uiValues; ui++){
        spIt->sppValues[ui] = &spValues[ui];
    }
    spIt->uiCount = sCounts.uiValues;
    return (void*)spIt;
}

static void vCountWalk(build* spBld, counts* spCounts, bvalue* spRoot){
    bvalue* spChild;
    aint ui;
    spCounts->uiValues++;
    if(spRoot->bKey){
        spCounts->uiStrings++;
        spCounts->uiAsciis += spRoot->uiKeyLength + 1;
    }
    switch(spRoot->uiId){
    case JSON_ID_STRING:
        spCounts->uiStrings++;
        spCounts->uiAsciis += spRoot->uiStringLength + 1;
        break;
    case JSON_ID_NUMBER:
        spCounts->uiNumbers++;
        break;
    case JSON_ID_TRUE:
    case JSON_ID_FALSE:
    case JSON_ID_NULL:
        break;
    case JSON_ID_OBJECT:
    case JSON_ID_ARRAY:
        spCounts->uiChildren += spRoot->uiChildCount;
        spChild = spBld->spBValues + spRoot->uiChildOffset;
        for(ui = 0; ui < spRoot->uiChildCount; ui++){
            vCountWalk(spBld, spCounts, spChild);
            spChild = spBld->spBValues + spChild->uiNext;

            // !!!! DEBUG sanity check
            if(ui == (spRoot->uiChildCount - 1)){
                if(spChild->uiNext != 0){
                    XTHROW(spBld->spException, "vCountWalk() sanity check: last child's next index is not zero");
                    break;
                }
            }
            // !!!! DEBUG sanity check
        }
        break;
    default:
        XTHROW(spBld->spException, "unrecognized value type");
        break;
    }
}
static u32_phrase* spMakeString(build* spBld, nexts* spNexts, aint uiOffset, aint uiLength){
    u32_phrase* spReturn = spNexts->spString;
    spNexts->spString++;
    spReturn->uipPhrase = spBld->uipChars + uiOffset;
    spReturn->uiLength = (uint32_t)uiLength;
    return spReturn;
}
static json_number* spMakeNumber(build* spBld, nexts* spNexts, bvalue* spBValue){
    json_number* spReturn = spNexts->spNumber;
    spNexts->spNumber++;
    spReturn->uiType = spBValue->uiNumberId;
    switch(spBValue->uiNumberId){
    case JSON_ID_FLOAT:
        spReturn->dFloat = spBValue->dNumber;
        break;
    case JSON_ID_SIGNED:
        spReturn->iSigned = spBValue->iNumber;
        break;
    case JSON_ID_UNSIGNED:
        spReturn->uiUnsigned = spBValue->uiNumber;
        break;
    default:
        XTHROW(spBld->spException, "unrecognized number type");
        break;
    }
    return spReturn;
}
static json_value* spBuildWalk(build* spBld, nexts* spNexts, bvalue* spBValue){
    bvalue* spChild;
    aint ui;
    json_value* spThis = spNexts->spValue;
    spThis->uiId = spBValue->uiId;
    spNexts->spValue++;
    if(spBValue->bKey){
        spThis->spKey = spMakeString(spBld, spNexts, spBValue->uiKeyOffset, spBValue->uiKeyLength);
    }
    switch(spBValue->uiId){
    case JSON_ID_STRING:
        spThis->spString = spMakeString(spBld, spNexts, spBValue->uiStringOffset, spBValue->uiStringLength);
        break;
    case JSON_ID_NUMBER:
        spThis->spNumber = spMakeNumber(spBld, spNexts, spBValue);
        break;
    case JSON_ID_TRUE:
    case JSON_ID_FALSE:
    case JSON_ID_NULL:
        break;
    case JSON_ID_OBJECT:
    case JSON_ID_ARRAY:
        spThis->sppChildren = spNexts->sppList;
        spNexts->sppList += spBValue->uiChildCount;
        spThis->uiChildCount = spBValue->uiChildCount;
        spChild = spBld->spBValues + spBValue->uiChildOffset;
        for(ui = 0; ui < spBValue->uiChildCount; ui++){
            spThis->sppChildren[ui] = spBuildWalk(spBld, spNexts, spChild);
            spChild = spBld->spBValues + spChild->uiNext;
        }
        break;
    default:
        XTHROW(spBld->spException, "unrecognized value type");
        break;
    }
    return spThis;
}
static aint uiMakeSimple(build* spBld, aint uiType){
    aint uiNext = uiVecLen(spBld->vpVecb);
    bvalue* spValue = (bvalue*)vpVecPush(spBld->vpVecb, NULL);
    memset(spValue, 0, sizeof(bvalue));
    spValue->uiId = uiType;
    return uiNext;
}
static void vAscii(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        uint32_t uiChar = (uint32_t)spData->acpString[spData->uiParserOffset];
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vRSolidus(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        uint32_t uiChar = 0x5C;
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vSolidus(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        uint32_t uiChar = 0x2F;
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vQuote(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        uint32_t uiChar = 0x22;
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vBackSpace(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        uint32_t uiChar = 0x08;
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vFormFeed(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        uint32_t uiChar = 0x0C;
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vLineFeed(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        uint32_t uiChar = 0x0A;
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vCr(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        uint32_t uiChar = 0x0D;
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vTab(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        uint32_t uiChar = 0x09;
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vUtf82(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        char caHex[] = {
                (char)spData->acpString[spData->uiParserOffset],
                (char)spData->acpString[spData->uiParserOffset+1],
        };
        uint32_t uiChar = uiUtf8_2byte(caHex);
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vUtf83(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        char caHex[] = {
                (char)spData->acpString[spData->uiParserOffset],
                (char)spData->acpString[spData->uiParserOffset+1],
                (char)spData->acpString[spData->uiParserOffset+2],
        };
        uint32_t uiChar = uiUtf8_3byte(caHex);
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vUtf84(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        char caHex[] = {
                (char)spData->acpString[spData->uiParserOffset],
                (char)spData->acpString[spData->uiParserOffset+1],
                (char)spData->acpString[spData->uiParserOffset+2],
                (char)spData->acpString[spData->uiParserOffset+3],
        };
        uint32_t uiChar = uiUtf8_4byte(caHex);
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vUtf161(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
        const achar* acpDigits = &spData->acpString[spData->uiParserOffset];
        char caHex[5];
        caHex[0] = (char)acpDigits[2];
        caHex[1] = (char)acpDigits[3];
        caHex[2] = (char)acpDigits[4];
        caHex[3] = (char)acpDigits[5];
        caHex[4] = 0;
        uint32_t uiChar;
        if(uiUtf16_1(caHex, &uiChar) != JSON_UTF16_MATCH){
            XTHROW(spBld->spException, "UTF-16 encoding error - surrogate pair range not allowed");
        }
        vpVecPush(spBld->vpVec32, &uiChar);
    }
}
static void vUtf162(callback_data* spData){
    if(spData->uiParserState == ID_MATCH){
        build* spBld = (build*)spData->vpUserData;
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
        uint32_t uiChar;
        aint uiRet = uiUtf16_2(caHex, &uiChar);
        switch(uiRet){
        case JSON_UTF16_MATCH:
            vpVecPush(spBld->vpVec32, &uiChar);
            return;
        case JSON_UTF16_NOMATCH:
            spData->uiCallbackState = ID_NOMATCH;
            return;
        case JSON_UTF16_BAD_HIGH:
            XTHROW(spBld->spException, "UTF-16 encoding error - low surrogate not preceded by high surrogate");
            return;
        case JSON_UTF16_BAD_LOW:
            XTHROW(spBld->spException, "UTF-16 encoding error - high surrogate not followed by low surrogate");
            return;
        }
    }
}
static void vJsonBuilderCallbacks(void* vpParserCtx){
    aint ui;
    parser_callback cb[RULE_COUNT_JSON_GRAMMAR];
    memset((void*)cb, 0, sizeof(cb));
    cb[JSON_GRAMMAR_ASCII] = vAscii;
    cb[JSON_GRAMMAR_R_SOLIDUS] = vRSolidus;
    cb[JSON_GRAMMAR_SOLIDUS] = vSolidus;
    cb[JSON_GRAMMAR_QUOTE] = vQuote;
    cb[JSON_GRAMMAR_BACKSPACE] = vBackSpace;
    cb[JSON_GRAMMAR_FORM_FEED] = vFormFeed;
    cb[JSON_GRAMMAR_LINE_FEED] = vLineFeed;
    cb[JSON_GRAMMAR_CR] = vCr;
    cb[JSON_GRAMMAR_TAB] = vTab;
    cb[JSON_GRAMMAR_UTF16_1] = vUtf161;
    cb[JSON_GRAMMAR_UTF16_2] = vUtf162;
    cb[JSON_GRAMMAR_UTF8_2] = vUtf82;
    cb[JSON_GRAMMAR_UTF8_2] = vUtf82;
    cb[JSON_GRAMMAR_UTF8_3] = vUtf83;
    cb[JSON_GRAMMAR_UTF8_4] = vUtf84;

    for(ui = 0; ui < (aint)RULE_COUNT_JSON_GRAMMAR; ui++){
        vParserSetRuleCallback(vpParserCtx, ui, cb[ui]);
    }
}
