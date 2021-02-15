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
/** \dir examples/ex-sip
 * \brief Examples of parsing SIP messages..
 *
 */

/** \file examples/ex-sip/main.c
 * \brief Driver for SIP message testing and timing..
 *

  - application code must include header files:
       - ../../json/json.h
  - application compilation must include source code from the directories:
       - ../../library
       - ../../utilities
       - ../../json
  - application compilation must define macros:
      - APG_TRACE
      - APG_STATS
      - APG_NO_PPPT (optional, for comparison with and without PPPTs)

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Build the JSON composite file of all SIP torture test messages.
 - case 3: Parse all valid SIP messages.
 - case 4: Parse all invalid SIP messages.
 - case 5: Parse all semantically invalid SIP messages.
 - case 6: Parse all SIP messages and measure the times, with an without UDTs.
 - case 7: Parse all SIP messages and display the node-hit statistics, with an without UDTs.
 */

/**
\page exsip Session Initiation Protocol &ndash; a Telecom Example

The Session Initiation Protocol (SIP) is described in [RFC 3261](https://tools.ietf.org/html/rfc3261).
From the abstract:
<pre>
"... Session Initiation Protocol (SIP), [is] an
application-layer control (signaling) protocol for creating,
modifying, and terminating sessions with one or more participants.
These sessions include Internet telephone calls, multimedia
distribution, and multimedia conferences."
</pre>

SIP has been chosen for this example because:
  - it is a large grammar (300+ rules)
  - it is a grammar of significant [commercial interest](https://en.wikipedia.org/wiki/List_of_SIP_software)
  - it has a large set of well-designed and well-explained [test cases](https://tools.ietf.org/html/rfc4475)

Full disclosure - I am not a SIP or telecom expert. All of the examples and
discussion here deal strictly with the syntax and parsing of the ABNF grammar.
The ABNF grammar used here has been slightly modified from the exact grammar
extracted from RFC 3261 to work correctly with the
[disambiguation rules](\ref disambiguation) of APG;

The test messages<sup>*</sup> used here, the so-called "torture tests", have been extracted
from [RFC 4475](https://tools.ietf.org/html/rfc4475) using the base64,
gzip-compressed TAR archive of files included there. Those files have been separated into
three directories, `./tests/valid` (section 3.1.1), `./tests/invalid` (section 3.1.2)
and `./tests/semantics` (sections 3.2, 3.3 and 3.4). Additionally, for each of these
data files a text file of the same base name has been created which contains
a copy of the discussion of that message from the RFC as well as my own
comments about the parsing of the message. For example,
    - `./valid/dblreq.dat`
    - `./valid/dblreq.txt`.

Again, my comments in the `*.txt` files are directed
strictly to the syntax. Nothing in them is intended to contradict or critique
the authors.

Case 2 of this example reads all of the files from all three of the directories
and collects them into a single JSON file, `./sip-tests.json`. This JSON file is then used as the
data source for all of the remaining test cases.

<sup>*</sup> Note that these test message are copyrighted by The Internet Society (2006).

<strong>Timing Tests</strong><br>
Comparison of the timing<sup>*</sup> from the various configurations is shown in the table below.
\htmlonly
<table style="align: left">
<tr align=right>
<th>App</th><th>PPPT</th><th>UDT</th><th>msec/msg</th><th>factor</th>
</tr>
<tr align=right>
<td>APG 7.0</td><td>no</td><td>no</td><td>0.0281</td><td>1.00</td>
</tr>
<tr align=right>
<td>APG 7.0</td><td>no</td><td>yes</td><td>0.0116</td><td>2.41</td>
</tr>
<tr align=right>
<td>APG 7.0</td><td>yes</td><td>no</td><td>0.0151</td><td>1.86</td>
</tr>
<tr align=right>
<td>APG 7.0</td><td>yes</td><td>yes</td><td>0.0117</td><td>2.40</td>
</tr>
<tr align=right>
<td>APG 6.3</td><td>no</td><td>no</td><td>0.0713</td><td>.394</td>
</tr>
</table>
\endhtmlonly
<br>
Notes:
  - UDT "yes" indicates a grammar with 14 UDTs replacing the 14 ABNF grammar rules with the highest number of node hits.
  - PPPT "no" indicates that PPPTs were not used, "yes" indicates that PPPTs were used.
  - factor is the base value/test value, i.e. a factor of 2 means that the test ran 2 times faster than the base.
  - msec is for the time to parse all 51 tests messages a 1000 times each.
  - Both the PPPT and UDT tests by themselves round to a factor of 2.0.
  - While the UDT test is as effective as the PPPT test, selecting, writing and testing the UDTs is very laborius.
  - Combining PPPTs and UDTs does not lead to any improvement.
  - Best to just use PPPTs and save the UDT writing for complex phrases that are difficult or impossible to describe with SABNF.
  - APG 7.0 with no PPPTs or UDTs is a factor of 2.5 faster than APG 6.3. While the APG 6.3 test did have a UDT grammar
  that gave a significantly greater improvement, it was a very aggressive grammar and that effort has not been undertaken here.

<sup>*</sup> OS: Ubuntu 20.04, processor: Intel i7-10710U, 6 cores, RAM: 64GB

<strong>Statistics Tests</strong><br>
Comparison of the total number of parse tree node hits from the various configurations is shown in the table below.
The hit total count is the cumulative total from parsing all 51 of the test messages.
\htmlonly
<table style="align: left">
<tr align=right>
<th>App</th><th>PPPT</th><th>UDT</th><th>hits/msg</th><th>factor</th>
</tr>
<tr align=right>
<td>APG 7.0</td><td>no</td><td>no</td><td>2953</td><td>1.00</td>
</tr>
<tr align=right>
<td>APG 7.0</td><td>no</td><td>yes</td><td>1024</td><td>2.88</td>
</tr>
<tr align=right>
<td>APG 7.0</td><td>yes</td><td>no</td><td>572</td><td>5.16</td>
</tr>
<tr align=right>
<td>APG 7.0</td><td>yes</td><td>yes</td><td>557</td><td>5.30</td>
</tr>
</table>
\endhtmlonly
<br>
Notes:
  - While PPPTs reduce the number of node hits by a factor of 5 they only speed the parser by a factor of roughly 2.
  This is indicative of the table look up times. Profiling also confirms this.
  - In terms of node hits, PPPTs and UDTs actually compete against one another.
  While UDTs reduce the number of node hits significantly, there is little further improvement due to the fact
  that all UDT table values are "ACTIVE", meaning that they require a full parse.

<strong>Application Requirements</strong>
  - application code must include header files:
       - <limits.h>
       - <time.h>
       - <dirent.h>
       - ../../utilities/utilities.h
       - ../../json/json.h
       - ./sip-0.h
       - ./sip-1.h
       - ./udtlib.h
  - application compilation must include source code from the directories:
       - ../../library
       - ../../utilities
       - ../../json
  - application compilation must define macros:
      - APG_TRACE
      - APG_STATS
      - APG_NO_PPPT (optional, for comparison with and without PPPTs)

The compiled example will execute the following cases. Run the application with no arguments for application usage.
 - case 1: Display application information. (type names, type sizes and defined macros)
 - case 2: Build the JSON composite file of all SIP torture test messages.
 - case 3: Parse and trace all valid SIP messages, with and without UDTs.
 - case 4: Parse and trace all invalid SIP messages, with and without UDTs.
 - case 5: Parse and trace all semantically invalid SIP messages, with and without UDTs.
 - case 6: Parse all SIP messages and measure the times, with and without UDTs.
 - case 7: Parse all SIP messages and display the node-hit statistics, with and without UDTs.
*/

#include <limits.h>
#include <time.h>
#include <dirent.h>
#include "../../utilities/utilities.h"
#include "../../json/json.h"
#include "./sip-0.h"
#include "./sip-1.h"
#include "./udtlib.h"

static char* s_cpDescription =
        "Illustrate parsing and time tests for SIP messages.";

static char* s_cppCases[] = {
        "Display application information.",
        "Build the JSON composite file of all SIP torture test messages.",
        "Parse and trace all valid SIP messages, with and without UDTs..",
        "Parse and trace all invalid SIP messages, with and without UDTs..",
        "Parse and trace all semantically invalid SIP messages, with and without UDTs..",
        "Parse all SIP messages and measure the times, with and without UDTs.",
        "Parse all SIP messages and display the node-hit statistics, with and without UDTs.",
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

typedef struct {
    char* cpSipJsonObject;
    char* cpValidKey;
    char* cpInvalidKey;
    char* cpSemanticsKey;
    char* cpValidIn;
    char* cpInvalidIn;
    char* cpSemanticsIn;
    char* cpStatsOut;
} test_context;
static test_context s_sTestsCtx = {
        "./sip-tests.json",
        "valid",
        "invalid",
        "semantics",
        "tests/valid/",
        "tests/invalid/",
        "tests/semantics/"
};
typedef struct {
    char* cpOutObject;
    void* vpMem;
    exception* spException;
    void* vpVecNames;
    void* vpVecNameIndexes;
    void* vpVecOut;
    void* vpVecIn;
    void* vpVecUnicode;
    void* vpVecChars;
} json_context;

static json_context sSetup(exception* spE, void* vpMem){
    json_context sCtx = {};
    sCtx.cpOutObject = "../output/sip-tests.json";
    sCtx.vpMem = vpMem;
    sCtx.spException = spE;
    sCtx.vpVecNames = vpVecCtor(sCtx.vpMem, sizeof(char), 10000);
    sCtx.vpVecNameIndexes = vpVecCtor(sCtx.vpMem, sizeof(aint), 100);
    sCtx.vpVecOut = vpVecCtor(sCtx.vpMem, sizeof(uint8_t), 10000);
    sCtx.vpVecIn = vpVecCtor(sCtx.vpMem, sizeof(uint8_t), 10000);
    sCtx.vpVecUnicode = vpVecCtor(sCtx.vpMem, sizeof(uint32_t), 10000);
    sCtx.vpVecChars = vpVecCtor(sCtx.vpMem, sizeof(achar), 10000);
    return sCtx;
}

static void vMakeList(json_context* spCtx, const char* cpDirName){
    vVecClear(spCtx->vpVecNames);
    vVecClear(spCtx->vpVecNameIndexes);
    struct dirent* spEnt;
    char* cpDat = ".dat";
    char* cpExt;
    aint uiChars, uiIndex, uiCount;
    void* vpName;
    char* cZero = 0;
    DIR* spDir = opendir(cpDirName);
    if(!spDir){
        char caBuf[1024];
        snprintf(caBuf, sizeof(caBuf), "can't open directory: %s", cpDirName);
        XTHROW(spCtx->spException, caBuf);
    }
    printf("directory name: %s\n", cpDirName);
    uiCount = 0;
    while((spEnt = readdir(spDir)) != NULL){
        if(spEnt->d_type == DT_REG){
            cpExt = strstr(spEnt->d_name, cpDat);
            if(cpExt){
                uiChars = (aint)(cpExt - &spEnt->d_name[0]);
                if(uiChars){
                    uiIndex = uiVecLen(spCtx->vpVecNames);
                    vpName = vpVecPushn(spCtx->vpVecNames, &spEnt->d_name[0], uiChars);
                    vpVecPush(spCtx->vpVecNames, &cZero);
                    vpVecPush(spCtx->vpVecNameIndexes, &uiIndex);
                    uiCount++;
                    printf("%d: file name found: %s\n", (int)uiCount, (char*)vpName);
                }
            }
        }
    }
}

/*
 * Make a JSON object for the given directory of data.
 * Object is of the form:
 * {
 *      "test name" : {"description" : "named text", "data" : "named data"},
 *      ...
 * }
 */
static aint uiMakeObject(json_context* spCtx, void* vpB, const char* cpDirName){
    aint ui, uii, uiCount, uiObjRoot, uiObjTest;
    aint uiKeyName, uiKeyDesc, uiKeyData;
    char caFileName[PATH_MAX];
    char* cpFile = caFileName; // keeps the compiler warnings happy
    char* cpDat = ".dat";
    char* cpTxt = ".txt";
    uint8_t* ucpIn8;
    uint32_t* uipIn32;
    uint32_t uiInLen;
    char* cpName;
    char* cpNames;
    aint* uipIndexes;
    void* vpVec = spCtx->vpVecIn;
    aint uiLen;
    uint8_t* ucpData;
    vVecClear(spCtx->vpVecOut);
    vMakeList(spCtx, cpDirName);
    cpNames = (char*)vpVecFirst(spCtx->vpVecNames);
    uipIndexes = (aint*)vpVecFirst(spCtx->vpVecNameIndexes);
    uiCount = uiVecLen(spCtx->vpVecNameIndexes);

    // create the object for this section of tests
    uiObjRoot = uiJsonBuildMakeObject(vpB);

    // create the keys used for each test object
    uiKeyDesc = uiJsonBuildMakeStringA(vpB, "description");
    uiKeyData = uiJsonBuildMakeStringA(vpB, "data");
    for(ui = 0; ui < uiCount; ui++){
        // create the object for this test
        uiObjTest = uiJsonBuildMakeObject(vpB);

        // create the key (test name) for this test object
        cpName = &cpNames[uipIndexes[ui]];
        uiKeyName = uiJsonBuildMakeStringA(vpB, cpName);

        // add the test description to the test object
        strcpy(cpFile, cpDirName);
        strcat(cpFile, cpName);
        strcat(cpFile, cpTxt);
        vVecClear(vpVec);
        uiLen = 0;
        vUtilFileRead(spCtx->vpMem, cpFile, NULL, &uiLen);
        ucpData = vpVecPushn(vpVec, NULL, uiLen);
        vUtilFileRead(spCtx->vpMem, cpFile, ucpData, &uiLen);
        vVecClear(spCtx->vpVecUnicode);
        uiInLen = (uint32_t)uiVecLen(vpVec);
        ucpIn8 = (uint8_t*)vpVecFirst(vpVec);
        vpVecPushn(spCtx->vpVecUnicode, NULL, uiInLen);
        uipIn32 = (uint32_t*)vpVecFirst(spCtx->vpVecUnicode);
        for(uii = 0; uii < uiInLen; uii++){
            uipIn32[uii] = (uint32_t)ucpIn8[uii];
        }
        uiJsonBuildAddToObject(vpB, uiObjTest, uiKeyDesc, uiJsonBuildMakeStringU(vpB, uipIn32, uiInLen));

        // add the test data to the test object
        strcpy(cpFile, cpDirName);
        strcat(cpFile, cpName);
        strcat(cpFile, cpDat);
        vVecClear(vpVec);
        vUtilFileRead(spCtx->vpMem, cpFile, NULL, &uiLen);
        ucpData = vpVecPushn(vpVec, NULL, uiLen);
        vUtilFileRead(spCtx->vpMem, cpFile, ucpData, &uiLen);
        vVecClear(spCtx->vpVecUnicode);
        uiInLen = (uint32_t)uiVecLen(vpVec);
        ucpIn8 = (uint8_t*)vpVecFirst(vpVec);
        vpVecPushn(spCtx->vpVecUnicode, NULL, uiInLen);
        uipIn32 = (uint32_t*)vpVecFirst(spCtx->vpVecUnicode);
        for(uii = 0; uii < uiInLen; uii++){
            uipIn32[uii] = (uint32_t)ucpIn8[uii];
        }
        uiJsonBuildAddToObject(vpB, uiObjTest, uiKeyData, uiJsonBuildMakeStringU(vpB, uipIn32, uiInLen));

        // add the test object to the root object for this section of tests
        uiJsonBuildAddToObject(vpB, uiObjRoot, uiKeyName, uiObjTest);
    }
    return uiObjRoot;
}

static int iBuilder() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpJson = NULL;
    static void* vpB = NULL;
    json_context sCtx;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpJson = vpJsonCtor(&e);
        vpB = vpJsonBuildCtor(vpJson);

        // display the information header
        char* cpHeader =
                "This function will read the text and data SIP torture test files and\n"
                "wrap them all into a single JSON file for later use by other example cases.\n";
        printf("\n%s", cpHeader);

        sCtx = sSetup(&e, vpMem);
        aint uiRoot, uiKey;
        void* vpIt;
        json_value* spValue;
        uint8_t* ucpBytes;
        aint uiCount;
        uiRoot = uiJsonBuildMakeObject(vpB);

        // add the valid tests
        uiKey = uiJsonBuildMakeStringA(vpB, s_sTestsCtx.cpValidKey);
        uiJsonBuildAddToObject(vpB, uiRoot, uiKey, uiMakeObject(&sCtx, vpB, s_sTestsCtx.cpValidIn));

        // add the invalid tests
        uiKey = uiJsonBuildMakeStringA(vpB, s_sTestsCtx.cpInvalidKey);
        uiJsonBuildAddToObject(vpB, uiRoot, uiKey, uiMakeObject(&sCtx, vpB, s_sTestsCtx.cpInvalidIn));

        // add the semantics tests
        uiKey = uiJsonBuildMakeStringA(vpB, s_sTestsCtx.cpSemanticsKey);
        uiJsonBuildAddToObject(vpB, uiRoot, uiKey, uiMakeObject(&sCtx, vpB, s_sTestsCtx.cpSemanticsIn));

        // make a single JSON object file which holds ALL of the tests
        vpIt = vpJsonBuild(vpB, uiRoot);
        spValue = spJsonIteratorFirst(vpIt);
        ucpBytes = ucpJsonWrite(vpJson, spValue, &uiCount);

        // write the UTF-8 byte stream to a file
        vUtilFileWrite(sCtx.vpMem, sCtx.cpOutObject, ucpBytes, uiCount);
        printf("JSON file created: %s\n", sCtx.cpOutObject);

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

typedef struct{
    aint uiNameOffset;
    aint uiNameLength;
    aint uiDescOffset;
    aint uiDescLength;
    aint uiMsgOffset;
    aint uiMsgLength;
} msg_offset;

#define SECTION_VALID       0
#define SECTION_INVALID     1
#define SECTION_SEMANTICS   2
typedef struct{
    void* vpVecMsgs;
    void* vpVecNames;
    void* vpVecDesc;
    void* vpVecOffsets;
    char* cpSectionName;
    char* cpJsonFileName;
    aint uiCount;
    aint uiSection;
} section_def;
static char s_cZero = 0;
static void vGetMsgs(void* vpMem, section_def* spSection, abool bDisplay){
    if(!bMemValidate(vpMem)){
        vExContext();
    }
    exception* spEx = spMemException(vpMem);
    spSection->vpVecMsgs = vpVecCtor(vpMem, sizeof(achar), 8192);
    spSection->vpVecNames= vpVecCtor(vpMem, sizeof(char), 8192);
    spSection->vpVecDesc = vpVecCtor(vpMem, sizeof(char), 8192);
    spSection->vpVecOffsets = vpVecCtor(vpMem, sizeof(msg_offset), 128);
    void* vpItRoot, *vpItSection, *vpItTests, *vpItContent;
    json_value* spValue, *spTest;
    u32_phrase* spPhrase;
    msg_offset* spOffset;
    aint ui;
    char cChar;
    achar acaOctet;
    char* cpTest;
    void* vpJson = vpJsonCtor(spEx);
    vpItRoot = vpJsonReadFile(vpJson, spSection->cpJsonFileName);
    spValue = spJsonIteratorFirst(vpItRoot);

    // get an iterator over the section name files
    vpItSection = vpJsonFindKeyA(vpJson, spSection->cpSectionName, spValue);
    if(!vpItSection){
        vJsonDtor(vpJson);
        XTHROW(spEx, "expected key not found");
    }

    // iterator over the tests in the section name files
    vpItTests = vpJsonChildren(vpJson, spJsonIteratorFirst(vpItSection));
    spTest = spJsonIteratorFirst(vpItTests);
    spSection->uiCount = 1;
    while(spTest){
        // the key for this test is the test name
        spOffset = (msg_offset*)vpVecPush(spSection->vpVecOffsets, NULL);
        spOffset->uiNameOffset = uiVecLen(spSection->vpVecNames);
        spPhrase = spTest->spKey;
        spOffset->uiNameLength = spPhrase->uiLength;
        for(ui = 0; ui < spPhrase->uiLength; ui++){
            if(spPhrase->uipPhrase[ui] > 126){
                vJsonDtor(vpJson);
                XTHROW(spEx, "test names must be ASCII characters only");
            }
            cChar = (char)spPhrase->uipPhrase[ui];
            vpVecPush(spSection->vpVecNames, &cChar);
        }
        vpVecPush(spSection->vpVecNames, &s_cZero);
        cpTest = (char*)vpVecAt(spSection->vpVecNames, spOffset->uiNameOffset);
        if(bDisplay){
            printf("\n%2d: test name: %s\n", (int)spSection->uiCount, cpTest);
        }

        // the children of the test object are the description and the test SIP message
        // get the description as the first child of the test object
        vpItContent = vpJsonChildren(vpJson, spTest);
        spValue = spJsonIteratorFirst(vpItContent);
        spPhrase = spValue->spString;
        spOffset->uiDescOffset = uiVecLen(spSection->vpVecDesc);
        spOffset->uiDescLength = spPhrase->uiLength;
        for(ui = 0; ui < spPhrase->uiLength; ui++){
            if(spPhrase->uipPhrase[ui] > 126){
                vJsonDtor(vpJson);
                XTHROW(spEx, "test descriptions must be ASCII characters only");
            }
            cChar = (char)spPhrase->uipPhrase[ui];
            vpVecPush(spSection->vpVecDesc, &cChar);
        }
        vpVecPush(spSection->vpVecDesc, &s_cZero);
        cpTest = (char*)vpVecAt(spSection->vpVecDesc, spOffset->uiDescOffset);
        if(bDisplay){
            printf("\n%2d test description: %s\n", (int)spSection->uiCount, cpTest);
        }

        // get the SIP message as the second child of the test object
        spValue = spJsonIteratorNext(vpItContent);
        spPhrase = spValue->spString;
        spOffset->uiMsgOffset = uiVecLen(spSection->vpVecMsgs);
        spOffset->uiMsgLength = spPhrase->uiLength;
        if(bDisplay){
            printf("offset: %d: length %d\n", (int)spOffset->uiMsgOffset, (int)spOffset->uiMsgLength);
        }
        for(ui = 0; ui < spPhrase->uiLength; ui++){
            if(spPhrase->uipPhrase[ui] > 255){
                vJsonDtor(vpJson);
                XTHROW(spEx, "test messages must be octets only");
            }
            acaOctet= (achar)spPhrase->uipPhrase[ui];
            vpVecPush(spSection->vpVecMsgs, &acaOctet);
        }
        spTest = spJsonIteratorNext(vpItTests);
        vJsonIteratorDtor(vpItContent);
        spSection->uiCount++;
    }
    spSection->uiCount--;
    if(bDisplay){
        printf("number of msgs: %d\n", (int)spSection->uiCount);
    }
    vJsonDtor(vpJson);
}
static void vParseTheMsgs(exception* spEx, section_def* spSection, char* cpPrefix){
    parser_config sConfig = {};
    parser_state sState;
    achar* acpMsgs;
    char* cpNameBeg;
    msg_offset* spOffsetBeg, *spOffset;
    char caTraceName[1024];
    char* cpTestName;
    char* cpTraceConfig = "./trace-config.txt";
    aint ui;
    void* vpParser = NULL;
    void* vpTrace;
    acpMsgs = (achar*)vpVecFirst(spSection->vpVecMsgs);
    cpNameBeg = (char*)vpVecFirst(spSection->vpVecNames);
    spOffsetBeg = (msg_offset*)vpVecFirst(spSection->vpVecOffsets);

    // parse the messages
    printf("\nParse the Messages without UDTs\n");
    vpParser = vpParserCtor(spEx, vpSip0Init);
    vpTrace = vpTraceCtor(vpParser);
    vTraceConfig(vpTrace, cpTraceConfig);
    spOffset = spOffsetBeg;
    for(ui = 0; ui < spSection->uiCount; ui++){
        cpTestName = cpNameBeg + spOffset->uiNameOffset;
        snprintf(caTraceName, sizeof(caTraceName), "%s-%d-%s.out", cpPrefix, (int)(ui + 1), cpTestName);
        vTraceSetOutput(vpTrace, caTraceName);
        sConfig.acpInput = acpMsgs + spOffset->uiMsgOffset;
        sConfig.uiInputLength = spOffset->uiMsgLength;
        sConfig.uiStartRule = 0; // assumes that start rule is first rule, index 0 - use uiParseRuleLookup() if not sure
        vParserParse(vpParser, &sConfig, &sState);
        if(sState.uiSuccess){
            printf("%2d:  test name: %s: success\n", (int)(ui + 1), cpTestName);
        }else{
            printf("%2d:  test name: %s: failure\n", (int)(ui + 1), cpTestName);
        }
        vUtilPrintParserState(&sState);
        printf("\n");
        spOffset++;
    }

    printf("\nParse the Messages with UDTs\n");
    vParserDtor(vpParser);
    vpParser = vpParserCtor(spEx, vpSip1Init);
    vSip1UdtCallbacks(vpParser);
    vpTrace = vpTraceCtor(vpParser);
    vTraceConfig(vpTrace, cpTraceConfig);
    spOffset = spOffsetBeg;
    for(ui = 0; ui < spSection->uiCount; ui++){
        cpTestName = cpNameBeg + spOffset->uiNameOffset;
        snprintf(caTraceName, sizeof(caTraceName), "%s-udt-%d-%s.out", cpPrefix, (int)(ui + 1), cpTestName);
        vTraceSetOutput(vpTrace, caTraceName);
        sConfig.acpInput = acpMsgs + spOffset->uiMsgOffset;
        sConfig.uiInputLength = spOffset->uiMsgLength;
        sConfig.uiStartRule = 0; // assumes that start rule is first rule, index 0 - use uiParseRuleLookup() if not sure
        vParserParse(vpParser, &sConfig, &sState);
        if(sState.uiSuccess){
            printf("%2d:  test name: %s: success\n", (int)(ui + 1), cpTestName);
        }else{
            printf("%2d:  test name: %s: failure\n", (int)(ui + 1), cpTestName);
        }
        vUtilPrintParserState(&sState);
        printf("\n");
        spOffset++;
    }
    vParserDtor(vpParser);
}
static int iValid() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpJson = NULL;
    section_def sSection = {};
    char* cpOutPrefix;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpJson = vpJsonCtor(&e);

        // display the information header
        char* cpHeader =
                "This function will read the SIP torture test valid messages and parse them,\n"
                "with and without UDTs, displaying the test name and parsing result.\n"
                "The parse is traced to the ../output folder.\n"
                "To test without PPPTs, compile with APG_NO_PPPT defined.\n"
                "To test with PPPTs, leave APG_NO_PPPT undefined.\n";
        printf("\n%s", cpHeader);

        sSection.cpJsonFileName = s_sTestsCtx.cpSipJsonObject;
        sSection.cpSectionName = s_sTestsCtx.cpValidKey;
        vGetMsgs(vpMem, &sSection, APG_TRUE);

#ifdef APG_NO_PPPT
        cpOutPrefix = "../output/valid-trace";
#else
        cpOutPrefix = "../output/valid-trace-pppt";
#endif
        vParseTheMsgs(&e, &sSection, cpOutPrefix);

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

static int iInvalid() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpJson = NULL;
    section_def sSection = {};
    char* cpOutPrefix;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpJson = vpJsonCtor(&e);

        // display the information header
        char* cpHeader =
                "This function will read the SIP torture test invalid messages and parse them,\n"
                "with and without UDTs, displaying the test name and parsing result.\n"
                "The parse is traced to the ../output folder.\n"
                "To test without PPPTs, compile with APG_NO_PPPT defined.\n"
                "To test with PPPTs, leave APG_NO_PPPT undefined.\n";
        printf("\n%s", cpHeader);

        sSection.cpJsonFileName = s_sTestsCtx.cpSipJsonObject;
        sSection.cpSectionName = s_sTestsCtx.cpInvalidKey;
        vGetMsgs(vpMem, &sSection, APG_TRUE);

#ifdef APG_NO_PPPT
        cpOutPrefix = "../output/invalid-trace";
#else
        cpOutPrefix = "../output/invalid-trace-pppt";
#endif
        vParseTheMsgs(&e, &sSection, cpOutPrefix);

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

static int iSemantics() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpJson = NULL;
    section_def sSection = {};
    char* cpOutPrefix;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpJson = vpJsonCtor(&e);

        // display the information header
        char* cpHeader =
                "This function will read the SIP torture test semantics messages and parse them,\n"
                "with and without UDTs, displaying the test name and parsing result.\n"
                "The parse is traced to the ../output folder.\n"
                "To test without PPPTs, compile with APG_NO_PPPT defined.\n"
                "To test with PPPTs, leave APG_NO_PPPT undefined.\n"
                "Note that all of these tests are syntactically correct and parse successfully.\n"
                "Since the errors are in the semantics no attempt at critiquing them is done.\n";
        printf("\n%s", cpHeader);

        sSection.cpJsonFileName = s_sTestsCtx.cpSipJsonObject;
        sSection.cpSectionName = s_sTestsCtx.cpSemanticsKey;
        vGetMsgs(vpMem, &sSection, APG_TRUE);
#ifdef APG_NO_PPPT
        cpOutPrefix = "../output/semantics-trace";
#else
        cpOutPrefix = "../output/semantics-trace-pppt";
#endif

        vParseTheMsgs(&e, &sSection, cpOutPrefix);

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

typedef struct{
    aint uiMsgs;
    aint uiChars;
    double dTime;
    double dTimePerMsg;
} time_test;
static int iTime() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpVecConfig = NULL;
    static void* vpJson = NULL;
    static void* vpParser = NULL;
    static FILE* spFp = NULL;
    static FILE* spFpUdt = NULL;
    char* cpOut, *cpOutUdt;
    section_def sSection[3];
    char caBuf[1024];
    aint ui, uj, uiTests;
    clock_t tStartTime, tEndTime;
    double  dMSec, dMsgsPerMSec;
    luint uiMsgs, uiCharCount, uiMsgCount;
    parser_config* spConfig, *spStart, *spEnd;
    parser_state sState;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpJson = vpJsonCtor(&e);

        // display the information header
        char* cpHeader =
                "This function will read all the SIP torture tests and parse them all multiple times.\n"
                "Timing results will be collected and written to the ../output folder\n"
                "To test without PPPTs, compile with APG_NO_PPPT defined.\n"
                "To test with PPPTs, leave APG_NO_PPPT undefined.\n";
        printf("\n%s", cpHeader);

#ifdef APG_NO_PPPT
        cpOut = "../output/time.out";
        cpOutUdt = "../output/time-udt.out";
#else
        cpOut = "../output/time-pppt.out";
        cpOutUdt = "../output/time-pppt-udt.out";
#endif
        spFp = fopen(cpOut, "wb");
        if(!spFp){
            snprintf(caBuf, sizeof(caBuf), "can't open output file %s", cpOut);
            XTHROW(&e, caBuf);
        }
        spFpUdt = fopen(cpOutUdt, "wb");
        if(!spFpUdt){
            snprintf(caBuf, sizeof(caBuf), "can't open output file %s", cpOutUdt);
            XTHROW(&e, caBuf);
        }

        sSection[0].cpJsonFileName = s_sTestsCtx.cpSipJsonObject;
        sSection[0].cpSectionName = s_sTestsCtx.cpValidKey;
        vGetMsgs(vpMem, &sSection[0], APG_FALSE);
        sSection[1].cpJsonFileName = s_sTestsCtx.cpSipJsonObject;
        sSection[1].cpSectionName = s_sTestsCtx.cpInvalidKey;
        vGetMsgs(vpMem, &sSection[1], APG_FALSE);
        sSection[2].cpJsonFileName = s_sTestsCtx.cpSipJsonObject;
        sSection[2].cpSectionName = s_sTestsCtx.cpSemanticsKey;
        vGetMsgs(vpMem, &sSection[2], APG_FALSE);
        vpVecConfig = vpVecCtor(vpMem, sizeof(parser_config), 60);

        for(uj = 0; uj < 3; uj++){
            achar* acpMsgs = (achar*)vpVecFirst(sSection[uj].vpVecMsgs);
            msg_offset* spOffset = (msg_offset*)vpVecFirst(sSection[uj].vpVecOffsets);
            for(ui = 0; ui < sSection[uj].uiCount; ui++){
                spConfig = (parser_config*)vpVecPush(vpVecConfig, NULL);
                memset(spConfig, 0, sizeof(parser_config));
                spConfig->acpInput = acpMsgs + spOffset->uiMsgOffset;
                spConfig->uiInputLength = spOffset->uiMsgLength;
                spConfig->uiStartRule = 0; // assumes that start rule is first rule, index 0 - use uiParseRuleLookup() if not sure
                spOffset++;
            }
        }


        spStart = (parser_config*)vpVecFirst(vpVecConfig);
        uiMsgs = (luint)uiVecLen(vpVecConfig);
        spEnd = spStart + uiMsgs;
        uiTests = 1000;

        uiCharCount = 0;
        uiMsgCount = 0;
        vpParser = vpParserCtor(&e, vpSip0Init);
        printf("\nBeginning Tests without UDTs: be patient\n");
        tStartTime = clock();
        for(ui = 0; ui < uiTests; ui++){
            for(spConfig = spStart; spConfig < spEnd; spConfig++){
                uiCharCount += (luint)spConfig->uiInputLength;
                vParserParse(vpParser, spConfig, &sState);
            }
            uiMsgCount += uiMsgs;
        }
        tEndTime = clock();
        dMSec = (double)((tEndTime - tStartTime) * 1000) / (double)CLOCKS_PER_SEC;
        dMsgsPerMSec = (double)uiMsgCount / dMSec;
        fprintf(spFp, "Timing Tests without UDTs\n");
        fprintf(spFp, "  Messages: %"PRIuMAX"\n", uiMsgCount);
        fprintf(spFp, "Characters: %"PRIuMAX"\n", uiCharCount);
        fprintf(spFp, "      msec: %e\n", dMSec);
        fprintf(spFp, "  msec/msg: %e\n", (1.0 / dMsgsPerMSec));
        fprintf(spFp, " msgs/msec: %e\n", dMsgsPerMSec);
        printf("Results written to %s\n", cpOut);

        uiCharCount = 0;
        uiMsgCount = 0;
        vParserDtor(vpParser);
        vpParser = vpParserCtor(&e, vpSip1Init);
        vSip1UdtCallbacks(vpParser);
        printf("\nBeginning Tests with UDTs: be patient\n");
        tStartTime = clock();
        for(ui = 0; ui < uiTests; ui++){
            for(spConfig = spStart; spConfig < spEnd; spConfig++){
                uiCharCount += (luint)spConfig->uiInputLength;
                vParserParse(vpParser, spConfig, &sState);
            }
            uiMsgCount += uiMsgs;
        }
        tEndTime = clock();
        dMSec = (double)((tEndTime - tStartTime) * 1000) / (double)CLOCKS_PER_SEC;
        dMsgsPerMSec = (double)uiMsgCount /dMSec;
        fprintf(spFpUdt, "Timing Tests with UDTs\n");
        fprintf(spFpUdt, "  Messages: %"PRIuMAX"\n", uiMsgCount);
        fprintf(spFpUdt, "Characters: %"PRIuMAX"\n", uiCharCount);
        fprintf(spFpUdt, "      msec: %e\n", dMSec);
        fprintf(spFpUdt, "  msec/msg: %e\n", (1.0 / dMsgsPerMSec));
        fprintf(spFpUdt, " msgs/msec: %e\n", dMsgsPerMSec);
        printf("Results written to %s\n", cpOutUdt);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    if(spFp){
        fclose(spFp);
    }
    if(spFpUdt){
        fclose(spFpUdt);
    }
    vParserDtor(vpParser);
    vJsonDtor(vpJson);
    vMemDtor(vpMem);
    return iReturn;
}

static int iStats() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpStats = NULL;
    static void* vpVecConfig = NULL;
    static void* vpJson = NULL;
    static void* vpParser = NULL;
    char* cpOut, *cpOutUdt;
    section_def sSection[3];
    aint ui, uj;
    parser_config* spConfig, *spStart, *spEnd;
    parser_state sState;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpJson = vpJsonCtor(&e);

        // display the information header
        char* cpHeader =
                "This function will parse all of the SIP torture tests and display the node-hit statistics.\n"
                "Comparisons will show the differences between parsing with and without PPPTs,\n"
                " and with and without UDTs.\n";
        printf("\n%s", cpHeader);

        sSection[0].cpJsonFileName = s_sTestsCtx.cpSipJsonObject;
        sSection[0].cpSectionName = s_sTestsCtx.cpValidKey;
        vGetMsgs(vpMem, &sSection[0], APG_FALSE);
        sSection[1].cpJsonFileName = s_sTestsCtx.cpSipJsonObject;
        sSection[1].cpSectionName = s_sTestsCtx.cpInvalidKey;
        vGetMsgs(vpMem, &sSection[1], APG_FALSE);
        sSection[2].cpJsonFileName = s_sTestsCtx.cpSipJsonObject;
        sSection[2].cpSectionName = s_sTestsCtx.cpSemanticsKey;
        vGetMsgs(vpMem, &sSection[2], APG_FALSE);
        vpVecConfig = vpVecCtor(vpMem, sizeof(parser_config), 60);

        for(uj = 0; uj < 3; uj++){
            achar* acpMsgs = (achar*)vpVecFirst(sSection[uj].vpVecMsgs);
            msg_offset* spOffset = (msg_offset*)vpVecFirst(sSection[uj].vpVecOffsets);
            for(ui = 0; ui < sSection[uj].uiCount; ui++){
                spConfig = (parser_config*)vpVecPush(vpVecConfig, NULL);
                memset(spConfig, 0, sizeof(parser_config));
                spConfig->acpInput = acpMsgs + spOffset->uiMsgOffset;
                spConfig->uiInputLength = spOffset->uiMsgLength;
                spConfig->uiStartRule = 0; // assumes that start rule is first rule, index 0 - use uiParseRuleLookup() if not sure
                spOffset++;
            }
        }

#ifdef APG_NO_PPPT
        cpOut = "../output/stats.out";
        cpOutUdt = "../output/stats-udt.out";
#else
        cpOut = "../output/stats-pppt.out";
        cpOutUdt = "../output/stats-pppt-udt.out";
#endif
        aint uiMsgs = uiVecLen(vpVecConfig);
        spStart = (parser_config*)vpVecFirst(vpVecConfig);
        spEnd = spStart + uiMsgs;
        vpParser = vpParserCtor(&e, vpSip0Init);
        vpStats = vpStatsCtor(vpParser);
        spConfig = spStart;
        printf("\nStats without UDTs: cumulative for %d messages\n", (int)uiMsgs);
        for(; spConfig < spEnd; spConfig++){
            vParserParse(vpParser, spConfig, &sState);
        }
        vStatsToAscii(vpStats, NULL, cpOut);
        printf("Results written to %s\n", cpOut);

        vParserDtor(vpParser);
        vpParser = vpParserCtor(&e, vpSip1Init);
        vSip1UdtCallbacks(vpParser);
        vpStats = vpStatsCtor(vpParser);
        spConfig = spStart;
        printf("\nStats with UDTs: cumulative for %d messages\n", (int)uiMsgs);
        for(; spConfig < spEnd; spConfig++){
            vParserParse(vpParser, spConfig, &sState);
        }
        vStatsToAscii(vpStats, NULL, cpOutUdt);
        printf("Results written to %s\n", cpOutUdt);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vParserDtor(vpParser);
    vJsonDtor(vpJson);
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
        return iBuilder();
    case 3:
        return iValid();
    case 4:
        return iInvalid();
    case 5:
        return iSemantics();
    case 6:
        return iTime();
    case 7:
        return iStats();
    default:
        return iHelp();
    }
}

