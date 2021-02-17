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
/** \dir ./examples/ex-trace
 * \brief Examples of tracing the parser and parser, memory and vector statistics,.
 */

/** \file ./examples/ex-trace/main.c
 * \brief Driver for the APG tracing and statistics examples..
 *
This example will demonstrate basic parser tracing, the primary debugging tool.
When a parser unexpectedly fails it could be that the grammar is in error or that input string is an invalid grammar phase.
The best way to find out what is going on is to examine each step the parser takes.
The default trace will show every step, but it is highly configurable to eliminate unneeded steps and zero in on the error.

APG also has detailed statistics gathering capabilities. Parser, memory and vector statistics are available.

Note that trace and statistics have display functions and therefore require <stdio.h>.
The APG library specifically excludes all I/O functions except when trace and/or statistics are enabled.

  - application code must include header files:
      - ../../utilities/utilities.h
      - ../../api/api.h (for parser generation)
  - application compilation must include source code from the directories:
      - ../../api
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - APG_TRACE
      - APG_STATS
      - APG_MEM_STATS
      - APG_VEC_STATS
      - APG_AST         (required for API)

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case  1: Display application information. (type names, type sizes and defined macros)
 - case  2: Illustrate default tracing with and without PPPT.
 - case  3: Generate a trace configuration file template.
 - case  4: Trace a restricted record range.
 - case  5: Trace rule names only.
 - case  6: Trace only specific rule names.
 - case  7: Parsing statistics, hit count vs alphabetical.
 - case  8: Parsing statistics, with and without PPPT.
 - case  9: Parsing statistics, cumulative for multiple parses.",
 - case 10: Illustrate memory statistics.
 - case 11: Illustrate vector statistics.
 */

/**
\page extrace Parser Tracing and Statistics
This example will demonstrate basic parser tracing, the primary debugging tool.
When a parser unexpectedly fails it could be that the grammar is in error or that input string is an invalid grammar phase
or both.
The best way to find out what is going on is to examine each step the parser takes.
The default trace will show every step, but it is highly configurable to eliminate unneeded steps and zero in on the error.

APG also has detailed statistics gathering capabilities. Parser, memory and vector statistics are available.

Note that trace and statistics have display functions and therefore require <stdio.h>.
The APG library specifically excludes all I/O functions except when trace and/or statistics are enabled.

  - application code must include header files:
      - ../../utilities/utilities.h
      - ../../api/api.h (for parser generation)
  - application compilation must include source code from the directories:
      - ../../api
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - APG_TRACE
      - APG_STATS
      - APG_MEM_STATS
      - APG_VEC_STATS
      - APG_AST         (required for API)

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case  1: Display application information. (type names, type sizes and defined macros)
 - case  2: Illustrate default tracing with and without PPPT.
 - case  3: Generate a trace configuration file template.
 - case  4: Trace a restricted record range.
 - case  5: Trace rule names only.
 - case  6: Trace only specific rule names.
 - case  7: Parsing statistics, hit count vs alphabetical.
 - case  8: Parsing statistics, with and without PPPT.
 - case  9: Parsing statistics, cumulative for multiple parses.",
 - case 10: Illustrate memory statistics.
 - case 11: Illustrate vector statistics.
*/
#include "../../utilities/utilities.h"
#include "../../api/api.h"

static char* s_cpDescription =
        "Illustrate parser tracing and parser, memory and vector statistics.";

static char* s_cppCases[] = {
        "Display application information.",
        "Illustrate default tracing with and without PPPT.",
        "Generate a trace configuration file template.",
        "Trace a restricted record range.",
        "Trace rule names only.",
        "Trace only specific rule names.",
        "Parsing statistics, hit count vs alphabetical.",
        "Parsing statistics, with and without PPPT.",
        "Parsing statistics, cumulative for multiple parses.",
        "Illustrate memory statistics.",
        "Illustrate vector statistics.",
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

static int iTraceDefault() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    static void* vpMem = NULL;
    static void* vpParser = NULL;
    static void* vpParserPppt = NULL;
    char* cpInput = "+123456789.0987654321E+100";
    parser_config sConfig;
    parser_state sState;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        vpApi = vpApiCtor(&e);

        // convert the input string to alphabet characters (in general, sizeof(achar) != sizeof(char))
        vpMem = vpMemCtor(&e);
        spPhrase = spUtilStrToPhrase(vpMem, cpInput);

        // construct a floating point parser without PPPT
        vApiFile(vpApi, "../input/float.abnf", APG_FALSE, APG_FALSE);
        vpParser = vpApiOutputParser(vpApi, APG_FALSE);

        // display the trace without PPPT
        printf("\nTrace without PPPT\n");
        vpTraceCtor(vpParser);
        memset(&sConfig, 0, sizeof(sConfig));
        sConfig.acpInput = spPhrase->acpPhrase;
        sConfig.uiInputLength = spPhrase->uiLength;
        sConfig.uiStartRule = uiParserRuleLookup(vpParser, "float");
        vParserParse(vpParser, &sConfig, &sState);

        // display the state without PPPT
        printf("\nParser State without PPPT\n");
        vUtilPrintParserState(&sState);

        // construct a floating point parser with PPPT
        vApiFile(vpApi, "../input/float.abnf", APG_FALSE, APG_TRUE);
        vpParserPppt = vpApiOutputParser(vpApi, APG_FALSE);

        // display the trace with PPPT
        printf("\nTrace with PPPT\n");
        vpTraceCtor(vpParserPppt);

        // NOTE: input string and sConfig remain the same
        vParserParse(vpParserPppt, &sConfig, &sState);

        // display the state without PPPT
        printf("\nParser State with PPPT\n");
        vUtilPrintParserState(&sState);

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
    vParserDtor(vpParserPppt);
    vApiDtor(vpApi);
    vMemDtor(vpMem);
    return iReturn;
}

static int iTraceConfigGen() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    static void* vpParser = NULL;
    static void* vpTrace = NULL;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        vpApi = vpApiCtor(&e);

        // construct a floating point parser without PPPT
        vApiFile(vpApi, "../input/float.abnf", APG_FALSE, APG_FALSE);
        vpParser = vpApiOutputParser(vpApi, APG_FALSE);

        // display the trace without PPPT
        printf("\nDisplay the Trace Configuration File to stdout\n");
        vpTrace = vpTraceCtor(vpParser);
        vTraceConfigGen(vpTrace, NULL);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // free up all allocated resources
    vParserDtor(vpParser);
    vApiDtor(vpApi);
    return iReturn;
}

static int iTraceConfigRange() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    static void* vpMem = NULL;
    static void* vpParser = NULL;
    static void* vpTrace = NULL;
    char* cpInput = "+123456789.0987654321E+100";
    parser_config sConfig;
    parser_state sState;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        vpApi = vpApiCtor(&e);

        // convert the input string to alphabet characters (in general, sizeof(achar) != sizeof(char))
        vpMem = vpMemCtor(&e);
        spPhrase = spUtilStrToPhrase(vpMem, cpInput);

        // construct a floating point parser without PPPT
        vApiFile(vpApi, "../input/float.abnf", APG_FALSE, APG_FALSE);
        vpParser = vpApiOutputParser(vpApi, APG_FALSE);

        // display the trace without PPPT
        char* cpConfig = "../input/float-config-range";
        printf("\nUsing trace configuration file %s \n", cpConfig);
        vpTrace = vpTraceCtor(vpParser);
        vTraceConfig(vpTrace, cpConfig);
        memset(&sConfig, 0, sizeof(sConfig));
        sConfig.acpInput = spPhrase->acpPhrase;
        sConfig.uiInputLength = spPhrase->uiLength;
        sConfig.uiStartRule = uiParserRuleLookup(vpParser, "float");
        vParserParse(vpParser, &sConfig, &sState);

        // display the state without PPPT
        printf("\nParser State without PPPT\n");
        vUtilPrintParserState(&sState);

        // free the memory allocation
        vMemFree(vpMem, spPhrase);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // free up all allocated resources
    vParserDtor(vpParser);
    vApiDtor(vpApi);
    vMemDtor(vpMem);
    return iReturn;
}

static int iTraceConfigRules() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    static void* vpMem = NULL;
    static void* vpParser = NULL;
    static void* vpTrace = NULL;
    char* cpInput = "+123456789.0987654321E+100";
    parser_config sConfig;
    parser_state sState;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        vpApi = vpApiCtor(&e);

        // convert the input string to alphabet characters (in general, sizeof(achar) != sizeof(char))
        vpMem = vpMemCtor(&e);
        spPhrase = spUtilStrToPhrase(vpMem, cpInput);

        // construct a floating point parser without PPPT
        vApiFile(vpApi, "../input/float.abnf", APG_FALSE, APG_FALSE);
        vpParser = vpApiOutputParser(vpApi, APG_FALSE);

        // display the trace without PPPT
        char* cpConfig = "../input/float-config-rules";
        printf("\nUsing trace configuration file %s \n", cpConfig);
        vpTrace = vpTraceCtor(vpParser);
        vTraceConfig(vpTrace, cpConfig);
        memset(&sConfig, 0, sizeof(sConfig));
        sConfig.acpInput = spPhrase->acpPhrase;
        sConfig.uiInputLength = spPhrase->uiLength;
        sConfig.uiStartRule = uiParserRuleLookup(vpParser, "float");
        vParserParse(vpParser, &sConfig, &sState);

        // display the state without PPPT
        printf("\nParser State without PPPT\n");
        vUtilPrintParserState(&sState);

        // free the memory allocation
        vMemFree(vpMem, spPhrase);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // free up all allocated resources
    vParserDtor(vpParser);
    vApiDtor(vpApi);
    vMemDtor(vpMem);
    return iReturn;
}

static int iTraceConfigSelect() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    static void* vpMem = NULL;
    static void* vpParser = NULL;
    static void* vpTrace = NULL;
    char* cpInput = "+123456789.0987654321E+100";
    parser_config sConfig;
    parser_state sState;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        vpApi = vpApiCtor(&e);

        // convert the input string to alphabet characters (in general, sizeof(achar) != sizeof(char))
        vpMem = vpMemCtor(&e);
        spPhrase = spUtilStrToPhrase(vpMem, cpInput);

        // construct a floating point parser without PPPT
        vApiFile(vpApi, "../input/float.abnf", APG_FALSE, APG_FALSE);
        vpParser = vpApiOutputParser(vpApi, APG_FALSE);

        // display the trace without PPPT
        char* cpConfig = "../input/float-config-select";
        printf("\nUsing trace configuration file %s \n", cpConfig);
        vpTrace = vpTraceCtor(vpParser);
        vTraceConfig(vpTrace, cpConfig);
        memset(&sConfig, 0, sizeof(sConfig));
        sConfig.acpInput = spPhrase->acpPhrase;
        sConfig.uiInputLength = spPhrase->uiLength;
        sConfig.uiStartRule = uiParserRuleLookup(vpParser, "float");
        vParserParse(vpParser, &sConfig, &sState);

        // display the state without PPPT
        printf("\nParser State without PPPT\n");
        vUtilPrintParserState(&sState);

        // free the memory allocation
        vMemFree(vpMem, spPhrase);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // free up all allocated resources
    vParserDtor(vpParser);
    vApiDtor(vpApi);
    vMemDtor(vpMem);
    return iReturn;
}

static int iStatsHits() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    static void* vpMem = NULL;
    static void* vpParser = NULL;
    static void* vpStats = NULL;
    char* cpInput =
            "{\n"
            "\"array\": [1, true, false, null, 2345],\n"
            "\"object\": {\n"
            "\"inner\": {\n"
            "\"t\": true,\n"
            "\"f\": false,\n"
            "\"s\": \"string\"\n"
            "},\n"
            "\"more\": [12345, 6789809, 234, 3456,456],\n"
            "\"key1\": 1234,\n"
            "\"key2\": \"end of object\"\n"
            "}\n"
    "}\n";
    parser_config sConfig;
    parser_state sState;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        vpApi = vpApiCtor(&e);

        // convert the input string to alphabet characters (in general, sizeof(achar) != sizeof(char))
        vpMem = vpMemCtor(&e);
        spPhrase = spUtilStrToPhrase(vpMem, cpInput);

        // construct a JSON parser without PPPT
        vApiFile(vpApi, "../input/json.abnf", APG_FALSE, APG_FALSE);
        vpParser = vpApiOutputParser(vpApi, APG_FALSE);
        vpStats = vpStatsCtor(vpParser);

        // parse without PPPT
        memset(&sConfig, 0, sizeof(sConfig));
        sConfig.acpInput = spPhrase->acpPhrase;
        sConfig.uiInputLength = spPhrase->uiLength;
        sConfig.uiStartRule = uiParserRuleLookup(vpParser, "JSON-text");
        vParserParse(vpParser, &sConfig, &sState);

        // display the state
        printf("\nParser State\n");
        vUtilPrintParserState(&sState);

        // display the stats with hit count
        printf("\nStatistics ordered on hit count.\n");
        vStatsToAscii(vpStats, "h", NULL);

        // display the stats alphabetically
        printf("\nStatistics ordered alphabetically on rule names.\n");
        vStatsToAscii(vpStats, "a", NULL);

        // free the memory allocation
        vMemFree(vpMem, spPhrase);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // free up all allocated resources
    // NOTE: the statistics objects are destroyed by the parser destructor
    //       no need to destroy them separately
    vParserDtor(vpParser);
    vApiDtor(vpApi);
    vMemDtor(vpMem);
    return iReturn;
}

static int iStatsPppt() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    static void* vpMem = NULL;
    static void* vpParser = NULL;
    static void* vpParserPppt = NULL;
    static void* vpStats = NULL;
    static void* vpStatsPppt = NULL;
    char* cpInput =
            "{\n"
            "\"array\": [1, true, false, null, 2345],\n"
            "\"object\": {\n"
            "\"inner\": {\n"
            "\"t\": true,\n"
            "\"f\": false,\n"
            "\"s\": \"string\"\n"
            "},\n"
            "\"more\": [12345, 6789809, 234, 3456,456],\n"
            "\"key1\": 1234,\n"
            "\"key2\": \"end of object\"\n"
            "}\n"
    "}\n";
    parser_config sConfig;
    parser_state sState;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        vpApi = vpApiCtor(&e);

        // convert the input string to alphabet characters (in general, sizeof(achar) != sizeof(char))
        vpMem = vpMemCtor(&e);
        spPhrase = spUtilStrToPhrase(vpMem, cpInput);

        // construct a JSON parser without PPPT
        vApiFile(vpApi, "../input/json.abnf", APG_FALSE, APG_FALSE);
        vpParser = vpApiOutputParser(vpApi, APG_FALSE);
        vpStats = vpStatsCtor(vpParser);

        // construct a JSON parser with PPPT
        vApiFile(vpApi, "../input/json.abnf", APG_FALSE, APG_TRUE);
        vpParserPppt = vpApiOutputParser(vpApi, APG_FALSE);
        vpStatsPppt = vpStatsCtor(vpParserPppt);

        // parse without PPPT
        memset(&sConfig, 0, sizeof(sConfig));
        sConfig.acpInput = spPhrase->acpPhrase;
        sConfig.uiInputLength = spPhrase->uiLength;
        sConfig.uiStartRule = uiParserRuleLookup(vpParser, "JSON-text");
        vParserParse(vpParser, &sConfig, &sState);

        // display the state
        printf("\nParser State\n");
        vUtilPrintParserState(&sState);

        // display the stats with hit count
        printf("\nStatistics without PPPT ordered on hit count.\n");
        vStatsToAscii(vpStats, "h", NULL);

        // parse with PPPT
        vParserParse(vpParserPppt, &sConfig, &sState);

        // display the state
        printf("\nParser State\n");
        vUtilPrintParserState(&sState);

        // display the stats with hit count
        printf("\nStatistics with PPPT ordered on hit count.\n");
        vStatsToAscii(vpStatsPppt, "h", NULL);

        // free the memory allocation
        vMemFree(vpMem, spPhrase);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // free up all allocated resources
    // NOTE: the statistics objects are destroyed by the parser destructor
    //       no need to destroy them separately
    vParserDtor(vpParser);
    vParserDtor(vpParserPppt);
    vApiDtor(vpApi);
    vMemDtor(vpMem);
    return iReturn;
}

static int iStatsCumulative() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    static void* vpMem = NULL;
    static void* vpParser = NULL;
    static void* vpStats = NULL;
    char* cpInput2 = "[true, false, 123456, {\"key\": \"string\"}]";
    char* cpInput =
            "{\n"
            "\"array\": [1, true, false, null, 2345],\n"
            "\"object\": {\n"
            "\"inner\": {\n"
            "\"t\": true,\n"
            "\"f\": false,\n"
            "\"s\": \"string\"\n"
            "},\n"
            "\"more\": [12345, 6789809, 234, 3456,456],\n"
            "\"key1\": 1234,\n"
            "\"key2\": \"end of object\"\n"
            "}\n"
    "}\n";
    parser_config sConfig;
    parser_state sState;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block - construct the API object
        vpApi = vpApiCtor(&e);

        // convert the input string to alphabet characters (in general, sizeof(achar) != sizeof(char))
        vpMem = vpMemCtor(&e);

        // construct a JSON parser without PPPT
        vApiFile(vpApi, "../input/json.abnf", APG_FALSE, APG_FALSE);
        vpParser = vpApiOutputParser(vpApi, APG_FALSE);
        vpStats = vpStatsCtor(vpParser);

        // parse input 1
        spPhrase = spUtilStrToPhrase(vpMem, cpInput);
        memset(&sConfig, 0, sizeof(sConfig));
        sConfig.acpInput = spPhrase->acpPhrase;
        sConfig.uiInputLength = spPhrase->uiLength;
        sConfig.uiStartRule = uiParserRuleLookup(vpParser, "JSON-text");
        vParserParse(vpParser, &sConfig, &sState);

        // display the state
        printf("\nParser State Input 1\n");
        vUtilPrintParserState(&sState);

        // display the stats with hit count
        printf("\nStatistics for input 1.\n");
        vStatsToAscii(vpStats, "h", NULL);

        // parse input 2
        spPhrase = spUtilStrToPhrase(vpMem, cpInput2);
        memset(&sConfig, 0, sizeof(sConfig));
        sConfig.acpInput = spPhrase->acpPhrase;
        sConfig.uiInputLength = spPhrase->uiLength;
        sConfig.uiStartRule = uiParserRuleLookup(vpParser, "JSON-text");
        vParserParse(vpParser, &sConfig, &sState);

        // display the state
        printf("\nParser State Input 2\n");
        vUtilPrintParserState(&sState);

        // display the stats with hit count
        printf("\nStatistics for input 1 + input 2.\n");
        printf("Notice that the start rule, JSON-text, appears twice, once for each parsed string.\n");
        vStatsToAscii(vpStats, "h", NULL);

        // free the memory allocation
        vMemFree(vpMem, spPhrase);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // free up all allocated resources
    // NOTE: the statistics objects are destroyed by the parser destructor
    //       no need to destroy them separately
    vParserDtor(vpParser);
    vApiDtor(vpApi);
    vMemDtor(vpMem);
    return iReturn;
}

static int iMemStats() {
    int iReturn = EXIT_SUCCESS;
    mem_stats sStats = {};
    static void* vpMem = NULL;
    char *cp1, *cp2;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        // allocate some memory
        vpMem = vpMemCtor(&e);
        cp1 = (char*)vpMemAlloc(vpMem, 128);
        cp2 = (char*)vpMemAlloc(vpMem, 1280);
        vpMemAlloc(vpMem, 12800);

        // check the stats
        printf("\nMemory Statistics: 3 allocations.\n");
        vMemStats(vpMem, &sStats);
        vUtilPrintMemStats(&sStats);

        // test free & realloc
        vMemFree(vpMem, cp1);
        printf("\nMemory Statistics: 2 allocations after 1 free.\n");
        vMemStats(vpMem, &sStats);
        vUtilPrintMemStats(&sStats);

        cp2 = (char*)vpMemRealloc(vpMem, cp2, 2056);
        printf("\nMemory Statistics: 2 allocations after 1 free and 1 reallocation.\n");
        vMemStats(vpMem, &sStats);
        vUtilPrintMemStats(&sStats);

        // clear and test
        vMemClear(vpMem);
        printf("\nMemory Statistics: after clearing the memory with vMemClear().\n");
        vMemStats(vpMem, &sStats);
        vUtilPrintMemStats(&sStats);

    }else{
        // catch block
        vUtilPrintException(&e);
    }
    vMemDtor(vpMem);
    return iReturn;
}

static int iVecStats() {
    int iReturn = EXIT_SUCCESS;
    vec_stats sStats = {};
    struct info{
        aint uiIndex;
        char caInfo[265];
    };
    static void* vpMem = NULL;
    static void* vpVec = NULL;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        struct info sInfo;
        struct info* spTest;
        vpMem = vpMemCtor(&e);
        vpVec = vpVecCtor(vpMem, sizeof(struct info), 15);

        // push some data
        sInfo.uiIndex = 1;
        strcpy(sInfo.caInfo, "first");
        vpVecPush(vpVec, &sInfo);

        // check the stats
        printf("\nVector Statistics: 1 push.\n");
        vVecStats(vpVec, &sStats);
        vUtilPrintVecStats(&sStats);

        // push some more data
        spTest = (struct info*)vpVecPushn(vpVec, NULL, 3);
        spTest[0].uiIndex = 1;
        strcpy(spTest[0].caInfo, "second");
        printf("\nVector Statistics: more data.\n");
        vVecStats(vpVec, &sStats);
        vUtilPrintVecStats(&sStats);

        // make it grow and confirm that everything is still there
        vpVecPushn(vpVec, NULL, 20);
        printf("\nVector Statistics: make the vector grow.\n");
        vVecStats(vpVec, &sStats);
        vUtilPrintVecStats(&sStats);

        // pop some data
        vpVecPop(vpVec);
        vpVecPop(vpVec);
        vpVecPopn(vpVec, 3);
        printf("\nVector Statistics: pop some data.\n");
        vVecStats(vpVec, &sStats);
        vUtilPrintVecStats(&sStats);

        // clean up
        vVecClear(vpVec);
        printf("\nVector Statistics: clear the vector.\n");
        vVecStats(vpVec, &sStats);
        vUtilPrintVecStats(&sStats);

    }else{
        // catch block
        vUtilPrintException(&e);
    }
    // NOTE: Memory object destruction frees all vector allocations.
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
        return iTraceDefault();
    case 3:
        return iTraceConfigGen();
    case 4:
        return iTraceConfigRange();
    case 5:
        return iTraceConfigRules();
    case 6:
        return iTraceConfigSelect();
    case 7:
        return iStatsHits();
    case 8:
        return iStatsPppt();
    case 9:
        return iStatsCumulative();
    case 10:
        return iMemStats();
    case 11:
        return iVecStats();
    default:
        return iHelp();
    }
}

