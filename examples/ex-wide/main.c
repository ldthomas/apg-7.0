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
/** \dir examples/ex-wide
 * \brief Examples of using the wide alphabet characters - 32-bit UTF-32..
 *
 */

/** \file examples/ex-wide/main.c
 * \brief Driver for the wide alphabet character examples..
 *
This example explores the use of wide alphabet characters.
Parsing 32-bit, UTF-32 characters.

  - application code must include header files:
       - ../../api/api.h (includes %utilities.h)
  - application compilation must include source code from the directories:
       - ../../api
       - ../../library
       - ../../utilities
  - application compilation must define macros:
       - APG_ACHAR=32
       - APG_AINT=64
       - APG_AST (required for the API)

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Parse lines of Cherokee language UTF-32 Unicode text
 */

/**
\page exunicode Wide Characters &ndash; Parsing UTF-32

This example explores the use of wide alphabet characters.
Parsing 32-bit, UTF-32 characters.

  - application code must include header files:
       - ../../api/api.h (includes %utilities.h)
  - application compilation must include source code from the directories:
       - ../../api
       - ../../library
       - ../../utilities
  - application compilation must define macros:
       - APG_ACHAR=32
       - APG_AINT=64
       - APG_AST (required for the API)

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Parse lines of Cherokee language UTF-32 Unicode text
*/
#include "../../api/api.h"

#include "source.h"

static const char* cpMakeFileName(char* cpBuffer, const char* cpBase, const char* cpDivider, const char* cpName){
	strcpy(cpBuffer, cpBase);
	strcat(cpBuffer, cpDivider);
	strcat(cpBuffer, cpName);
	return cpBuffer;
}

static char* s_cpDescription =
        "Illustrate parsing of wide characters.";

static char* s_cppCases[] = {
        "Display application information.",
        "Parse lines of Cherokee language UTF-32 Unicode text.",
};
static long int s_iCaseCount = (long int)(sizeof(s_cppCases) / sizeof(s_cppCases[0]));

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

typedef struct{
    aint uiOffset;
    aint uiLength;
} my_line;
static void vLine(callback_data* spData) {
    if (spData->uiParserState == ID_MATCH) {
        // increment the A node count
        void* vpVec = spData->vpUserData;
        my_line* spLine = (my_line*)vpVecPush(vpVec, NULL);
        spLine->uiOffset = spData->uiParserOffset;
        spLine->uiLength = spData->uiParserPhraseLength;
    }
}
static int iLines() {
    int iReturn = EXIT_SUCCESS;
    static void* vpApi = NULL;
    static void* vpMem = NULL;
    static void* vpVec = NULL;
    static void* vpParser = NULL;
    static FILE* spOut = NULL;
    char* cpCherokee =
            "lines = 1*line\n"
            "line = line-text %d13.10\n"
            "line-text = *(%x13A0-13F4 / %x20 / %x2e)\n";
    uint8_t ucaBuf[1024];
    aint uiBufSize = 1024;
    aint ui, uiSize;
    my_line* spMyLine;
    const achar* acpBeg, *acpEnd;
    const char* cpInput, *cpInputBig, *cpInputLittle;
    char* cpOutName = "cherokee.html";
    char* cpInBig = "cherokee.utf32be";
    char* cpInLittle = "cherokee.utf32le";
    const char* cpOutput;
    char caBufOut[PATH_MAX], caBufIn[PATH_MAX];
    parser_config sConfig;
    parser_state sState;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpApi = vpApiCtor(&e);
        vpMem = vpMemCtor(&e);
        vpVec = vpVecCtor(vpMem, sizeof(my_line), 64);

        // validate the alphabet character width
        if(sizeof(achar) != 4){
            XTHROW(&e, "sizeof(achar) must be == 4");
        }

        // get and display the input string
        if(bIsBigEndian()){
        	cpInput = cpMakeFileName(caBufIn, SOURCE_DIR, "/../input/", cpInBig);
        }else{
        	cpInput = cpMakeFileName(caBufIn, SOURCE_DIR, "/../input/", cpInLittle);
        }
        cpOutput = cpMakeFileName(caBufOut, SOURCE_DIR, "/../output/", cpOutName);

        // !!!! DEBUG
        printf(" input file name: %s\n", cpInput);
        printf("output file name: %s\n", cpOutput);
        // !!!! DEBUG

        uiSize = uiBufSize;
        vUtilFileRead(vpMem, cpInput, ucaBuf, &uiSize);
        if(uiSize > uiBufSize){
            XTHROW(&e, "buffer size too small for input file");
        }

        // concatenate the grammar from three sources
        vApiString(vpApi, cpCherokee, APG_FALSE, APG_TRUE);
        vpParser = vpApiOutputParser(vpApi);

        // validate the input
        memset(&sConfig, 0, sizeof(sConfig));
        sConfig.acpInput = (achar*)ucaBuf;
        sConfig.uiInputLength = (uiSize/4);
        sConfig.uiStartRule = 0;
        sConfig.vpUserData = vpVec;
        vParserSetRuleCallback(vpParser, uiParserRuleLookup(vpParser, "line-text"), vLine);
        vParserParse(vpParser, &sConfig, &sState);

        // display the state
        printf("\nParser State\n");
        vUtilPrintParserState(&sState);
        printf("\nlines parsed: %"PRIuMAX"\n", (luint)uiVecLen(vpVec));


        // generate the HTML file
        spOut = fopen(cpOutput, "wb");
        if(!spOut){
            XTHROW(&e, "can't open output file for HTML");
        }
        fprintf(spOut, "<!DOCTYPE html>\n");
        fprintf(spOut, "<html lang=\"en\">\n");
        fprintf(spOut, "  <head>\n");
        fprintf(spOut, "    <meta charset=\"utf-8\">\n");
        fprintf(spOut, "    <title>Cherokee Text</title>\n");
        fprintf(spOut, "  </head>\n");
        fprintf(spOut, "  <body>\n");
        fprintf(spOut, "  <h1>Cherokee Sample</h1>\n");
        fprintf(spOut, "  <p>Wikipedia <a href=\"https://en.wikipedia.org/wiki/Cherokee_language#Samples\">source</a>. </p>\n");
        fprintf(spOut, "  <p>\n");
        for(ui = 0; ui < uiVecLen(vpVec); ui++){
            spMyLine = (my_line*)vpVecAt(vpVec, ui);
            acpBeg = sConfig.acpInput + spMyLine->uiOffset;
            acpEnd = acpBeg + spMyLine->uiLength;
            for(; acpBeg < acpEnd; acpBeg++){
                fprintf(spOut, "&#%"PRIuMAX";", (luint)*acpBeg);
            }
            fprintf(spOut, "  <br>\n");
        }
        fprintf(spOut, "  </p>\n");
        fprintf(spOut, "  </body>\n");
        fprintf(spOut, "</html>\n");

        printf("\nOpen file %s in browser to view parsed lines.\n", cpOutput);


    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    if(spOut){
        fclose(spOut);
    }
    vParserDtor(vpParser);
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
    if((iCase > 0) && (iCase <= s_iCaseCount)){
        printf("%s\n", s_cppCases[iCase -1]);
    }
    switch(iCase){
    case 1:
        return iApp();
    case 2:
        return iLines();
    default:
        return iHelp();
    }
}

