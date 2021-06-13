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
/** \dir examples/ex-ast
 * \brief Example demonstrating the use and usefulness of the AST..
 *
 */

/** \file examples/ex-odata/main.c
 * \brief Driver for the OData example.
 *

\page exodata The OData Grammar and Test Cases

The [OASIS Open Data Protocol (OData) Technical Committee](https://www.oasis-open.org/committees/tc_home.php?wg_abbrev=odata)
has the stated purpose "...to define an open data protocol for sharing data and exposing data models interoperably on the Web."
As part of that project, an [OData ABNF grammar and a large set of test cases](https://github.com/oasis-tcs/odata-abnf)
has been developed. Along with that is a test tool built from [apg-java](https://github.com/ldthomas/apg-java).

This example builds a C-language equivalent of the parser and test tool for the
core grammar and test cases.

  - application code must include header files:
      - ../../library/lib.h
      - ../../utilities/utilities.h
      - ../../json/json.h
      - ../../xml/xml.h
      - ./odata.h
  - application compilation must include source code from the directories:
      - ../../json
      - ../../xml
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - APG_AST
      - APG_TRACE

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case  1: Display application information. (type names, type sizes and defined macros)
 - case  2: Build the JSON test file from the XML test file using the APG XML parser. Must be run before other tests.
 - case  3: Parse all of the valid tests.
 - case  4: Parse all of the invalid tests.
 - case  5: Trace test with JSON ID number = arg2.
*/
#include "main.h"

#include "source.h"

static char* cpMakeFileName(char* cpBuffer, const char* cpBase, const char* cpDivider, const char* cpName){
    strcpy(cpBuffer, cpBase);
    strcat(cpBuffer, cpDivider);
    strcat(cpBuffer, cpName);
    return cpBuffer;
}

static char s_caBuf[2*PATH_MAX];

static char* s_cpDescription =
        "Example demonstrating the use and usefulness of the AST.";

static char* s_cppCases[] = {
        "Display application information.",
        "Build the JSON test file from the XML test file using the APG XML parser. Must be run before other tests.",
        "Parse all of the valid tests.",
        "Parse all of the invalid tests.",
        "Trace test with JSON ID number = arg2.",
};
static long int s_iCaseCount = (long int)(sizeof(s_cppCases) / sizeof(s_cppCases[0]));

static int iHelp(void){
    long int i = 0;
    vUtilCurrentWorkingDirectory();
    printf("description: %s\n", s_cpDescription);
    printf("      usage: ex-trace [arg]\n");
    printf("             arg = n, 1 <= n <= %ld\n", s_iCaseCount);
    printf("                   execute case number n\n");
    printf("             arg = anything else, or nothing at all\n");
    printf("                   print this help screen\n");
    printf("\n");
    for(; i < s_iCaseCount; i++){
        printf("case %ld %s\n", (i + 1), s_cppCases[i]);
    }
    return EXIT_SUCCESS;
}

static int iApp() {
    // print the current working directory
    vUtilCurrentWorkingDirectory();
    printf("\n");

    // display the current APG sizes and macros
    vUtilApgInfo();
    return EXIT_SUCCESS;
}
static abool bMatchU32PhraseAscii(const char* cpString, u32_phrase* spPhrase){
    abool bReturn = APG_FALSE;
    if(strlen(cpString) == (size_t)spPhrase->uiLength){
        aint ui = 0;
        for(; ui < spPhrase->uiLength; ui++){
            if(spPhrase->uipPhrase[ui] != (uint32_t)cpString[ui]){
                return bReturn;
            }
        }
        bReturn = APG_TRUE;
    }
    return bReturn;
}
static abool bMatchU32PhraseAchar(const achar* acpString, u32_phrase* spPhrase){
    abool bReturn = APG_FALSE;
    aint ui = 0;
    for(; ui < spPhrase->uiLength; ui++){
        if(spPhrase->uipPhrase[ui] != (uint32_t)acpString[ui]){
            return bReturn;
        }
    }
    bReturn = APG_TRUE;
    return bReturn;
}
static void* vpGetConstraintsIterator(exception* spException, void* vpJson, void* vpItRoot){
    // set up the constraints iterator
    void* vpItKey = vpJsonFindKeyA(vpJson, "constraints", spJsonIteratorFirst(vpItRoot));
    if(!vpItKey){
        XTHROW(spException, "could not find \"constraints\" key");
    }
    json_value* spArray = spJsonIteratorFirst(vpItKey);
    if(spArray->uiId != JSON_ID_ARRAY){
        XTHROW(spException, "\"constraints\" member not an array");
    }
    void* vpItConstraints = vpJsonChildren(vpJson, spArray);
    if(!vpItConstraints){
        XTHROW(spException, "could not find \"constraint\" members");
    }
    return vpItConstraints;
}

static aint uiGetTestId(exception* spEx, void* vpJson, json_value* spTest){
    void* vpItKey;
    json_value* spChild, *spChildEnd;
    spChild = *spTest->sppChildren;
    spChildEnd = spChild + spTest->uiChildCount;
    for(; spChild < spChildEnd; spChild++){
        if(bMatchU32PhraseAscii("ID", spChild->spKey)){
            // found ruleId
            goto foundruleid;
        }
    }
    XTHROW(spEx, "expected key ID not found");
    foundruleid:;
    if((spChild->uiId != JSON_ID_NUMBER) || (spChild->spNumber->uiType != JSON_ID_UNSIGNED)){
        XTHROW(spEx, "ID value not unsigned int");
    }
    if(spChild->spNumber->uiUnsigned > APG_MAX_AINT){
        XTHROW(spEx, "ID value too big - > APG_MAX_AINT");
    }
    return (aint)spChild->spNumber->uiUnsigned;
}
static aint uiGetRuleId(exception* spEx, void* vpJson, json_value* spTest){
    void* vpItKey;
    json_value* spChild, *spChildEnd;
    spChild = *spTest->sppChildren;
    spChildEnd = spChild + spTest->uiChildCount;
    for(; spChild < spChildEnd; spChild++){
        if(bMatchU32PhraseAscii("ruleId", spChild->spKey)){
            // found ruleId
            goto foundruleid;
        }
    }
    XTHROW(spEx, "expected key ruleId not found");
    foundruleid:;
    if((spChild->uiId != JSON_ID_NUMBER) || (spChild->spNumber->uiType != JSON_ID_UNSIGNED)){
        XTHROW(spEx, "ruleId value not unsigned int");
    }
    if(spChild->spNumber->uiUnsigned > APG_MAX_AINT){
        XTHROW(spEx, "ruleId value too big - > APG_MAX_AINT");
    }
    return (aint)spChild->spNumber->uiUnsigned;
}
static aint uiGetInput(exception* spEx, void* vpJson, json_value* spTest, achar* acpBuf, aint uiBufLen){
    void* vpItKey;
    json_value* spChild, *spChildEnd;
    spChild = *spTest->sppChildren;
    spChildEnd = spChild + spTest->uiChildCount;
    for(; spChild < spChildEnd; spChild++){
        if(bMatchU32PhraseAscii("input", spChild->spKey)){
            // found ruleId
            goto foundruleid;
        }
    }
    XTHROW(spEx, "expected key input not found");
    foundruleid:;
    if((spChild->uiId != JSON_ID_STRING)){
        XTHROW(spEx, "input value not string");
    }
    if(spChild->spString->uiLength > uiBufLen){
        XTHROW(spEx, "input string too long - larger than uiBufLen - increase buffer size and try again");
    }
    aint ui = 0;
    for(; ui < spChild->spString->uiLength; ui++){
        acpBuf[ui] = (achar)spChild->spString->uipPhrase[ui];
    }
    return (aint)spChild->spString->uiLength;
}
static void pfnConstraintCallback(callback_data* spData){
    void* vpIt = ((user_data*)spData->vpUserData)->vpIt;
    abool bTrace = ((user_data*)spData->vpUserData)->bTrace;
    void* vpItValue = NULL;
    json_value* spValue;
    aint ui, uj, uiCount, uiRuleIndex;
    const uint32_t* uipString;
    const achar *acpPhrase;
    json_value* spChild, *spChildEnd;
    json_value* spMatch, *spMatchEnd;
    if(spData->uiParserState != ID_ACTIVE){
        // see if the rule index is in the map
        json_value* spConstraint = spJsonIteratorFirst(vpIt);
        while(spConstraint){
            if(spConstraint->uiId != JSON_ID_OBJECT){
                XTHROW(spData->spException, "bad constraint");
            }
            // find the rule index
            spChild = *spConstraint->sppChildren;
            spChildEnd = spChild + spConstraint->uiChildCount;
            for(; spChild < spChildEnd; spChild++){
                if(spChild->uiId == JSON_ID_NUMBER && bMatchU32PhraseAscii("ruleId", spChild->spKey)){
                    uiRuleIndex = (aint)spChild->spNumber->uiUnsigned;
                    goto foundruleid;
                }
            }
            XTHROW(spData->spException, "ruleId key not found in constraint object");
            foundruleid:;
            if(uiRuleIndex == spData->uiRuleIndex){
                // find the matched strings
                spChild = *spConstraint->sppChildren;
                spChildEnd = spChild + spConstraint->uiChildCount;
                for(; spChild < spChildEnd; spChild++){
                    if(spChild->uiId == JSON_ID_ARRAY && bMatchU32PhraseAscii("match", spChild->spKey)){
                        // find the string in the matched array
                        acpPhrase = spData->acpString + spData->uiParserOffset;
                        spMatch = *spChild->sppChildren;
                        spMatchEnd = spMatch + spChild->uiChildCount;
                        for(; spMatch < spMatchEnd; spMatch++){
                            if(spMatch->spString->uiLength == (uint32_t)spData->uiParserPhraseLength){
                                if(bMatchU32PhraseAchar(acpPhrase, spMatch->spString)){
                                    // match found
                                    return;
                                }
                            }
                        }
                        // match not found
                        if(bTrace){
                            printf("=> for rule index %d the parsed phrase did not match any strings in the list\n", (int)uiRuleIndex);
                        }
                        spData->uiCallbackPhraseLength = 0;
                        spData->uiCallbackState = ID_NOMATCH;
                        return;
                    }
                    // keep looking for match key
                }
                XTHROW(spData->spException, "match key not found in constraint object");
            }
            // keep looking for a rule index match
            spConstraint = spJsonIteratorNext(vpIt);
        }
    }
}
static void vParseTests(exception* spEx, void* vpParser, void* vpJson, abool bValid, void* vpItConstraints, void* vpItTests){
    char* cpTestName = bValid ? "valid" : "invalid";
    printf("Parsing tests: %s\n", cpTestName);
    parser_config sConfig = {};
    parser_state sState;
    aint uiTestId, uiRuleId, uiInputLen;
    aint uiCount = 0;
    aint uiSuccessCount = 0;
    aint uiFailCount = 0;
    achar acaBuf[1024];
    aint uiBufLen = 1024;
    aint ui = 0;
    for(; ui < RULE_COUNT_ODATA; ui++){
        vParserSetRuleCallback(vpParser, ui, pfnConstraintCallback);
    }
    json_value* spTest = spJsonIteratorFirst(vpItTests);
    while(spTest){
        uiTestId = uiGetTestId(spEx, vpJson, spTest);
        uiRuleId = uiGetRuleId(spEx, vpJson, spTest);
        uiInputLen = uiGetInput(spEx, vpJson, spTest, acaBuf, uiBufLen);
        sConfig.acpInput = acaBuf;
        sConfig.uiInputLength = uiInputLen;
        sConfig.uiStartRule = uiRuleId;
        user_data sUserData = {vpItConstraints, APG_FALSE};
        sConfig.vpUserData = &sUserData;
        vParserParse(vpParser, &sConfig, &sState);
        if(sState.uiSuccess){
            uiSuccessCount++;
            if(!bValid){
                printf("test ID: %d: succeeded\n", (int)uiTestId);
            }
        }else{
            if(bValid){
                printf("test ID: %d: failed\n", (int)uiTestId);
            }
            uiFailCount++;
        }
        uiCount++;
        spTest = spJsonIteratorNext(vpItTests);
    }
    printf("count: %d: success: %d: fail: %d\n", (int)uiCount, (int)uiSuccessCount, (int)uiFailCount);

}
static int iTraceTestId(aint uiTraceId){
    int iReturn = EXIT_SUCCESS;
    static void* vpJson = NULL;
    static void* vpParser = NULL;
    static void* vpTrace = NULL;
    void* vpItRoot, *vpItKey, *vpItTests, *vpItConstraints;
    char* cpWhich;
    parser_config sConfig = {};
    parser_state sState;
    json_value* spTest;
    aint uiTestId, uiRuleId, uiInputLen;
    achar acaBuf[1024];
    aint uiBufLen = 1024;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        char* cpDesc = "This program will read the JSON file built in case 2 and parse all of the valid tests.\n";
        printf("\n");
        printf("%s", cpDesc);
        printf("\n");

        // setup
        vpParser = vpParserCtor(&e, vpOdataInit);
        vpTrace = vpTraceCtor(vpParser);
//        vTraceSetOutput(vpTrace, cpMakeFileName(&s_caBuf[0], SOURCE_DIR, "/../output/", "odata-trace.txt"));
        aint ui = 0;
        for(; ui < RULE_COUNT_ODATA; ui++){
            vParserSetRuleCallback(vpParser, ui, pfnConstraintCallback);
        }
        vpJson = vpJsonCtor(&e);
        char* cpJsonName = cpMakeFileName(&s_caBuf[PATH_MAX], SOURCE_DIR, "/../output/", "odata-abnf-testcases.json");
        vpItRoot = vpJsonReadFile(vpJson, cpJsonName);
        vpItConstraints = vpGetConstraintsIterator(&e, vpJson, vpItRoot);
        cpWhich = "valid";
        vpItKey = vpJsonFindKeyA(vpJson, "valid", spJsonIteratorFirst(vpItRoot));
        if(!vpItKey){
            XTHROW(&e, "could not find \"valid\" key");
        }
        json_value* spArray = spJsonIteratorFirst(vpItKey);
        if(spArray->uiId != JSON_ID_ARRAY){
            XTHROW(&e, "\"valid\" member not an array");
        }
        vpItTests = vpJsonChildren(vpJson, spArray);
        if(!vpItTests){
            XTHROW(&e, "could not find \"valid\" tests");
        }
        spTest = spJsonIteratorFirst(vpItTests);
        while(spTest){
            uiTestId = uiGetTestId(&e, vpJson, spTest);
            if(uiTestId == uiTraceId){
                printf("Trace test: tracing %s test id %d\n", cpWhich, (int)uiTraceId);
                uiRuleId = uiGetRuleId(&e, vpJson, spTest);
                uiInputLen = uiGetInput(&e, vpJson, spTest, acaBuf, uiBufLen);
                sConfig.acpInput = acaBuf;
                sConfig.uiInputLength = uiInputLen;
                sConfig.uiStartRule = uiRuleId;
                user_data sUserData = {vpItConstraints, APG_TRUE};
                sConfig.vpUserData = &sUserData;
                vParserParse(vpParser, &sConfig, &sState);
                if(sState.uiSuccess){
                    printf("TRACE TEST: %d: success\n", (int)uiTraceId);
                }else{
                    printf("TRACE TEST: %d: failure\n", (int)uiTraceId);
                }
                return iReturn;
            }
            spTest = spJsonIteratorNext(vpItTests);
        }
        // not found in valid tests
        cpWhich = "invalid";
        vpItKey = vpJsonFindKeyA(vpJson, "invalid", spJsonIteratorFirst(vpItRoot));
        if(!vpItKey){
            XTHROW(&e, "could not find \"invalid\" key");
        }
        spArray = spJsonIteratorFirst(vpItKey);
        if(spArray->uiId != JSON_ID_ARRAY){
            XTHROW(&e, "\"valid\" member not an array");
        }
        vpItTests = vpJsonChildren(vpJson, spArray);
        if(!vpItTests){
            XTHROW(&e, "could not find \"invalid\" tests");
        }
        spTest = spJsonIteratorFirst(vpItTests);
        while(spTest){
            uiTestId = uiGetTestId(&e, vpJson, spTest);
            if(uiTestId == uiTraceId){
                printf("Trace test: tracing %s test id %d\n", cpWhich, (int)uiTraceId);
                uiRuleId = uiGetRuleId(&e, vpJson, spTest);
                uiInputLen = uiGetInput(&e, vpJson, spTest, acaBuf, uiBufLen);
                sConfig.acpInput = acaBuf;
                sConfig.uiInputLength = uiInputLen;
                sConfig.uiStartRule = uiRuleId;
                user_data sUserData = {vpItConstraints, APG_TRUE};
                sConfig.vpUserData = &sUserData;
                vParserParse(vpParser, &sConfig, &sState);
                if(sState.uiSuccess){
                    printf("TRACE TEST: %d: success\n", (int)uiTraceId);
                }else{
                    printf("TRACE TEST: %d: failure\n", (int)uiTraceId);
                }
                return iReturn;
            }
            spTest = spJsonIteratorNext(vpItTests);
        }
        printf("TRACE TEST: test id %d not found\n", (int)uiTraceId);
    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    vParserDtor(vpParser);
    vJsonDtor(vpJson);
    return iReturn;
}

static int iParseValid(){
    int iReturn = EXIT_SUCCESS;
    static void* vpJson = NULL;
    static void* vpParser = NULL;
    void* vpItRoot, *vpItKey, *vpItTests, *vpItConstraints;
    json_value* spArray;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        char* cpDesc = "This program will read the JSON file built in case 2 and parse all of the valid tests.\n";
        printf("\n");
        printf("%s", cpDesc);
        printf("\n");

        // setup
        vpParser = vpParserCtor(&e, vpOdataInit);
        vpJson = vpJsonCtor(&e);
        char* cpJsonName = cpMakeFileName(&s_caBuf[PATH_MAX], SOURCE_DIR, "/../output/", "odata-abnf-testcases.json");
        vpItRoot = vpJsonReadFile(vpJson, cpJsonName);
        vpItConstraints = vpGetConstraintsIterator(&e, vpJson, vpItRoot);

        // set up the valid tests iterator
        vpItKey = vpJsonFindKeyA(vpJson, "valid", spJsonIteratorFirst(vpItRoot));
        if(!vpItKey){
            XTHROW(&e, "could not find \"valid\" key");
        }
        spArray = spJsonIteratorFirst(vpItKey);
        if(spArray->uiId != JSON_ID_ARRAY){
            XTHROW(&e, "\"valid\" member not an array");
        }
        vpItTests = vpJsonChildren(vpJson, spArray);
        if(!vpItTests){
            XTHROW(&e, "could not find \"valid\" tests");
        }
        vParseTests(&e, vpParser, vpJson, APG_TRUE, vpItConstraints, vpItTests);
    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    vParserDtor(vpParser);
    vJsonDtor(vpJson);
    return iReturn;
}

static int iParseInvalid(){
    int iReturn = EXIT_SUCCESS;
    static void* vpJson = NULL;
    static void* vpParser = NULL;
    void* vpItRoot, *vpItKey, *vpItTests, *vpItConstraints;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        char* cpDesc = "This program will read the JSON file built in case 2 and parse all of the valid tests.\n";
        printf("\n");
        printf("%s", cpDesc);
        printf("\n");

        // setup
        vpParser = vpParserCtor(&e, vpOdataInit);
        vpJson = vpJsonCtor(&e);
        char* cpJsonName = cpMakeFileName(&s_caBuf[PATH_MAX], SOURCE_DIR, "/../output/", "odata-abnf-testcases.json");
        vpItRoot = vpJsonReadFile(vpJson, cpJsonName);
        vpItConstraints = vpGetConstraintsIterator(&e, vpJson, vpItRoot);
        vpItKey = vpJsonFindKeyA(vpJson, "invalid", spJsonIteratorFirst(vpItRoot));
        if(!vpItKey){
            XTHROW(&e, "could not find \"valid\" key");
        }
        json_value* spArray = spJsonIteratorFirst(vpItKey);
        if(spArray->uiId != JSON_ID_ARRAY){
            XTHROW(&e, "\"invalid\" member not an array");
        }
        vpItTests = vpJsonChildren(vpJson, spArray);
        if(!vpItTests){
            XTHROW(&e, "could not find \"invalid\" tests");
        }
        vParseTests(&e, vpParser, vpJson, APG_FALSE, vpItConstraints, vpItTests);
    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    vParserDtor(vpParser);
    vJsonDtor(vpJson);
    return iReturn;
}
static abool bCompU32(u32_phrase* spL, u32_phrase* spR){
    abool bReturn = APG_FALSE;
    if(spL->uiLength == spR->uiLength){
        uint32_t ui = 0;
        for(;ui < spL->uiLength; ui++){
            if(spL->uipPhrase[ui] != spR->uipPhrase[ui]){
                goto fail;
            }
        }
        bReturn = APG_TRUE;
    }
    fail:;
    return bReturn;
}
static void vStartTag(u32_phrase* spName, u32_phrase* spAttNames, u32_phrase* spAttValues, uint32_t uiAttCount, void* vpCtx){
    xml_context* spCtx = (xml_context*)vpCtx;
    if(bCompU32(spName, spCtx->spConstraint)){
        if(bCompU32(spAttNames, spCtx->spRule)){
            cpUint32ToStr((uint32_t*)spAttValues->uipPhrase, (aint)spAttValues->uiLength, spCtx->caBuf);
            aint uiRuleId = uiParserRuleLookup(spCtx->vpOdataParser, spCtx->caBuf);
            if(uiRuleId >= spCtx->uiRuleCount){
                XTHROW(spCtx->spException, "rule name not found");
            }
            spCtx->spCurrentConstraint = (rule_constraint*)vpVecAt(spCtx->vpVecContraintRules, uiRuleId);
            spCtx->spCurrentConstraint->uiRuleIndex = uiRuleId;
            spCtx->spCurrentConstraint->cpRuleName = cpParserRuleName(spCtx->vpOdataParser, uiRuleId);
            spCtx->spCurrentConstraint->uiOffset = uiVecLen(spCtx->vpVecConstraints);
            spCtx->spCurrentConstraint->uiCount = 0;
        }else{
            XTHROW(spCtx->spException, "vStartTag: Constraint node must have a \"Rule\" attribute");
        }
    }else if(bCompU32(spName, spCtx->spTestCase)){
        abool bName = APG_FALSE;
        abool bRule = APG_FALSE;
        spCtx->spCurrentTest = (test*)vpVecPush(spCtx->vpVecTests, NULL);
        uint32_t ui = 0;
        for(; ui < uiAttCount; ui++, spAttNames++, spAttValues++){
            if(bCompU32(spAttNames, spCtx->spName)){
                bName = APG_TRUE;
                spCtx->spCurrentTest->sName.uiIndex = uiVecLen(spCtx->vpVec32);
                spCtx->spCurrentTest->sName.uiLength = spAttValues->uiLength;
                vpVecPushn(spCtx->vpVec32, (uint32_t*)spAttValues->uipPhrase, spAttValues->uiLength);
            }
            if(bCompU32(spAttNames, spCtx->spRule)){
                bRule = APG_TRUE;
                spCtx->spCurrentTest->sRule.uiIndex = uiVecLen(spCtx->vpVec32);
                spCtx->spCurrentTest->sRule.uiLength = spAttValues->uiLength;
                vpVecPushn(spCtx->vpVec32, (uint32_t*)spAttValues->uipPhrase, spAttValues->uiLength);
                if(spAttValues->uiLength >= sizeof(spCtx->caBuf)){
                    XTHROW(spCtx->spException, "character buffer too small for name conversion to string");
                }
                cpUint32ToStr((uint32_t*)spAttValues->uipPhrase, (aint)spAttValues->uiLength, spCtx->caBuf);
                spCtx->spCurrentTest->uiRuleId = uiParserRuleLookup(spCtx->vpOdataParser, spCtx->caBuf);
                if(spCtx->spCurrentTest->uiRuleId >= spCtx->uiRuleCount){
                    XTHROW(spCtx->spException, "rule name not found");
                }
            }
            if(bCompU32(spAttNames, spCtx->spFailAt)){
                spCtx->spCurrentTest->bFail = APG_TRUE;
                const uint32_t* uipDigit = spAttValues->uipPhrase;
                const uint32_t* uipDigitEnd = uipDigit + spAttValues->uiLength;
                uint32_t uiNum = *uipDigit++ - 48;
                for(; uipDigit < uipDigitEnd; uipDigit++){
                    uiNum = 10 * uiNum + (aint)(*uipDigit - 48);
                }
                spCtx->spCurrentTest->uiFailAt = (aint)uiNum;
            }
        }
        if(!bName){
            XTHROW(spCtx->spException, "expected Name attribute not found");
        }
        if(!bRule){
            XTHROW(spCtx->spException, "expected Rule attribute not found");
        }
    }
}

static void vEndTag(u32_phrase* spName, u32_phrase* spContent, void* vpCtx){
    xml_context* spCtx = (xml_context*)vpCtx;
    if(bCompU32(spName, spCtx->spConstraint)){
        spCtx->spCurrentConstraint = NULL;

    }else if(bCompU32(spName, spCtx->spMatch)){
        if(!spCtx->spCurrentConstraint){
            XTHROW(spCtx->spException, "vStartTag: \"Match\" node not child of \"Constraint\" node");
        }
        spCtx->spCurrentConstraint->uiCount++;
        // push a U32_phrase on vpVecConstraints
        u32_phrase* spPhrase = (u32_phrase*)vpVecPush(spCtx->vpVecConstraints, NULL);
        uint32_t* uipMem = (uint32_t*)vpMemAlloc(spCtx->vpMem, (sizeof(uint32_t) * spContent->uiLength));
        memcpy((void*)uipMem, (void*)spContent->uipPhrase, (sizeof(uint32_t) * spContent->uiLength));
        spPhrase->uipPhrase = (const uint32_t*)uipMem;
        spPhrase->uiLength = spContent->uiLength;
    }else if(bCompU32(spName, spCtx->spInput)){
        spCtx->spCurrentTest->sContent.uiIndex = uiVecLen(spCtx->vpVec32);
        spCtx->spCurrentTest->sContent.uiLength = spContent->uiLength;
        vpVecPushn(spCtx->vpVec32, (uint32_t*)spContent->uipPhrase, spContent->uiLength);
    }
}

static xml_context* spSetup(exception* spEx, void* vpMem, void* vpParser){
    xml_context* spCtx = (xml_context*)vpMemAlloc(vpMem, sizeof(xml_context));
    memset(spCtx, 0, sizeof(xml_context));
    spCtx->vpMem = vpMem;
    spCtx->spException = spEx;
    spCtx->vpOdataParser = vpParser;
    spCtx->vpVec32 = vpVecCtor(vpMem, sizeof(uint32_t), 8192);
    spCtx->vpVecTests = vpVecCtor(vpMem, sizeof(test), 1000);
    spCtx->vpVecContraintRules = vpVecCtor(vpMem, sizeof(rule_constraint), 1000);
    spCtx->vpVecConstraints = vpVecCtor(vpMem, sizeof(u32_phrase), 2000);
    spCtx->cpXmlName = cpMakeFileName(s_caBuf, SOURCE_DIR, "/../input/", "odata-abnf-testcases.xml");
    spCtx->cpJsonName = cpMakeFileName(&s_caBuf[PATH_MAX], SOURCE_DIR, "/../output/", "odata-abnf-testcases.json");
//    printf("XML INPUT: %s\n", spCtx->cpXmlName);
//    printf("JSON OUTPUT: %s\n", spCtx->cpJsonName);
    spCtx->spTestCase = spUtilStrToPhrase32(vpMem, "TestCase");
    spCtx->spConstraint = spUtilStrToPhrase32(vpMem, "Constraint");
    spCtx->spMatch = spUtilStrToPhrase32(vpMem, "Match");
    spCtx->spInput = spUtilStrToPhrase32(vpMem, "Input");
    spCtx->spRule = spUtilStrToPhrase32(vpMem, "Rule");
    spCtx->spName = spUtilStrToPhrase32(vpMem, "Name");
    spCtx->spFailAt = spUtilStrToPhrase32(vpMem, "FailAt");
    spCtx->uiRuleCount = RULE_COUNT_ODATA;

    // initialize all of the constraints
    rule_constraint* spConstraint;
    spConstraint = (rule_constraint*)vpVecPushn(spCtx->vpVecContraintRules, NULL, spCtx->uiRuleCount);
    aint ui = 0;
    for(; ui < spCtx->uiRuleCount; ui++, spConstraint++){
        spConstraint->uiRuleIndex = ui;
        spConstraint->uiCount = 0;
        spConstraint->uiOffset = 0;
    }
    return spCtx;
}

static int iMakeJsonApgXml() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpXml = NULL;
    static void* vpParser = NULL;
    static void* vpJson = NULL;
    static void* vpBuilder = NULL;

    char* cpDesc = "This program will build a JSON file that will be used for the parsing tests.\n"
            "It first reads the complete list of ABNF grammar rules for the OData grammar.\n"
            "It then reads the XML file of test cases and an APG XML parser is used to extract the test information.\n"
            "The XML file has valid tests and invalid tests with an attribute that indicates that the test is to fail.\n"
            "The valid and invalid tests are interspersed. This program will separate the valid and invalid tests.\n"
            "The root JSON object will have two members - an array of valid test object and an array of invalid test objects.\n";
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        printf("\n");
        printf("%s", cpDesc);
        printf("\n");

        // setup
        vpMem = vpMemCtor(&e);
        vpXml = vpXmlCtor(&e);
        vpJson = vpJsonCtor(&e);
        vpBuilder = vpJsonBuildCtor(vpJson);
        vpParser = vpParserCtor(&e, vpOdataInit);
        xml_context* spCtx = spSetup(&e, vpMem, vpParser);

        // get the test cases from the original XML format
        vXmlGetFile(vpXml, spCtx->cpXmlName);
        vXmlSetStartTagCallback(vpXml, vStartTag, (void*)spCtx);
        vXmlSetEndTagCallback(vpXml, vEndTag, (void*)spCtx);
        vXmlParse(vpXml);

        // count the number of successful visits to the A node.
        aint uiRoot, uiKey, uiValue, uiObj, uiMatchArray, uiConstraintsArray, uiValidArray, uiInvalidArray;
        rule_constraint* spConstraint = (rule_constraint*)vpVecFirst(spCtx->vpVecContraintRules);
        rule_constraint* spConstraintEnd = spConstraint + uiVecLen(spCtx->vpVecContraintRules);
        test* spTest = (test*)vpVecFirst(spCtx->vpVecTests);
        test* spTestEnd = spTest + uiVecLen(spCtx->vpVecTests);
        void* vpIt;
        json_value* spValue;
        uint32_t* uipData;
        aint uiCount;
        uint64_t ui64;

        // set up the root and arrays
        uiRoot = uiJsonBuildMakeObject(vpBuilder);
        uiConstraintsArray = uiJsonBuildMakeArray(vpBuilder);
        uiValidArray = uiJsonBuildMakeArray(vpBuilder);
        uiInvalidArray = uiJsonBuildMakeArray(vpBuilder);

        // set up the keys
        aint uiMatch = uiJsonBuildMakeStringA(vpBuilder, "match");
        aint uiID = uiJsonBuildMakeStringA(vpBuilder, "ID");
        aint uiName = uiJsonBuildMakeStringA(vpBuilder, "name");
        aint uiRule = uiJsonBuildMakeStringA(vpBuilder, "rule");
        aint uiRuleId = uiJsonBuildMakeStringA(vpBuilder, "ruleId");
        aint uiInput = uiJsonBuildMakeStringA(vpBuilder, "input");
        aint uiFailAt = uiJsonBuildMakeStringA(vpBuilder, "failAt");

        // make the constraint objects
        for(; spConstraint < spConstraintEnd; spConstraint++){
            if(spConstraint->cpRuleName){
                uiObj = uiJsonBuildMakeObject(vpBuilder);
                uiMatchArray = uiJsonBuildMakeArray(vpBuilder);
                // add the rule name
                uiValue = uiJsonBuildMakeStringA(vpBuilder, spConstraint->cpRuleName);
                uiJsonBuildAddToObject(vpBuilder, uiObj, uiRule, uiValue);
                // add the rule index
                uiValue = uiJsonBuildMakeNumberU(vpBuilder, (uint64_t)spConstraint->uiRuleIndex);
                uiJsonBuildAddToObject(vpBuilder, uiObj, uiRuleId, uiValue);
                // add the match strings to this rule
                u32_phrase* spPhrase = (u32_phrase*)vpVecAt(spCtx->vpVecConstraints, spConstraint->uiOffset);
                u32_phrase* spPhraseEnd = spPhrase + spConstraint->uiCount;
                for(; spPhrase < spPhraseEnd; spPhrase++){
                    uiValue = uiJsonBuildMakeStringU(vpBuilder, spPhrase->uipPhrase, spPhrase->uiLength);
                    uiJsonBuildAddToArray(vpBuilder, uiMatchArray, uiValue);
                }
                // add the match array
                uiJsonBuildAddToObject(vpBuilder, uiObj, uiMatch, uiMatchArray);
                // add the rule constraint object to the constraint array
                uiJsonBuildAddToArray(vpBuilder, uiConstraintsArray, uiObj);
            }
        }

        // make the the valid and invalid test objects
        for(ui64 = 0;spTest < spTestEnd; spTest++, ui64++){
            // make a test object
            uiObj = uiJsonBuildMakeObject(vpBuilder);
            uiValue = uiJsonBuildMakeNumberU(vpBuilder, ui64);
            uiJsonBuildAddToObject(vpBuilder, uiObj, uiID, uiValue);
            // name
            uipData = (uint32_t*)vpVecAt(spCtx->vpVec32, spTest->sName.uiIndex);
            uiValue = uiJsonBuildMakeStringU(vpBuilder, uipData, spTest->sName.uiLength);
            uiJsonBuildAddToObject(vpBuilder, uiObj, uiName, uiValue);
            // rule name
            uipData = (uint32_t*)vpVecAt(spCtx->vpVec32, spTest->sRule.uiIndex);
            uiValue = uiJsonBuildMakeStringU(vpBuilder, uipData, spTest->sRule.uiLength);
            uiJsonBuildAddToObject(vpBuilder, uiObj, uiRule, uiValue);
            // rule ID
            uiValue = uiJsonBuildMakeNumberU(vpBuilder, (uint64_t)spTest->uiRuleId);
            uiJsonBuildAddToObject(vpBuilder, uiObj, uiRuleId, uiValue);
            // input
            uipData = (uint32_t*)vpVecAt(spCtx->vpVec32, spTest->sContent.uiIndex);
            uiValue = uiJsonBuildMakeStringU(vpBuilder, uipData, spTest->sContent.uiLength);
            uiJsonBuildAddToObject(vpBuilder, uiObj, uiInput, uiValue);
            if(spTest->bFail){
                // fail at
                uiValue = uiJsonBuildMakeNumberU(vpBuilder, (uint64_t)spTest->uiFailAt);
                uiJsonBuildAddToObject(vpBuilder, uiObj, uiFailAt, uiValue);
                // add the test object to the "invalid" array
                uiJsonBuildAddToArray(vpBuilder, uiInvalidArray, uiObj);
            }else{
                // add the test object to the "valid" array
                uiJsonBuildAddToArray(vpBuilder, uiValidArray, uiObj);
            }
        }
        // add the "constraints" array to the root object
        uiKey = uiJsonBuildMakeStringA(vpBuilder, "constraints");
        uiJsonBuildAddToObject(vpBuilder, uiRoot, uiKey, uiConstraintsArray);

        // add the "valid" array to the root object
        uiKey = uiJsonBuildMakeStringA(vpBuilder, "valid");
        uiJsonBuildAddToObject(vpBuilder, uiRoot, uiKey, uiValidArray);

        // add the "invalid" array to the root object
        uiKey = uiJsonBuildMakeStringA(vpBuilder, "invalid");
        uiJsonBuildAddToObject(vpBuilder, uiRoot, uiKey, uiInvalidArray);

        // make a single JSON object file which holds ALL of the tests
        vpIt = vpJsonBuild(vpBuilder, uiRoot);
        spValue = spJsonIteratorFirst(vpIt);

        // write the JSON (root) object to a byte stream (uint8_t array)
        uint8_t* ucpBytes = ucpJsonWrite(vpJson, spValue, &uiCount);

        // display
//        vJsonDisplayValue(vpJson, spValue, 0);

        // write the UTF-8 byte stream to a file
        vUtilFileWrite(spCtx->vpMem, spCtx->cpJsonName, ucpBytes, uiCount);
        printf("\nJSON file written to %s\n", spCtx->cpJsonName);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    vParserDtor(vpParser);
    vXmlDtor(vpXml);
    vJsonDtor(vpJson);
    vMemDtor(vpMem);
    return iReturn;
}

/**
 * \brief Main function for the basic application.
 *
 * This example has several cases. Run the main program with no arguments
 * to see a help screen with usage and a list of the cases with a brief description of each.
 * \param argc The number of command line arguments.
 * \param argv An array of pointers to the command line arguments.
 * \return The application's exit code.
 *
 */
int main(int argc, char **argv) {
    long int iCase = 0;
    long int iTestId = 0;
    // NOTE: sizeof(achar) must == sizeof(uint8_t)
    if(sizeof(achar) != sizeof(uint8_t)){
        char* cpMsg = "For these tests, sizeof(achar) == sizeof(uint8_t) must be true.\n"
                "Insure that in the build the symbol APG_ACHAR is undefined or defined with a value of 8.\n";
        printf("%s", cpMsg);
        return EXIT_SUCCESS;
    }
    if(argc > 1){
        iCase = atol(argv[1]);
    }
    if((iCase > 0) && (iCase <= s_iCaseCount)){
        printf("%s\n", s_cppCases[iCase -1]);
    }
    switch(iCase){
    case 1:
        return iApp();
    case 2:
        return iMakeJsonApgXml();
    case 3:
        return iParseValid();
    case 4:
        return iParseInvalid();
    case 5:
        if(argc > 2){
            iTestId = atol(argv[2]);
            return iTraceTestId((aint)iTestId);
        }
        iHelp();
        break;
    default:
        return iHelp();
    }
}
