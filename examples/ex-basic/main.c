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
/** \dir examples
 * \brief Examples and cases for APG usage from simple set ups to complex applications..
 */

/** \dir examples/ex-basic
 * \brief Examples of constructing a simple basic parser from a pre-generated grammar file..
 */

/** \file examples/ex-basic/main.c
 * \brief Driver for the basic examples.
 *
This is a simple demonstration of constructing a parser from pre-generated grammar files.
The parser state is displayed and a second case demonstrates the use of rule name call back functions.

  - application code must include header files:
      - ../../utilities/utilities.h
      - %float.h
  - application compilation must include source code from the directories:
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - none

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: display the current state of the APG library (type names, type sizes and defined macros)
 - case 2: parse a simple input string and display the final parsed state
 - case 3: set call back functions for some of the grammar's rule names and execute them during the parse

 */

/**
\page exbasic Basic Parser Application
 This is a simple demonstration of constructing a parser from pre-generated grammar files.
 The parser state is displayed and a second case demonstrates the use of rule name call back functions.

  - application code must include header files:
      - ../../utilities/utilities.h
      - %float.h
  - application compilation must include source code from the directories:
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - none

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: display the current state of the APG library (type names, type sizes and defined macros)
 - case 2: parse a simple input string and display the final parsed state
 - case 3: set call back functions for some of the grammar's rule names and execute them during the parse
*/
#include "../../utilities/utilities.h"
#include "./float.h"

static char* s_cpDescription =
        "Illustrate the simple basics of constructing a parser from pre-generated grammar files.";

static char* s_cppCases[] = {
        "Display application information.",
        "Parse an input string and display the parser state.",
        "Define rule call back functions to display phrase information during the parser's traversal of the parse tree.",
        "Illustrate reporting a fatal parsing error.",
};
static long int s_iCaseCount = (long int)(sizeof(s_cppCases) / sizeof(s_cppCases[0]));

static int iHelp(void){
    long int i = 0;
    vUtilCurrentWorkingDirectory();
    printf("description: %s\n", s_cpDescription);
    printf("      usage: ex-basic arg\n");
    printf("             arg = n, 1 <= n <= 4\n");
    printf("                   execute case number n\n");
    printf("             arg = anthing else\n");
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

static int iParseFile() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpParser = NULL;
    parser_state sState;
    parser_config sInput;
    apg_phrase sPhrase;
    apg_phrase* spPhrase;
    achar acaBuffer[128];
    aint uiBufSize = 128;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block

        // construct the parser from the pre-generated grammar files
        vpParser = vpParserCtor(&e, vpFloatInit);

        // convert the input string to APG alphabet characters (in general not necessarily same size as char)
        char* cpInput = "-12.3456e+10";
        sPhrase.acpPhrase = acaBuffer;
        sPhrase.uiLength = uiBufSize;
        spPhrase = spStrToPhrase(cpInput, &sPhrase);
        if(!spPhrase){
            XTHROW(&e, "alphabet buffer size to small for input string");
        }


        // configure the parser
        memset((void*)&sInput, 0, sizeof(sInput));
        sInput.acpInput = spPhrase->acpPhrase;
        sInput.uiInputLength = spPhrase->uiLength;
        sInput.uiStartRule = 0;

        // parse the input string
        vParserParse(vpParser, &sInput, &sState);

        // display the state
        vUtilPrintParserState(&sState);
    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }
    vMemDtor(vpMem);
    return iReturn;
}

// buffer and buffer size for converting alphabet character phrases to strings
static char s_acBuffer[1024];
static aint s_uiBufferSize = 1024;

// data structure for passing user information to the call back functions
typedef struct{
    exception* spException;
} my_data;

static void vFloat(callback_data* spData) {
    if (spData->uiParserState == ID_ACTIVE) {
        printf("float: going down\n");
    }else if (spData->uiParserState == ID_MATCH) {
        // convert the phrase to a string
        my_data* spMyData = (my_data*)spData->vpUserData;
        apg_phrase sPhrase;
        sPhrase.acpPhrase = spData->acpString + spData->uiParserOffset;
        sPhrase.uiLength = spData->uiParserPhraseLength;
        if(sPhrase.uiLength >= s_uiBufferSize){
            XTHROW(spMyData->spException, "string conversion buffer too small");
        }
        char* cpStr = cpPhraseToStr(&sPhrase, s_acBuffer);
        printf("float: going up\n");
        printf("       phrase: %s\n", cpStr);
    }
}

static void vSign(callback_data* spData) {
    if (spData->uiParserState == ID_ACTIVE) {
        printf("sign: going down\n");
    }else if (spData->uiParserState == ID_MATCH) {
        // convert the phrase to a string
        my_data* spMyData = (my_data*)spData->vpUserData;
        apg_phrase sPhrase;
        sPhrase.acpPhrase = spData->acpString + spData->uiParserOffset;
        sPhrase.uiLength = spData->uiParserPhraseLength;
        if(sPhrase.uiLength >= s_uiBufferSize){
            XTHROW(spMyData->spException, "string conversion buffer too small");
        }
        char* cpStr = cpPhraseToStr(&sPhrase, s_acBuffer);
        printf("sign: going up\n");
        printf("      phrase: %s\n", cpStr);
    }
}

static void vDecimal(callback_data* spData) {
    if (spData->uiParserState == ID_ACTIVE) {
        printf("decimal: going down\n");
    }else if (spData->uiParserState == ID_MATCH) {
        // convert the phrase to a string
        my_data* spMyData = (my_data*)spData->vpUserData;
        apg_phrase sPhrase;
        sPhrase.acpPhrase = spData->acpString + spData->uiParserOffset;
        sPhrase.uiLength = spData->uiParserPhraseLength;
        if(sPhrase.uiLength >= s_uiBufferSize){
            XTHROW(spMyData->spException, "string conversion buffer too small");
        }
        char* cpStr = cpPhraseToStr(&sPhrase, s_acBuffer);
        printf("decimal: going up\n");
        printf("         phrase: %s\n", cpStr);
    }
}

static void vExponent(callback_data* spData) {
    if (spData->uiParserState == ID_ACTIVE) {
        printf("exponent: going down\n");
    }else if (spData->uiParserState == ID_MATCH) {
        // convert the phrase to a string
        my_data* spMyData = (my_data*)spData->vpUserData;
        apg_phrase sPhrase;
        sPhrase.acpPhrase = spData->acpString + spData->uiParserOffset;
        sPhrase.uiLength = spData->uiParserPhraseLength;
        if(sPhrase.uiLength >= s_uiBufferSize){
            XTHROW(spMyData->spException, "string conversion buffer too small");
        }
        char* cpStr = cpPhraseToStr(&sPhrase, s_acBuffer);
        printf("exponent: going up\n");
        printf("          phrase: %s\n", cpStr);
    }
}

static void vBadExponent(callback_data* spData) {
    if (spData->uiParserState == ID_ACTIVE) {
        printf("exponent: going down\n");
    }else if (spData->uiParserState == ID_MATCH) {
        // convert the phrase to a string
        my_data* spMyData = (my_data*)spData->vpUserData;
        XTHROW(spMyData->spException, "demonstration of reporting a fatal error from a call back function");
    }
}

static int iParseString() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpParser = NULL;
    parser_state sState;
    parser_config sInput;
    apg_phrase sPhrase;
    apg_phrase* spPhrase;
    achar acaBuffer[128];
    aint uiBufSize = 128;
    exception e;
    my_data sMyData = {&e};
    XCTOR(e);
    if(e.try){
        // try block

        // construct the parser from the pre-generated grammar files
        vpParser = vpParserCtor(&e, vpFloatInit);

        // convert the input string to APG alphabet characters (in general not necessarily same size as char)
        char* cpInput = "-12.3456e+10";
        sPhrase.acpPhrase = acaBuffer;
        sPhrase.uiLength = uiBufSize;
        spPhrase = spStrToPhrase(cpInput, &sPhrase);
        if(!spPhrase){
            XTHROW(&e, "alphabet buffer size to small for input string");
        }

        // set the callback functions
        // the rule identifiers are defined in float.h by the generator
        vParserSetRuleCallback(vpParser, FLOAT_FLOAT, vFloat);
        vParserSetRuleCallback(vpParser, FLOAT_SIGN, vSign);
        vParserSetRuleCallback(vpParser, FLOAT_DECIMAL, vDecimal);
        vParserSetRuleCallback(vpParser, FLOAT_EXPONENT, vExponent);

        // configure the parser
        memset((void*)&sInput, 0, sizeof(sInput));
        sInput.acpInput = spPhrase->acpPhrase;
        sInput.uiInputLength = spPhrase->uiLength;
        sInput.uiStartRule = 0;
        sInput.vpUserData = (void*)&sMyData;

        // parse the input string
        vParserParse(vpParser, &sInput, &sState);

        // display the state
        printf("\n");
        vUtilPrintParserState(&sState);
    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }
    vMemDtor(vpMem);
    return iReturn;
}

static int iParseError() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpParser = NULL;
    parser_state sState;
    parser_config sInput;
    apg_phrase sPhrase;
    apg_phrase* spPhrase;
    achar acaBuffer[128];
    aint uiBufSize = 128;
    exception e;
    my_data sMyData = {&e};
    XCTOR(e);
    if(e.try){
        // try block

        // construct the parser from the pre-generated grammar files
        vpParser = vpParserCtor(&e, vpFloatInit);

        // convert the input string to APG alphabet characters (in general not necessarily same size as char)
        char* cpInput = "-12.3456e+10";
        sPhrase.acpPhrase = acaBuffer;
        sPhrase.uiLength = uiBufSize;
        spPhrase = spStrToPhrase(cpInput, &sPhrase);
        if(!spPhrase){
            XTHROW(&e, "alphabet buffer size to small for input string");
        }

        // set the callback functions
        // the rule identifiers are defined in float.h by the generator
        vParserSetRuleCallback(vpParser, FLOAT_FLOAT, vFloat);
        vParserSetRuleCallback(vpParser, FLOAT_SIGN, vSign);
        vParserSetRuleCallback(vpParser, FLOAT_DECIMAL, vDecimal);
        vParserSetRuleCallback(vpParser, FLOAT_EXPONENT, vBadExponent);

        // configure the parser
        memset((void*)&sInput, 0, sizeof(sInput));
        sInput.acpInput = spPhrase->acpPhrase;
        sInput.uiInputLength = spPhrase->uiLength;
        sInput.uiStartRule = 0;
        sInput.vpUserData = (void*)&sMyData;

        // parse the input string
        vParserParse(vpParser, &sInput, &sState);

        // display the state
        printf("\n");
        vUtilPrintParserState(&sState);
    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }
    vMemDtor(vpMem);
    return iReturn;
}

/**
 * \brief Main function for the basic application.

 * This example has several cases. Run the main program with no arguments
 * to see a help screen with usage and a list of the cases with a brief description of each.
 * \param argc The number of command line arguments.
 * \param argv An array of pointers to the command line arguments.
 * \return The application's exit code.
 *
 */
int main(int argc, char **argv) {
    long int iCase = 0;
    if(argc > 1){
        iCase = atol(argv[1]);
    }
    if(iCase == 0 || (iCase > s_iCaseCount)){
        return iHelp();
    }
    switch(iCase){
    case 1:
        return iApp();
    case 2:
        return iParseFile();
    case 3:
        return iParseString();
    case 4:
        return iParseError();
    }
}

