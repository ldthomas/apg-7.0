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
/** \dir examples/ex-format
 * \brief Examples of using the data formatting utility..
 */

/** \file examples/ex-format/main.c
 * \brief Driver for the data formatting utility examples.
 *
This example will demonstrate the construction and use of the data formatting utility.

Displaying bytes and Unicode code points which often do not have printing ASCII character counterparts
is a common problem with many applications. This object provides a common solution to this problem.
It is roughly patterned after the Linux [`hexdump`](https://www.man7.org/linux/man-pages/man1/hexdump.1.html) command.
Once the object has been created it can be used as an iterator to display fixed line length displays
of the data in several common formats.

  - application code must include header files:
      - ../../utilities/utilities.h
  - application compilation must include source code from the directories:
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - (none)

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Display 8-bit bytes, illustrating indentation and limits.
 - case 3: Display a mix of ASCII and non-ASCII bytes in all formats.
 - case 4: Display Unicode data in the Unicode format.
 */

/**
\page exformat The Data Formatting Utility

This example will demonstrate the construction and use of the data formatting utility.

Displaying bytes and Unicode code points which often do not have printing ASCII character counterparts
is a common problem with many applications. This object provides a common solution to this problem.
It is roughly patterned after the Linux [`hexdump`](https://www.man7.org/linux/man-pages/man1/hexdump.1.html) command.
Once the object has been created it can be used as an iterator to display fixed line length displays
of the data in several common formats.

  - application code must include header files:
      - ../../utilities/utilities.h
  - application compilation must include source code from the directories:
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - (none)

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Display 8-bit bytes, illustrating indentation and limits.
 - case 3: Display a mix of ASCII and non-ASCII bytes in all formats.
 - case 4: Display Unicode data in the Unicode format.
*/
#include <limits.h>
#include "../../utilities/utilities.h"

#include "source.h"

static const char* cpMakeFileName(char* cpBuffer, const char* cpBase, const char* cpDivider, const char* cpName){
    strcpy(cpBuffer, cpBase);
    strcat(cpBuffer, cpDivider);
    strcat(cpBuffer, cpName);
    return cpBuffer;
}

static char s_caBuf[PATH_MAX];

static char* s_cpDescription =
        "Illustrate the construction and use of the data formatting utility object.";

static char* s_cppCases[] = {
        "Display application information.",
        "Display 8-bit bytes, illustrating indentation and limits.",
        "Display a mix of ASCII and non-ASCII bytes in all formats.",
        "Display Unicode data in the Unicode format.",
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

static int iLimits() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpFmt = NULL;
    aint uiBufSize = 1024;
    uint8_t ucaData[1024];
    aint uiDataLen;
    const char* cpDataFile = cpMakeFileName(s_caBuf, SOURCE_DIR, "/../input/", "display-data");
    const char* cpLine;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpFmt = vpFmtCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case uses the utilities format object to do hexdump-style display\n"
                "illustrating offsets, limits and indentation.\n\n";
        printf("\n%s", cpHeader);

        // get the data to format and display
        uiDataLen = uiBufSize;
        vUtilFileRead(vpMem, cpDataFile, ucaData, &uiDataLen);
        if(uiDataLen > uiBufSize){
            XTHROW(&e, "data buffer too small for file");
        }

        // display as 8-bit bytes from file
        cpLine = cpFmtFirstFile(vpFmt, cpDataFile, FMT_HEX, 0, 0);
        printf("File %s data as 8-bit bytes.\n", cpDataFile);
        while(cpLine){
            printf("%s", cpLine);
            cpLine = cpFmtNext(vpFmt);
        }

        // display as 8-bit bytes with limits
        cpLine = cpFmtFirstFile(vpFmt, cpDataFile, FMT_HEX, 4, 10);
        printf("\nFile %s data as 8-bit bytes. Display only 10 bytes from offset 4 .\n", cpDataFile);
        while(cpLine){
            printf("%s", cpLine);
            cpLine = cpFmtNext(vpFmt);
        }

        // display as 8-bit bytes with indentation
        vFmtIndent(vpFmt, 4);
        cpLine = cpFmtFirstFile(vpFmt, cpDataFile, FMT_HEX, 0, 0);
        printf("\nFile %s data as 8-bit bytes, indented 4 spaces.\n", cpDataFile);
        while(cpLine){
            printf("%s", cpLine);
            cpLine = cpFmtNext(vpFmt);
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vFmtDtor(vpFmt);
    vMemDtor(vpMem);
    return iReturn;
}

static int iAscii() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpFmt = NULL;
    aint uiBufSize = 1024;
    uint8_t ucaData[1024];
    aint uiDataLen;
    const char* cpDataFile = cpMakeFileName(s_caBuf, SOURCE_DIR, "/../input/", "display-data");
    const char* cpLine;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpFmt = vpFmtCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case uses the utilities format object to do a hexdump-style display\n"
                "of a mix of printing ASCII characters and non-printing bytes in several formats.\n"
                "Data from both files and arrays are used.\n";
        printf("\n%s", cpHeader);

        // get the data to format and display
        uiDataLen = uiBufSize;
        vUtilFileRead(vpMem, cpDataFile, ucaData, &uiDataLen);
        if(uiDataLen > uiBufSize){
            XTHROW(&e, "data buffer too small for file");
        }

        // display as 8-bit bytes from file
        cpLine = cpFmtFirstFile(vpFmt, cpDataFile, FMT_HEX, 0, 0);
        printf("\nData as 8-bit bytes from file %s.\n", cpDataFile);
        while(cpLine){
            printf("%s", cpLine);
            cpLine = cpFmtNext(vpFmt);
        }

        // display as 8-bit bytes from array
        cpLine = cpFmtFirstBytes(vpFmt, ucaData, (uint64_t)uiDataLen, FMT_HEX, 0, 0);
        printf("\nData as 8-bit bytes from an array.\n");
        while(cpLine){
            printf("%s", cpLine);
            cpLine = cpFmtNext(vpFmt);
        }

        // display as 16-bit words from array
        cpLine = cpFmtFirstBytes(vpFmt, ucaData, (uint64_t)uiDataLen, FMT_HEX2, 0, 0);
        char* cpEndian = bIsBigEndian() ? "big endian" : "little endian";
        printf("\nData as 16-bit, %s words.\n", cpEndian);
        while(cpLine){
            printf("%s", cpLine);
            cpLine = cpFmtNext(vpFmt);
        }

        // display as ASCII characters
        cpLine = cpFmtFirstBytes(vpFmt, ucaData, (uint64_t)uiDataLen, FMT_ASCII, 0, 0);
        printf("\nData as ASCII characters. When non-printing decimal digit is displayed.\n");
        while(cpLine){
            printf("%s", cpLine);
            cpLine = cpFmtNext(vpFmt);
        }

        // display as canonical display of bytes and characters
        cpLine = cpFmtFirstBytes(vpFmt, ucaData, (uint64_t)uiDataLen, FMT_CANONICAL, 0, 0);
        printf("\nData in canonical display of both bytes and characters when possible.\n");
        while(cpLine){
            printf("%s", cpLine);
            cpLine = cpFmtNext(vpFmt);
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vFmtDtor(vpFmt);
    vMemDtor(vpMem);
    return iReturn;
}

static int iUnicode() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpFmt = NULL;
    aint uiBufSize = 1024;
    uint8_t ucaData[1024];
    aint uiDataLen;
    uint32_t* uip32;
    aint uiLen32;
    const char* cpLine;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpFmt = vpFmtCtor(&e);

        // make the Unicode data
//        uint32_t uiaOriginalData[] = {
//          0, 65, 66, 67, 97, 98, 99, 126, 127, 128, 0xff, 0x1ff, 0x1fff, 0xd7ff, 0xe000, 0xffff, 0x10ffff
//        };
//        aint uiOriginalLength = (aint)(sizeof(uiaOriginalData) / sizeof(uiaOriginalData[0]));
//        if(bIsBigEndian()){
//            vUtilFileWrite(vpMem, "unicode-data-be", uiaOriginalData, (uiOriginalLength * 4));
//        }else{
//            vUtilFileWrite(vpMem, "unicode-data-le", uiaOriginalData, (uiOriginalLength * 4));
//        }

        // get the Linux iconv data
        char* cpHeader =
                "This example case uses the utilities format object to do a hexdump-style\n"
                "display of Unicode code points.\n";
        printf("\n%s", cpHeader);

        // get the file
        uiDataLen = uiBufSize;
        if(bIsBigEndian()){
            vUtilFileRead(vpMem, cpMakeFileName(s_caBuf, SOURCE_DIR, "/../input/", "unicode-data-be"), ucaData, &uiDataLen);
        }else{
            vUtilFileRead(vpMem, cpMakeFileName(s_caBuf, SOURCE_DIR, "/../input/", "unicode-data-le"), ucaData, &uiDataLen);
        }
        if(uiDataLen > uiBufSize){
            XTHROW(&e, "data buffer too small for file");
        }

        // display as Unicode code points
        printf("\nDisplay Unicode format.\n");
        uip32 = (uint32_t*)ucaData;
        uiLen32 = uiDataLen / 4;
        cpLine = cpFmtFirstUnicode(vpFmt, uip32, (uint64_t)uiLen32, 0, 0);
        while(cpLine){
            printf("%s", cpLine);
            cpLine = cpFmtNext(vpFmt);
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vFmtDtor(vpFmt);
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
        return iLimits();
    case 3:
        return iAscii();
    case 4:
        return iUnicode();
    default:
        return iHelp();
    }
}

