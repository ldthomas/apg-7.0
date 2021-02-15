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
/** \dir examples/ex-conv
 * \brief Examples of using the data conversion utility..
 */

/** \file examples/ex-conv/main.c
 * \brief Driver for the data conversion utility examples.
 *
This example will demonstrate the construction and use of the data conversion utility.

  - application code must include header files:
      - ../../utilities/utilities.h
  - application compilation must include source code from the directories:
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - (none)

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Compare the conversion object results with the Linux iconv command-line application.
 - case 3: Get the raw, 32-bit decoded data.
 - case 4: Encode raw, 32-bit data.
 - case 5: Add base64 encoding and decoding.
 */

/**
\page exconv The Data Conversion Utility

This example will demonstrate the construction and use of the data conversion utility.

  - application code must include header files:
      - ../../utilities/utilities.h
  - application compilation must include source code from the directories:
      - ../../library
      - ../../utilities
  - application compilation must define macros:
      - (none)

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Compare the conversion object results with the Linux iconv command-line application.
 - case 3: Get the raw, 32-bit decoded data.
 - case 4: Encode raw, 32-bit data.
 - case 5: Add base64 encoding and decoding.
*/
#include "../../utilities/utilities.h"

static char* s_cpDescription =
        "Illustrate the construction and use of the data conversion utility object.";

static char* s_cppCases[] = {
        "Display application information.",
        "Compare the conversion object results with the Linux iconv command-line application.",
        "Get the raw, 32-bit decoded data.",
        "Encode raw, 32-bit data.",
        "Add base64 encoding and decoding.",
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
    char* cpName;
    uint8_t* ucpData;
    aint uiBytes;
    aint uiFormat;
} conv_file;
static int iConv() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpConv = NULL;
    char caBuf[1024];
    aint uiFileSize = 1024;
    conv_file saFiles[] = {
            { "../input/data8", NULL, 0, UTF_8},
            { "../input/data16le", NULL, 0, UTF_16LE},
            { "../input/data16be", NULL, 0, UTF_16BE},
            { "../input/data32le", NULL, 0, UTF_32LE},
            { "../input/data32be", NULL, 0, UTF_32BE},
    };
    conv_file* spFileIn, *spFileOut;
    aint uiFileCount = (aint)(sizeof(saFiles) / sizeof(saFiles[0]));
    aint ui, uj;
    conv_src sSrc;
    conv_dst sDst;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpConv = vpConvCtor(&e);

        // get the Linux iconv data
        char* cpHeader =
                "The Linux command \"iconv\" has been used to convert a simple array of data spanning\n"
                "the full range of Unicode characters. The files and formats are:\n"
                " - UTF-8,    data8\n"
                " - UTF-16LE, data16le\n"
                " - UTF-16BE, data16be\n"
                " - UTF-32LE, data32le\n"
                " - UTF-32BE, data32be\n"
                "\n"
                "This example case uses the utilities conv object to do all possible conversions\n"
                "and compare the converted data to the Linux file data.\n\n";
        printf("\n%s", cpHeader);
        for(ui = 0; ui < uiFileCount; ui++){
            spFileIn = &saFiles[ui];
            spFileIn->ucpData = vpMemAlloc(vpMem, uiFileSize);
            spFileIn->uiBytes = uiFileSize;
            vUtilFileRead(vpMem, spFileIn->cpName, spFileIn->ucpData, &spFileIn->uiBytes);
            if(spFileIn->uiBytes > uiFileSize){
                snprintf(caBuf, uiFileSize, "buffer not big enough for input file %s", spFileIn->cpName);
                XTHROW(&e, caBuf);
            }
        }

        // make all possible conversions and compare result with Linux iconv data
        printf("Make all possible comparisons doing the encoding and decoding steps separately.\n");
        for(ui = 0; ui < uiFileCount; ui++){
            spFileIn = &saFiles[ui];
            sSrc.ucpData = spFileIn->ucpData;
            sSrc.uiDataLen = spFileIn->uiBytes;
            sSrc.uiDataType = spFileIn->uiFormat;
            vConvDecode(vpConv, &sSrc);
            for(uj = 0; uj < uiFileCount; uj++){
                spFileOut = &saFiles[uj];
                memset(&sDst, 0, sizeof(sDst));
                sDst.uiDataType = spFileOut->uiFormat;
                vConvEncode(vpConv, &sDst);
                if(sDst.uiDataLen != spFileOut->uiBytes){
                    snprintf(caBuf, uiFileSize, "source (%s), destination (%s) conversion lengths not the same",
                            cpUtilUtfTypeName(spFileIn->uiFormat), cpUtilUtfTypeName(spFileOut->uiFormat));
                    XTHROW(&e, caBuf);
                }
                if(memcmp(sDst.ucpData, spFileOut->ucpData, spFileOut->uiBytes) != 0){
                    snprintf(caBuf, uiFileSize, "source (%s), destination (%s) conversion comparison failed",
                            cpUtilUtfTypeName(spFileIn->uiFormat), cpUtilUtfTypeName(spFileOut->uiFormat));
                    XTHROW(&e, caBuf);
                }
                printf("conversion (%s) -> (%s) successful\n",
                        cpUtilUtfTypeName(spFileIn->uiFormat), cpUtilUtfTypeName(spFileOut->uiFormat));
            }
        }

        printf("\nMake all possible comparisons encoding and decoding in a single step.\n");
        for(ui = 0; ui < uiFileCount; ui++){
            spFileIn = &saFiles[ui];
            sSrc.ucpData = spFileIn->ucpData;
            sSrc.uiDataLen = spFileIn->uiBytes;
            sSrc.uiDataType = spFileIn->uiFormat;
            for(uj = 0; uj < uiFileCount; uj++){
                spFileOut = &saFiles[uj];
                memset(&sDst, 0, sizeof(sDst));
                sDst.uiDataType = spFileOut->uiFormat;
                vConvConvert(vpConv, &sSrc, &sDst);
                if(sDst.uiDataLen != spFileOut->uiBytes){
                    snprintf(caBuf, uiFileSize, "source (%s), destination (%s) conversion lengths not the same",
                            cpUtilUtfTypeName(spFileIn->uiFormat), cpUtilUtfTypeName(spFileOut->uiFormat));
                    XTHROW(&e, caBuf);
                }
                if(memcmp(sDst.ucpData, spFileOut->ucpData, spFileOut->uiBytes) != 0){
                    snprintf(caBuf, uiFileSize, "source (%s), destination (%s) conversion comparison failed",
                            cpUtilUtfTypeName(spFileIn->uiFormat), cpUtilUtfTypeName(spFileOut->uiFormat));
                    XTHROW(&e, caBuf);
                }
                printf("conversion (%s) -> (%s) successful\n",
                        cpUtilUtfTypeName(spFileIn->uiFormat), cpUtilUtfTypeName(spFileOut->uiFormat));
            }
        }

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vConvDtor(vpConv);
    vMemDtor(vpMem);
    return iReturn;
}

static int iGet() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpConv = NULL;
    char caBuf[1024];
    aint uiFileSize = 1024;
    uint32_t uiaGetCodePoints[1024];
    uint32_t uiLength;
    uint32_t* uipCodePoints = &uiaGetCodePoints[0];
    conv_file saFiles[] = {
            { "../input/data8", NULL, 0, UTF_8},
            { "../input/data16le", NULL, 0, UTF_16LE},
            { "../input/data16be", NULL, 0, UTF_16BE},
            { "../input/data32le", NULL, 0, UTF_32LE},
            { "../input/data32be", NULL, 0, UTF_32BE},
    };
    conv_file* spFileIn;
    aint uiFileCount = (aint)(sizeof(saFiles) / sizeof(saFiles[0]));
    aint ui;
    uint32_t uiaOriginalData[] = {
      0, 126, 127, 128, 0xff, 0x1ff, 0x1fff, 0xd7ff, 0xe000, 0xffff, 0x10ffff
    };
    aint uiOriginalLength = (aint)(sizeof(uiaOriginalData) / sizeof(uiaOriginalData[0]));
    conv_src sSrc;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpConv = vpConvCtor(&e);

        // get the Linux iconv data
        char* cpHeader =
                "The Linux command \"iconv\" has been used to convert a simple array of data spanning\n"
                "the full range of Unicode characters. The files and formats are:\n"
                " - UTF-8,    data8\n"
                " - UTF-16LE, data16le\n"
                " - UTF-16BE, data16be\n"
                " - UTF-32LE, data32le\n"
                " - UTF-32BE, data32be\n"
                "\n"
                "This example case uses the utilities conv object to decode the files and compare the\n"
                "decoded data to the original data used to create the files.\n\n";
        printf("\n%s", cpHeader);
        printf("Original Code Points\n");
        for(ui = 0; ui < uiOriginalLength; ui++){
            printf("0x%06X ", uiaOriginalData[ui]);
        }
        printf("\n");

        for(ui = 0; ui < uiFileCount; ui++){
            spFileIn = &saFiles[ui];
            spFileIn->ucpData = vpMemAlloc(vpMem, uiFileSize);
            spFileIn->uiBytes = uiFileSize;
            vUtilFileRead(vpMem, spFileIn->cpName, spFileIn->ucpData, &spFileIn->uiBytes);
            if(spFileIn->uiBytes > uiFileSize){
                snprintf(caBuf, uiFileSize, "buffer not big enough for input file %s", spFileIn->cpName);
                XTHROW(&e, caBuf);
            }
        }

        // make all possible conversions and compare result with Linux iconv data
        printf("\nDecode all files, get decoded code points and compare to original.\n");
        for(ui = 0; ui < uiFileCount; ui++){
            spFileIn = &saFiles[ui];
            sSrc.ucpData = spFileIn->ucpData;
            sSrc.uiDataLen = spFileIn->uiBytes;
            sSrc.uiDataType = spFileIn->uiFormat;
            vConvDecode(vpConv, &sSrc);
            uiLength = uiFileSize;
            vConvGetCodePoints(vpConv, uipCodePoints, &uiLength);
            if(uiLength > (uint32_t)uiFileSize){
                XTHROW(&e, "Code point buffer too small for converted data.");
            }
            if(uiLength != uiOriginalLength){
                snprintf(caBuf, uiFileSize, "source (%s), converted code points length incorrect",
                        spFileIn->cpName);
                XTHROW(&e, caBuf);
            }
            if(memcmp(uipCodePoints, uiaOriginalData, (uiLength * sizeof(*uipCodePoints))) != 0){
                snprintf(caBuf, uiFileSize, "source (%s), conversion failed",
                        spFileIn->cpName);
                XTHROW(&e, caBuf);
            }
            printf("conversion (%s) successful\n",
                    spFileIn->cpName);
        }


    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vConvDtor(vpConv);
    vMemDtor(vpMem);
    return iReturn;
}
static int iUse() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpConv = NULL;
    static void* vpFmt = NULL;
    char caBuf[1024];
    aint uiFileSize = 1024;
    conv_file saFiles[] = {
            { "../input/data8", NULL, 0, UTF_8},
            { "../input/data16le", NULL, 0, UTF_16LE},
            { "../input/data16be", NULL, 0, UTF_16BE},
            { "../input/data32le", NULL, 0, UTF_32LE},
            { "../input/data32be", NULL, 0, UTF_32BE},
    };
    conv_file* spFileIn;
    aint uiFileCount = (aint)(sizeof(saFiles) / sizeof(saFiles[0]));
    aint ui;
    uint32_t uiaOriginalData[] = {
      0, 126, 127, 128, 0xff, 0x1ff, 0x1fff, 0xd7ff, 0xe000, 0xffff, 0x10ffff
    };
    aint uiOriginalLength = (aint)(sizeof(uiaOriginalData) / sizeof(uiaOriginalData[0]));
    conv_dst sDst;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpConv = vpConvCtor(&e);
        vpFmt = vpFmtCtor(&e);

        // get the Linux iconv data
        char* cpHeader =
                "The Linux command \"iconv\" has been used to convert a simple array of data spanning\n"
                "the full range of Unicode characters. The files and formats are:\n"
                " - UTF-8,    data8\n"
                " - UTF-16LE, data16le\n"
                " - UTF-16BE, data16be\n"
                " - UTF-32LE, data32le\n"
                " - UTF-32BE, data32be\n"
                "\n"
                "This example case uses the utilities conv object endocde the original data\n"
                "and compare the results to the files.\n\n";
        printf("\n%s", cpHeader);
        printf("Original Code Points\n");
        for(ui = 0; ui < uiOriginalLength; ui++){
            printf("0x%06X ", uiaOriginalData[ui]);
        }
        printf("\n");

        // read the files into memory
        for(ui = 0; ui < uiFileCount; ui++){
            spFileIn = &saFiles[ui];
            spFileIn->ucpData = vpMemAlloc(vpMem, uiFileSize);
            spFileIn->uiBytes = uiFileSize;
            vUtilFileRead(vpMem, spFileIn->cpName, spFileIn->ucpData, &spFileIn->uiBytes);
            if(spFileIn->uiBytes > uiFileSize){
                snprintf(caBuf, uiFileSize, "buffer not big enough for input file %s", spFileIn->cpName);
                XTHROW(&e, caBuf);
            }
        }

        // encode the original data to all formats and compare the results with Linux iconv data
        printf("\nCompare encoded original data to Linux iconv files.\n");
        for(ui = 0; ui < uiFileCount; ui++){
            spFileIn = &saFiles[ui];
            memset(&sDst, 0, sizeof(sDst));
            sDst.uiDataType = spFileIn->uiFormat;
            vConvUseCodePoints(vpConv, uiaOriginalData, uiOriginalLength);
            vConvEncode(vpConv, &sDst);
            if(sDst.uiDataLen != spFileIn->uiBytes){
                snprintf(caBuf, uiFileSize, "%s conversion lengths not the same", spFileIn->cpName);
                XTHROW(&e, caBuf);
            }
            if(memcmp(sDst.ucpData, spFileIn->ucpData, spFileIn->uiBytes) != 0){
                snprintf(caBuf, uiFileSize, "%s conversion data not the same", spFileIn->cpName);
                XTHROW(&e, caBuf);
            }
            printf("%s encoding successful\n", spFileIn->cpName);
        }


    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vFmtDtor(vpFmt);
    vConvDtor(vpConv);
    vMemDtor(vpMem);
    return iReturn;
}
static void vRemoveLineEnds(uint8_t* ucpData, aint uiLen, uint8_t* ucpBuf, aint* uipBufLen){
    // skip the validity checks
    aint ui = 0;
    aint uiLength = 0;
    for(; ui < uiLen; ui++){
        uint8_t ucChar = ucpData[ui];
        if(ucChar == 9 || ucChar == 13){
            continue;
        }
        ucpBuf[uiLength] = ucChar;
        uiLength++;
    }
    *uipBufLen = uiLength;
}
static int iBase64() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpConv = NULL;
    static void* vpFmt = NULL;
    char caBuf[1024];
    aint uiBufSize = 1024;
    uint8_t *ucpRand, *ucpRand64, *ucpRandBuf1, *ucpRandBuf2;
    aint uiRandLen, uiRand64Len, uiBuf1Len, uiBuf2Len;
    char* cpRandFile = "../input/rand512";
    char* cpRand64File = "../input/rand512b64";
    conv_src sSrc;
    conv_dst sDst;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpConv = vpConvCtor(&e);
        vpFmt = vpFmtCtor(&e);

        // display the header
        char* cpHeader =
                "The Linux command \"base64\" has been used to convert a file of 512 random bytes\n"
                "to base64 format. The files are \"rand512\" and \"rand512b64\", respectively.\n"
                "\n"
                "This example case uses the utilities conv object to do a base64 encoding of \"rand512\"\n"
                "and compare to \"rand512b64\".\n"
                "Note that the set of 256 8-bit bytes constitute ISO-8859-1 encoding.\n"
                "ISO_8859_1, LATIN1 and BINARY are used by the conv object as aliases.\n\n";
        printf("\n%s", cpHeader);

        // allocate all of the buffer space needed
        ucpRand = (uint8_t*)vpMemAlloc(vpMem, uiBufSize);
        ucpRand64 = (uint8_t*)vpMemAlloc(vpMem, uiBufSize);
        ucpRandBuf1 = (uint8_t*)vpMemAlloc(vpMem, uiBufSize);
        ucpRandBuf2 = (uint8_t*)vpMemAlloc(vpMem, uiBufSize);
        uiRandLen = uiBufSize;

        // read the files
        vUtilFileRead(vpMem, cpRandFile, ucpRand, &uiRandLen);
        if(uiRandLen > uiBufSize){
            snprintf(caBuf, uiBufSize, "buffer not big enough for input file %s", cpRandFile);
            XTHROW(&e, caBuf);
        }
        uiRand64Len = uiBufSize;
        vUtilFileRead(vpMem, cpRand64File, ucpRand64, &uiRand64Len);
        if(uiRand64Len > uiBufSize){
            snprintf(caBuf, uiBufSize, "buffer not big enough for input file %s", cpRand64File);
            XTHROW(&e, caBuf);
        }

        // base64 encode the file and compare
        sSrc.ucpData = ucpRand;
        sSrc.uiDataLen = uiRandLen;
        sSrc.uiDataType = BINARY;
        memset(&sDst, 0, sizeof(sDst));
        sDst.uiDataType = BINARY | BASE64;
        vConvConvert(vpConv, &sSrc, &sDst);
        if(sDst.uiDataLen > uiBufSize){
            XTHROW(&e, "buffer not big enough for input converted data");
        }
        vRemoveLineEnds(ucpRand64, uiRand64Len, ucpRandBuf1, &uiBuf1Len);
        vRemoveLineEnds(sDst.ucpData, sDst.uiDataLen, ucpRandBuf2, &uiBuf2Len);
        if(uiBuf1Len != uiBuf2Len){
            XTHROW(&e, "converted data not correct length");
        }
        if(memcmp(ucpRandBuf1, ucpRandBuf2, uiBuf1Len) != 0){
            XTHROW(&e, "converted data does not match file data");
        }
        printf("base64 conversion success\n");

        // configure the base64 output line length
        printf("\nThe default base64 file is has line feed line breaks at 76 characters.\n");
        printf("The line length and line ending can be configured with the conv utility.\n");

        printf("\nDefault conversion.\n");
        memcpy(ucpRandBuf1, sDst.ucpData, sDst.uiDataLen);
        ucpRandBuf1[sDst.uiDataLen] = 0;
        printf("%s\n", (char*)ucpRandBuf1);

        printf("Conversion with 100 characters per line.\n");
        vConvConfigureBase64(vpConv, 100, BASE64_LF);
        memset(&sDst, 0, sizeof(sDst));
        sDst.uiDataType = BINARY | BASE64;
        vConvConvert(vpConv, &sSrc, &sDst);
        if(sDst.uiDataLen > uiBufSize){
            XTHROW(&e, "buffer not big enough for input converted data");
        }
        memcpy(ucpRandBuf1, sDst.ucpData, sDst.uiDataLen);
        ucpRandBuf1[sDst.uiDataLen] = 0;
        printf("%s\n", (char*)ucpRandBuf1);

        printf("Conversion with 50 characters per line.\n");
        vConvConfigureBase64(vpConv, 50, BASE64_LF);
        memset(&sDst, 0, sizeof(sDst));
        sDst.uiDataType = BINARY | BASE64;
        vConvConvert(vpConv, &sSrc, &sDst);
        if(sDst.uiDataLen > uiBufSize){
            XTHROW(&e, "buffer not big enough for input converted data");
        }
        memcpy(ucpRandBuf1, sDst.ucpData, sDst.uiDataLen);
        ucpRandBuf1[sDst.uiDataLen] = 0;
        printf("%s\n", (char*)ucpRandBuf1);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vFmtDtor(vpFmt);
    vConvDtor(vpConv);
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
        return iConv();
    case 3:
        return iGet();
    case 4:
        return iUse();
    case 5:
        return iBase64();
    default:
        return iHelp();
    }
}

