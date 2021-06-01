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

/** \file examples/ex-ast/main.c
 * \brief Driver for the AST example.
 *
The [Abstract Syntax Tree](https://en.wikipedia.org/wiki/Abstract_syntax_tree),
or AST, is a subset of the [concrete syntax tree](https://en.wikipedia.org/wiki/Parse_tree).
That is, a partial representation of the full syntax of the parsed input string.
As such, the AST has three features of interest here.
 - it only has nodes representing the rules of interest to the application
 - it only has successful phrase matches to those rules
 - the full matched phrase for the rule is known to the application at both the downward and upward traversal of the node

While call back functions from the rule name node operations during the actual parsing traversal of the full syntax tree
can be useful and often complete,
there is one serious pitfall that the application needs to be aware of and able to handle that is not present
when traversing or translating from the AST.
Namely, successfully matched rule nodes in branches that ultimately fail.
If attempting to translate directly from the syntax tree, precautions need to be taken to recognize this situation
and delete any translated information from failed branches.

Many real-life examples no doubt exist, but specifically
see the `"domainlabel"` rule
in the [Session Initiation Protocol](\ref exsip) (SIP) example.

Here, however, only an extremely simple grammar will be used to illustrate the problem.

  - application code must include header files:
      - ../../api/api.h (for parser generation, includes utlities.h)
  - application compilation must include source code from the directories:
      - ../../api
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - APG_AST

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case  1: Display application information. (type names, type sizes and defined macros)
 - case  2: Illustrate rule call back function pitfall and solution with AST.
 */

/**
\page exast The Abstract Syntax Tree &ndash; Using the AST

The [Abstract Syntax Tree](https://en.wikipedia.org/wiki/Abstract_syntax_tree),
or AST, is a subset of the [concrete syntax tree](https://en.wikipedia.org/wiki/Parse_tree).
That is, a partial representation of the full syntax of the parsed input string.
As such, the AST has three features of interest here.
 - it only has nodes representing the rules of interest to the application
 - it only has successful phrase matches to those rules
 - the full matched phrase for the rule is known to the application at both the downward and upward traversal of the node

While call back functions from the rule name node operations during the actual parsing traversal of the full syntax tree
can be useful and often complete,
there is one serious pitfall that the application needs to be aware of and able to handle that is not present
when traversing or translating from the AST.
Namely, successfully matched rule nodes in branches that ultimately fail.
If attempting to translate directly from the syntax tree, precautions need to be taken to recognize this situation
and delete any translated information from failed branches.

Many real-life examples no doubt exist, but specifically
see the `"domainlabel"` rule
in the [Session Initiation Protocol](\ref exsip) (SIP) example
or the `"sub-domain"` rule in case 2 of the [apgex](\ref exapgex) example.

Here, however, only an extremely simple grammar will be used to illustrate the problem.

  - application code must include header files:
      - ../../api/api.h (for parser generation, includes utlities.h)
  - application compilation must include source code from the directories:
      - ../../api
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - APG_AST

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case  1: Display application information. (type names, type sizes and defined macros)
 - case  2: Illustrate rule call back function pitfall and solution with AST.
*/
#include "../../api/api.h"

static char* s_cpDescription =
        "Example demonstrating the use and usefulness of the AST.";

static char* s_cppCases[] = {
        "Display application information.",
        "Illustrate the rule call back function pitfall and solution with AST.",
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

typedef struct{
    exception* spException;
    aint uiCountA;
    aint uiCountB;
    aint uiCountC;
} my_data;

static void vRuleA(callback_data* spData) {
    if (spData->uiParserState == ID_MATCH) {
        // increment the A node count
        my_data* spMyData = (my_data*)spData->vpUserData;
        spMyData->uiCountA++;
    }
}
static void vRuleB(callback_data* spData) {
    if (spData->uiParserState == ID_MATCH) {
        // increment the A node count
        my_data* spMyData = (my_data*)spData->vpUserData;
        spMyData->uiCountB++;
    }
}
static void vRuleC(callback_data* spData) {
    if (spData->uiParserState == ID_MATCH) {
        // increment the A node count
        my_data* spMyData = (my_data*)spData->vpUserData;
        spMyData->uiCountC++;
    }
}
static aint uiAstA(ast_data* spData) {
    if (spData->uiState == ID_AST_POST) {
        // increment the A node count
        my_data* spMyData = (my_data*)spData->vpUserData;
        spMyData->uiCountA++;
    }
    return ID_AST_OK;
}
static aint uiAstB(ast_data* spData) {
    if (spData->uiState == ID_AST_POST) {
        // increment the A node count
        my_data* spMyData = (my_data*)spData->vpUserData;
        spMyData->uiCountB++;
    }
    return ID_AST_OK;
}
static aint uiAstC(ast_data* spData) {
    if (spData->uiState == ID_AST_POST) {
        // increment the A node count
        my_data* spMyData = (my_data*)spData->vpUserData;
        spMyData->uiCountC++;
    }
    return ID_AST_OK;
}
static int iAst() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    static void* vpAst = NULL;
    static void* vpMem = NULL;
    static void* vpParser = NULL;
    char* cpGrammar = "S = (1*A 1*B) / (1*A 1*C)\n"
            "A = \"a\"\n"
            "B = \"b\"\n"
            "C = \"c\"\n";
    char* cpInput = "aaaacc";
    parser_config sConfig;
    parser_state sState;
    apg_phrase* spPhrase;
    my_data sMyData = {};
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        vpApi = vpApiCtor(&e);

        // convert the input string to alphabet characters (in general, sizeof(achar) != sizeof(char))
        vpMem = vpMemCtor(&e);
        spPhrase = spUtilStrToPhrase(vpMem, cpInput);
        memset(&sMyData, 0, sizeof(sMyData));
        sMyData.spException = &e;

        // construct a simple parser without PPPT
        vApiString(vpApi, cpGrammar, APG_FALSE, APG_FALSE);
        vpParser = vpApiOutputParser(vpApi);
        vpAst = vpAstCtor(vpParser);

        // the parser call back functions
        vParserSetRuleCallback(vpParser, uiParserRuleLookup(vpParser, "A" ), vRuleA);
        vParserSetRuleCallback(vpParser, uiParserRuleLookup(vpParser, "B" ), vRuleB);
        vParserSetRuleCallback(vpParser, uiParserRuleLookup(vpParser, "C" ), vRuleC);

        // the AST call back functions
        vAstSetRuleCallback(vpAst, uiParserRuleLookup(vpParser, "A" ), uiAstA);
        vAstSetRuleCallback(vpAst, uiParserRuleLookup(vpParser, "B" ), uiAstB);
        vAstSetRuleCallback(vpAst, uiParserRuleLookup(vpParser, "C" ), uiAstC);

        // count the number of successful visits to the A node.
        printf("\nThe parsing problem: count the occurrences of A in the input string.\n");
        printf("                     the parser, without built-in protection against failed branches, counts the As twice\n");
        printf("                     generating and translating an AST solves the problem\n");
        memset(&sConfig, 0, sizeof(sConfig));
        sConfig.acpInput = spPhrase->acpPhrase;
        sConfig.uiInputLength = spPhrase->uiLength;
        sConfig.uiStartRule = 0;
        sConfig.vpUserData = (void*)&sMyData;
//        vpTraceCtor(vpParser, NULL);
        vParserParse(vpParser, &sConfig, &sState);

        // display the input string
        printf("\nThe Input String\n");
        printf("input string: %s\n", cpInput);

        // display the state without PPPT
        printf("\nParser State without PPPT\n");
        vUtilPrintParserState(&sState);

        // display the matched parser rule counts
        printf("\nMatched Rule Counts from Parser (notice the A rules get counted twice)\n");
        printf("A: %"PRIuMAX"\n", (luint)sMyData.uiCountA);
        printf("B: %"PRIuMAX"\n", (luint)sMyData.uiCountB);
        printf("C: %"PRIuMAX"\n", (luint)sMyData.uiCountC);

        // translate the AST
        memset(&sMyData, 0, sizeof(sMyData));
        sMyData.spException = &e;
        vAstTranslate(vpAst, (void*)&sMyData);

        // display the matched AST rule counts
        printf("\nMatched Rule Counts from AST\n");
        printf("A: %"PRIuMAX"\n", (luint)sMyData.uiCountA);
        printf("B: %"PRIuMAX"\n", (luint)sMyData.uiCountB);
        printf("C: %"PRIuMAX"\n", (luint)sMyData.uiCountC);

        // display the AST in XML format
        printf("\nThe AST in XML Format\n");
        bUtilAstToXml(vpAst, "u", NULL);

        // free the memory allocation
        vMemFree(vpMem, spPhrase);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // free up all allocated resources
    // NOTE: the trace objects are destroyed by the parser destructor
    //       no need to destroy them separately
    vParserDtor(vpParser);
    vApiDtor(vpApi);
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
        return iAst();
    default:
        return iHelp();
    }
}

