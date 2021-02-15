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
/** \dir examples/ex-lines
 * \brief Examples of using the line parsing utilities..
 */

/** \file examples/ex-lines/main.c
 * \brief Driver for the line parsing utilities examples..
 *
This example will demonstrate the construction and use of the line parsing utilities.

SABNF grammars and many other files used by parser applications are defined as "lines" of text or characters.
Lines are typically ended with one or more of the ASCII control characters
 - 0x0A   line feed
 - 0x0D   carriage return
 - 0x0D 0x0A, carriage return, line feed pair

Parsing or separating the text into separate line and line ending data is needed in many APG applications and objects.
The `lines` object provides a unified means for identifying separate lines and iterating over them.

Similarly, arrays of Unicode characters often need the same facility.
The `linesu` object will do much the same for arrays of 32-bit Unicode code points as the `lines` object does for character strings.

Unicode recognizes the following [line ending characters](https://en.wikipedia.org/wiki/Newline#Unicode):
 - LF   0x0A   Line Feed
 - VT   0x0B   Vertical Tab
 - FF   0x0C   Form Feed
 - CR   0x0D   Carriage Return
 - CRLF 0x0D 0x0A Carriage Return followed by Line Feed
 - NEL  0x85   Next Line
 - LS   0x2028 Line Separator
 - PS   0x2029 Paragraph Separator

Application requirements.
  - application code must include header files:
      - ../../utilities/utilities.h
  - application compilation must include source code from the directories:
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - (none)

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Illustrate the use of the lines object for strings of characters.
 - case 3: Illustrate the use of the linesu object for arrays of 32-bit Unicode code points.
 */

/**
\page exlines The Line Parsing Utilities

This example will demonstrate the construction and use of the line parsing utilities.

SABNF grammars and many other files used by parser applications are defined as "lines" of text or characters.
Lines are typically ended with one or more of the ASCII control characters
 - 0x0A   line feed
 - 0x0D   carriage return
 - 0x0D 0x0A, carriage return, line feed pair

Parsing or separating the text into separate line and line ending data is needed in many APG applications and objects.
The `lines` object provides a unified means for identifying separate lines and iterating over them.

Similarly, arrays of Unicode characters often need the same facility.
The `linesu` object will do much the same for arrays of 32-bit Unicode code points as the `lines` object does for character strings.

Unicode recognizes the following [line ending characters](https://en.wikipedia.org/wiki/Newline#Unicode):
 - LF   0x0A   Line Feed
 - VT   0x0B   Vertical Tab
 - FF   0x0C   Form Feed
 - CR   0x0D   Carriage Return
 - CRLF 0x0D 0x0A Carriage Return followed by Line Feed
 - NEL  0x85   Next Line
 - LS   0x2028 Line Separator
 - PS   0x2029 Paragraph Separator

Application requirements.
  - application code must include header files:
      - ../../utilities/utilities.h
  - application compilation must include source code from the directories:
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - (none)

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Illustrate the use of the lines object for strings of characters.
 - case 3: Illustrate the use of the linesu object for arrays of 32-bit Unicode code points.
*/
#include "../../utilities/utilities.h"

static char* s_cpDescription =
        "Illustrate the construction and use of the line parsing object.";

static char* s_cppCases[] = {
        "Display application information.",
        "Illustrate the use of the lines object for strings of characters.",
        "Illustrate the use of the linesu object for arrays of 32-bit Unicode code points.",
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

static int iLines() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpLines = NULL;
    aint uiBufSize = 1024;
    char caBuf[1024];
    char* cpGrammar =
            "float    = sign decimal exponent\n"
            "sign     = [\"+\" / \"-\"]\r\n"
            "decimal  = integer [dot fraction]\n"
            "           / dot fraction\r\n"
            "integer  = 1*%d48-57\n"
            "dot      = \".\"\r\n"
            "fraction = *%d48-57\n"
            "exponent = [\"e\" esign exp]\r"
            "esign    = [\"+\" / \"-\"]\n\r"
            "exp      = 1*%d48-57";
    line* spLine, *spLineFirst;
    aint ui, uiChar, uiLine, uiRelIndex;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpLines = vpLinesCtor(&e, cpGrammar, strlen(cpGrammar));

        // display the information header
        char* cpHeader =
                "This example case uses the utilities lines object to parse an SABNF grammar\n"
                "with multiple types of line endings, including no line ending on the last line,\n"
                "and iterate over the lines, displaying the line information.\n";
        printf("\n%s", cpHeader);

        // display the totals
        printf("\nThe number of characters: %"PRIuMAX"\n", (luint)uiLinesLength(vpLines));
        printf("     The number of lines: %"PRIuMAX"\n", (luint)uiLinesCount(vpLines));

        // display the line information
        spLine = spLinesFirst(vpLines);
        printf("\nThe line information from the iterator.\n");
        printf("%12s %12s %12s %12s line\n", "line index", "char index", "line length", "text length");
        while(spLine){
            if(spLine->uiTextLength >= uiBufSize){
                XTHROW(&e, "buffer size too small for line");
            }
            memcpy(caBuf, &cpGrammar[spLine->uiCharIndex], spLine->uiTextLength);
            caBuf[spLine->uiTextLength] = 0;
            printf("%12d %12d %12d %12d %s\n",
                    (int)spLine->uiLineIndex, (int)spLine->uiCharIndex,
                    (int)spLine->uiLineLength, (int)spLine->uiTextLength, caBuf);
            spLine = spLinesNext(vpLines);
        }

        spLineFirst = spLinesFirst(vpLines);
        printf("\nThe line information from the array of lines.\n");
        printf("%12s %12s %12s %12s line\n", "line index", "char index", "line length", "text length");
        for(ui = 0; ui < uiLinesCount(vpLines); ui++){
            spLine = &spLineFirst[ui];
            if(spLine->uiTextLength >= uiBufSize){
                XTHROW(&e, "buffer size too small for line");
            }
            memcpy(caBuf, &cpGrammar[spLine->uiCharIndex], spLine->uiTextLength);
            caBuf[spLine->uiTextLength] = 0;
            printf("%12d %12d %12d %12d %s\n",
                    (int)spLine->uiLineIndex, (int)spLine->uiCharIndex,
                    (int)spLine->uiLineLength, (int)spLine->uiTextLength, caBuf);
            spLine = spLinesNext(vpLines);
        }

        // find lines
        printf("\nFind some lines.\n");
        for(uiChar = 50; uiChar < 10000; uiChar += 50){
            if(!bLinesFindLine(vpLines, uiChar, &uiLine, &uiRelIndex)){
                printf("Character %d is out of range (beyond the end of the last line.)\n", (int)uiChar);
                break;
            }
            printf("Character %d is in line %d at relative character offset %d.\n",
                    (int)uiChar, (int)uiLine, (int)uiRelIndex);
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vLinesDtor(vpLines);
    vMemDtor(vpMem);
    return iReturn;
}

static int iLinesu() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpLinesu = NULL;
    uint32_t uiaWords[] = {
        0x000013C2, 0x000013A6, 0x000013D3, 0x00000020, 0x000013A0, 0x0A,
        0x000013C2, 0x000013F4, 0x000013EB, 0x00000020, 0x0D,
        0x000013C2, 0x000013A8, 0x000013AB, 0x000013D3, 0x000013B8,  0x0D,
        0x000013BE, 0x00000020, 0x000013A0, 0x000013B4, 0x0B,
        0x000013A4, 0x000013C2, 0x000013B6, 0x000013F1, 0x00000020, 0x0D, 0x0A,
        0x000013A4, 0x000013BE, 0x000013D5, 0x000013BF, 0x2028,
        0x0000002E, 0x00000020, 0x000013A8, 0x000013E5, 0x000013C1, 0x000013B3,  0x0C,
        0x00000020, 0x000013A4, 0x000013C3, 0x000013B5, 0x000013CD, 0x000013D7,  0x85,
        0x000013D9, 0x00000020, 0x000013AC, 0x000013D7, 0x0000002E,  0x2029
    };
    line_u* spLine, *spLineFirst;
    aint ui, uj, uiChar, uiLine, uiRelIndex;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpLinesu = vpLinesuCtor(&e, uiaWords, (sizeof(uiaWords) / sizeof(uiaWords[0])));

        // display the information header
        char* cpHeader =
                "This example case uses the utilities linesu object to parse an array of \n"
                "32-bit Unicode code points.\n";
        printf("\n%s", cpHeader);

        printf("\nUnicode recognizes the following line ending characters:\n");
        printf("LF   0x0A      Line Feed\n");
        printf("VT   0x0B      Vertical Tab\n");
        printf("FF   0x0C      Form Feed\n");
        printf("CR   0x0D      Carriage Return\n");
        printf("CRLF 0x0D 0x0A Carriage Return, Line Feed pair\n");
        printf("NEL  0x85      Next Line\n");
        printf("LS   0x2028    Line Separator\n");
        printf("PS   0x2029    Paragraph Separator\n");

        // display the totals
        printf("\nThe number of code points: %"PRIuMAX"\n", (luint)uiLinesuLength(vpLinesu));
        printf("      The number of lines: %"PRIuMAX"\n", (luint)uiLinesuCount(vpLinesu));

        // display the line information
        spLine = spLinesuFirst(vpLinesu);
        printf("\nThe line information from the iterator.\n");
        printf("%12s %12s %12s %12s line\n", "line index", "char index", "line words", "text words");
        while(spLine){
            printf("%12d %12d %12d %12d ",
                    (int)spLine->uiLineIndex, (int)spLine->uiCharIndex,
                    (int)spLine->uiLineLength, (int)spLine->uiTextLength);
            for(ui = 0; ui < spLine->uiTextLength; ui++){
                printf("0x%08X, ", uiaWords[spLine->uiCharIndex + ui]);
            }
            printf("\n");
            spLine = spLinesuNext(vpLinesu);
        }

        spLineFirst = spLinesuFirst(vpLinesu);
        printf("\nThe line information from the array of lines.\n");
        printf("%12s %12s %12s %12s line\n", "line index", "char index", "line words", "text words");
        for(ui = 0; ui < uiLinesuCount(vpLinesu); ui++){
            spLine = &spLineFirst[ui];
            printf("%12d %12d %12d %12d ",
                    (int)spLine->uiLineIndex, (int)spLine->uiCharIndex,
                    (int)spLine->uiLineLength, (int)spLine->uiTextLength);
            for(uj = 0; uj < spLine->uiTextLength; uj++){
                printf("0x%08X, ", uiaWords[spLine->uiCharIndex + uj]);
            }
            printf("\n");
        }

        // find lines
        printf("\nFind some lines.\n");
        for(uiChar = 10; uiChar < 10000; uiChar += 10){
            if(!bLinesuFindLine(vpLinesu, uiChar, &uiLine, &uiRelIndex)){
                printf("Code point %d is out of range (beyond the end of the last line.)\n", (int)uiChar);
                break;
            }
            printf("Code point %d is in line %d at relative code point offset %d.\n",
                    (int)uiChar, (int)uiLine, (int)uiRelIndex);
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vLinesuDtor(vpLinesu);
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
    case 3:
        return iLinesu();
    default:
        return iHelp();
    }
}

