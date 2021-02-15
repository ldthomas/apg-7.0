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
#ifndef TOOLS_H_
#define TOOLS_H_

/** \file library/tools.h
 * \brief A few simple, commonly used functions.
 */

int iStriCmp(const char* cpStrl, const char* cpStrr);
abool bIsBigEndian(void);

/** @name Multiply and Add with Overflow Protection
 * Occasionally, it is necessary to implement overflow protection on multiplication and summation.
 * For example, ABNF defines a character with the syntax %%xHHHHH....
 * There is nothing in the ABNF syntax that limits the number of digits.
 * Therefore, it is the syntax parser's job to check that the character does not overflow the computer's integer length.
 * This is a set of such functions specialized for various integer types.
 */
///@{
abool bMultiplyLong(luint luiL, luint luiR, luint* luipA);
abool bSumLong(luint uiL, luint uiR, luint* uipA);
abool bMultiply32(uint32_t uiL, uint32_t uiR, uint32_t* uipA);
abool bSum32(uint32_t uiL, uint32_t uiR, uint32_t* uipA);
abool bMultiply(aint uiL, aint uiR, aint* uipA);
abool bSum(aint uiL, aint uiR, aint* uipA);
///@}

/** @name Alphabet Character Arrays and Phrases
 * ABNF is syntax for defining phrases, a phrase being an array of alphabet characters.
 * The size of the alphabet character integers is configurable in apg.h and, in general,
 * a phrase does not lend itself well to the null-terminated string convention.
 * For this reason, a phrase structure, \ref apg_phrase, has been defined.
 * This is a suite of functions for converting strings to phrases and vice versa.
 * See also the utility functions
 * - acpUtilStrToAchar()
 * - cpUtilAcharToStr()
 * - spUtilStrToPhrase()
 * - cpUtilPhraseToStr()
 */
///@{
apg_phrase* spStrToPhrase(const char* cpStr, apg_phrase* spPhrase);
char* cpPhraseToStr(apg_phrase* spPhrase, char* cpStr);
abool bIsPhraseAscii(apg_phrase* spPhrase);
///@}

/** @name Unicode Character Arrays and Phrases
 * The special case of Unicode characters is common enough to warrant a special set of phrase conversion functions.
 * The Unicode phrase, \ref u32_phrase, is analogous to the alphabet character phrase, \ref apg_phrase.
 * This suite of functions is likewise analogous to the apg_phrase functions.
 * See also the utility functions
  - uipUtilStrToUint32()
  - cpUtilUint32ToStr()
  - spUtilStrToPhrase32()
  - cpUtilPhrase32ToStr()
 */
///@{
uint32_t* uipStrToUint32(const char* cpStr, uint32_t* uipBuf, aint* uipLen);
char* cpUint32ToStr(uint32_t* uipBuf, aint uiLen, char* cpStr);
u32_phrase sStrToPhrase32(const char* cpStr, uint32_t* acpBuf);
char* cpPhrase32ToStr(u32_phrase* spPhrase, char* cpStr);
abool bIsPhrase32Ascii(u32_phrase* spPhrase);
///@}

#endif /* TOOLS_H_ */
