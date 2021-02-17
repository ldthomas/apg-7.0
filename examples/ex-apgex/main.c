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
/** \dir ./examples/ex-apgex
 * \brief Examples of using the APG pattern-matching utility..
 */

/** \file examples/ex-apgex/main.c
 * \brief Driver for the APG pattern-matching utility examples.
 *
See \ref exapgex_req.
 */

/**
\page exapgex apgex &ndash; The Phrase Matching Engine

  - application code must include header files:
      - ../../apgex/apgex.h (includes %api.h & %utilities.h)
  - application compilation must include source code from the directories:
      - ../../apgex
      - ../../api
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - APG_BKR
      - APG_AST
      - APG_TRACE
      - APG_ACHAR=32

Note that some examples will require 32-bit alphabet character codes. Therefore, all parsers in the application
including the API for the pattern must use 32-bit characters. This emphasizes the importance of always
converting input strings to `achar` alphabet character codes prior to parsing.

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Illustrate the basic use of the default mode, verifying and matching the parts of an email address.
 - case 3: Illustrate all the details of an email pattern match.
 - case 4: Illustrate the apgex object's properties before and after a successful match
 - case 5: Illustrate pattern-matching in global mode.
 - case 6: Illustrate pattern-matching in sticky mode.
 - case 7: Illustrate the trace mode, with and without PPPT, ASCII and HTML displays.
 - case 8: Illustrate patterns with User-Defined Terminals (UDTs).
 - case 9: Illustrate simple testing for a matched pattern without detailed results.
 - case 10: Illustrate using the AST for a complex translation of the matched pattern.
 - case 11: Illustrate the replacement of matched phrases with simple phrases and compound phrases.
 - case 12: Illustrate using matched phrases as delimiters to split a phrase into an array of sub-phrases.
 - case 13: Illustrate defining word and line boundaries to find words and lines.
 - case 14: Illustrate extracting quoted and unquoted fields from Comma Separated Value (CSV) records.
 - case 15: Illustrate the use of patterns with wide characters.
 - case 16: Illustrate back references, universal and parent modes.
*/
#include "../../apgex/apgex.h"

static char* s_cpDescription =
        "Illustrate the construction and use of the apgex pattern-matching object.";

static char* s_cppCases[] = {
        "Display application information.",
        "Illustrate the basic use of the default mode, verifying and matching the parts of an email address.",
        "Illustrate all the details of an email pattern match.",
        "Illustrate the apgex object's properties before and after a successful match",
        "Illustrate pattern-matching in global mode.",
        "Illustrate pattern-matching in sticky mode.",
        "Illustrate the trace mode, with and without PPPT, ASCII and HTML displays.",
        "Illustrate patterns with User-Defined Terminals (UDTs).",
        "Illustrate simple testing for a matched pattern without detailed results.",
        "Illustrate using the AST for a complex translation of a recursive pattern.",
        "Illustrate the replacement of matched phrases with simple phrases and compound phrases.",
        "Illustrate using matched phrases as delimiters to split a phrase into an array of sub-phrases.",
        "Illustrate defining word and line boundaries to find words and lines.",
        "Illustrate extracting quoted and unquoted fields from Comma Separated Value (CSV) records.",
        "Illustrate the use of patterns with wide characters.",
        "Illustrate back references, universal and parent modes.",
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

static int iPatterns() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApgex = NULL;
    static void* vpApi = NULL;
    static void* vpParser = NULL;
    char* cpEmailGrammar =
            "email-address   = %^ local \"@\" domain %$\n"
            "local           = local-word *(\".\" local-word)\n"
            "domain          = 1*(sub-domain \".\") top-domain\n"
            "local-word      = 1*local-char\n"
            "local-char      = alpha / num / special\n"
            "sub-domain      = 1*sub-domain-char\n"
            "top-domain      = 2*6top-domain-char\n"
            "sub-domain-char = alpha / num / \"-\"\n"
            "top-domain-char = alpha\n"
            "alpha           = %d65-90 / %d97-122\n"
            "num             = %d48-57\n"
            "special         = %d33 / %d35 / %d36-39 / %d42-43 / %d45 / %d47\n"
            "                / %d61    / %d63 / %d94-96 / %d123-126\n";
    char* cpEmail = "just.me@my.email.domain.com";
    char* cpGrammarFile = "../input/email.abnf";
    apgex_result sResult;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);
        vpApi = vpApiCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates the three methods of defining the pattern for\n"
                "verification of an email address.\n";
        printf("\n%s", cpHeader);

        // convert the input string to an alphabet character phrase
        spPhrase = spUtilStrToPhrase(vpMem, cpEmail);
        printf("\nThe email pattern:\n%s", cpEmailGrammar);
        printf("\nThe email to match: %s\n", cpEmail);

        // use the string version of the grammar
        printf("\nvApgexPattern: use an application-defined string to define the pattern.\n");
        vApgexPattern(vpApgex, cpEmailGrammar, "");
        sResult = sApgexExec(vpApgex, spPhrase);
        vApgexDisplayResult(vpApgex, &sResult, NULL);

        // use the string version of the grammar
        printf("\nvApgexPatternFile: use a file to define the pattern.\n");
        vApgexPatternFile(vpApgex, cpGrammarFile, "");
        sResult = sApgexExec(vpApgex, spPhrase);
        vApgexDisplayResult(vpApgex, &sResult, NULL);

        // use the parser version with its intrinsic grammar
        printf("\nvApgexPatternParser: use a pre-constructed parser to define the pattern.\n");
        vApiFile(vpApi, cpGrammarFile, APG_FALSE, APG_FALSE);
        vpParser = vpApiOutputParser(vpApi, APG_FALSE);
        vApgexPatternParser(vpApgex, vpParser, "");
        sResult = sApgexExec(vpApgex, spPhrase);
        vApgexDisplayResult(vpApgex, &sResult, NULL);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vParserDtor(vpParser);
    vApiDtor(vpApi);
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}

static int iResults() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApgex = NULL;
    static void* vpApi = NULL;
    char* cpEmail = "just.me@my.email.domain.com";
    char* cpEmail2 = "This email address is a fake just.me@my.email.domain.com so don't share it with anyone.";
    char* cpGrammarFile = "../input/email.abnf";
    apgex_result sResult;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);
        vpApi = vpApiCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates details of the pattern-matching result.\n"
                "Three cases show minimal, partial and full rule results.\n";
        printf("\n%s", cpHeader);

        // convert the input string to an alphabet character phrase
        spPhrase = spUtilStrToPhrase(vpMem, cpEmail);
        vApiInClear(vpApi);
        printf("\nThe email pattern:\n%s", cpApiInFile(vpApi, cpGrammarFile));
        printf("\nThe email to match: %s\n", cpEmail);

        // display minimal result
        printf("\nMinimal result: By default the result only shows the full pattern match.\n");
        vApgexPatternFile(vpApgex, cpGrammarFile, "");
        sResult = sApgexExec(vpApgex, spPhrase);
        vApgexDisplayResult(vpApgex, &sResult, NULL);

        // enable select rules
        printf("\nIntermediate result: Display the sub-phrases for a few select rules.\n");
        vApgexEnableRules(vpApgex, "local, domain, local-word, sub-domain, top-domain", APG_TRUE);
        sResult = sApgexExec(vpApgex, spPhrase);
        vApgexDisplayResult(vpApgex, &sResult, NULL);

        // enable all rules
        printf("\nFull result: Display the sub-phrases for all rules.\n");
        vApgexEnableRules(vpApgex, "--all", APG_TRUE);
        sResult = sApgexExec(vpApgex, spPhrase);
        vApgexDisplayResult(vpApgex, &sResult, NULL);


        // display minimal result with left and right context
        printf("\n    Embedded phrase: Find the pattern in a longer string. Display left and right context\n");
        printf("The embedded phrase: %s\n", cpEmail2);
        spPhrase = spUtilStrToPhrase(vpMem, cpEmail2);
        vApgexPatternFile(vpApgex, cpGrammarFile, "");
        sResult = sApgexExec(vpApgex, spPhrase);
        vApgexDisplayResult(vpApgex, &sResult, NULL);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApiDtor(vpApi);
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}

static int iProperties() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApgex = NULL;
    static void* vpApi = NULL;
    char* cpEmailGood = "This, just.me@my.email.domain.com, is an email address.";
    char* cpEmailBad = "Not an email address.";
    char* cpGrammarFile = "../input/email.abnf";
    apgex_result sResult;
    apgex_properties sProperties;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);
        vpApi = vpApiCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates the pattern-matching properties.\n"
                "Properties are illustrated before a pattern match, after a successful match\n"
                "and after an unsuccessful match.\n";
        printf("\n%s", cpHeader);

        // before the match
        printf("\nProperties before the pattern match:\n");
        vApgexPatternFile(vpApgex, cpGrammarFile, "gpth");
        sProperties = sApgexProperties(vpApgex);
        vApgexDisplayProperties(vpApgex, &sProperties, NULL);

        // after successful match
        printf("\nProperties after successful pattern match:\n");
        vApgexPatternFile(vpApgex, cpGrammarFile, "");
        spPhrase = spUtilStrToPhrase(vpMem, cpEmailGood);
        sResult = sApgexExec(vpApgex, spPhrase);
        sProperties = sApgexProperties(vpApgex);
        vApgexDisplayProperties(vpApgex, &sProperties, NULL);

        // after an unsuccessful match
        printf("\nProperties after an unsuccessful pattern match:\n");
        vApgexPatternFile(vpApgex, cpGrammarFile, "");
        spPhrase = spUtilStrToPhrase(vpMem, cpEmailBad);
        sResult = sApgexExec(vpApgex, spPhrase);
        sProperties = sApgexProperties(vpApgex);
        vApgexDisplayProperties(vpApgex, &sProperties, NULL);

        // just to silence unused warnings
        sProperties.uiLastIndex = sResult.uiLastIndex;

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApiDtor(vpApi);
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}

static int iGlobalMode() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApgex = NULL;
    char* cpGrammar = "word = %s\"abc\"\n";
    char* cpStr = "Learn your abcs by repeating, abc, abc, abc over an over again.";
    apgex_result sResult;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates pattern-matching in global mode.\n"
                "Setting the global flag \"g\" facilitates finding all occurrences of a phrase in a sting.\n";
        printf("\n%s", cpHeader);
        printf("\nThe Grammar\n");
        printf("%s\n", cpGrammar);
        printf("The Input String\n");
        printf("%s\n", cpStr);

        // before the match
        printf("\nFind all occurrences of the grammar phrase in the input string.\n");
        spPhrase = spUtilStrToPhrase(vpMem, cpStr);
        vApgexPattern(vpApgex, cpGrammar, "g");
        sResult = sApgexExec(vpApgex, spPhrase);
        while(sResult.spResult){
            vApgexDisplayResult(vpApgex, &sResult, NULL);
            sResult = sApgexExec(vpApgex, spPhrase);
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}

static int iStickyMode() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApgex = NULL;
    char* cpGrammar = "word = %s\"abc\"\n";
    char* cpStr = "Learn your abcs by repeating, abcabcabc over an over again.";
    apgex_result sResult;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates pattern-matching in sticky mode.\n"
                "Setting the sticky flag \"y\" facilitates finding only occurrences\n"
                "at a fixed position in the input string. Additionally, it can find consecutive\n"
                "occurrences as long as there are no characters in between.\n";
        printf("\n%s", cpHeader);
        printf("\nThe Grammar\n");
        printf("%s\n", cpGrammar);
        printf("The Input String\n");
        printf("%s\n", cpStr);

        // beginning of string
        printf("\nAttempt finding a match at the beginning of the string.\n");
        spPhrase = spUtilStrToPhrase(vpMem, cpStr);
        vApgexPattern(vpApgex, cpGrammar, "y");
        sResult = sApgexExec(vpApgex, spPhrase);
        vApgexDisplayResult(vpApgex, &sResult, NULL);

        // at position of first occurrence
        printf("\nFind a match at a set position.\n");
        vApgexSetLastIndex(vpApgex, 11);
        sResult = sApgexExec(vpApgex, spPhrase);
        vApgexDisplayResult(vpApgex, &sResult, NULL);

        // consecutive phrases
        printf("\nFind consecutive phrases.\n");
        vApgexSetLastIndex(vpApgex, 30);
        sResult = sApgexExec(vpApgex, spPhrase);
        while(sResult.spResult){
            vApgexDisplayResult(vpApgex, &sResult, NULL);
            sResult = sApgexExec(vpApgex, spPhrase);
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}

static int iTraceMode() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApgex = NULL;
    char* cpGrammar = "word = \"abc\" / \"xyz\"\n";
    char* cpStr = "---xyz---";
    char* cpOutput = "../output/trace.html";
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates the trace mode.\n"
                "Setting the trace flag \"t\" will generate a trace of each phrase-matching attempt.\n"
                "By default, the display will be in ASCII mode and displayed to stdout.\n"
                "With the PPPT flag \"p\" set the PPPT trace can be compared to the previous without.\n"
                "Finally, with the \"th\" flags set, the trace will be generated in HTML format.\n";
        printf("\n%s", cpHeader);
        printf("\nThe Grammar\n");
        printf("%s\n", cpGrammar);
        printf("The Input String\n");
        printf("%s\n", cpStr);

        // beginning of string
        printf("\nTrace of all phrase-matching attempts.\n");
        spPhrase = spUtilStrToPhrase(vpMem, cpStr);
        vApgexPattern(vpApgex, cpGrammar, "t");
        sApgexExec(vpApgex, spPhrase);

        printf("\nCompare trace with PPPT to previous without PPPT.\n");
        vApgexPattern(vpApgex, cpGrammar, "tp");
        sApgexExec(vpApgex, spPhrase);

        printf("\nDisplay trace in HTML format.\n");
        printf("Display %s in any browser.\n",cpOutput);
        vApgexPattern(vpApgex, cpGrammar, "th");
        void* vpTrace = vpApgexGetTrace(vpApgex);
        if(!vpTrace){
            XTHROW(&e, "should have a trace context pointer here");
        }
        vTraceSetOutput(vpTrace, cpOutput);
        sApgexExec(vpApgex, spPhrase);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}
#define HYPHEN 45
#define BANG   33
#define GT     62
#define LT     60
static void vCommentUdt(callback_data* spData){
    if(spData->uiParserState == ID_ACTIVE){
        const achar* acpChar = spData->acpString + spData->uiParserOffset;
        const achar* acpEnd = spData->acpString + spData->uiStringLength;
        achar acPrevChar1 = 0;
        achar acPrevChar2 = 0;
        achar acPrevChar3 = 0;
        achar acChar;
        aint uiLen = 0;
        spData->uiCallbackState = ID_NOMATCH;
        spData->uiCallbackPhraseLength = 0;
        if((acpChar +3) < acpEnd){
            if(*acpChar++ != LT){
                return;
            }
            if(*acpChar++ != BANG){
                return;
            }
            if(*acpChar++ != HYPHEN){
                return;
            }
            if(*acpChar++ != HYPHEN){
                return;
            }
        }
        while(acpChar < acpEnd){
            if(acPrevChar2 == HYPHEN && acPrevChar3 == HYPHEN){
                return;
            }
            acChar = *acpChar;
            if(acChar == GT){
                if(acPrevChar1 == HYPHEN && acPrevChar2 == HYPHEN){
                    // success
                    spData->uiCallbackState = ID_MATCH;
                    spData->uiCallbackPhraseLength = uiLen + 5;
                    return;
                }
            }
            acPrevChar3 = acPrevChar2;
            acPrevChar2 = acPrevChar1;
            acPrevChar1 = acChar;
            acpChar++;
            uiLen++;
        }
    }
}


static int iUdt() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApgex = NULL;
    char* cpGrammar =
            "tags      = start-tag text end-tag\n"
            "          / empty-tag\n"
            "          / u_comment\n"
            "start-tag = %d60 name %d62\n"
            "end-tag   = %d60.47 name %d62\n"
            "empty-tag = %d60 name %d47.62\n"
            "name      = %d97-122 *(%d97-122 / %d48-57)\n"
            "text      = *%d97-122\n";
    char* cpStr = "<start>text</start>  <!-- comment --> <empty/>";
    apgex_result sResult;
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates the patterns with User-Defined Terminals, UDTs.\n"
                "A simple mockup of XML tags is used with a UDT for the comment tag.\n";
        printf("\n%s", cpHeader);
        printf("\nThe Grammar\n");
        printf("%s\n", cpGrammar);
        printf("The Input String\n");
        printf("%s\n", cpStr);

        // beginning of string
        printf("\nFind all tags.\n");
        spPhrase = spUtilStrToPhrase(vpMem, cpStr);
        vApgexPattern(vpApgex, cpGrammar, "g");
        vApgexDefineUDT(vpApgex, "u_comment", vCommentUdt);
        vApgexEnableRules(vpApgex, "start-tag, end-tag, empty-tag, u_comment", APG_TRUE);
        sResult = sApgexExec(vpApgex, spPhrase);
        while(sResult.spResult){
            vApgexDisplayResult(vpApgex, &sResult, NULL);
            sResult = sApgexExec(vpApgex, spPhrase);
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}
static int iTest() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApgex = NULL;
    char* cpGrammar = "word = %s\"abc\"\n";
    char* cpStr = "Learn your abcs by repeating, abcabcabc over an over again.";
    abool bTest;
    char* cpYes = "yes";
    char* cpNo = "no";
    apg_phrase* spPhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates testing for a match.\n"
                "The modes are treated exactly the same as for executing a phrase match.\n"
                "The difference is that testing only gives a yes or no answer. The matched phrase is not captured.\n";
        printf("\n%s", cpHeader);
        printf("\nThe Grammar\n");
        printf("%s\n", cpGrammar);
        printf("The Input String\n");
        printf("%s\n", cpStr);
        spPhrase = spUtilStrToPhrase(vpMem, cpStr);

        // default from beginning of string
        printf("\nTest default mode at the beginning of the string.\n");
        vApgexPattern(vpApgex, cpGrammar, "");
        bTest = bApgexTest(vpApgex, spPhrase);
        printf("test = %s\n", (bTest ? cpYes : cpNo));

        // global for all successes
        printf("\nTest global mode for all successes.\n");
        vApgexPattern(vpApgex, cpGrammar, "g");
        bTest = bApgexTest(vpApgex, spPhrase);
        printf("test = %s\n", (bTest ? cpYes : cpNo));
        while(bTest){
            bTest = bApgexTest(vpApgex, spPhrase);
            printf("test = %s\n", (bTest ? cpYes : cpNo));
        }

        // beginning of string
        printf("\nTest sticky mode at the beginning of the string.\n");
        vApgexPattern(vpApgex, cpGrammar, "y");
        bTest = bApgexTest(vpApgex, spPhrase);
        printf("test = %s\n", (bTest ? cpYes : cpNo));

        // at position of first occurrence
        printf("\nTest sticky mode at the beginning of a pattern.\n");
        vApgexSetLastIndex(vpApgex, 11);
        bTest = bApgexTest(vpApgex, spPhrase);
        printf("test = %s\n", (bTest ? cpYes : cpNo));

        // consecutive phrases
        printf("\nTest sticky mode for consecutive patterns.\n");
        printf("\nFind consecutive phrases.\n");
        vApgexSetLastIndex(vpApgex, 30);
        bTest = bApgexTest(vpApgex, spPhrase);
        printf("test = %s\n", (bTest ? cpYes : cpNo));
        while(bTest){
            bTest = bApgexTest(vpApgex, spPhrase);
            printf("test = %s\n", (bTest ? cpYes : cpNo));
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}

typedef struct{
    apg_phrase sNamePhrase;
    void* vpVecStack;
} ast_translate;
static aint uiHtml(ast_data* spData){
    ast_translate* spTrans = (ast_translate*)spData->vpUserData;
    void* vpTest;

    // only print from the root node
    if(spData->uiState == ID_AST_PRE){
        vpTest = vpVecFirst(spTrans->vpVecStack);
        if(!vpTest){
            printf("HTML translating...\n");
        }
    }else if(spData->uiState == ID_AST_POST){
        vpTest = vpVecFirst(spTrans->vpVecStack);
        if(!vpTest){
            printf("\n");
        }
    }
    return ID_AST_OK;
}
static aint uiOpen(ast_data* spData){
    if(spData->uiState == ID_AST_POST){
        ast_translate* spTrans = (ast_translate*)spData->vpUserData;

        // push the name on a stack so that the closing tag will match the tree depth of the opening tag
        vpVecPush(spTrans->vpVecStack, &spTrans->sNamePhrase);
        printf("<");
        aint ui = 0;
        for(; ui < spTrans->sNamePhrase.uiLength; ui++){
            char cChar = (char)spTrans->sNamePhrase.acpPhrase[ui];
            if(cChar >= 65 && cChar <= 90){
                cChar += 32;
            }
            printf("%c", cChar);
        }
        printf(">");
    }
    return ID_AST_OK;
}
static aint uiClose(ast_data* spData){
    if(spData->uiState == ID_AST_POST){
        ast_translate* spTrans = (ast_translate*)spData->vpUserData;

        // get the phrase for this tree depth saved by the open tag
        apg_phrase* spPhrase = (apg_phrase*)vpVecPop(spTrans->vpVecStack);
        printf("</");
        aint ui = 0;
        for(; ui < spPhrase->uiLength; ui++){
            char cChar = (char)spPhrase->acpPhrase[ui];
            if(cChar >= 65 && cChar <= 90){
                cChar += 32;
            }
            printf("%c", cChar);
        }
        printf(">");
    }
    return ID_AST_OK;
}
static aint uiName(ast_data* spData){
    if(spData->uiState == ID_AST_POST){
        // capture the name phrase
        ast_translate* spTrans = (ast_translate*)spData->vpUserData;
        spTrans->sNamePhrase.acpPhrase = spData->acpString + spData->uiPhraseOffset;
        spTrans->sNamePhrase.uiLength = spData->uiPhraseLength;
    }
    return ID_AST_OK;
}
static int iAst() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpAst = NULL;
    static void* vpParser = NULL;
    static void* vpApgex = NULL;
    char* cpGrammar =
            "html     = (open html close) / (open close)\n"
            "open     = %d60 name %d62\n"
            "close    = %d60.47 name %d62\n"
            "name     = alpha *alphanum\n"
            "alpha    = %d97-122 / %d65-90\n"
            "alphanum = alpha / %d48-57\n";
    char* cpStr = "<h1><P></Q></H2>";
    apg_phrase* spPhrase;
    apgex_result sResult;
    ast_translate sTranslate;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates the use of the AST for a complex translation of the matched phrase.\n"
                "The pattern matches HTML-like tags. The translation will normalize the tag names.\n"
                "Convert to lower case and match closing names to opening names.\n"
                "Incidentally, this also illustrates matching of recursive patterns.\n";
        printf("\n%s", cpHeader);
        printf("\nThe Grammar\n");
        printf("%s\n", cpGrammar);
        printf("The Input String\n");
        printf("%s\n", cpStr);
        spPhrase = spUtilStrToPhrase(vpMem, cpStr);

        // default from beginning of string
        printf("\nTranslate the matched phrase\n");
        vApgexPattern(vpApgex, cpGrammar, "");
        sResult = sApgexExec(vpApgex, spPhrase);
        if(!sResult.spResult){
            XTHROW(&e, "pattern match failed");
        }

        // get the AST and translate
        vpParser = vpApgexGetParser(vpApgex);
        vpAst = vpApgexGetAst(vpApgex);
        void* vpVecStack = vpVecCtor(vpMem, sizeof(apg_phrase), 512);
        vAstSetRuleCallback(vpAst, uiParserRuleLookup(vpParser, "html"), uiHtml);
        vAstSetRuleCallback(vpAst, uiParserRuleLookup(vpParser, "open"), uiOpen);
        vAstSetRuleCallback(vpAst, uiParserRuleLookup(vpParser, "close"), uiClose);
        vAstSetRuleCallback(vpAst, uiParserRuleLookup(vpParser, "name"), uiName);
        sTranslate.vpVecStack = vpVecStack;
        vAstTranslate(vpAst, &sTranslate);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}

static apg_phrase sMyReplacement(apgex_result* spResult, apgex_properties* spProperties, void* vpUser){
    apg_phrase* spReturn = NULL;
    char* cpYes = "yes";
    char* cpNo = "no";
    char caBuf[1024];
    size_t uiBufSize = 1024;
    int n = 0;
    n += snprintf(&caBuf[n], (uiBufSize - n), "\nReplaced matched phrase with custom stuff. ");
    n += snprintf(&caBuf[n], (uiBufSize - n), "\nI have access to the results (node hits = %"PRIuMAX") and ",
            (luint)spResult->uiNodeHits);
    n += snprintf(&caBuf[n], (uiBufSize - n), "the properties (default mode = %s.)\n", (spProperties->bDefaultMode ? cpYes : cpNo));
    spReturn = spUtilStrToPhrase(vpUser, caBuf);
    return *spReturn;
}
static int iReplace() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApgex = NULL;
    char* cpGrammar =
            "word = abc / xyz\n"
            "abc = \"abc\"\n"
            "xyz = \"xyz\"\n";
    char* cpStr = "-abc-xyz-";
    char* cpSimple = "555";
    char* cpEscape = "$$";
    char* cpLeft = "($`)";
    char* cpRight = "($')";
    char* cpSelf = "($&)";
    char* cpRulea = "($<abc>)";
    char* cpRulex = "($<xyz>)";
    apg_phrase* spPhrase;
    apg_phrase* spReplacement;
    apg_phrase sReplaced;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates the replacement of matched phrases with simple phrases and compound phrases.\n"
                "Simple replacement simply replaces the matched phrase with a specified phrase.\n"
                "Compound replacement uses various parts of the result for the replacement phrase.\n";
        printf("\n%s", cpHeader);
        printf("\nThe Grammar\n");
        printf("%s\n", cpGrammar);
        printf("The Input String\n");
        printf("%s\n", cpStr);
        spPhrase = spUtilStrToPhrase(vpMem, cpStr);

        // simple replacement
        printf("\nSimple replacement in default mode\n");
        vApgexPattern(vpApgex, cpGrammar, "");
        spReplacement = spUtilStrToPhrase(vpMem, cpSimple);
        sReplaced = sApgexReplace(vpApgex, spPhrase, spReplacement);
        if(!sReplaced.acpPhrase){
            XTHROW(&e, "replacement failed");
        }
        printf("Replace matched phrases with %s\n", cpSimple);
        printf("The matched phrases with replacements:\n%s\n", cpUtilPhraseToStr(vpMem, &sReplaced));

        printf("\nSimple replacement in global mode\n");
        vApgexPattern(vpApgex, cpGrammar, "g");
        spReplacement = spUtilStrToPhrase(vpMem, cpSimple);
        sReplaced = sApgexReplace(vpApgex, spPhrase, spReplacement);
        if(!sReplaced.acpPhrase){
            XTHROW(&e, "replacement failed");
        }
        printf("Replace matched phrases with %s\n", cpSimple);
        printf("The matched phrases with replacements:\n%s\n", cpUtilPhraseToStr(vpMem, &sReplaced));

        // compound replacements
        vApgexPattern(vpApgex, cpGrammar, "");
        vApgexEnableRules(vpApgex, "--all", APG_TRUE);
        spReplacement = spUtilStrToPhrase(vpMem, cpEscape);
        sReplaced = sApgexReplace(vpApgex, spPhrase, spReplacement);
        if(!sReplaced.acpPhrase){
            XTHROW(&e, "replacement failed");
        }
        printf("Replace matched phrases with %s - escape character\n", cpEscape);
        printf("The matched phrases with replacements:\n%s\n", cpUtilPhraseToStr(vpMem, &sReplaced));

        spReplacement = spUtilStrToPhrase(vpMem, cpLeft);
        sReplaced = sApgexReplace(vpApgex, spPhrase, spReplacement);
        if(!sReplaced.acpPhrase){
            XTHROW(&e, "replacement failed");
        }
        printf("Replace matched phrases with %s - left context\n", cpLeft);
        printf("The matched phrases with replacements:\n%s\n", cpUtilPhraseToStr(vpMem, &sReplaced));

        spReplacement = spUtilStrToPhrase(vpMem, cpSelf);
        sReplaced = sApgexReplace(vpApgex, spPhrase, spReplacement);
        if(!sReplaced.acpPhrase){
            XTHROW(&e, "replacement failed");
        }
        printf("Replace matched phrases with %s - self, the matched phrase\n", cpSelf);
        printf("The matched phrases with replacements:\n%s\n", cpUtilPhraseToStr(vpMem, &sReplaced));

        spReplacement = spUtilStrToPhrase(vpMem, cpRight);
        sReplaced = sApgexReplace(vpApgex, spPhrase, spReplacement);
        if(!sReplaced.acpPhrase){
            XTHROW(&e, "replacement failed");
        }
        printf("Replace matched phrases with %s - the right context\n", cpRight);
        printf("The matched phrases with replacements:\n%s\n", cpUtilPhraseToStr(vpMem, &sReplaced));

        spReplacement = spUtilStrToPhrase(vpMem, cpRulea);
        sReplaced = sApgexReplace(vpApgex, spPhrase, spReplacement);
        if(!sReplaced.acpPhrase){
            XTHROW(&e, "replacement failed");
        }
        printf("Replace matched phrases with %s - the rule 'abc'\n", cpRulea);
        printf("The matched phrases with replacements:\n%s\n", cpUtilPhraseToStr(vpMem, &sReplaced));

        spReplacement = spUtilStrToPhrase(vpMem, cpRulex);
        sReplaced = sApgexReplace(vpApgex, spPhrase, spReplacement);
        if(!sReplaced.acpPhrase){
            XTHROW(&e, "replacement failed");
        }
        printf("Replace matched phrases with %s - the rule 'xyz'\n", cpRulex);
        printf("The matched phrases with replacements:\n%s\n", cpUtilPhraseToStr(vpMem, &sReplaced));

        sReplaced = sApgexReplaceFunc(vpApgex, spPhrase, sMyReplacement, vpMem);
        if(!sReplaced.acpPhrase){
            XTHROW(&e, "replacement failed");
        }
        printf("Replace matched phrases with custom replacement function.\n");
        printf("The matched phrases with replacements:\n%s\n", cpUtilPhraseToStr(vpMem, &sReplaced));

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}

static int iSplit() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApgex = NULL;
    char* cpPatternSep = "sep = *%d32 \";\" *%d32\n";
    char* cpPatternEmpty = "sep = \"\"\n";
    char* cpPatternLetters = "letters = 1*%d97-122\n";
    char* cpStr;
    char caStrBuf[1024];
    char* cpSub;
    apg_phrase* spPhrase;
    apg_phrase* spArray;
    aint ui, uiCount;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates the using matched phrases as separators to split a phrase into an array of sub-phrases.\n";
        printf("\n%s", cpHeader);

        // split at separator
        printf("\nThe Separator Pattern\n");
        printf("%s\n", cpPatternSep);
        printf("The Input Phrase\n");
        cpStr = "one   ;   two;three";
        printf("'%s'\n", cpStr);
        spPhrase = spUtilStrToPhrase(vpMem, cpStr);
        vApgexPattern(vpApgex, cpPatternSep, "");
        spArray = spApgexSplit(vpApgex, spPhrase, 0, &uiCount);
        if(!uiCount){
            XTHROW(&e, "split failed");
        }
        printf("\nArray of sub-phrases - split at separators\n");
        for(ui = 0; ui < uiCount; ui++){
            cpSub = cpPhraseToStr(&spArray[ui], caStrBuf);
            printf("index: %d: phrase: %s\n", (int)ui, cpSub);

        }

        printf("\nThe Separator Pattern\n");
        printf("%s\n", cpPatternSep);
        printf("The Input Phrase - separator is entire string, array is empty\n");
        cpStr = "   ;  ";
        printf("'%s'\n", cpStr);
        spPhrase = spUtilStrToPhrase(vpMem, cpStr);
        vApgexPattern(vpApgex, cpPatternSep, "");
        spArray = spApgexSplit(vpApgex, spPhrase, 0, &uiCount);
        if(uiCount){
            XTHROW(&e, "split failed");
        }
        printf("\nArray of sub-phrases - separator is entire string, array is empty\n");
        printf("none\n");

        printf("\nThe Separator Pattern\n");
        printf("%s\n", cpPatternSep);
        printf("The Input Phrase\n");
        cpStr = "word";
        printf("'%s'\n", cpStr);
        spPhrase = spUtilStrToPhrase(vpMem, cpStr);
        vApgexPattern(vpApgex, cpPatternSep, "");
        spArray = spApgexSplit(vpApgex, spPhrase, 0, &uiCount);
        if(!uiCount){
            XTHROW(&e, "split failed");
        }
        printf("\nArray of sub-phrases - no separators, array is the whole string\n");
        for(ui = 0; ui < uiCount; ui++){
            cpSub = cpPhraseToStr(&spArray[ui], caStrBuf);
            printf("index: %d: phrase: %s\n", (int)ui, cpSub);

        }

        printf("\nThe Separator Pattern\n");
        printf("%s\n", cpPatternEmpty);
        printf("The Input Phrase\n");
        cpStr = "word";
        printf("'%s'\n", cpStr);
        spPhrase = spUtilStrToPhrase(vpMem, cpStr);
        vApgexPattern(vpApgex, cpPatternEmpty, "");
        spArray = spApgexSplit(vpApgex, spPhrase, 0, &uiCount);
        if(!uiCount){
            XTHROW(&e, "split failed");
        }
        printf("\nArray of sub-phrases - separate into individual characters\n");
        for(ui = 0; ui < uiCount; ui++){
            cpSub = cpPhraseToStr(&spArray[ui], caStrBuf);
            printf("index: %d: phrase: %s\n", (int)ui, cpSub);

        }

        printf("\nThe Separator Pattern\n");
        printf("%s\n", cpPatternLetters);
        printf("The Input Phrase\n");
        cpStr = "123abc4d56e";
        printf("'%s'\n", cpStr);
        spPhrase = spUtilStrToPhrase(vpMem, cpStr);
        vApgexPattern(vpApgex, cpPatternLetters, "");
        spArray = spApgexSplit(vpApgex, spPhrase, 0, &uiCount);
        if(!uiCount){
            XTHROW(&e, "split failed");
        }
        printf("\nArray of sub-phrases - letters as separators\n");
        for(ui = 0; ui < uiCount; ui++){
            cpSub = cpPhraseToStr(&spArray[ui], caStrBuf);
            printf("index: %d: phrase: %s\n", (int)ui, cpSub);

        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}

static int iBoundaries() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApgex = NULL;
    char* cpWordPattern =
            "word-to-find = abw \"cat\" aew\n"
            "word-char    = %d65-90/%d97-122\n"
            "abw          = (!!word-char / %^) ; define word beginning\n"
            "aew          = (!word-char / %$)  ; define word end\n";
    char* cpLinePattern =
            "phrase-to-find = abl \"The \" animal \" in the hat.\" ael\n"
            "animal         = \"cat\" / \"dog\" / \"bird\" / \"mouse\"\n"
            "line-end       = %d13.10 / %d10 / %d13\n"
            "abl            = (&&line-end / %^) ; define line beginning\n"
            "ael            = (&line-end / %$)  ; define line end\n";
    char* cpWordStr = "Cat - a Bobcat is a cat but a caterpillar is not a cat.";
    char* cpLineStr =
            "The cat in the hat.\n"
            "The dog in the hat.\r\n"
            "The bird in the hat.\r"
            "The dog is not in the hat.\n"
            "The cat in the hat is black.\n"
            "The mouse in the hat.";
    apg_phrase* spPhrase;
    apgex_result sResult;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates the definition and use of word and line boundaries.\n"
                "Unlike most \"regex\" engines, apgex makes no assumptions about what constitutes\n"
                "a word or line boundary. Nonetheless it is very easy, using look around and anchors,\n"
                "to define word and line boundaries according to the needs of the problem at hand.\n";
        printf("\n%s", cpHeader);

        // word boundaries
        printf("\nFind Words\n");
        printf("%s\n", cpWordPattern);
        printf("The Input Phrase\n");
        printf("'%s'\n", cpWordStr);
        spPhrase = spUtilStrToPhrase(vpMem, cpWordStr);
        vApgexPattern(vpApgex, cpWordPattern, "g");
        sResult = sApgexExec(vpApgex, spPhrase);
        while(sResult.spResult){
            vApgexDisplayResult(vpApgex, &sResult, NULL);
            sResult = sApgexExec(vpApgex, spPhrase);
        }

        // line boundaries
        printf("\nFind Lines\n");
        printf("%s\n", cpLinePattern);
        printf("The Input Phrase\n");
        printf("'%s'\n", cpLineStr);
        spPhrase = spUtilStrToPhrase(vpMem, cpLineStr);
        vApgexPattern(vpApgex, cpLinePattern, "g");
        sResult = sApgexExec(vpApgex, spPhrase);
        while(sResult.spResult){
            vApgexDisplayResult(vpApgex, &sResult, NULL);
            sResult = sApgexExec(vpApgex, spPhrase);
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}

#define DQUOTE 34
static char s_cZero = 0;
static aint uiRecord(ast_data* spData){
    if(spData->uiState == ID_AST_POST){
        printf("\n");
    }
    return ID_AST_OK;
}

static aint uiNonEscaped(ast_data* spData){
    if(spData->uiState == ID_AST_POST){
        void* vpVec = spData->vpUserData;
        const achar* acpChar = spData->acpString + spData->uiPhraseOffset;
        const achar* acpEnd = acpChar + spData->uiPhraseLength;
        char cChar;
        vVecClear(vpVec);
        while(acpChar < acpEnd){
            cChar = (char)*acpChar++;
            vpVecPush(vpVec, &cChar);
        }
        vpVecPush(vpVec, &s_cZero);
        char* cpStr = (char*)vpVecFirst(vpVec);
        printf("[%-15s]", cpStr);
    }
    return ID_AST_OK;
}

static aint uiEscaped(ast_data* spData){
    void* vpVec = spData->vpUserData;
    if(spData->uiState == ID_AST_PRE){
        vVecClear(vpVec);
    }else if(spData->uiState == ID_AST_POST){
        vpVecPush(vpVec, &s_cZero);
        char* cpStr = (char*)vpVecFirst(vpVec);
        printf("[%-15s]", cpStr);
    }
    return ID_AST_OK;
}

static aint uiText(ast_data* spData){
    if(spData->uiState == ID_AST_POST){
        void* vpVec = spData->vpUserData;
        const achar* acpChar = spData->acpString + spData->uiPhraseOffset;
        char cChar = (char)*acpChar;
        vpVecPush(vpVec, &cChar);
    }
    return ID_AST_OK;
}

static aint uiDdquote(ast_data* spData){
    if(spData->uiState == ID_AST_POST){
        void* vpVec = spData->vpUserData;
        char cChar = DQUOTE;
        vpVecPush(vpVec, &cChar);
    }
    return ID_AST_OK;
}

static int iCSV() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpVec = NULL;
    static void* vpAst = NULL;
    static void* vpParser = NULL;
    static void* vpApgex = NULL;
    char* cpCSVPattern =
            "; the record and field formats from RFC4180\n"
            "; slightly modified for easier phrase capture and replacement\n"
            "record      = field *(COMMA field) [CRLF]\n"
            "field       = (escaped / non-escaped)\n"
            "escaped     = LQUOTE *(text / DDQUOTE) RQUOTE\n"
            "text        = TEXTDATA / COMMA / CR / LF\n"
            "DDQUOTE     = 2%x22\n"
            "non-escaped = *TEXTDATA\n"
            "COMMA       = %x2C\n"
            "CR          = %x0D\n"
            "LQUOTE      = %x22\n"
            "RQUOTE      = %x22\n"
            "LF          = %x0A\n"
            "CRLF        = CR LF / LF / CR ; modified from RFC4180 to include all forms of line ends\n"
            "TEXTDATA    = %x20-21 / %x23-2B / %x2D-7E\n";
    char* cpFileStr =
            "ITEM,DESCRIPTION,VALUE\n"
            "Cup,\"coffee,tea,etc\",$10.00\n"
            "Camero,Sedan,\"$25,000\"\n"
            "Empty Desc.,,\"$0,000\"\n"
            "Junker,empty price,\n"
            "Aston Martin,\"$316,300\",\"He said, \"\"That's way too much moola, man.\"\"\"\n";
    apg_phrase* spPhrase;
    char caBuf[1024];
    aint uiBufSize = 1024;
    aint ui, uj;
    const char* cpaStrings[10];
    const char* cpStr;
    apgex_result sResult;
    apgex_rule* spRule;
    apg_phrase* spRulePhrase;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates the use of apgex for extracting the values from\n"
                "Comma Separated Value (CSV) formatted data. There seems to be no standard format\n"
                "but the field format used here is from RFC 4180. For comparison to \"regex\"\n"
                "see the solution for the similar Microsoft format described in Jeffrey Friedl's\n"
                "book \"Mastering Regular Expressions\", O'Reilly, 2006, pg. 213.\n";
        printf("\n%s", cpHeader);

        printf("\nThe Pattern\n");
        printf("%s\n", cpCSVPattern);
        printf("\nThe CSV File\n");
        printf("%s\n", cpFileStr);

        // raw fields
        printf("\nDisplay the raw fields in each record.\n");
        spPhrase = spUtilStrToPhrase(vpMem, cpFileStr);
        vApgexPattern(vpApgex, cpCSVPattern, "g");
        vApgexEnableRules(vpApgex, "field", APG_TRUE);
        sResult = sApgexExec(vpApgex, spPhrase);
        while(sResult.spResult){
            spRule = sResult.spRules;
            for(ui = 0; ui < spRule->uiPhraseCount; ui++){
                spRulePhrase = &spRule->spPhrases[ui].sPhrase;
                cpaStrings[ui] = cpUtilPhraseToStr(vpMem, spRulePhrase);
                printf("[%-15s]", cpaStrings[ui]);
            }
            printf("\n");
            sResult = sApgexExec(vpApgex, spPhrase);
        }

        // useful fields
        printf("\nExtract unquoted fields.\n");
        printf("Brute Force\n");
        spPhrase = spUtilStrToPhrase(vpMem, cpFileStr);
        vApgexPattern(vpApgex, cpCSVPattern, "g");
        vApgexEnableRules(vpApgex, "--all", APG_FALSE);
        vApgexEnableRules(vpApgex, "field", APG_TRUE);
        sResult = sApgexExec(vpApgex, spPhrase);
        while(sResult.spResult){
            spRule = sResult.spRules;
            for(ui = 0; ui < spRule->uiPhraseCount; ui++){
                spRulePhrase = &spRule->spPhrases[ui].sPhrase;
                cpStr = cpUtilPhraseToStr(vpMem, spRulePhrase);
                if(cpStr[0] == DQUOTE){
                    aint uiLen = (aint)strlen(cpStr);
                    if(uiLen >= uiBufSize){
                        XTHROW(&e, "buffer size too small for field conversion");
                    }
                    aint uiNewLen = 0;
                    caBuf[uiNewLen] = 0;
                    if(uiLen > 2){
                        for(uj = 1; uj < (uiLen - 1); uj++){
                            if(cpStr[uj] == DQUOTE){
                                caBuf[uiNewLen++] = DQUOTE;
                                uj++;
                            }else{
                                caBuf[uiNewLen++] = cpStr[uj];
                            }
                        }
                        caBuf[uiNewLen++] = 0;
                    }
                    printf("[%-15s]", caBuf);
                }else{
                    printf("[%-15s]", cpStr);
                }
                vMemFree(vpMem, cpStr);
            }
            printf("\n");
            sResult = sApgexExec(vpApgex, spPhrase);
        }

        // AST translation fields
        printf("\nExtract unquoted fields.\n");
        printf("AST Translation\n");
        vpVec = vpVecCtor(vpMem, sizeof(char), 1024);
        spPhrase = spUtilStrToPhrase(vpMem, cpFileStr);
        vApgexPattern(vpApgex, cpCSVPattern, "g");
        vpAst = vpApgexGetAst(vpApgex);
        vpParser = vpApgexGetParser(vpApgex);
        vApgexEnableRules(vpApgex, "--all", APG_FALSE);
        vApgexEnableRules(vpApgex, "record, ddquote, text, non-escaped, escaped", APG_TRUE);
        sResult = sApgexExec(vpApgex, spPhrase);
        while(sResult.spResult){
            vAstSetRuleCallback(vpAst, uiParserRuleLookup(vpParser, "record"), uiRecord);
            vAstSetRuleCallback(vpAst, uiParserRuleLookup(vpParser, "escaped"), uiEscaped);
            vAstSetRuleCallback(vpAst, uiParserRuleLookup(vpParser, "non-escaped"), uiNonEscaped);
            vAstSetRuleCallback(vpAst, uiParserRuleLookup(vpParser, "text"), uiText);
            vAstSetRuleCallback(vpAst, uiParserRuleLookup(vpParser, "ddquote"), uiDdquote);
            vAstTranslate(vpAst, vpVec);
            sResult = sApgexExec(vpApgex, spPhrase);
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApgexDtor(vpApgex);
    vMemDtor(vpMem);
    return iReturn;
}

static int iWide() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpFmt = NULL;
    static void* vpApgex = NULL;
    char* cpCherokee =
            "word = 1*%x13A0-13F4\n";
    apg_phrase sPhrase;
    apg_phrase* spWord;
    char* cpInput;
    const char* cpLine;
    uint8_t ucaBuf[1024];
    aint uiBufSize = 1024;
    aint uiSize;
    apgex_result sResult;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpFmt = vpFmtCtor(&e);
        vpApgex = vpApgexCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates patterns with Unicode UTF-32 characters.\n"
                "The pattern will match Cherokee words in 32-bit UTF-32 format.\n";
        printf("\n%s", cpHeader);

        // validate the alphabet character width
        if(sizeof(achar) != 4){
            XTHROW(&e, "recompile with APG_ACHAR=32, sizeof(achar) must = 4");
        }

        if(bIsBigEndian()){
            cpInput = "../input/cherokee.utf32be";
        }else{
            cpInput = "../input/cherokee.utf32le";
        }
        uiSize = uiBufSize;
        vUtilFileRead(vpMem, cpInput, ucaBuf, &uiSize);
        if(uiSize > uiBufSize){
            XTHROW(&e, "buffer size too small for input file");
        }

        // word boundaries
        printf("\nThe Cherokee Word Pattern\n");
        printf("%s\n", cpCherokee);

        printf("The Input Phrase\n");
        cpLine = cpFmtFirstUnicode(vpFmt, (uint32_t*)ucaBuf, (uint64_t)(uiSize / 4), 0, 0);
        while(cpLine){
            printf("%s", cpLine);
            cpLine = cpFmtNext(vpFmt);
        }

        printf("\nThe Cherokee Words\n");
        vApgexPattern(vpApgex, cpCherokee, "g");
        sPhrase.acpPhrase = (achar*)ucaBuf;
        sPhrase.uiLength = uiSize / 4;
        sResult = sApgexExec(vpApgex, &sPhrase);
        while(sResult.spResult){
            spWord = &sResult.spResult->sPhrase;
            cpLine = cpFmtFirstUnicode(vpFmt, (uint32_t*)spWord->acpPhrase, spWord->uiLength, 0, 0);
            while(cpLine){
                printf("%s", cpLine);
                cpLine = cpFmtNext(vpFmt);
            }
            sResult = sApgexExec(vpApgex, &sPhrase);
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApgexDtor(vpApgex);
    vFmtDtor(vpFmt);
    vMemDtor(vpMem);
    return iReturn;
}

static int iBackReference() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpApgex = NULL;
    char* cpPatternUI =
            "pattern  = %^ tag %$\n"
            "tag      = (open tag close) / (open close)\n"
            "name     = 1*alpha\n"
            "alpha    = %d97-122 / %d65-90\n"
            "open     = %d60 name %d62\n"
            "close    = %d60.47 \\name %d62\n";
    char* cpPatternUS =
            "pattern  = %^ tag %$\n"
            "tag      = (open tag close) / (open close)\n"
            "name     = 1*alpha\n"
            "alpha    = %d97-122 / %d65-90\n"
            "open     = %d60 name %d62\n"
            "close    = %d60.47 \\%s%uname %d62\n";
    char* cpPatternPI =
            "pattern  = %^ tag %$\n"
            "tag      = (open tag close) / (open close)\n"
            "name     = 1*alpha\n"
            "alpha    = %d97-122 / %d65-90\n"
            "open     = %d60 name %d62\n"
            "close    = %d60.47 \\%pname %d62\n";
    char* cpPatternPS =
            "pattern  = %^ tag %$\n"
            "tag      = (open tag close) / (open close)\n"
            "name     = 1*alpha\n"
            "alpha    = %d97-122 / %d65-90\n"
            "open     = %d60 name %d62\n"
            "close    = %d60.47 \\%p%sname %d62\n";
    char* cpTagsUI = "<div><span></SPAN></SPAN>";
    char* cpTagsUS = "<div><span></span></span>";
    char* cpTagsPI = "<div><span></SPAN></DIV>";
    char* cpTagsPS = "<div><span></span></div>";
    apg_phrase* spPhraseUI, *spPhraseUS, *spPhrasePI, *spPhrasePS;
    abool bResult;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpApgex = vpApgexCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case illustrates back references in both \"universal\" and \"parent\" modes.\n"
                "The patterns match XML-like tags. The strings have both matching and non-matching node names.\n"
                "The opening and closing tags have both case-sensitive and case-insensitive corresponding names.\n"
                "Phrase-matching results are shown for all possible combinations, illustrating the differences\n"
                "between the different modes and case sensitivities.\n"
                "Note that, due to the begin-or-string and end-of-string anchors,\n"
                "the patterns require that the entire source phrase must be matched.\n";
        printf("\n%s", cpHeader);
        spPhraseUI = spUtilStrToPhrase(vpMem, cpTagsUI);
        spPhraseUS = spUtilStrToPhrase(vpMem, cpTagsUS);
        spPhrasePI = spUtilStrToPhrase(vpMem, cpTagsPI);
        spPhrasePS = spUtilStrToPhrase(vpMem, cpTagsPS);

        printf("\nUniversal I: universal mode, case insenstive pattern\n");
        printf("%s", cpPatternUI);
        printf("\nUniversal S: universal mode, case senstive pattern\n");
        printf("%s", cpPatternUS);
        printf("\nParent I: parent mode, case insenstive pattern\n");
        printf("%s", cpPatternPI);
        printf("\nParent S: parent mode, case senstive pattern\n");
        printf("%s", cpPatternPS);

        // universal case insensitive
        printf("\n%-12s %-26s %-10s\n", "grammar", "source", "result");
        vApgexPattern(vpApgex, cpPatternUI, "");
        bResult = bApgexTest(vpApgex, spPhraseUI);
        printf("%-12s %-26s %-10s\n", "Universal I", cpTagsUI, cpUtilTrueFalse(bResult));
        bResult = bApgexTest(vpApgex, spPhraseUS);
        printf("%-12s %-26s %-10s\n", "Universal I", cpTagsUS, cpUtilTrueFalse(bResult));
        bResult = bApgexTest(vpApgex, spPhrasePI);
        printf("%-12s %-26s %-10s\n", "Universal I", cpTagsPI, cpUtilTrueFalse(bResult));
        bResult = bApgexTest(vpApgex, spPhrasePS);
        printf("%-12s %-26s %-10s\n", "Universal I", cpTagsPS, cpUtilTrueFalse(bResult));
        printf("\n");

        // universal case sensitive
        vApgexPattern(vpApgex, cpPatternUS, "");
        bResult = bApgexTest(vpApgex, spPhraseUI);
        printf("%-12s %-26s %-10s\n", "Universal S", cpTagsUI, cpUtilTrueFalse(bResult));
        bResult = bApgexTest(vpApgex, spPhraseUS);
        printf("%-12s %-26s %-10s\n", "Universal S", cpTagsUS, cpUtilTrueFalse(bResult));
        bResult = bApgexTest(vpApgex, spPhrasePI);
        printf("%-12s %-26s %-10s\n", "Universal S", cpTagsPI, cpUtilTrueFalse(bResult));
        bResult = bApgexTest(vpApgex, spPhrasePS);
        printf("%-12s %-26s %-10s\n", "Universal S", cpTagsPS, cpUtilTrueFalse(bResult));
        printf("\n");

        // parent case insensitive
        vApgexPattern(vpApgex, cpPatternPI, "");
        bResult = bApgexTest(vpApgex, spPhraseUI);
        printf("%-12s %-26s %-10s\n", "Parent I", cpTagsUI, cpUtilTrueFalse(bResult));
        bResult = bApgexTest(vpApgex, spPhraseUS);
        printf("%-12s %-26s %-10s\n", "Parent I", cpTagsUS, cpUtilTrueFalse(bResult));
        bResult = bApgexTest(vpApgex, spPhrasePI);
        printf("%-12s %-26s %-10s\n", "Parent I", cpTagsPI, cpUtilTrueFalse(bResult));
        bResult = bApgexTest(vpApgex, spPhrasePS);
        printf("%-12s %-26s %-10s\n", "Parent I", cpTagsPS, cpUtilTrueFalse(bResult));
        printf("\n");

        // parent case sensitive
        vApgexPattern(vpApgex, cpPatternPS, "");
        bResult = bApgexTest(vpApgex, spPhraseUS);
        printf("%-12s %-26s %-10s\n", "Parent S", cpTagsUI, cpUtilTrueFalse(bResult));
        bResult = bApgexTest(vpApgex, spPhraseUS);
        printf("%-12s %-26s %-10s\n", "Parent S", cpTagsUS, cpUtilTrueFalse(bResult));
        bResult = bApgexTest(vpApgex, spPhrasePI);
        printf("%-12s %-26s %-10s\n", "Parent S", cpTagsPI, cpUtilTrueFalse(bResult));
        bResult = bApgexTest(vpApgex, spPhrasePS);
        printf("%-12s %-26s %-10s\n", "Parent S", cpTagsPS, cpUtilTrueFalse(bResult));
        printf("\n");

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        vApgexDisplayPatternErrors(vpApgex, NULL);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vApgexDtor(vpApgex);
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
        return iPatterns();
    case 3:
        return iResults();
    case 4:
        return iProperties();
    case 5:
        return iGlobalMode();
    case 6:
        return iStickyMode();
    case 7:
        return iTraceMode();
    case 8:
        return iUdt();
    case 9:
        return iTest();
    case 10:
        return iAst();
    case 11:
        return iReplace();
    case 12:
        return iSplit();
    case 13:
        return iBoundaries();
    case 14:
        return iCSV();
    case 15:
        return iWide();
    case 16:
        return iBackReference();
    default:
        return iHelp();
    }
}

