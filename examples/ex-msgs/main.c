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
/** \dir examples/ex-msgs
 * \brief Examples of using the message log utility..
 */

/** \file examples/ex-msgs/main.c
 * \brief Driver for the message log utility examples..
 *
This example will demonstrate the construction and use of the message log utility.

Within the APG applications and objects there are many times the need to maintain a log of messages.
This object is a simple, but consistent method for logging multiple messages,
whether error messages or warning messages or otherwise,
with an iterator for retrieving them.

See \ref vUtilPrintMsgs() for a utility that will display all messages in a message log object.

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
 - case 2: Illustrate the use of the message log object.
 */

/**
\page exmsglog The Message Log Utility

This example will demonstrate the construction and use of the message log utility.

Within the APG applications and objects there are many times the need to maintain a log of messages.
This object is a simple, but consistent method for logging multiple messages,
whether error messages or warning messages or otherwise,
with an iterator for retrieving them.

See \ref vUtilPrintMsgs() for a utility that will display all messages in a message log object.

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
 - case 2: Illustrate the use of the message log object.
*/
#include "../../utilities/utilities.h"

static char* s_cpDescription =
        "Illustrate the construction and use of the message log object.";

static char* s_cppCases[] = {
        "Display application information.",
        "Illustrate the use of the message log object.",
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

static int iMsgs() {
    int iReturn = EXIT_SUCCESS;
    static void* vpMem = NULL;
    static void* vpMsgs = NULL;
    const char* cpMsg;
    exception e;
    XCTOR(e);
    if(e.try){
        // try block
        vpMem = vpMemCtor(&e);
        vpMsgs = vpMsgsCtor(&e);

        // display the information header
        char* cpHeader =
                "This example case uses the message logging object to log, display and clear a few messages.\n";
        printf("\n%s", cpHeader);

        // log a few messages
        vMsgsLog(vpMsgs, "bad character here");
        vMsgsLog(vpMsgs, "bad format there");
        vMsgsLog(vpMsgs, "wrong thing to do here");
        vMsgsLog(vpMsgs, "too many errors to continue");
        printf("\nDisplay the %d logged messages with the iterator.\n", (int)uiMsgsCount(vpMsgs));
        cpMsg = cpMsgsFirst(vpMsgs);
        while(cpMsg){
            printf("%s\n", cpMsg);
            cpMsg = cpMsgsNext(vpMsgs);
        }

        printf("\nDisplay the %d logged messages with vUtilPrintMsgs().\n", (int)uiMsgsCount(vpMsgs));
        vUtilPrintMsgs(vpMsgs);

        printf("\nClear the message log and start again.\n");
        vMsgsClear(vpMsgs);
        vMsgsLog(vpMsgs, "bad start with the new app");
        vMsgsLog(vpMsgs, "errors abound");
        vMsgsLog(vpMsgs, "time to quit");
        vUtilPrintMsgs(vpMsgs);

    }else{
        // catch block - display the exception location and message
        vUtilPrintException(&e);
        iReturn = EXIT_FAILURE;
    }

    // clean up resources
    vMsgsDtor(vpMsgs);
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
        return iMsgs();
    default:
        return iHelp();
    }
}

