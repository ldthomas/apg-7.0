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
/** \dir ./examples/ex-xml
 * \brief Examples of using the XML parser..
 *
 */

/** \file examples/ex-xml/main.c
 * \brief Driver for the XML parser examples..
 *
This example illustrates the use of the XML parser.

  - application code must include header files:
       - ../../xml/xml.h (includes %utilities.h)
  - application compilation must include source code from the directories:
       - ../../xml
       - ../../library
       - ../../utilities
  - application compilation must define macros:
       - none

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Illustrate parsing a simple XML file with no XML declaration or DTD.
 - case 3: Illustrate displaying the Processing Instructions and comments found in an XML document.
 - case 4: Illustrate displaying implicit and explicit XML declaration and parsing UTF-16 encoded files.
 - case 5: Illustrate parsing the DTD and applying entities and default attributes.
 */

/**
\page exxml The XML Parser

This example illustrates the use of the XML parser.

  - application code must include header files:
       - ../../xml/xml.h (includes %utilities.h)
  - application compilation must include source code from the directories:
       - ../../xml
       - ../../library
       - ../../utilities
  - application compilation must define macros:
       - none

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Illustrate parsing a simple XML file with no XML declaration or DTD.
 - case 3: Illustrate displaying the Processing Instructions and comments found in an XML document.
 - case 4: Illustrate displaying implicit and explicit XML declaration and parsing UTF-16 encoded files.
 - case 5: Illustrate parsing the DTD and applying entities and default attributes.
 */

#include "../../xml/xml.h"

static char* s_cpDescription =
        "Illustrate using the XML parser.";

static char* s_cppCases[] = {
        "Display application information.",
        "Illustrate parsing a simple XML file with no XML declaration or DTD.",
        "Illustrate displaying the Processing Instructions and comments found in an XML document.",
        "Illustrate displaying implicit and explicit XML declaration and parsing UTF-16 encoded files.",
        "Illustrate parsing the DTD and applying entities and default attributes.",
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

static int iSimple() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpXml = NULL;
    char* cpInput = "../input/simple.xml";
    uint8_t ucaBuf[1024];
    aint uiBufSize = 1024;
    aint uiSize;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpXml = vpXmlCtor(&e);

        // display a case header
        char* cpHeader =
                "This example case illustrates parsing a simple XML file.\n"
                "The file has no XML declaration or DTD.\n"
                "Only the tag names, attributes and tagged content are captured and displayed.\n";
        printf("\n%s", cpHeader);

        printf("\nGet the XML file and use default call back functions for display of captured items.\n");
        printf("XML data from file %s\n", cpInput);
        uiSize = uiBufSize;
        vUtilFileRead(vpMem, cpInput, ucaBuf, &uiSize);
        if(uiSize >= uiBufSize){
            XTHROW(&e, "buffer size too small for file");
        }
        ucaBuf[uiSize] = 0;
        printf("%s\n", (char*)ucaBuf);
        vXmlGetFile(vpXml, cpInput);
        vXmlSetStartTagCallback(vpXml, DEFAULT_CALLBACK, NULL);
        vXmlSetEndTagCallback(vpXml, DEFAULT_CALLBACK, NULL);
        vXmlParse(vpXml);


    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vXmlDtor(vpXml);
    vMemDtor(vpMem);
    return iReturn;
}

static int iComment() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpXml = NULL;
    char* cpInput = "../input/comment.xml";
    uint8_t ucaBuf[1024];
    aint uiBufSize = 1024;
    aint uiSize;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpXml = vpXmlCtor(&e);

        // display a case header
        char* cpHeader =
                "This example case illustrates displaying the Processing Instructions and comments\n"
                "optionally found in an XML document.\n";
        printf("\n%s", cpHeader);

        printf("\nGet the XML file and use default call back functions for display PIs and comments.\n");
        printf("XML data from file %s\n", cpInput);
        uiSize = uiBufSize;
        vUtilFileRead(vpMem, cpInput, ucaBuf, &uiSize);
        if(uiSize >= uiBufSize){
            XTHROW(&e, "buffer size too small for file");
        }
        ucaBuf[uiSize] = 0;
        printf("%s\n", (char*)ucaBuf);
        vXmlGetFile(vpXml, cpInput);
        vXmlSetCommentCallback(vpXml, DEFAULT_CALLBACK, NULL);
        vXmlSetPICallback(vpXml, DEFAULT_CALLBACK, NULL);
        vXmlParse(vpXml);


    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vXmlDtor(vpXml);
    vMemDtor(vpMem);
    return iReturn;
}

static int iXmlDecl() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpXml = NULL;
    static void* vpFmt = NULL;
    const char* cpLine;
    char* cpSimple = "../input/simple.xml";
    char* cpDecl16le = "../input/xml-decl-16le.xml";
    uint8_t ucaBuf[1024];
    aint uiBufSize = 1024;
    aint uiSize;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpXml = vpXmlCtor(&e);
        vpFmt = vpFmtCtor(&e);

        // display a case header
        char* cpHeader =
                "This example case illustrates the XML declaration.\n"
                "A display of the XML declaration is compared between\n"
                "XML files with and without XML declarations.\n"
                "Furthermore, the file with the declaration is UTF-16 encoded.";
        printf("\n%s", cpHeader);

        printf("\nGet the XML files and use default call back functions for display of the XML declaration.\n");
        printf("XML data from file %s\n", cpSimple);
        uiSize = uiBufSize;
        vUtilFileRead(vpMem, cpSimple, ucaBuf, &uiSize);
        if(uiSize >= uiBufSize){
            XTHROW(&e, "buffer size too small for file");
        }
        ucaBuf[uiSize] = 0;
        printf("%s\n", (char*)ucaBuf);
        vXmlGetFile(vpXml, cpSimple);
        vXmlSetXmlDeclCallback(vpXml, DEFAULT_CALLBACK, NULL);
        vXmlSetStartTagCallback(vpXml, DEFAULT_CALLBACK, NULL);
        vXmlParse(vpXml);

        printf("XML data from file %s\n", cpDecl16le);
        uiSize = uiBufSize;
        vUtilFileRead(vpMem, cpDecl16le, ucaBuf, &uiSize);
        if(uiSize >= uiBufSize){
            XTHROW(&e, "buffer size too small for file");
        }
        ucaBuf[uiSize] = 0;
        cpLine = cpFmtFirstBytes(vpFmt, ucaBuf, uiSize, FMT_CANONICAL, 0, 0);
        while(cpLine){
            printf("%s", cpLine);
            cpLine = cpFmtNext(vpFmt);
        }
        vXmlGetFile(vpXml, cpDecl16le);
        vXmlSetXmlDeclCallback(vpXml, DEFAULT_CALLBACK, NULL);
        vXmlSetStartTagCallback(vpXml, DEFAULT_CALLBACK, NULL);
        vXmlParse(vpXml);


    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vXmlDtor(vpXml);
    vMemDtor(vpMem);
    return iReturn;
}

static int iDTD() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpXml = NULL;
    char* cpInput = "../input/dtd-entity-attr.xml";
    uint8_t ucaBuf[1024];
    aint uiBufSize = 1024;
    aint uiSize;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpXml = vpXmlCtor(&e);

        // display a case header
        char* cpHeader =
                "This example case illustrates parsing an XML file with a Document Type Declaration (DTD).\n"
                "The DTD will define entities and default attributes.\n"
                "These will be reflected in the parse of the input file.\n";
        printf("\n%s", cpHeader);

        printf("\nGet the XML file and use default call back functions for display of captured items.\n");
        printf("XML data from file %s\n", cpInput);
        uiSize = uiBufSize;
        vUtilFileRead(vpMem, cpInput, ucaBuf, &uiSize);
        if(uiSize >= uiBufSize){
            XTHROW(&e, "buffer size too small for file");
        }
        ucaBuf[uiSize] = 0;
        printf("%s\n", (char*)ucaBuf);
        vXmlGetFile(vpXml, cpInput);
        vXmlSetXmlDeclCallback(vpXml, DEFAULT_CALLBACK, NULL);
        vXmlSetDTDCallback(vpXml, DEFAULT_CALLBACK, NULL);
        vXmlSetStartTagCallback(vpXml, DEFAULT_CALLBACK, NULL);
        vXmlSetEndTagCallback(vpXml, DEFAULT_CALLBACK, NULL);
        vXmlParse(vpXml);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vXmlDtor(vpXml);
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
        return iSimple();
    case 3:
        return iComment();
    case 4:
        return iXmlDecl();
    case 5:
        return iDTD();
    default:
        return iHelp();
    }
}

