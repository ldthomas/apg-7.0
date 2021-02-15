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
/** \file json.c
 * \brief The JSON parser API.
 *
 * This JSON parser is meant to be fully compliant with RFC8259. Some of the main features are:
 *  - Data encapsulation keeps each parser distinct and thread-safe.
 *  - JSON data is parsed into an array of value structures. The sequence of values is in the order
 * of a [depth-first](https://en.wikipedia.org/wiki/Depth-first_search) traversal of the JSON object tree.
 *  - Values (see \ref json_value_tag) are very general, accommodating all of the types:
 *    - object (Contains a count of and pointer to its list members. Note that members are also \ref json_value_tag structs.)
 *    - array (Contains a count of and pointer to its list values.)
 *    - u32_phrase pointer to both a null-terminated C-string,
 *    if possible, as well as an array of 32-bit Unicode code points.
 *    - json_number Has a double, unsigned or signed integer value, depending on the number's form.
 *    - true
 *    - false
 *    - null
 *  - A number of tools are available for, among other things, display and navigation of the JSON input and values.
 *      - functions for input of the JSON data byte stream from a file or array
 *          - vpJsonReadFile()
 *          - vpJsonReadArray()
 *      - functions for finding and displaying the JSON data in a [hex dump](https://en.wikipedia.org/wiki/Hex_dump) format
 *      (the UTF-8 requirement for JSON input means that, in general, not all bytes will be printable.)
 *          - vpJsonTree()
 *          - vpJsonChildren()
 *          - vJsonDisplayInput()
 *      - functions for displaying the parsed values
 *          - vJsonDisplayValue()
 *      - functions for finding specific key named values (global and for a specific object)
 *          - vpJsonFindKeyA() - for finding null-terminated C-string keys
 *          - vpJsonFindKeyU() - for finding keys that are an array of Unicode code points.
 */

#include <limits.h>

#include "json.h"
#include "jsonp.h"
#include "json-grammar.h"

/** \def CHARS_LINE_LEN
 * \brief Private Object use Only
 */
#define CHARS_LINE_LEN 8

static const void* s_vpJsonValid = (void*)"JSON";
static const void* s_vpIteratorValid = (void*)"iterator";

static uint32_t s_uiaTrue[] = { 116, 114, 117, 101 };
static uint32_t s_uiaFalse[] = { 102, 97, 108, 115, 101 };
static uint32_t s_uiaNull[] = { 110, 117, 108, 108 };
static aint s_uiTrueLen = 4;
static aint s_uiFalseLen = 5;
static aint s_uiNullLen = 4;
static char* s_cpTrue = "true";
static char* s_cpFalse = "false";
static char* s_cpNull = "null";
static uint32_t s_uiOpenCurly = 123;
static uint32_t s_uiCloseCurly = 125;
static uint32_t s_uiOpenSquare = 91;
static uint32_t s_uiCloseSquare = 93;
static uint32_t s_uiColon = 58;
static uint32_t s_uiComma = 44;
static uint32_t s_uiSpace = 32;
static uint32_t s_uiLineEnd = 10;
static uint32_t s_uiQuote = 34;
static uint32_t s_uiReverseSolidus = 0x5C;
static uint32_t s_uiLowerU = 0x75;

static void vParseInput(json* spJson);
static void vReintrantValueDisplay(json *spJson, struct json_value_tag *spValue);
static void vIndent(aint uiIndent);
static void vFindKeyValues(json* spJson, json_iterator* spIt, const uint32_t *uipKey, uint32_t uiLength, json_value* spValue);
static void vBreakIndent(json* spJson, aint uiIndent);
static void vPushObject(json* spJson, json_value* spValue, abool bArray);
static void vPushString(json* spJson, u32_phrase* spString);
static void vPushNumber(json* spJson, json_number* spNumber);
static void vPushValue(json* spJson, json_value* spValue);
static void vWalkCount(json* spJson, json_value* spValue);

/**
 * \brief The JSON constructor.
 *
 * \param spEx Pointer to a valid exception structure initialized with \ref XCTOR().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Pointer to the object context on success.
 */
void* vpJsonCtor(exception* spEx){
    if(!bExValidate(spEx)){
        vExContext();
    }
    json* spJson = NULL;
    void* vpMem = vpMemCtor(spEx);
    spJson = (json*) vpMemAlloc(vpMem, sizeof(json));
    memset((void*) spJson, 0, sizeof(json));

    // basics for input and output
    spJson->vpVecInput = vpVecCtor(vpMem, sizeof(uint8_t), 4096);
    spJson->vpVecOutput = vpVecCtor(vpMem, sizeof(uint32_t), 8192);

    // vectors used to give caller access to resulting values
    spJson->vpVecTreeList = vpVecCtor(vpMem, sizeof(json_value*), 512);
    spJson->vpVecChildList = vpVecCtor(vpMem, sizeof(json_value*), 512);
    spJson->vpVecKeyList = vpVecCtor(vpMem, sizeof(json_value*), 512);
    spJson->vpVecScratch32 = vpVecCtor(vpMem, sizeof(uint32_t), 4096);

    spJson->vpVecIterators = vpVecCtor(vpMem, sizeof(void*), 32);
    spJson->vpVecBuilders = vpVecCtor(vpMem, sizeof(void*), 32);

    // vectors used by the parser for collecting data
    spJson->vpVecChars = vpVecCtor(vpMem, sizeof(uint32_t), 4096);
    spJson->vpVecAscii = vpVecCtor(vpMem, sizeof(uint8_t), 4096);
    spJson->vpVecValuesr = vpVecCtor(vpMem, sizeof(value_r), 1024);
    spJson->vpVecStringsr = vpVecCtor(vpMem, sizeof(string_r), 1024);
    spJson->vpVecNumbers = vpVecCtor(vpMem, sizeof(json_number), 1024);
    spJson->vpVecChildIndexes = vpVecCtor(vpMem, sizeof(aint), 1024);
    spJson->vpVecFrames = vpVecCtor(vpMem, sizeof(frame), 128);
    spJson->vpVecValues = vpVecCtor(vpMem, sizeof(json_value), 128);
    spJson->vpVecStrings = vpVecCtor(vpMem, sizeof(u32_phrase), 128);
    spJson->vpVecChildPointers = vpVecCtor(vpMem, sizeof(json_value*), 128);
    spJson->vpFmt = vpFmtCtor(spEx);

    // success
    spJson->vpMem = vpMem;
    spJson->spException = spEx;
    spJson->vpValidate = s_vpJsonValid;
    return (void*) spJson;
}

/**
 * \brief The JSON Parser component destructor.
 *
 * Clears the context to prevent accidental use after release and frees all memory allocated by this object.
 * Freed memory includes any and all memory that may have been allocated to iterator or builder objects.
 * \param vpCtx Pointer to a valid parser context previously returned from  vpJsonCtor.
 * A NULL pointer is silently ignored. However, a non-NULL pointers must be a valid JSON context pointer.
 */
void vJsonDtor(void *vpCtx) {
    json* spJson = (json*) vpCtx;
    if (vpCtx) {
        if (spJson->vpValidate == s_vpJsonValid) {
            void* vpMem = spJson->vpMem;
            if(spJson->spIn){
                fclose(spJson->spIn);
            }
            vConvDtor(spJson->vpConv);
            vFmtDtor(spJson->vpFmt);
            vLinesDtor(spJson->vpLines);
            memset((void*) spJson, 0, sizeof(json));
            vMemDtor(vpMem);
        }else{
            vExContext();
        }
    }
}

/** \brief Validate a JSON context pointer.
 * \param vpCtx Pointer to a possibly valid JSON context returned from vpJsonCtor().
 * \return True if the pointer is valid, false otherwise.
 */
abool bJsonValidate(void* vpCtx){
    if(vpCtx && (((json*)vpCtx)->vpValidate == s_vpJsonValid)){
        return APG_TRUE;
    }
    return APG_FALSE;
}

/**
 * \brief The JSON file reader.
 *
 * Reads a file containing JSON text in UTF-8 format
 * and parses the file into a value tree.
 *
 * \param vpCtx Pointer to a valid parser context previously returned from  vpJsonCtor.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpFileName The name of the file to read.
 * \return Returns an iterator to the values in the value tree. The order of values is that
 * of a depth-first walk of the value tree.
 */
void* vpJsonReadFile(void *vpCtx, const char *cpFileName){
    json* spJson = (json*) vpCtx;
    if((vpCtx == NULL) || (spJson->vpValidate != s_vpJsonValid)){
        vExContext();
    }
    char caBuf[PATH_MAX + 128];
    size_t uiSize = PATH_MAX + 128;
    uint8_t ucaBom[3];
    int iaBom[3];
    vVecClear(spJson->vpVecInput);
    vLinesDtor(spJson->vpLines);
    spJson->vpLines = NULL;
    if(spJson->spIn){
        fclose(spJson->spIn);
        spJson->spIn = NULL;
    }
    if(!cpFileName || !cpFileName[0]){
        XTHROW(spJson->spException, "file name cannot be NULL or empty");

    }
    spJson->spIn = fopen(cpFileName, "rb");
    if (!spJson->spIn) {
        snprintf(caBuf, uiSize, "can't open input file: %s", cpFileName);
        XTHROW(spJson->spException, caBuf);
    }
    // get the first 3 bytes and test for BOM
    iaBom[0] = fgetc(spJson->spIn);
    iaBom[1] = fgetc(spJson->spIn);
    iaBom[2] = fgetc(spJson->spIn);
    ucaBom[0] = (uint8_t) iaBom[0];
    ucaBom[1] = (uint8_t) iaBom[1];
    ucaBom[2] = (uint8_t) iaBom[2];
    if (iaBom[0] == EOF) {
        snprintf(caBuf, uiSize, "input file is empty: %s", cpFileName);
        XTHROW(spJson->spException, caBuf);
    }
    if (iaBom[1] == EOF) {
        // only one character - push and continue
        vpVecPush(spJson->vpVecInput, &ucaBom[0]);
    }else if (iaBom[2] == EOF) {
        // only two characters - push and continue
        vpVecPushn(spJson->vpVecInput, &ucaBom[0], 2);
    }else{
        if (!(ucaBom[0] == 0xEF && ucaBom[1] == 0xBB && ucaBom[2] == 0xBF)) {
            // no BOM - save the first three characters and continue
            vpVecPushn(spJson->vpVecInput, &ucaBom[0], 3);
        }
        uint8_t ucGotBuf[1024];
        size_t uiGot = fread(ucGotBuf, 1, 1024, spJson->spIn);
        while (uiGot != 0) {
            vpVecPushn(spJson->vpVecInput, ucGotBuf, (aint)uiGot);
            uiGot = fread(ucGotBuf, 1, 1024, spJson->spIn);
        }
    }
    fclose(spJson->spIn);
    spJson->spIn = NULL;
    spJson->vpLines = vpLinesCtor(spJson->spException, (char*) vpVecFirst(spJson->vpVecInput), uiVecLen(spJson->vpVecInput));
    vParseInput(spJson);
    json_iterator* spIt = spJsonIteratorCtor(spJson);
    json_value** sppValues = (json_value**)vpVecPushn(spIt->vpVec, NULL, spJson->uiValueCount);
    aint ui = 0;
    for(; ui < spJson->uiValueCount; ui++){
        sppValues[ui] = &spJson->spValues[ui];
    }
    spIt->sppValues = sppValues;
    spIt->uiCount = spJson->uiValueCount;
    return (void*)spIt;
}

/**
 * \brief The JSON array reader.
 *
 * Reads a an array containing JSON text in UTF-8 format
 * and parses the file into a value tree.
 *
 * \param vpCtx Pointer to a valid parser context previously returned from  vpJsonCtor.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param ucpData Pointer to the UTF-8 encoded JSON text.
 * \param uiDataLen The number of bytes of JSON text data.
 * \return Returns an iterator to the values in the value tree. The order of values is that
 * of a depth-first walk of the value tree.
 */
void* vpJsonReadArray(void *vpCtx, uint8_t *ucpData, aint uiDataLen){
    json* spJson = (json*) vpCtx;
    if((vpCtx == NULL) || (spJson->vpValidate != s_vpJsonValid)){
        vExContext();
    }
    vVecClear(spJson->vpVecInput);
    vLinesDtor(spJson->vpLines);
    spJson->vpLines = NULL;
    // check for BOM
    if (uiDataLen >= 3) {
        if (ucpData[0] == 0xEF && ucpData[1] == 0xBB && ucpData[2] == 0xBF) {
            ucpData += 3;
            uiDataLen -= 3;
        }
    }
    vpVecPushn(spJson->vpVecInput, ucpData, uiDataLen);
    spJson->vpLines = vpLinesCtor(spJson->spException, (char*) vpVecFirst(spJson->vpVecInput),
            uiVecLen(spJson->vpVecInput));
    vParseInput(spJson);
    json_iterator* spIt = spJsonIteratorCtor(spJson);
    json_value** sppValues = (json_value**)vpVecPushn(spIt->vpVec, NULL, spJson->uiValueCount);
    aint ui = 0;
    for(; ui < spJson->uiValueCount; ui++){
        sppValues[ui] = &spJson->spValues[ui];
    }
    spIt->sppValues = sppValues;
    spIt->uiCount = spJson->uiValueCount;
    return (void*)spIt;
}

/** \brief Converts a sub-tree of values into UTF-8 byte stream of JSON text.
 *
 * \param vpCtx Pointer to a valid parser context previously returned from  vpJsonCtor.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spValue The root node of the sub-tree to write.
 * spValue can be any value, including the root value, of an existing value tree.
 * \param uipCount Pointer to an unsigned integer.
 * On completion the number of bytes in the output JSON text is written here.
 * \return Returns a pointer to the JSON text byte stream. The pointer is valid until another call to this function.
 */
uint8_t* ucpJsonWrite(void* vpCtx, json_value* spValue, aint* uipCount) {
    json* spJson = (json*) vpCtx;
    if((vpCtx == NULL) || (spJson->vpValidate != s_vpJsonValid)){
        vExContext();
    }
    uint8_t* ucpReturn = NULL;
    if(!uipCount){
        XTHROW(spJson->spException, "pointer for return count cannot be NULL");
    }
    *uipCount = 0;
    spJson->uiCurrentDepth = 0;
    vVecClear(spJson->vpVecOutput);
    spJson->bFirstNode = APG_TRUE;
    vPushValue(spJson, spValue);
    // convert output to UTF-8 byte stream
    if (!spJson->vpConv) {
        spJson->vpConv = vpConvCtor(spJson->spException);
    }
    conv_dst sDst = { };
    sDst.uiDataType = UTF_8;
    vConvUseCodePoints(spJson->vpConv, (uint32_t*) vpVecFirst(spJson->vpVecOutput),
            uiVecLen(spJson->vpVecOutput));
    vConvEncode(spJson->vpConv, &sDst);
    ucpReturn = sDst.ucpData;
    *uipCount = sDst.uiDataLen;
    return ucpReturn;
}

/** \brief Display the input JSON byte stream.
 *
 * This will use a [format](\ref library) object to display the JSON input byte stream in a hexdump-style format.
 *
 * \param vpCtx Pointer to a valid parser context previously returned from  vpJsonCtor.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param bShowLines - if true, the display will show a separate hexdump-type display for each line.
 */
void vJsonDisplayInput(void *vpCtx, abool bShowLines) {
    json* spJson = (json*) vpCtx;
    if((vpCtx == NULL) || (spJson->vpValidate != s_vpJsonValid)){
        vExContext();
    }
    line* spLine;
    aint ui, uiLines;
    aint uiOffset, uiChars;
    uint8_t* ucpChars = (uint8_t*) vpVecFirst(spJson->vpVecInput);
    const char* cpNextLine;
    spLine = spLinesFirst(spJson->vpLines);
    uiLines = uiLinesCount(spJson->vpLines);
    if (!ucpChars || !spLine || !uiLines) {
        XTHROW(spJson->spException, "no JSON input");
    }
    if (bShowLines) {
        for (ui = 0; ui < uiLines; ui++, spLine++) {
            printf("%04"PRIuMAX" ", (luint) spLine->uiLineIndex);
            uiOffset = spLine->uiCharIndex;
            cpNextLine = cpFmtFirstBytes(spJson->vpFmt, &ucpChars[uiOffset], spLine->uiLineLength, FMT_CANONICAL, 0, 0);
            vFmtIndent(spJson->vpFmt, 5);
            while(cpNextLine){
                printf("%s", cpNextLine);
                cpNextLine = cpFmtNext(spJson->vpFmt);
            }
            vFmtIndent(spJson->vpFmt, 0);
        }
    } else {
        uiChars = uiVecLen(spJson->vpVecInput);
        cpNextLine = cpFmtFirstBytes(spJson->vpFmt, ucpChars, (long int)uiChars, FMT_CANONICAL, 0, 0);
        vFmtIndent(spJson->vpFmt, 0);
        while(cpNextLine){
            printf("%s", cpNextLine);
            cpNextLine = cpFmtNext(spJson->vpFmt);
        }
    }
}

/** \brief Display a value and optionally the values in the branch below, if any.
 *
 * \param vpCtx Pointer to a valid parser context previously returned from  vpJsonCtor.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spValue - pointer to value to display
 * \param uiDepth - Indicates the tree depth of the value's sub branch to display.
 * Use zero(0) to display the full sub-tree, one(1) to display just the given value.
 * \return APG_SUCCESS if successful, APG_FAILURE otherwise.
 */
void vJsonDisplayValue(void *vpCtx, json_value *spValue, aint uiDepth) {
    json* spJson = (json*) vpCtx;
    if((vpCtx == NULL) || (spJson->vpValidate != s_vpJsonValid)){
        vExContext();
    }
    spJson->uiMaxDepth = uiDepth ? uiDepth : APG_INFINITE;
    spJson->uiCurrentDepth = 0;
    vReintrantValueDisplay(spJson, spValue);
}

/** \brief Find JSON values with a specified ASCII key
 *
 * Search for all values with the specified key.
 *
 * \param vpCtx Pointer to a valid parser context previously returned from  vpJsonCtor.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpKey The key as a null-terminated ASCII string.
 * \param spValue - Pointer to the root node of the sub-tree to search.
 * The given node and all nodes in the sub-tree below, if any, will be searched.
 * \return A pointer to an [iterator](\ref anchor_json_iterator) for the values with the specified key.
 * NULL if no key matches are found.
 */
void* vpJsonFindKeyA(void *vpCtx, const char *cpKey, json_value* spValue) {
    void* vpReturn = NULL;
    json* spJson = (json*) vpCtx;
    if((vpCtx == NULL) || (spJson->vpValidate != s_vpJsonValid)){
        vExContext();
    }
    uint32_t uiLength = (uint32_t) strlen(cpKey);
    if(uiLength){
        vVecClear(spJson->vpVecScratch32);
        uint32_t* uipKey = (uint32_t*) vpVecPushn(spJson->vpVecScratch32, NULL, (aint)uiLength);
        uint32_t ui = 0;
        const uint8_t* ucpUnsignedKey = (uint8_t*)cpKey;
        for (; ui < uiLength; ui++) {
            uipKey[ui] = (uint32_t) ucpUnsignedKey[ui];
        }
        json_iterator* spIt = spJsonIteratorCtor(spJson);
        vFindKeyValues(spJson, spIt, uipKey, uiLength, spValue);
        spIt->uiCount = uiVecLen(spIt->vpVec);
        if(spIt->uiCount){
            spIt->sppValues = (json_value**)vpVecFirst(spIt->vpVec);
            vpReturn = (void*)spIt;
        }else{
            vJsonIteratorDtor(spIt);
        }
    }
    return vpReturn;
}
/** \brief Find JSON values with the specified 32-bit Unicode key
 *
 * Search for all values with the specified key.
 *
 * \param vpCtx Pointer to a valid parser context previously returned from  vpJsonCtor.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param uipKey Pointer to the array of key string Unicode code points.
 * \param uiLength The number of code points in the string.
 * \param spValue Pointer to the root node of the sub-tree to search.
 * The given value and all values in the sub-tree below, if any, will be searched.
 * \return A pointer to an [iterator](\ref anchor_json_iterator) for the values with the specified key.
 * NULL if no key matches are found.
 */
void* vpJsonFindKeyU(void *vpCtx, const uint32_t *uipKey, aint uiLength, json_value* spValue) {
    void* vpReturn = NULL;
    json* spJson = (json*) vpCtx;
    if((vpCtx == NULL) || (spJson->vpValidate != s_vpJsonValid)){
        vExContext();
    }
    json_iterator* spIt = spJsonIteratorCtor(spJson);
    vFindKeyValues(spJson, spIt, uipKey, uiLength, spValue);
    spIt->sppValues = (json_value**)vpVecFirst(spIt->vpVec);
    spIt->uiCount = uiVecLen(spIt->vpVec);
    vpReturn = (spIt->sppValues && spIt->uiCount) ? (void*)spIt : NULL;
    return vpReturn;
}

/** \brief Initialize the iterator to walk a value tree.
 *
 * "Walk" refers to a
 * [depth-first traversal](https://en.wikipedia.org/wiki/Tree_traversal#Depth-first_search_of_binary_tree)
 * of the value tree.
 *
 * \param vpCtx Pointer to a valid parser context previously returned from  vpJsonCtor.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spValue Pointer to the start value.
 * The given value and all values in the sub-tree below, if any, will be walked.
 * \return An initialized iterator to the first node.
 * In this case it is the root node or spValue.
 */
void* vpJsonTree(void* vpCtx, json_value* spValue){
    void* vpReturn = NULL;
    json* spJson = (json*) vpCtx;
    if((vpCtx == NULL) || (spJson->vpValidate != s_vpJsonValid)){
        vExContext();
    }
    spJson->uiWalkCount = 0;
    vWalkCount(spJson, spValue);
    if(spJson->uiWalkCount){
        json_iterator* spIt = spJsonIteratorCtor(spJson);
        spIt->sppValues = (json_value**)vpVecPushn(spIt->vpVec, NULL, spJson->uiWalkCount);
        spIt->uiCount = spJson->uiWalkCount;
        aint ui = 0;
        for(; ui < spJson->uiWalkCount; ui++){
            spIt->sppValues[ui] = &spValue[ui];
        }
        vpReturn = (void*)spIt;
    }
    return vpReturn;
}
/** \brief Initialize the iterator over the children of the given value as the parent node.
 *
 * \param vpCtx Pointer to a valid parser context previously returned from  vpJsonCtor.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spValue Pointer to the tree value to use at the root node of the walk.
  \return Initializes an iterator for a horizontal walk of the sibling children of the root value.
  Walks the children left to right.
 * Returns NULL if the value has no children.
 */
void* vpJsonChildren(void* vpCtx, json_value* spValue){
    void* vpReturn = NULL;
    json* spJson = (json*) vpCtx;
    if((vpCtx == NULL) || (spJson->vpValidate != s_vpJsonValid)){
        vExContext();
    }
    if (!spValue) {
        XTHROW(spJson->spException, "value pointer cannot be NULL");
    }
    if(spValue->uiChildCount){
        json_iterator* spIt = spJsonIteratorCtor(spJson);
        spIt->sppValues = (json_value**)vpVecPushn(spIt->vpVec, NULL, spValue->uiChildCount);
        spIt->uiCount = spValue->uiChildCount;
        aint ui = 0;
        for(; ui < spValue->uiChildCount; ui++){
            spIt->sppValues[ui] = spValue->sppChildren[ui];
        }
        vpReturn = (void*)spIt;
    }
    return vpReturn;
}

/** \brief Private function for internal object use only. Never called by the application.
 *
 * JSON iterators are constructed internally by the JSON object.
 * These functions will return iterators.
 *  - vpJsonFindKeyA()
 *  - vpJsonFindKeyU()
 *  - vpJsonTree()
 *  - vpJsonChildren()
 */
json_iterator* spJsonIteratorCtor(json* spJson){
    json_iterator* spIt = (json_iterator*)vpMemAlloc(spJson->vpMem, sizeof(json_iterator));
    memset(spIt, 0, sizeof(json_iterator));
    spIt->vpVec = vpVecCtor(spJson->vpMem, sizeof(json_value*), 512);
    spIt->spJson = spJson;
    spIt->uiContextIndex = uiVecLen(spJson->vpVecIterators);
    vpVecPush(spJson->vpVecIterators, &spIt);
    spIt->vpValidate = s_vpIteratorValid;
    return spIt;
}
/** \brief The JSON iterator destructor.
 *
 * Can be called to clean up all memory allocations used by this iterator.
 * However, the JSON destructor \ref vJsonDtor() will free all memory associated with the JSON object,
 * including all iterators and builders.
 *
 *  \anchor anchor_json_itgen
 * \param vpIteratorCtx A valid context pointer returned from any of the iterator-generating functions.
 *  - vpJsonReadFile()
 *  - vpJsonReadArray()
 *  - vpJsonFindKeyA()
 *  - vpJsonFindKeyU()
 *  - vpJsonTree()
 *  - vpJsonChildren()
 *
 * NULL is silently ignored. However, if non-NULL must be valid.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 *
 * \return void. Frees all memory associated with this iterator object.
 */
void vJsonIteratorDtor(void* vpIteratorCtx){
    json_iterator* spIt = (json_iterator*)vpIteratorCtx;
    if(vpIteratorCtx){
        if(spIt->vpValidate == s_vpIteratorValid){
            void** vppContext = (void**)vpVecFirst(spIt->spJson->vpVecIterators);
            vppContext[spIt->uiContextIndex] = NULL;
            void* vpMem = spIt->spJson->vpMem;
            vVecDtor(spIt->vpVec);
            memset(vpIteratorCtx, 0, sizeof(json_iterator));
            vMemFree(vpMem, vpIteratorCtx);
        }else{
            vExContext();
        }
    }
}

/** \brief Find the first value in the list represented by this iterator.
 *
 * \param vpIteratorCtx A valid context pointer returned from any of the [iterator-generating](\ref anchor_json_itgen) functions.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return A pointer to the first value in the list.
 */
json_value* spJsonIteratorFirst(void* vpIteratorCtx){
    json_iterator* spIt = (json_iterator*)vpIteratorCtx;
    if((vpIteratorCtx == NULL) || (spIt->vpValidate != s_vpIteratorValid)){
        vExContext();
    }
    spIt->uiCurrent = 0;
    return *spIt->sppValues;
}
/** \brief Find the last value in the list represented by this iterator.
 *
 * \param vpIteratorCtx A valid context pointer returned from any of the [iterator-generating](\ref anchor_json_itgen) functions.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return A pointer to the last value in the list.
 */
json_value* spJsonIteratorLast(void* vpIteratorCtx){
    json_value* spReturn = NULL;
    json_iterator* spIt = (json_iterator*)vpIteratorCtx;
    if((vpIteratorCtx == NULL) || (spIt->vpValidate != s_vpIteratorValid)){
        vExContext();
    }
    if(spIt->uiCount){
        spIt->uiCurrent = spIt->uiCount - 1;
        spReturn = spIt->sppValues[spIt->uiCurrent];
    }
    return spReturn;
}
/** \brief Find the next value in the list represented by this iterator.
 *
 * It should be preceded by a call to spJsonIteratorFirst()
 *
 * \param vpIteratorCtx A valid context pointer returned from any of the [iterator-generating](\ref anchor_json_itgen) functions.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return A pointer to the next value in the list. NULL if there are no more values in the list.
 */
json_value* spJsonIteratorNext(void* vpIteratorCtx){
    json_value* spReturn = NULL;
    json_iterator* spIt = (json_iterator*)vpIteratorCtx;
    if((vpIteratorCtx == NULL) || (spIt->vpValidate != s_vpIteratorValid)){
        vExContext();
    }
    if(spIt->uiCount){
        if(spIt->uiCurrent < spIt->uiCount){
            spIt->uiCurrent++;
            if(spIt->uiCurrent < spIt->uiCount){
                spReturn = spIt->sppValues[spIt->uiCurrent];
            }
        }
    }
    return spReturn;
}
/** \brief Find the prev value in the list represented by this iterator.
 *
 * It should be preceded by a call to spJsonIteratorNext()
 *
 * \param vpIteratorCtx A valid context pointer returned from any of the [iterator-generating](\ref anchor_json_itgen) functions.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return A pointer to the previous value in the list. NULL if the current value is the first value in the list.
 */
json_value* spJsonIteratorPrev(void* vpIteratorCtx){
    json_value* spReturn = NULL;
    json_iterator* spIt = (json_iterator*)vpIteratorCtx;
    if((vpIteratorCtx == NULL) || (spIt->vpValidate != s_vpIteratorValid)){
        vExContext();
    }
    if(spIt->uiCount){
        if(spIt->uiCurrent > 0){
            spIt->uiCurrent--;
            spReturn = spIt->sppValues[spIt->uiCurrent];
        }
    }
    return spReturn;
}
/** \brief Find the number of values in the list represented by this iterator.
 *
 * \param vpIteratorCtx A valid context pointer returned from any of the [iterator-generating](\ref anchor_json_itgen) functions.
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return The number of values in the list.
 */
aint uiJsonIteratorCount(void* vpIteratorCtx){
    json_iterator* spIt = (json_iterator*)vpIteratorCtx;
    if((vpIteratorCtx == NULL) || (spIt->vpValidate != s_vpIteratorValid)){
        vExContext();
    }
    return spIt->uiCount;
}

/**
 * \brief Parse the JSON-text byte stream into a tree of json_value nodes.
 *
 * The parser is built for reuse without memory leaks. All previously parsed values and related data, if any, are released
 * and re-initialized. An APG parser for the JSON SABNF grammar is constructed and used to parse
 * the JSON byte stream into an array of values.
 *
 * \param vpCtx pointer to the parser's context - previously returned from  vpJsonCtor.
 * \param cpTraceOut - A file name for the output of the parser's trace. If NULL, no trace is done.
 * \return void - throws exception on error
 */
static void vParseInput(json* spJson) {
    parser_state sState;
    parser_config sInput;
    aint uiCharCount;
    aint ui;
    uint8_t* ucpChars;

    // clear previous parsing data, if any
    spJson->spCurrentFrame = NULL;
    spJson->uiValueCount = 0;
    spJson->uiStringCount = 0;
    spJson->uiChar = 0;
    spJson->bHasFrac = APG_FALSE;
    spJson->bHasMinus = APG_FALSE;
    if(spJson->vpParser){
        vParserDtor(spJson->vpParser);
        spJson->vpParser = NULL;
    }
    if(spJson->acpInput){
        vMemFree(spJson->vpMem, spJson->acpInput);
        spJson->acpInput = NULL;
    }
    ucpChars = (uint8_t*) vpVecFirst(spJson->vpVecInput);
    uiCharCount = uiVecLen(spJson->vpVecInput);
    if (!uiCharCount) {
        XTHROW(spJson->spException, "JSON text is empty");
    }
    // construct the parser
    if(spJson->vpParser){
        vParserDtor(spJson->vpParser);
    }
    spJson->vpParser = vpParserCtor(spJson->spException, vpJsonGrammarInit);
//    if (spJson->cpTraceOut) {
//        // attach the trace - no trace if NULL
//        void* vpTrace = vpTraceCtor(spJson->vpParser);
//        if(spJson->cpTraceOut[0] != 0){
//            // use the specified file name
//            vTraceSetOutput(vpTrace, spJson->cpTraceOut);
//        }
//    }

    // set the rule callback functions
    vJsonGrammarRuleCallbacks(spJson->vpParser);

    // configure the parser
    spJson->acpInput = (achar*) vpMemAlloc(spJson->vpMem, ((aint) sizeof(achar) * uiCharCount));
    for (ui = 0; ui < uiCharCount; ui++) {
        // NOTE: since the input is a byte stream and sizeof(achar) >= 1 always, no data truncation is possible here
        spJson->acpInput[ui] = (achar) ucpChars[ui];
    }
    memset((void*) &sInput, 0, sizeof(sInput));
    sInput.acpInput = spJson->acpInput;
    sInput.uiInputLength = uiCharCount;
    sInput.uiStartRule = 0;
    sInput.vpUserData = (void*) spJson;
    sInput.bParseSubString = APG_FALSE;

    // parse the JSON text
    vParserParse(spJson->vpParser, &sInput, &sState);
    if (!sState.uiSuccess) {
        // display the parser state
        vUtilPrintParserState(&sState);
        XTHROW(spJson->spException, "JSON parser failed");
    }
    vParserDtor(spJson->vpParser); // destroys vpTrace, if not NULL
    spJson->vpParser = NULL;
    vMemFree(spJson->vpMem, spJson->acpInput);
    spJson->acpInput = NULL;
}

// static functions
static abool bKeyComp(u32_phrase* spKey, const uint32_t *uipKey, uint32_t uiLength){
    abool bReturn = APG_FALSE;
    if(spKey->uiLength == uiLength){
        uint32_t ui = 0;
        for(; ui < uiLength; ui++){
            if(spKey->uipPhrase[ui] != uipKey[ui]){
                goto fail;
            }
        }
        bReturn = APG_TRUE;
    }
    fail:;
    return bReturn;
}
static void vFindKeyValues(json* spJson, json_iterator* spIt, const uint32_t *uipKey, uint32_t uiLength, json_value* spValue){
    if(spValue->spKey && bKeyComp(spValue->spKey, uipKey, uiLength)){
        vpVecPush(spIt->vpVec, &spValue);
    }
    if ((spValue->uiId == JSON_ID_OBJECT) || (spValue->uiId == JSON_ID_ARRAY)) {
        json_value** sppChildren = spValue->sppChildren;
        json_value** sppEnd = sppChildren + spValue->uiChildCount;
        for (; sppChildren < sppEnd; sppChildren++) {
            vFindKeyValues(spJson, spIt, uipKey, uiLength, *sppChildren);
        }
    }
}
static void vIndent(aint uiIndent) {
    aint ui = 0;
    for (; ui < uiIndent; ui++) {
        printf(" ");
    }
}
static void vReintrantValueDisplay(json *spJson, struct json_value_tag *spValue) {
    spJson->uiCurrentDepth++;
    if (spJson->uiCurrentDepth > spJson->uiMaxDepth) {
        return;
    }
    char caBuf[128];
    aint uiIndent = 2 * (spJson->uiCurrentDepth - 1);
    u32_phrase* spString;
    json_number* spNumber;
    aint uiChildCount;
    struct json_value_tag** sppChildren;
    aint ui;
    const char* cpNextLine;
    char* cpStr;
    if (spValue->spKey) {
        vIndent(uiIndent);
        if (bIsPhrase32Ascii(spValue->spKey)) {
            cpStr = (char*)vpVecPushn(spJson->vpVecAscii, NULL, spValue->spKey->uiLength);
            cpPhrase32ToStr(spValue->spKey, cpStr);
            printf("key: \"%s\"\n", cpStr);
            vVecClear(spJson->vpVecAscii);
        } else {
            printf("key: (some or all not printable ASCII)\n");
            vFmtIndent(spJson->vpFmt, (int)uiIndent);
            cpNextLine = cpFmtFirstUnicode(spJson->vpFmt, spValue->spKey->uipPhrase, (long int)spValue->spKey->uiLength, 0, 0);
            while(cpNextLine){
                printf("%s", cpNextLine);
                cpNextLine = cpFmtNext(spJson->vpFmt);
            }
        }
    }
    switch (spValue->uiId) {
    case JSON_ID_ARRAY:
        uiChildCount = spValue->uiChildCount;
        sppChildren = spValue->sppChildren;
        vIndent(uiIndent);
        printf("[ values: %"PRIuMAX"\n", (luint) uiChildCount);
        for (ui = 0; ui < uiChildCount; ui++, sppChildren++) {
            vReintrantValueDisplay(spJson, *sppChildren);
            spJson->uiCurrentDepth--;
        }
        vIndent(uiIndent);
        printf("] values: %"PRIuMAX"\n", (luint) uiChildCount);
        break;
    case JSON_ID_OBJECT:
        uiChildCount = spValue->uiChildCount;
        sppChildren = spValue->sppChildren;
        vIndent(uiIndent);
        printf("{ values: %"PRIuMAX"\n", (luint) uiChildCount);
        for (ui = 0; ui < uiChildCount; ui++, sppChildren++) {
            vReintrantValueDisplay(spJson, *sppChildren);
            spJson->uiCurrentDepth--;
        }
        vIndent(uiIndent);
        printf("} values: %"PRIuMAX"\n", (luint) uiChildCount);
        break;
    case JSON_ID_STRING:
        spString = spValue->spString;
        vIndent(uiIndent);
        if (bIsPhrase32Ascii(spString)) {
            char* cpStr = (char*)vpVecPushn(spJson->vpVecAscii, NULL, (spString->uiLength + 1));
            cpPhrase32ToStr(spString, cpStr);
            printf("string: \"%s\"\n", cpStr);
            vVecClear(spJson->vpVecAscii);
        } else {
            printf("string: (some or all not printable ASCII)\n");
            vFmtIndent(spJson->vpFmt, (int)uiIndent);
            cpNextLine = cpFmtFirstUnicode(spJson->vpFmt, spString->uipPhrase, (long int)spString->uiLength, 0, 0);
            while(cpNextLine){
                printf("%s", cpNextLine);
                cpNextLine = cpFmtNext(spJson->vpFmt);
            }
        }
        break;
    case JSON_ID_NUMBER:
        spNumber = spValue->spNumber;
        vIndent(uiIndent);
        switch (spNumber->uiType) {
        case JSON_ID_UNSIGNED:
            printf("number: unsigned int: %"PRIuMAX"\n", (luint) spNumber->uiUnsigned);
            break;
        case JSON_ID_SIGNED:
            printf("number: signed int: %"PRIiMAX"\n", (intmax_t) spNumber->iSigned);
            break;
        case JSON_ID_FLOAT:
            printf("number: float: %g\n", spNumber->dFloat);
            break;
        }
        break;
    case JSON_ID_TRUE:
        vIndent(uiIndent);
        printf("literal: %s\n", s_cpTrue);
        break;
    case JSON_ID_FALSE:
        vIndent(uiIndent);
        printf("literal: %s\n", s_cpFalse);
        break;
    case JSON_ID_NULL:
        vIndent(uiIndent);
        printf("literal: %s\n", s_cpNull);
        break;
    default:
        snprintf(caBuf, 128, "unknown record type ID: %"PRIuMAX"", (luint) spValue->uiId);
        XTHROW(spJson->spException, caBuf);
    }
}

static void vBreakIndent(json* spJson, aint uiIndent) {
    aint ui;
    vpVecPush(spJson->vpVecOutput, &s_uiLineEnd);
    if (uiIndent) {
        for (ui = 0; ui < uiIndent; ui++) {
            vpVecPush(spJson->vpVecOutput, &s_uiSpace);
        }
    }
}
static void vPushObject(json* spJson, json_value* spValue, abool bArray) {
    uint32_t uiOpen = bArray ? s_uiOpenSquare : s_uiOpenCurly;
    uint32_t uiClose = bArray ? s_uiCloseSquare : s_uiCloseCurly;
    aint ui;
    vpVecPush(spJson->vpVecOutput, &uiOpen);
    spJson->uiCurrentDepth += 2;
    vBreakIndent(spJson, spJson->uiCurrentDepth);
    for (ui = 0; ui < spValue->uiChildCount; ui++) {
        if (ui) {
            vpVecPush(spJson->vpVecOutput, &s_uiComma);
            vBreakIndent(spJson, spJson->uiCurrentDepth);
        }
        vPushValue(spJson, spValue->sppChildren[ui]);
    }
    spJson->uiCurrentDepth -= 2;
    vBreakIndent(spJson, spJson->uiCurrentDepth);
    vpVecPush(spJson->vpVecOutput, &uiClose);
}
static void vPushString(json* spJson, u32_phrase* spString) {
    aint ui;
    char caBuf[64];
    vpVecPush(spJson->vpVecOutput, &s_uiQuote);
    uint32_t uiChar;
    for (ui = 0; ui < spString->uiLength; ui++) {
        uiChar = spString->uipPhrase[ui];
        if(uiChar == 0x5C){
            // escape reverse solidus
            vpVecPush(spJson->vpVecOutput, &uiChar);
            vpVecPush(spJson->vpVecOutput, &uiChar);
        }else if(uiChar == 0x22){
            // escape quote
            vpVecPush(spJson->vpVecOutput, &s_uiReverseSolidus);
            vpVecPush(spJson->vpVecOutput, &uiChar);
        }else if(uiChar >= 0 && uiChar < 0x20){
            // escape control characters
            vpVecPush(spJson->vpVecOutput, &s_uiReverseSolidus);
            vpVecPush(spJson->vpVecOutput, &s_uiLowerU);
            snprintf(caBuf, 64, "%04X", uiChar);
            uiChar = (uint32_t)caBuf[0];
            vpVecPush(spJson->vpVecOutput, &uiChar);
            uiChar = (uint32_t)caBuf[1];
            vpVecPush(spJson->vpVecOutput, &uiChar);
            uiChar = (uint32_t)caBuf[2];
            vpVecPush(spJson->vpVecOutput, &uiChar);
            uiChar = (uint32_t)caBuf[3];
            vpVecPush(spJson->vpVecOutput, &uiChar);
        }else if(uiChar >= 0xD800 && uiChar < 0xDFFF){
            // escape control characters
            XTHROW(spJson->spException, "string has code point value in surrogate pair range ([0xD800 - 0xDFFF])");
        }else if(uiChar > 0x10FFFF){
            // escape control characters
            XTHROW(spJson->spException, "string has code point out of range (<0x10FFFF)");
        }else{
            vpVecPush(spJson->vpVecOutput, &uiChar);
        }
    }
    vpVecPush(spJson->vpVecOutput, &s_uiQuote);
}
static void vPushNumber(json* spJson, json_number* spNumber) {
    aint uiLen;
    char caBuf[64];
    uint32_t uiBuf[64];
    if (spNumber->uiType == JSON_ID_FLOAT) {
        snprintf(caBuf, 64, "%.16G", spNumber->dFloat);
    } else if (spNumber->uiType == JSON_ID_UNSIGNED) {
        snprintf(caBuf, 64, "%"PRIuMAX"", spNumber->uiUnsigned);
    } else if (spNumber->uiType == JSON_ID_SIGNED) {
        snprintf(caBuf, 64, "%"PRIiMAX"", spNumber->iSigned);
    }
    uint32_t* uip = uiBuf;
    char* cp = caBuf;
    uiLen = 0;
    while (*cp) {
        *uip++ = (uint32_t) *cp++;
        uiLen++;
    }
    vpVecPushn(spJson->vpVecOutput, uiBuf, uiLen);
}

static void vPushValue(json* spJson, json_value* spValue) {
    if(spJson->bFirstNode){
        spJson->bFirstNode = APG_FALSE;
    }else{
        if (spValue->spKey) {
            vPushString(spJson, spValue->spKey);
            vpVecPush(spJson->vpVecOutput, &s_uiColon);
            vpVecPush(spJson->vpVecOutput, &s_uiSpace);
        }
    }
    if (spValue->uiId == JSON_ID_OBJECT) {
        vPushObject(spJson, spValue, APG_FALSE);
    } else if (spValue->uiId == JSON_ID_ARRAY) {
        vPushObject(spJson, spValue, APG_TRUE);
    } else {
        switch (spValue->uiId) {
        case JSON_ID_STRING:
            vPushString(spJson, spValue->spString);
            break;
        case JSON_ID_NUMBER:
            vPushNumber(spJson, spValue->spNumber);
            break;
        case JSON_ID_TRUE:
            vpVecPushn(spJson->vpVecOutput, s_uiaTrue, s_uiTrueLen);
            break;
        case JSON_ID_FALSE:
            vpVecPushn(spJson->vpVecOutput, s_uiaFalse, s_uiFalseLen);
            break;
        case JSON_ID_NULL:
            vpVecPushn(spJson->vpVecOutput, s_uiaNull, s_uiNullLen);
            break;
        default:
            XTHROW(spJson->spException, "unrecognized value type");
            break;
        }
    }
}
static void vWalkCount(json* spJson, json_value* spValue){
    aint ui;
    switch(spValue->uiId){
    case JSON_ID_STRING:
    case JSON_ID_NUMBER:
    case JSON_ID_TRUE:
    case JSON_ID_FALSE:
    case JSON_ID_NULL:
        spJson->uiWalkCount++;
        break;
    case JSON_ID_OBJECT:
    case JSON_ID_ARRAY:
        spJson->uiWalkCount++;
        for(ui = 0; ui < spValue->uiChildCount; ui++){
            vWalkCount(spJson, spValue->sppChildren[ui]);
        }
        break;
    default:
        XTHROW(spJson->spException, "tree walk sanity check: unrecognized value id");
        break;
    }
}

