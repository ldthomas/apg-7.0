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
/** \dir ./examples/ex-json
 * \brief Examples of using the JSON parser and builder..
 *
 */

/** \file examples/ex-json/main.c
 * \brief Driver for the JSON parser and builder examples.
 *
This example explores the use of JSON object for parsing and building JSON files.
See the [JSON application discussion](\ref json) for more information.

  - application code must include header files:
       - ../../json/json.h
  - application compilation must include source code from the directories:
       - ../../library
       - ../../utilities
       - ../../json
  - application compilation must define macros:
      - none

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Illustrate a simple case of reading and parsing a JSON file.
 - case 3: Illustrate finding keys in the tree of JSON values.
 - case 4: Illustrate walking a sub-tree and the siblings of a sub-root explicitly with the iterator.
 - case 5: Illustrate writing a JSON file from a value tree of parsed JSON values.
 - case 6: Illustrate building a JSON file.
 */

/**
\page exjson The JSON Parser and Builder

This example explores the use of JSON object for parsing and building JSON files.
See the [JSON application discussion](\ref json) for more information.

  - application code must include header files:
       - ../../json/json.h
  - application compilation must include source code from the directories:
       - ../../library
       - ../../utilities
       - ../../json
  - application compilation must define macros:
      - none

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Illustrate a simple case of reading and parsing a JSON file.
 - case 3: Illustrate finding keys in the tree of JSON values.
 - case 4: Illustrate walking a sub-tree and the siblings of a sub-root explicitly with the iterator.
 - case 5: Illustrate writing a JSON file from a value tree of parsed JSON values.
 - case 6: Illustrate building a JSON file.
*/
#include <limits.h>
#include "../../json/json.h"

#include "source.h"

static const char* cpMakeFileName(char* cpBuffer, const char* cpBase, const char* cpDivider, const char* cpName){
    strcpy(cpBuffer, cpBase);
    strcat(cpBuffer, cpDivider);
    strcat(cpBuffer, cpName);
    return cpBuffer;
}

static char s_caBuf[PATH_MAX];
static char s_caBufOut[5*PATH_MAX];

static char* s_cpDescription =
        "Illustrate the JSON object for parsing and building JSON files.";

static char* s_cppCases[] = {
        "Display application information.",
        "Illustrate a simple case of reading and parsing a JSON file.",
        "Illustrate finding keys in the tree of JSON values.",
        "Illustrate walking a sub-tree and the siblings of a sub-root explicitly with the iterator.",
        "Illustrate writing a JSON file from a value tree of parsed JSON values.",
        "Illustrate building a JSON file.",
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

static int iSimpleParse() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpJson = NULL;
    json_value* spRoot;
    void* vpIn;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpJson = vpJsonCtor(&e);
        const char* cpInput = cpMakeFileName(s_caBuf, SOURCE_DIR, "/../input/", "json-parse.json");

        // display the information header
        char* cpHeader =
                "This example case illustrates a simple parse and display of a JSON file.\n"
                "The file has many non-printing ASCII characters.\n";
        printf("\n%s", cpHeader);

        // read the file
        vpIn = vpJsonReadFile(vpJson, cpInput);

        // display the input file
        printf("\nThe Input File (with line numbers)\n");
        vJsonDisplayInput(vpJson, APG_TRUE);

        // display the JSON values
        printf("\nThe JSON Values\n");
        spRoot = spJsonIteratorFirst(vpIn);
        vJsonDisplayValue(vpJson, spRoot, 0);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vJsonDtor(vpJson);
    vMemDtor(vpMem);
    return iReturn;
}

static int iFindKeys() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpJson = NULL;
    json_value* spRoot, *spValue;
    void* vpIn, *vpIt;
    u32_phrase* spKey32;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpJson = vpJsonCtor(&e);
        const char* cpInput = cpMakeFileName(s_caBuf, SOURCE_DIR, "/../input/", "json-parse.json");

        // display the information header
        char* cpHeader =
                "This example case illustrates finding keys in a parsed JSON file.\n";
        printf("\n%s", cpHeader);

        // read the file
        vpIn = vpJsonReadFile(vpJson, cpInput);
        spRoot = spJsonIteratorFirst(vpIn);

        // display the input file
        printf("\nThe Input File (with line numbers)\n");
        vJsonDisplayInput(vpJson, APG_TRUE);

        // find the numbers key
        printf("\nFind the \"numbers\" Key\n");
        vpIt = vpJsonFindKeyA(vpJson, "numbers", spRoot);
        if(vpIt){
            spValue = spJsonIteratorFirst(vpIt);
            while(spValue){
                vJsonDisplayValue(vpJson, spValue, 0);
                spValue = spJsonIteratorNext(vpIt);
            }
        }else{
            XTHROW(&e, "numbers key not found");
        }

        // find all ctrl keys
        printf("\nFind the \"ctrl\" Keys\n");
        vpIt = vpJsonFindKeyA(vpJson, "ctrl", spRoot);
        if(vpIt){
            spValue = spJsonIteratorFirst(vpIt);
            while(spValue){
                vJsonDisplayValue(vpJson, spValue, 0);
                spValue = spJsonIteratorNext(vpIt);
            }
        }else{
            XTHROW(&e, "ctrl key not found");
        }

        // find the numbers key
        printf("\nFind the Non-ASCII Key\n");
        spKey32 = spUtilStrToPhrase32(vpMem, "odd-\xFF-key");
        vpIt = vpJsonFindKeyU(vpJson, spKey32->uipPhrase, spKey32->uiLength, spRoot);
        if(vpIt){
            spValue = spJsonIteratorFirst(vpIt);
            while(spValue){
                vJsonDisplayValue(vpJson, spValue, 0);
                spValue = spJsonIteratorNext(vpIt);
            }
        }else{
            XTHROW(&e, "non-ascii key not found");
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vJsonDtor(vpJson);
    vMemDtor(vpMem);
    return iReturn;
}

static int iWalker() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpJson = NULL;
    json_value* spRoot, *spValue;
    void* vpIn, *vpIt, *vpIt2;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpJson = vpJsonCtor(&e);
        const char* cpInput = cpMakeFileName(s_caBuf, SOURCE_DIR, "/../input/", "json-parse.json");

        // display the information header
        char* cpHeader =
                "This example case illustrates walking a tree, depth-first from any value as root\n"
                "or horizontally across children of a specific node.\n";
        printf("\n%s", cpHeader);

        // read the file
        vpIn = vpJsonReadFile(vpJson, cpInput);
        spRoot = spJsonIteratorFirst(vpIn);

        // display the input file
        printf("\nThe Input File (with line numbers)\n");
        vJsonDisplayInput(vpJson, APG_TRUE);

        // walk the children of the root node
        printf("\nWalk the Children of the Root Node\n");
        vpIt = vpJsonChildren(vpJson, spRoot);
        if(vpIt){
            spValue = spJsonIteratorFirst(vpIt);
            while(spValue){
                vJsonDisplayValue(vpJson, spValue, 1);
                spValue = spJsonIteratorNext(vpIt);
            }
        }else{
            XTHROW(&e, "no children of the root node found");
        }

        // walk the sub-tree with the "unsigned" node as the root node
        printf("\nWalk the Sub-Tree of the \"unsigned\" Node as the Root Node\n");
        vpIt = vpJsonFindKeyA(vpJson, "unsigned", spRoot);
        if(vpIt){
            vpIt2 = vpJsonTree(vpJson, spJsonIteratorFirst(vpIt));
            spValue = spJsonIteratorFirst(vpIt2);
            while(spValue){
                vJsonDisplayValue(vpJson, spValue, 1);
                spValue = spJsonIteratorNext(vpIt2);
            }
        }else{
            XTHROW(&e, "signed key not found");
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vJsonDtor(vpJson);
    vMemDtor(vpMem);
    return iReturn;
}

static int iWriter() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpJson = NULL;
    json_value* spRoot, *spValue;
    void* vpIn, *vpIt;
    char* cpaKeys[] = {
            "text",
            "unicode",
            "numbers",
            "odd-\xFF-key",
    };
    const char* cpaOutFile[] = {
            cpMakeFileName(&s_caBufOut[0], SOURCE_DIR, "/../output/", "text.json"),
            cpMakeFileName(&s_caBufOut[PATH_MAX], SOURCE_DIR, "/../output/", "unicode.json"),
            cpMakeFileName(&s_caBufOut[2*PATH_MAX], SOURCE_DIR, "/../output/", "numbers.json"),
            cpMakeFileName(&s_caBufOut[3*PATH_MAX], SOURCE_DIR, "/../output/", "odd.json"),
            cpMakeFileName(&s_caBufOut[4*PATH_MAX], SOURCE_DIR, "/../output/", "root.json"),
    };
    uint8_t* ucpOutput;
    aint uiOutputLen;
    aint ui;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpJson = vpJsonCtor(&e);
        const char* cpInput = cpMakeFileName(s_caBuf, SOURCE_DIR, "/../input/", "json-parse.json");

        // display the information header
        char* cpHeader =
                "This example case illustrates writing JSON files from trees of values.\n"
                "JSON files are generated for a series of tree values a root node.\n"
                "The generated files are written in the current working directory.\n";
        printf("\n%s", cpHeader);

        // read the file
        vpIn = vpJsonReadFile(vpJson, cpInput);
        spRoot = spJsonIteratorFirst(vpIn);

        // display the input file
        printf("\nThe Input File (with line numbers)\n");
        vJsonDisplayInput(vpJson, APG_TRUE);

        printf("\nWrite JSON Files\n");
        printf("For the JSON file with the named key as root node view these files.\n");
        for(ui = 0; ui < 4; ui++){
            vpIt = vpJsonFindKeyA(vpJson, cpaKeys[ui], spRoot);
            if(!vpIt){
                XTHROW(&e, "expected to find key");
            }
            spValue = spJsonIteratorFirst(vpIt);
            if(!spValue){
                XTHROW(&e, "expected to get value");
            }
            ucpOutput = ucpJsonWrite(vpJson, spValue, &uiOutputLen);
            vUtilFileWrite(vpMem, cpaOutFile[ui], ucpOutput, uiOutputLen);
            printf("%s\n", cpaOutFile[ui]);
        }
        ucpOutput = ucpJsonWrite(vpJson, spRoot, &uiOutputLen);
        vUtilFileWrite(vpMem, cpaOutFile[4], ucpOutput, uiOutputLen);
        printf("\nThe root node\n%s\n", cpaOutFile[4]);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vJsonDtor(vpJson);
    vMemDtor(vpMem);
    return iReturn;
}

static int iBuilder() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpBld = NULL;
    static void* vpJson = NULL;
    json_value* spValue;
    void* vpIt;
    typedef struct{
        aint uiIndex;
        char* cpKey;
        const char* cpFileName;
    } file_def;
    file_def saFiles[] = {
            {0, "single", cpMakeFileName(&s_caBufOut[0], SOURCE_DIR, "/../output/", "builder-single-value.json")},
            {1, "text", cpMakeFileName(&s_caBufOut[PATH_MAX], SOURCE_DIR, "/../output/", "builder-text.json")},
            {2, "unicode", cpMakeFileName(&s_caBufOut[2*PATH_MAX], SOURCE_DIR, "/../output/", "builder-unicode.json")},
            {3, "numbers", cpMakeFileName(&s_caBufOut[3*PATH_MAX], SOURCE_DIR, "/../output/", "builder-numbers.json")},
            {4, "root", cpMakeFileName(&s_caBufOut[4*PATH_MAX], SOURCE_DIR, "/../output/", "builder-root.json")},
    };
    uint8_t* ucpOutput;
    aint uiOutputLen;
    aint uiSingle, uiText, uiUnicode, uiNumbers, uiRoot, uiTemp;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpJson = vpJsonCtor(&e);
        vpBld = vpJsonBuildCtor(vpJson);

        // display the information header
        char* cpHeader =
                "This example case illustrates building JSON files from scratch.\n"
                "For simple ASCII files, this is most easily done with a text editor.\n"
                "However, when working with Unicode data a more general method is needed.\n"
                "This JSON builder works by creating root nodes (objects or arrays) and adding children to them.\n";
        printf("\n%s", cpHeader);

        printf("\nBuild a single-value JSON file.\n");
        uiSingle = uiJsonBuildMakeStringA(vpBld, "the quick brown fox jumps over the lazy dog");
        vpIt = vpJsonBuild(vpBld, uiSingle);
        spValue = spJsonIteratorFirst(vpIt);
        ucpOutput = ucpJsonWrite(vpJson, spValue, &uiOutputLen);
        vUtilFileWrite(vpMem, saFiles[0].cpFileName, ucpOutput, uiOutputLen);
        printf("%s node written to file %s\n", saFiles[0].cpKey, saFiles[0].cpFileName);
        vJsonDisplayValue(vpJson, spValue, 0);

        printf("\nBuild the text node.\n");
        vJsonBuildClear(vpBld);
        uiText = uiJsonBuildMakeObject(vpBld);
        uiJsonBuildAddToObject(vpBld, uiText,
                uiJsonBuildMakeStringA(vpBld, "simple"),
                uiJsonBuildMakeStringA(vpBld, "the quick brown fox jumps over the lazy dog"));
        uiJsonBuildAddToObject(vpBld, uiText,
                uiJsonBuildMakeStringA(vpBld, "ctrl"),
                uiJsonBuildMakeStringA(vpBld, "text with control characters: \\\\/\\\"\\b\\f\\n\\r\\tabc"));
        vpIt = vpJsonBuild(vpBld, uiText);
        spValue = spJsonIteratorFirst(vpIt);
        ucpOutput = ucpJsonWrite(vpJson, spValue, &uiOutputLen);
        vUtilFileWrite(vpMem, saFiles[1].cpFileName, ucpOutput, uiOutputLen);
        printf("%s node written to file %s\n", saFiles[1].cpKey, saFiles[1].cpFileName);
        vJsonDisplayValue(vpJson, spValue, 0);
        vJsonIteratorDtor(vpIt);

        printf("\nBuild the unicode node.\n");
        uiUnicode = uiJsonBuildMakeObject(vpBld);
        uiJsonBuildAddToObject(vpBld, uiUnicode,
                uiJsonBuildMakeStringA(vpBld, "text"),
                uiJsonBuildMakeStringA(vpBld, "simple"));
        uiJsonBuildAddToObject(vpBld, uiUnicode,
                uiJsonBuildMakeStringA(vpBld, "ctrl"),
                uiJsonBuildMakeStringA(vpBld, "abc\\tdef\\nghi"));
        uiJsonBuildAddToObject(vpBld, uiUnicode,
                uiJsonBuildMakeStringA(vpBld, "escaped"),
                uiJsonBuildMakeStringA(vpBld, "\\u0000\\u00ff\\ud800\\udc00\\udbff\\udfff"));
        uint32_t uiaBuf[] = {255, 939, 10348};
        uiJsonBuildAddToObject(vpBld, uiUnicode,
                uiJsonBuildMakeStringA(vpBld, "ctrl"),
                uiJsonBuildMakeStringU(vpBld, uiaBuf, 3));
        vpIt = vpJsonBuild(vpBld, uiUnicode);
        spValue = spJsonIteratorFirst(vpIt);
        ucpOutput = ucpJsonWrite(vpJson, spValue, &uiOutputLen);
        vUtilFileWrite(vpMem, saFiles[2].cpFileName, ucpOutput, uiOutputLen);
        printf("%s node written to file %s\n", saFiles[2].cpKey, saFiles[2].cpFileName);
        vJsonDisplayValue(vpJson, spValue, 0);
        vJsonIteratorDtor(vpIt);

        printf("\nBuild the numbers node.\n");
        uiNumbers = uiJsonBuildMakeObject(vpBld);
        uiTemp = uiJsonBuildMakeArray(vpBld);
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberS(vpBld, -1));
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberS(vpBld, -2));
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberS(vpBld, -9223372036854775807));
        uiJsonBuildAddToObject(vpBld, uiNumbers, uiJsonBuildMakeStringA(vpBld, "signed"), uiTemp);
        uiTemp = uiJsonBuildMakeArray(vpBld);
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberU(vpBld, 1));
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberU(vpBld, 255));
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberU(vpBld, 65535));
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberU(vpBld, 4294967295));
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberU(vpBld, 18446744073709551615U));
        uiJsonBuildAddToObject(vpBld, uiNumbers, uiJsonBuildMakeStringA(vpBld, "unsigned"), uiTemp);
        uiTemp = uiJsonBuildMakeArray(vpBld);
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberF(vpBld, 2.2250738585072014e-308));
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberF(vpBld, 2.2250738585072014e307));
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberF(vpBld, -1.1));
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberF(vpBld, 2.3));
        uiJsonBuildAddToArray(vpBld, uiTemp, uiJsonBuildMakeNumberF(vpBld, -0.001e-10));
        uiJsonBuildAddToObject(vpBld, uiNumbers, uiJsonBuildMakeStringA(vpBld, "floating point"), uiTemp);
        vpIt = vpJsonBuild(vpBld, uiNumbers);
        spValue = spJsonIteratorFirst(vpIt);
        ucpOutput = ucpJsonWrite(vpJson, spValue, &uiOutputLen);
        vUtilFileWrite(vpMem, saFiles[3].cpFileName, ucpOutput, uiOutputLen);
        printf("%s node written to file %s\n", saFiles[3].cpKey, saFiles[3].cpFileName);
        vJsonDisplayValue(vpJson, spValue, 0);
        vJsonIteratorDtor(vpIt);

        printf("\nAdd all keyed nodes to a single, parent root.\n");
        uiRoot = uiJsonBuildMakeObject(vpBld);
        uiJsonBuildAddToObject(vpBld, uiRoot, uiJsonBuildMakeStringA(vpBld, "text"), uiText);
        uiJsonBuildAddToObject(vpBld, uiRoot, uiJsonBuildMakeStringA(vpBld, "unicode"), uiUnicode);
        uiJsonBuildAddToObject(vpBld, uiRoot, uiJsonBuildMakeStringA(vpBld, "numbers"), uiNumbers);
        uiJsonBuildAddToObject(vpBld, uiRoot, uiJsonBuildMakeStringA(vpBld, "odd-\\u00FF\xc3\xbf-key"), uiJsonBuildMakeStringA(vpBld, "how do you like this key?"));
        vpIt = vpJsonBuild(vpBld, uiRoot);
        spValue = spJsonIteratorFirst(vpIt);
        ucpOutput = ucpJsonWrite(vpJson, spValue, &uiOutputLen);
        vUtilFileWrite(vpMem, saFiles[4].cpFileName, ucpOutput, uiOutputLen);
        printf("%s node written to file %s\n", saFiles[4].cpKey, saFiles[4].cpFileName);
        vJsonDisplayValue(vpJson, spValue, 0);
        vJsonIteratorDtor(vpIt);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vJsonDtor(vpJson); // deletes iterators and builders
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
        return iSimpleParse();
    case 3:
        return iFindKeys();
    case 4:
        return iWalker();
    case 5:
        return iWriter();
    case 6:
        return iBuilder();
    default:
        return iHelp();
    }
}

