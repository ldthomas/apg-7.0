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
/** \file library/tools.c
 * \brief A few commonly used functions.
 */

#include "lib.h"

static char s_cPeriod = 46;

    /** \brief Compare two strings. A case-insensitive version of strcmp().
 *
 * Two strings are considered equal if
 *  - all characters are case-insensitive equal
 *    - that is equal when both are converted to lower case
 *  - if both are NULL
 *  - if both are empty.
 *
 * \param cpLeft Pointer to the left string.
 * \param cpRight Pointer to the right string.
 * \return 0 if the two strings are case-insensitively the equal<br>
 *  -  0 indicates that the two strings are equal
 *  - -1 indicates that cpLeft is alphabetically lower than cpRight
 *  - +1 indicates that cpLeft is alphabetically higher than cpRight
 */
int iStriCmp(const char* cpLeft, const char* cpRight){
    if(cpLeft == NULL && cpRight == NULL){
        return 0;
    }
    if(cpLeft == NULL){
        return -1;
    }
    if(cpRight == NULL){
        return 1;
    }
    size_t uiLeftLen = strlen(cpLeft);
    size_t uiRightLen = strlen(cpRight);
    size_t uiLen = uiLeftLen < uiRightLen ? uiLeftLen : uiRightLen;
    size_t ui = 0;
    unsigned char ucL, ucR;
    for(; ui < uiLen; ui++){
        ucL = cpLeft[ui];
        ucR = cpRight[ui];
        if(ucL >= 65 && ucL <= 90){
            ucL += 32;
        }
        if(ucR >= 65 && ucR <= 90){
            ucR += 32;
        }
        if(ucL < ucR){
            return -1;
        }
        if(ucL > ucR){
            return 1;
        }
    }
    if(uiLeftLen < uiRightLen){
        return -1;
    }
    if(uiLeftLen > uiRightLen){
        return 1;
    }
    return 0;
}

/** \brief [Determine](https://stackoverflow.com/questions/1001307/detecting-endianness-programmatically-in-a-c-program)
 *  if the current machine uses big endian word storage.
 */
abool bIsBigEndian(void){
    static union {
        uint32_t ui32;
        char caChar[4];
    } sIsBig = {0x01020304};
    return (abool)(sIsBig.caChar[0] == 1);
}

/** \brief Multiply two long unsigned integers with overflow notification.
 * \param luiL left factor
 * \param luiR right factor
 * \param luipA pointer to the product
 * \return APG_FAILURE if the multiplication results in integer overflow, APG_SUCCESS otherwise.
 */
abool bMultiplyLong(luint luiL, luint luiR, luint* luipA){
    if(luiL == 0 || luiR == 0){
        *luipA = 0;
        return APG_SUCCESS;
    }
    luint luiTest = luiL * luiR;
    luint luiCheck = luiTest / luiR;
    if(luiCheck != luiL){
        *luipA = (luint)-1;
        return APG_FAILURE;
    }
    *luipA = luiTest;
    return APG_SUCCESS;
}

/** \brief Add two long unsigned integers with overflow notification.
 * \param uiL left term
 * \param uiR right term
 * \param uipA pointer to the sum
 * \return APG_FAILURE if the addition results in integer overflow, APG_SUCCESS otherwise.
 */
abool bSumLong(luint uiL, luint uiR, luint* uipA){
    luint uiTest = uiL + uiR;
    if(uiTest < uiR){
        return APG_FAILURE;
    }
    luint uiCheck = uiTest - uiR;
    if(uiCheck != uiL){
        return APG_FAILURE;
    }
    *uipA = uiTest;
    return APG_SUCCESS;
}

/** \brief Multiply two 32-bit unsigned integers with overflow notification.
 * \param uiL left factor
 * \param uiR right factor
 * \param uipA pointer to the product
 * \return APG_FAILURE if the multiplication results in integer overflow, APG_SUCCESS otherwise.
 */
abool bMultiply32(uint32_t uiL, uint32_t uiR, uint32_t* uipA){
    if(uiL == 0 || uiR == 0){
        *uipA = 0;
        return APG_SUCCESS;
    }
    uint32_t uiTest = uiL * uiR;
    uint32_t uiCheck = uiTest / uiR;
    if(uiCheck != uiL){
        return APG_FAILURE;
    }
    *uipA = uiTest;
    return APG_SUCCESS;
}

/** \brief Sum two 32-bit unsigned integers with overflow notification.
 * \param uiL left term
 * \param uiR right term
 * \param uipA pointer to the sum
 * \return APG_FAILURE if the addition results in integer overflow, APG_SUCCESS otherwise.
 */
abool bSum32(uint32_t uiL, uint32_t uiR, uint32_t* uipA){
    uint32_t uiTest = uiL + uiR;
    if(uiTest < uiR){
        return APG_FAILURE;
    }
    uint32_t uiCheck = uiTest - uiR;
    if(uiCheck != uiL){
        return APG_FAILURE;
    }
    *uipA = uiTest;
    return APG_SUCCESS;
}

/** \brief Multiply two APG unsigned integers with overflow notification.
 * \param uiL left factor
 * \param uiR right factor
 * \param uipA pointer to the product
 * \return APG_FAILURE if the multiplication results in integer overflow, APG_SUCCESS otherwise.
 */
abool bMultiply(aint uiL, aint uiR, aint* uipA){
    if(uiL == 0 || uiR == 0){
        *uipA = 0;
        return APG_SUCCESS;
    }
    aint uiTest = uiL * uiR;
    aint uiCheck = uiTest / uiR;
    if(uiCheck != uiL){
        return APG_FAILURE;
    }
    *uipA = uiTest;
    return APG_SUCCESS;
}

/** \brief Sum two unsigned integers with overflow notification.
 * \param uiL left term
 * \param uiR right term
 * \param uipA pointer to the sum
 * \return APG_FAILURE if the addition results in integer overflow, APG_SUCCESS otherwise.
 */
abool bSum(aint uiL, aint uiR, aint* uipA){
    aint uiTest = uiL + uiR;
    if(uiTest < uiR){
        return APG_FAILURE;
    }
    aint uiCheck = uiTest - uiR;
    if(uiCheck != uiL){
        return APG_FAILURE;
    }
    *uipA = uiTest;
    return APG_SUCCESS;
}

/** \brief Convert an ASCII, null-terminated string to an `achar` phrase.
 *
 * \param cpStr Pointer to the string to convert.
 * \param spPhrase Pointer to a phrase.
 *  - spPhrase->acpPhrase = an empty array of `achar` characters
 *  - spPhrase->uiLength = the length of the array
 * \return On success, returns a pointer to the input spPhrase with
 *  - spPhrase->acpPhrase = pointer to the converted to `achar` characters
 *  - spPhrase->uiLength = strlen(cpStr), the number of converted characters
 *
 * On failure returns NULL. However,
 *  - spPhrase->uiLength = strlen(cpStr), the number of converted characters
 *
 * This can be used to adjust the buffer size for successful conversion.
 */
apg_phrase* spStrToPhrase(const char* cpStr, apg_phrase* spPhrase){
    if(!spPhrase || !cpStr){
        return NULL;
    }
    aint uiStrLen = strlen(cpStr);
    aint uiBufLen = spPhrase->uiLength;
    aint uiLen = uiStrLen < uiBufLen ? uiStrLen : uiBufLen;
    aint ui = 0;
    achar* acpTmp = (achar*)spPhrase->acpPhrase;
    for(; ui < uiLen; ui++){
//        spPhrase->acpPhrase[ui] = (achar)((uint8_t)cpStr[ui]);
        acpTmp[ui] = (achar)((uint8_t)cpStr[ui]);
    }
    spPhrase->uiLength = uiStrLen;
    return spPhrase;
}

/** \brief Convert a phrase of `achar` characters to a null-terminated ASCII string.
 *
 * Any `achar` characters that are not valid printing ASCII characters will be represented with a period ('.' or 46).
 *
 * \param spPhrase Pointer to the apg_phrase to convert. May not be NULL or have a NULL array pointer.
 * \param cpStr Pointer to the buffer of characters to receive the string.
 * It is the caller's responsibility to ensure that cpStr points to an array of at least (spPhrase->uiLength + 1) characters.
 * Otherwise buffer overrun will occur.
 * \return Returns a pointer to cpStr, the caller's string buffer.
 */
char* cpPhraseToStr(apg_phrase* spPhrase, char* cpStr){
    aint ui = 0;
    achar acChar;
    for(; ui < spPhrase->uiLength; ui++){
        acChar = spPhrase->acpPhrase[ui];
        if(acChar == 9 || acChar == 10 || acChar == 13 || (acChar >= 32 && acChar <= 126)){
            cpStr[ui] = (char)acChar;
        }else{
            cpStr[ui] = s_cPeriod;
        }
    }
    cpStr[ui] = (char)0;
    return cpStr;
}

/** \brief Determine if a phrase consists entirely of printable ASCII characters.
 *
 * When displaying a phrase it is often convenient to use the C-language string functions
 * if possible. This function will tell when that is possible.
 *
 * \param spPhrase The phrase to check. It may not be NULL or have a NULL character pointer.
 * \return True if the string consists entirely of printable ASCII characters. False otherwise.
 */
abool bIsPhraseAscii(apg_phrase* spPhrase){
    aint ui = 0;
    achar acChar;
    if(!spPhrase || !spPhrase->acpPhrase || !spPhrase->uiLength){
        return APG_FALSE;
    }
    for(; ui < spPhrase->uiLength; ui++){
        acChar = spPhrase->acpPhrase[ui];
        if(!(acChar == 9 || acChar == 10 || acChar == 13 || (acChar >= 32 && acChar <= 126))){
            return APG_FALSE;
        }
    }
    return APG_TRUE;
}

/** \brief Convert an ASCII, null-terminated string to an 32-bit unsigned integer array.

 * \param cpStr Pointer to the string to convert. May not be NULL.
 * \param uipBuf Pointer to an 32-bit unsigned integer array to receive the converted string characters. May not be NULL.
 * It is the caller's responsibility to ensure that the array is at least strlen(cpStr) in length.
 * Otherwise buffer overrun will result.
 * \param uipLen Pointer to an unsigned integer to receive the number of characters in the ucpBuf array.
 * May be NULL if no returned length is needed.
 * \return Returns a pointer to the caller's 32-bit unsigned integer array.
 * Returns the number of integers in the array if uipLen != NULL.
 */
uint32_t* uipStrToUint32(const char* cpStr, uint32_t* uipBuf, aint* uipLen){
    aint uiStrLen = strlen(cpStr);
    aint ui = 0;
    for(; ui < uiStrLen; ui++){
        uipBuf[ui] = (uint32_t)((uint8_t)cpStr[ui]);
    }
    if(uipLen){
        *uipLen = uiStrLen;
    }
    return uipBuf;
}

/** \brief Convert an array of 32-bit unsigned integers to a null-terminated ASCII string.
 *
 * Any 32-bit unsigned integers that are not valid printing ASCII characters will be represented with a period ('.' or 46).
 *
 * \param uipBuf Pointer to an 32-bit unsigned integer array to convert. May not be NULL.
 * \param uiLen The number of characters in the uipBuf array.
 * \param cpStr Pointer to the buffer of characters to receive the string.
 * It is the caller's responsibility to ensure that cpStr points to an array of at least (uiLen + 1) characters.
 * Otherwise buffer overrun will occur.
 * \return Returns a pointer to cpStr, the caller's string buffer.
 */
char* cpUint32ToStr(uint32_t* uipBuf, aint uiLen, char* cpStr){
    aint ui = 0;
    uint32_t uiChar;
    for(; ui < uiLen; ui++){
        uiChar = uipBuf[ui];
        if(uiChar == 9 || uiChar == 10 || uiChar == 13 || (uiChar >= 32 && uiChar <= 126)){
            cpStr[ui] = (char)uiChar;
        }else{
            cpStr[ui] = s_cPeriod;
        }
    }
    cpStr[ui] = (char)0;
    return cpStr;
}

/** \brief Convert an ASCII, null-terminated string to a 32-bit phrase.
 *
 * \param cpStr Pointer to the string to convert. May not be NULL.
 * \param uipBuf Pointer to the 32-bit array to receive the converted string characters. May not be NULL.
 * It is the caller's responsibility to ensure that the array is at least `strlen(cpStr)` in length.
 * Otherwise buffer overrun will result.
 * \return Returns a u32_phrase structure.
 * NOTE: Returns a structure, not a pointer to a structure.
 */
u32_phrase sStrToPhrase32(const char* cpStr, uint32_t* uipBuf){
    u32_phrase sPhrase = {0, 0};
    aint uiStrLen = strlen(cpStr);
    aint ui = 0;
    for(; ui < uiStrLen; ui++){
        uipBuf[ui] = (uint32_t)((uint8_t)cpStr[ui]);
    }
    sPhrase.uipPhrase = uipBuf;
    sPhrase.uiLength = uiStrLen;
    return sPhrase;
}

/** \brief Convert a 32-bit phrase to a null-terminated ASCII string.
 *
 * Any 32-bit integers that are not valid printing ASCII characters will be represented with a period ('.' or 46).
 *
 * \param spPhrase Pointer to the u32_phrase to convert. May not be NULL or have a NULL array pointer.
 * \param cpStr Pointer to the buffer of characters to receive the string.
 * It is the caller's responsibility to ensure that cpStr points to an array of at least (spPhrase->uiLength + 1) characters.
 * Otherwise buffer overrun will occur.
 * \return Returns a pointer to cpStr, the caller's string buffer.
 */
char* cpPhrase32ToStr(u32_phrase* spPhrase, char* cpStr){
    aint ui = 0;
    uint32_t uiChar;
    for(; ui < spPhrase->uiLength; ui++){
        uiChar = spPhrase->uipPhrase[ui];
        if(uiChar == 9 || uiChar == 10 || uiChar == 13 || (uiChar >= 32 && uiChar <= 126)){
            cpStr[ui] = (char)uiChar;
        }else{
            cpStr[ui] = s_cPeriod;
        }
    }
    cpStr[ui] = (char)0;
    return cpStr;
}

/** \brief Determine if a 32-bit phrase consists entirely of printable ASCII characters.
 *
 * When displaying a phrase it is often convenient to use the C-language string functions
 * if possible. This function will tell when that is possible.
 *
 * \param spPhrase The 32-bit phrase to check. It may not be NULL or have a NULL character pointer.
 * \return True if the string consists entirely of printable ASCII characters. False otherwise.
 */
abool bIsPhrase32Ascii(u32_phrase* spPhrase){
    aint ui = 0;
    uint32_t uiChar;
    for(; ui < spPhrase->uiLength; ui++){
        uiChar = spPhrase->uipPhrase[ui];
        if(!(uiChar == 9 || uiChar == 10 || uiChar == 13 || (uiChar >= 32 && uiChar <= 126))){
            return APG_FALSE;
        }
    }
    return APG_TRUE;
}
