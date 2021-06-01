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
/** \dir ./examples/ex-api
 * \brief Examples of using the API..
 */

/** \file examples/ex-api/main.c
 * \brief Driver for the API examples.

 This example explores the use of the APG Application Programming Interface.
 The API object will be created and its major features explored.

  - application code must include header files:
      - ../../utilities/utilities.h
      - ../../api/api.h
  - application compilation must include source code from the directories:
      - ../../api
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - APG_AST

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Input, concatenate and display multiple grammar files.
 - case 3: Input, validation fails with bad characters and no final EOL.
 - case 4: Illustrate a grammar with bad syntax.
 - case 5: Illustrate a grammar with bad semantics.
 - case 6: Illustrate generating a parser with and without PPPT.

 */

/**
\page exapi Using the API
 This example explores the use of the APG Application Programming Interface.
 The API object will be created and its major features explored.

  - application code must include header files:
      - ../../utilities/utilities.h
      - ../../api/api.h
  - application compilation must include source code from the directories:
      - ../../api
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - APG_AST

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Input, concatenate and display multiple grammar files.
 - case 3: Input, validation fails with bad characters and no final EOL.
 - case 4: Illustrate a grammar with bad syntax.
 - case 5: Illustrate a grammar with bad semantics.
 - case 6: Illustrate generating a parser with and without PPPT.
*/
#include "../../utilities/utilities.h"
#include "../../api/api.h"

#include "source.h"

static const char* cpMakeFileName(char* cpBuffer, const char* cpBase, const char* cpDivider, const char* cpName){
    strcpy(cpBuffer, cpBase);
    strcat(cpBuffer, cpDivider);
    strcat(cpBuffer, cpName);
    return cpBuffer;
}

static char s_caBuf[PATH_MAX];

static char* s_cpDescription =
        "Illustrate construction of and API object and demonstrate its features.";

static char* s_cppCases[] = {
        "Display application information.",
        "Input, concatenate and display multiple grammar files.",
        "Input, validation fails with bad characters and no final EOL.",
        "Illustrate a grammar with bad syntax.",
        "Illustrate a grammar with bad semantics.",
        "Illustrate generating a parser with and without PPPT.",
};
static long int s_iCaseCount = (long int)(sizeof(s_cppCases) / sizeof(s_cppCases[0]));
static void vPrintCase(long int iCase){
    if((iCase > 0) && (iCase <= s_iCaseCount)){
        printf("%s\n", s_cppCases[iCase -1]);
    }else{
        printf("unknown case number %ld\n", iCase);
    }
}

static int iHelp(void){
    long int i = 0;
    vUtilCurrentWorkingDirectory();
    printf("description: %s\n", s_cpDescription);
    printf("      usage: ex-api arg\n");
    printf("             arg = n, 1 <= n <= %ld\n", s_iCaseCount);
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

static int iInCat() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    char* cpFloatMid =
            "integer  = 1*%d48-57\n"
            "dot      = \".\"\n";
    exception e;
    XCTOR(e);
    if(e.try){
        // try block

        // construct the parser from the pre-generated grammar files
        vpApi = vpApiCtor(&e);

        // concatenate the grammar from three sources
        cpApiInFile(vpApi, cpMakeFileName(s_caBuf, SOURCE_DIR, "/../input/", "float-top.abnf"));
        cpApiInString(vpApi, cpFloatMid);
        cpApiInFile(vpApi, cpMakeFileName(s_caBuf, SOURCE_DIR, "/../input/", "float-bot.abnf"));

        // display the concatenated grammar
        printf("\nThe Concatenated Grammar\n");
        vApiInToAscii(vpApi, NULL);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }
    vApiDtor(vpApi);
    return iReturn;
}

static int iInBadChars() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    char* cpFloat =
            "float    = sign decimal \x80 exponent\n"
            "sign     = [\"+\" / \"-\"]\n"
            "decimal  = integer [dot fraction]\n"
            "           / dot \xFF fraction\n"
            "integer  = 1*%d48-57\n"
            "dot      = \".\"\n"
            "fraction = *%d48-57\n"
            "exponent = [\"e\" esign exp]\n"
            "esign    = [\"+\" / \"-\"]\n"
            "\n"
            "exp      = 1*%d48-57";
    exception e;
    XCTOR(e);
    if(e.try){
        // try block

        // construct the parser from the pre-generated grammar files
        vpApi = vpApiCtor(&e);

        // validate the grammar
        cpApiInString(vpApi, cpFloat);
        vApiInValidate(vpApi, APG_FALSE);

        // display the concatenated grammar (will never be executed)
        vApiInToAscii(vpApi, NULL);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);

        // display the error log
        printf("\nThe Grammar Errors\n");
        void* vpLog = vpApiGetErrorLog(vpApi);
        vUtilPrintMsgs(vpLog);

        printf("\nThe Full Grammar\n");
        vApiInToAscii(vpApi, NULL);
        iReturn = EXIT_FAILURE;
    }
    vApiDtor(vpApi);
    return iReturn;
}

static int iInBadSyntax() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    char* cpFloat =
            "float    = sign decimal exponent\n"
            "sign     = [\"+\" / \"-\"]\n"
            "decimal  = integer [dot fraction]\n"
            "           / dot fraction\n"
            "integer  = 1*%d48-57\n"
            "1dot     = \".\"\n"
            "fraction = *%d48-57\n"
            "exponent = [\"e\" esign exp]\n"
            "esign    = [\"+\" / \"-\"]\n"
            "\n"
            "exp      = 1*%d48-57\n";
    exception e;
    XCTOR(e);
    if(e.try){
        // try block

        // construct the parser from the pre-generated grammar files
        vpApi = vpApiCtor(&e);

        // validate the grammar
        cpApiInString(vpApi, cpFloat);
        vApiInValidate(vpApi, APG_FALSE);
        vApiSyntax(vpApi, APG_FALSE);

        // display the concatenated grammar (will never be executed)
        vApiInToAscii(vpApi, NULL);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);

        // display the error log
        printf("\nThe Grammar Errors\n");
        void* vpLog = vpApiGetErrorLog(vpApi);
        vUtilPrintMsgs(vpLog);

        printf("\nThe Full Grammar\n");
        vApiInToAscii(vpApi, NULL);
        iReturn = EXIT_FAILURE;
    }
    vApiDtor(vpApi);
    return iReturn;
}

static int iInBadSemantics() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    char* cpFloat =
            "float    = sign decimal exponent\n"
            "sign     = [\"+\" / \"-\"]\n"
            "decimal  = integer [dot fraction]\n"
            "           / dot fraction\n"
            "integer  = 1*%d57-48\n"
            "dot     = \".\"\n"
            "fraction = *%d48-57\n"
            "exponent = [\"e\" esign exp]\n"
            "esign    = [\"+\" / \"-\"]\n"
            "\n"
            "exp      = 1*%d48-57\n";
    exception e;
    XCTOR(e);
    if(e.try){
        // try block

        // construct the parser from the pre-generated grammar files
        vpApi = vpApiCtor(&e);

        // validate the grammar
        cpApiInString(vpApi, cpFloat);
        vApiInValidate(vpApi, APG_FALSE);
        vApiSyntax(vpApi, APG_FALSE);
        vApiOpcodes(vpApi);

        // display the concatenated grammar (will never be executed)
        vApiInToAscii(vpApi, NULL);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);

        // display the error log
        printf("\nThe Grammar Errors\n");
        void* vpLog = vpApiGetErrorLog(vpApi);
        vUtilPrintMsgs(vpLog);

        printf("\nThe Full Grammar\n");
        vApiInToAscii(vpApi, NULL);
        iReturn = EXIT_FAILURE;
    }
    vApiDtor(vpApi);
    return iReturn;
}

static int iInPppt() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApi = NULL;
    static void* vpParser = NULL;
    static void* vpPpptParser  = NULL;
    char* cpInput = "{"
            "\"array\": [1,2,3,4],"
            "\"object\": {\"t\": true, \"f\": false, \"n\":null}"
            "}";
    pppt_size sPpptSize;
    parser_config sConfig;
    parser_state sState;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block

        // construct the parser from the pre-generated grammar files
        vpApi = vpApiCtor(&e);

        // construct a memory object for string conversion
        vpMem = vpMemCtor(&e);
        spPhrase = spUtilStrToPhrase(vpMem, cpInput);

        // validate the grammar
        cpApiInFile(vpApi, cpMakeFileName(s_caBuf, SOURCE_DIR, "/../input/", "json.abnf"));
        vApiInValidate(vpApi, APG_FALSE);
        vApiSyntax(vpApi, APG_FALSE);
        vApiOpcodes(vpApi);
        bApiAttrs(vpApi);

        // display the PPPT size
        printf("\nThe PPPT sizes\n");
        vApiPpptSize(vpApi, &sPpptSize);
        printf("minimum alphabet character: %"PRIuMAX"\n", sPpptSize.luiAcharMin);
        printf("maximum alphabet character: %"PRIuMAX"\n", sPpptSize.luiAcharMax);
        printf("        bytes per PPPT map: %"PRIuMAX"\n", sPpptSize.luiMapSize);
        printf("   number of maps in table: %"PRIuMAX"\n", sPpptSize.luiMaps);
        printf(" total table size in bytes: %"PRIuMAX"\n", sPpptSize.luiTableSize);

        // parse without PPPT
        vpParser = vpApiOutputParser(vpApi);
        memset(&sConfig, 0, sizeof(sConfig));
        sConfig.acpInput = spPhrase->acpPhrase;
        sConfig.uiInputLength = spPhrase->uiLength;
        sConfig.uiStartRule = uiParserRuleLookup(vpParser, "JSON-text");
        vParserParse(vpParser, &sConfig, &sState);
        printf("\nState showing node hits without PPPT\n");
        vUtilPrintParserState(&sState);

        // parse with PPPT
        vApiPppt(vpApi, NULL, 0);
        vpPpptParser = vpApiOutputParser(vpApi);
        memset(&sConfig, 0, sizeof(sConfig));
        sConfig.acpInput = spPhrase->acpPhrase;
        sConfig.uiInputLength = spPhrase->uiLength;
        sConfig.uiStartRule = uiParserRuleLookup(vpParser, "JSON-text");
        vParserParse(vpPpptParser, &sConfig, &sState);
        printf("\nState showing node hits with PPPT\n");
        vUtilPrintParserState(&sState);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }
    vParserDtor(vpParser);
    vParserDtor(vpPpptParser);
    vApiDtor(vpApi);
    vMemDtor(vpMem);
    return iReturn;
}

/**
 * \brief Main function for the basic application.
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
    switch(iCase){
    case 1:
        vPrintCase(1);
        return iApp();
    case 2:
        vPrintCase(2);
        return iInCat();
    case 3:
        vPrintCase(3);
        return iInBadChars();
    case 4:
        vPrintCase(4);
        return iInBadSyntax();
    case 5:
        vPrintCase(5);
        return iInBadSemantics();
    case 6:
        vPrintCase(6);
        return iInPppt();
    default:
        return iHelp();
    }
}

