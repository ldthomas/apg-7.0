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
/** \file udtlib.c
 * \brief Library of UDT functions for SIP messages.
 *
 * A few of the rules most often "hit" or evaluated during the parsing of a SIP message
 * are have been hand written as UDTs. Those functions are defined here.
 * - CRLF - the line end
 * - LWS - linear white space - white space including line continuations
 * - SWS - optional linear white space
 * - message-body - zero or more octets, or bytes.
 *   - Note that this rule makes the alphabet character range for SIP messages 0-255.
 *     For this reason, XML, which forbids the characters 0-31, cannot be used as a delivery format for SIP messages.
 * - domainlabel - the elements of a host name
 * - toplabel - the last or top-most element of a host name
 */
#include <stdio.h>
#include "../../library/lib.h"
#include "./sip-1.h"

#define isalphanum(c) (((c) >= 97 && (c) <= 122) || ((c) >= 48 && (c) <= 57) || ((c) >= 65 && (c) <= 90))
#define ishexdigit(c) (((c) >= 48 && (c) <= 57) || ((c) >= 65 && (c) <= 70) || ((c) >= 97 && (c) <= 102))

//paramchar         =  param-unreserved / unreserved / escaped
//;unreserved      =  u_alphanum / mark
//;mark            =  "-" / "_" / "." / "!" / "~" / "*" / "'" / "(" / ")"
//;escaped         =  "%" HEXDIG HEXDIG
//param-unreserved  =  "[" / "]" / "/" / ":" / "&" / "+" / "$"
static abool bParamchar(achar acChar){
    if(isalphanum(acChar)){
        // alphanum
        return APG_TRUE;
    }
    switch(acChar){
        // mark
    case 33:
    case 39:
    case 40:
    case 41:
    case 42:
    case 45:
    case 46:
    case 95:
    case 126:
        // param-unreserved
    case 36:
    case 43:
    case 47:
    case 58:
    case 63:
    case 91:
    case 93:
        return APG_TRUE;
    default:
        break;
    }
    return APG_FALSE;
    // escaped
}
static abool bEscaped(achar acChar1, achar acChar2, achar acChar3){
    if(acChar1 != 37){
        return APG_FALSE;
    }
    if(ishexdigit(acChar2)){
        if(ishexdigit(acChar3)){
            return APG_TRUE;
        }
    }
    return APG_FALSE;
}

void u_Digit(callback_data* spData){
    const achar* acpChar = &spData->acpString[spData->uiParserOffset];
    const achar* acpEnd = spData->acpString + spData->uiStringLength;
    spData->uiCallbackState = ID_NOMATCH;
    spData->uiCallbackPhraseLength = 0;
    if(acpChar < acpEnd){
        if(*acpChar >= 48 && *acpChar <= 57){
            spData->uiCallbackState = ID_MATCH;
            spData->uiCallbackPhraseLength = 1;
        }
    }
}
void u_Digit1(callback_data* spData){
    const achar* acpChar = &spData->acpString[spData->uiParserOffset];
    const achar* acpEnd = spData->acpString + spData->uiStringLength;
    aint uiLen = 0;
    while(acpChar < acpEnd){
        if(*acpChar >= 48 && *acpChar <= 57){
            uiLen++;
            acpChar++;
        }else{
            break;
        }
    }
    if(uiLen){
        spData->uiCallbackState = ID_MATCH;
        spData->uiCallbackPhraseLength = uiLen;
    }else{
        spData->uiCallbackState = ID_NOMATCH;
        spData->uiCallbackPhraseLength = 0;
    }
}
/** \brief Evaluates the lower elements of a host name.

hostname          =  *( domainlabel "." &(alphanum/"-")) toplabel [ "." ]
domainlabel       =  1*alphanum *(1*"-" 1*alphanum)
 *
 * eg. my.example.com<br>
 * "my" and "example" are domain labels
 * \param spData the \ref callback_data passed to the function by the parser
 * \return none
 */
void u_DomainLabel(callback_data* spData){
    achar acChar;
    const achar* acpChar = &spData->acpString[spData->uiParserOffset];
    const achar* acpEnd = spData->acpString + spData->uiStringLength;
    aint uiLen = 0;
    spData->uiCallbackState = ID_NOMATCH;
    spData->uiCallbackPhraseLength = 0;
    while(APG_TRUE){
        if(acpChar == acpEnd){
            // end of string
            break;
        }
        acChar = *acpChar;
        if(!isalphanum(acChar)){
            // first character is not alphanum
            break;
        }
        uiLen++;
        acpChar++;
        while(acpChar < acpEnd){
            acChar = *acpChar;
            if(isalphanum(acChar) || acChar == 45){
                uiLen++;
                acpChar++;
            }else{
                break;
            }
        }
        if(acpChar[-1] == 45){
            // last character cannot be hyphen
            break;
        }
        // must be followed by .alphanum
        if(acpChar < acpEnd && *acpChar == 46){
            acpChar++;
            acChar = *acpChar;
            if(acpChar < acpEnd && isalphanum(acChar)){
                // success - it's a domainlabel
                spData->uiCallbackState = ID_MATCH;
                spData->uiCallbackPhraseLength = uiLen;
                break;
            }
        }
        break;
    }
}

/** \brief Evaluates the message body.
 *
 * Note that this function never fails and simply accepts the remainder of the input string,
 * no matter what it is.
 * \param spData the \ref callback_data passed to the function by the parser
 * \return none
 */
void e_MessageBody(callback_data* spData){
    spData->uiCallbackState = ID_MATCH;
    spData->uiCallbackPhraseLength = spData->uiStringLength - spData->uiParserOffset;
}

/** \brief Evaluates the line end character sequence.
 *
 * CRLF =  CR LF / LF / CR<br>
 * The line end sequence has been modified from the original ABNF to be forgiving.
 * \param spData the \ref callback_data passed to the function by the parser
 * \return none
 */
void u_CRLF(callback_data* spData){
    const achar* acpChar = &spData->acpString[spData->uiParserOffset];
    const achar* acpEnd = spData->acpString + spData->uiStringLength;
    aint uiLen = 0;
    if(acpChar < acpEnd){
        if(*acpChar == 13){
            uiLen++;
            acpChar++;
            if((acpChar < acpEnd) && *acpChar == 10){
                uiLen++;
            }
        }else if(*acpChar == 10){
            uiLen++;
        }
    }
    if(uiLen){
        spData->uiCallbackState = ID_MATCH;
        spData->uiCallbackPhraseLength = uiLen;
    }else{
        spData->uiCallbackState = ID_NOMATCH;
        spData->uiCallbackPhraseLength = 0;
    }
    return;
}

/** \brief Linear white space. White space with possible line breaks allowed.
 *
 * LWS =  [*WSP u_CRLF] 1*WSP
 * \param spData the \ref callback_data passed to the function by the parser
 * \return none
 */
void u_LWS(callback_data* spData){
    const achar* acpChar = &spData->acpString[spData->uiParserOffset];
    const achar* acpEnd = spData->acpString + spData->uiStringLength;
    aint uiLen = 0;
    aint uiLenCR = 0;
    aint uiLenWS = 0;
    while(acpChar < acpEnd){
        if((*acpChar == 32) || (*acpChar == 9)){
            uiLen++;
            acpChar++;
        }else{
            break;
        }
    }
    if(acpChar < acpEnd){
        if(*acpChar == 13){
            uiLenCR++;
            acpChar++;
            if((acpChar < acpEnd) && *acpChar == 10){
                uiLenCR++;
            }
        }else if(*acpChar == 10){
            uiLenCR++;
            acpChar++;
        }
    }
    if(uiLenCR){
        while(acpChar < acpEnd){
            if((*acpChar == 32) || (*acpChar == 9)){
                uiLenWS++;
                acpChar++;
            }else{
                break;
            }
        }
        if(uiLenWS){
            spData->uiCallbackState = ID_MATCH;
            spData->uiCallbackPhraseLength = uiLen + uiLenCR + uiLenWS;
        }else{
            spData->uiCallbackState = ID_NOMATCH;
            spData->uiCallbackPhraseLength = 0;
        }
    }else{
        if(uiLen){
            spData->uiCallbackState = ID_MATCH;
            spData->uiCallbackPhraseLength = uiLen;
        }else{
            spData->uiCallbackState = ID_NOMATCH;
            spData->uiCallbackPhraseLength = 0;
        }
    }
    return;
}
/** \brief Optional linear white space. (See \ref u_LWS.)
 *
 * \param spData the \ref callback_data passed to the function by the parser
 * \return none
 */
void e_SWS(callback_data* spData){
    u_LWS(spData);
    if(spData->uiCallbackPhraseLength == 0){
        spData->uiCallbackState = ID_MATCH;
    }
}

void u_WSP(callback_data* spData){
    const achar* acpChar = &spData->acpString[spData->uiParserOffset];
    const achar* acpEnd = spData->acpString + spData->uiStringLength;
    spData->uiCallbackState = ID_NOMATCH;
    spData->uiCallbackPhraseLength = 0;
    if(acpChar < acpEnd){
        if((*acpChar == 32) || (*acpChar == 9)){
            spData->uiCallbackState = ID_MATCH;
            spData->uiCallbackPhraseLength = 1;
        }
    }
}
void e_Alphanum0(callback_data* spData){
    const achar* acpChar = &spData->acpString[spData->uiParserOffset];
    const achar* acpEnd = spData->acpString + spData->uiStringLength;
    achar acChar;
    aint uiLen = 0;
    while(acpChar < acpEnd){
        acChar = *acpChar;
        if(isalphanum(acChar)){
            uiLen++;
            acpChar++;
        }else{
            break;
        }
    }
    spData->uiCallbackState = ID_MATCH;
    spData->uiCallbackPhraseLength = uiLen;
}
void u_Alphanum1(callback_data* spData){
    const achar* acpChar = &spData->acpString[spData->uiParserOffset];
    const achar* acpEnd = spData->acpString + spData->uiStringLength;
    achar acChar;
    aint uiLen = 0;
    while(acpChar < acpEnd){
        acChar = *acpChar;
        if(isalphanum(acChar)){
            uiLen++;
            acpChar++;
        }else{
            break;
        }
    }
    if(uiLen){
        spData->uiCallbackState = ID_MATCH;
        spData->uiCallbackPhraseLength = uiLen;
    }else{
        spData->uiCallbackState = ID_NOMATCH;
        spData->uiCallbackPhraseLength = 0;
    }
}
void u_alphanum(callback_data* spData){
    const achar* acpChar = &spData->acpString[spData->uiParserOffset];
    const achar* acpEnd = spData->acpString + spData->uiStringLength;
    achar acChar = *acpChar;
    if((acpChar < acpEnd) && isalphanum(acChar)){
        spData->uiCallbackState = ID_MATCH;
        spData->uiCallbackPhraseLength = 1;
    }else{
        spData->uiCallbackState = ID_NOMATCH;
        spData->uiCallbackPhraseLength = 0;
    }
}
void u_ALPHA(callback_data* spData){
    const achar* acpChar = &spData->acpString[spData->uiParserOffset];
    const achar* acpEnd = spData->acpString + spData->uiStringLength;
    achar acChar = *acpChar;
    if((acpChar < acpEnd) && ((acChar >= 97 && acChar <= 122) || (acChar >= 65 && acChar <= 90))){
        spData->uiCallbackState = ID_MATCH;
        spData->uiCallbackPhraseLength = 1;
    }else{
        spData->uiCallbackState = ID_NOMATCH;
        spData->uiCallbackPhraseLength = 0;
    }
}

void u_paramchar1(callback_data* spData){
    const achar* acpChar = &spData->acpString[spData->uiParserOffset];
    const achar* acpEnd = spData->acpString + spData->uiStringLength;
    aint uiLen = 0;
    while(acpChar < acpEnd){
        if(bParamchar(*acpChar)){
            uiLen++;
            acpChar++;
            continue;
        }
        if((acpChar + 3) < acpEnd){
            if(bEscaped(acpChar[0], acpChar[1], acpChar[2])){
                uiLen += 3;
                acpChar += 3;
                continue;
            }
        }
        break;
    }
    if(uiLen){
        spData->uiCallbackState = ID_MATCH;
        spData->uiCallbackPhraseLength = uiLen;
    }else{
        spData->uiCallbackState = ID_NOMATCH;
        spData->uiCallbackPhraseLength = 0;
    }
}

//;unreserved      =  u_alphanum / mark
//mark            =  "-" / "_" / "." / "!" / "~" / "*" / "'" / "(" / ")"
void u_unreserved(callback_data* spData){
    const achar* acpChar = &spData->acpString[spData->uiParserOffset];
    const achar* acpEnd = spData->acpString + spData->uiStringLength;
    achar acChar;
    if(acpChar < acpEnd){
        acChar = *acpChar;
        if(isalphanum(acChar)){
            spData->uiCallbackState = ID_MATCH;
            spData->uiCallbackPhraseLength = 1;
            return;
        }
        switch(acChar){
            // mark
        case 33:
        case 39:
        case 40:
        case 41:
        case 42:
        case 45:
        case 46:
        case 95:
        case 126:
            spData->uiCallbackState = ID_MATCH;
            spData->uiCallbackPhraseLength = 1;
            return;
        default:
            break;
        }
    }
    spData->uiCallbackState = ID_NOMATCH;
    spData->uiCallbackPhraseLength = 0;
}

/** \brief Set the UDT callback functions for the SIP2.bnf grammar to their respective parse tree nodes.
 *
 * \param vpParserCtx - the context of the parser
 * \return none
 */
void vSip1UdtCallbacks(void* vpParserCtx){
    aint ui;
    parser_callback cb[UDT_COUNT_SIP_1];
    memset((void*)cb, 0, sizeof(cb));
    cb[SIP_1_E_ALPHANUM0] = e_Alphanum0;
    cb[SIP_1_U_DOMAINLABEL] = u_DomainLabel;
    cb[SIP_1_E_MESSAGEBODY] = e_MessageBody;
    cb[SIP_1_E_SWS] = e_SWS;
    cb[SIP_1_U_CRLF] = u_CRLF;
    cb[SIP_1_U_ALPHANUM1] = u_Alphanum1;
    cb[SIP_1_U_DIGIT] = u_Digit;
    cb[SIP_1_U_DIGIT1] = u_Digit1;
    cb[SIP_1_U_LWS] = u_LWS;
    cb[SIP_1_U_WSP] = u_WSP;
    cb[SIP_1_U_ALPHANUM] = u_alphanum;
    cb[SIP_1_U_ALPHA] = u_ALPHA;
    cb[SIP_1_U_PARAMCHAR1] = u_paramchar1;
    cb[SIP_1_U_UNRESERVED] = u_unreserved;
    for(ui = 0; ui < (aint)UDT_COUNT_SIP_1; ui++){
        vParserSetUdtCallback(vpParserCtx, ui, cb[ui]);
    }
}

//static void vLWSPhrase(const achar* acpBeg, aint uiOffset, aint uiLen, char* cpTitle){
//    char caBuf[2*uiLen + 1];
//    char* cpBuf = caBuf;
//    const achar* acpChar = acpBeg + uiOffset;
//    const achar* acpEnd = acpChar + uiLen;
//    while(acpChar < acpEnd){
//        switch(*acpChar){
//        case 9:
//            *cpBuf++ = '\\';
//            *cpBuf++ = 't';
//            break;
//        case 10:
//            *cpBuf++ = '\\';
//            *cpBuf++ = 'n';
//            break;
//        case 13:
//            *cpBuf++ = '\\';
//            *cpBuf++ = 'r';
//            break;
//        case 32:
//            *cpBuf++ = 's';
//            break;
//        default:
//            *cpBuf++ = (char)*acpChar++;
//            break;
//        }
//        acpChar++;
//    }
//    *cpBuf = 0;
//    printf("%s: offset: %"PRIuMAX": length: %"PRIuMAX": phrase: %s\n", cpTitle, (luint)uiOffset, (luint)uiLen, caBuf);
//
//}
//static void vPrintPhrase(const achar* acpBeg, aint uiOffset, aint uiLen, char* cpTitle){
//    char caBuf[2*uiLen + 1];
//    char* cpBuf = caBuf;
//    const achar* acpChar = acpBeg + uiOffset;
//    const achar* acpEnd = acpChar + uiLen;
//    while(acpChar < acpEnd){
//        *cpBuf++ = (char)*acpChar++;
//    }
//    *cpBuf = 0;
//    printf("%s: offset: %"PRIuMAX": length: %"PRIuMAX": phrase: %s\n", cpTitle, (luint)uiOffset, (luint)uiLen, caBuf);
//
//}
