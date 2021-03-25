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
/** \dir ./apg
 * \brief The desktop APG application..
 */

/** \file main.c
 * \brief APG - the SABNF Parser Generator
 *
 * This main function is the SABNF Parser Generator - APG.
 * It reads controls from the command line and then makes function calls
 * to the APG Application Programming Interface (API) to:
 * - validate the grammar's character set
 * - validate the grammar's syntax
 * - validate the grammar's semantics
 * - validate the grammar's attributes
 * - generate a pair of grammar files from which the parser can be constructed
 */
#include "../api/api.h"
#include "../api/apip.h"
#include "../api/attributes.h"
#include "../api/semantics.h"
#include "./config.h"
#include "../library/parserp.h"

/** \brief The executable from this main function is the
 *  <strong>A</strong>BNF <strong>P</strong>arser <strong>G</strong>enerator application, APG.
 *
 * Execute this application with the option --help for an explanation of all options.
 *
 * \param argc - the number of command line parameters
 * \param argv - pointer to an array of command line parameters (strings)
 */
int main(int argc, char **argv) {
    static void* vpApi = NULL;
    static void* vpCfg = NULL;
    static void* vpMem = NULL;
    const config* spConfig;
    aint ui;
    const char* cpGrammar;
    exception e;

    XCTOR(e);
    if(!e.try){
        // catch block
        printf("APG CATCH BLOCK\n");
        if(vpMem){
            // display the exception info
            printf("EXCEPTION INFORMATION\n");
            vUtilPrintException(&e);
            if(vpApi){
                void* vpLog = vpApiGetErrorLog(vpApi);
                vUtilPrintMsgs(vpLog);
            }
        }else{
            printf("memory allocation error\n");
        }

        // clean up all memory, if any
        vApiDtor(vpApi);
        vMemDtor(vpMem);
        return EXIT_FAILURE;
    }

    vpMem = vpMemCtor(&e);
    while (APG_TRUE) {
        // constructors (note: in release build these will be reset to NULL by setjmp() in catch block)
        vpCfg = vpConfigCtor(&e);
        vpApi = vpApiCtor(&e);

        // get the configuration and go
        spConfig = spConfigOptions(vpCfg, argc, argv);
        if (spConfig->bDc) {
            vConfigDisplay(spConfig, argc, argv);
            printf("\n");
        }

        if (spConfig->bHelp) {
            vConfigHelp();
            break;
        }
        if (spConfig->bVersion) {
            vConfigVersion();
            break;
        }
        if (spConfig->cpDefaultConfig) {
            vConfigDefault(vpCfg, spConfig->cpDefaultConfig);
            break;
        }

        // get the grammar file
        if (!spConfig->uiInputFiles) {
            XTHROW(&e, "no input file specified, use --input=filename");
        }
        for (ui = 0; ui < spConfig->uiInputFiles; ui += 1) {
            cpGrammar = cpApiInFile(vpApi, spConfig->cppInput[ui]);
        }
        if (spConfig->bDg) {
            vApiInToAscii(vpApi, NULL);
        }
        if (spConfig->cpGrammarHtml) {
            vApiInToHtml(vpApi, spConfig->cpGrammarHtml, NULL);
            printf("HTML formatted grammar written to \"%s\"\n", spConfig->cpGrammarHtml);
            printf("\n");
        }
        if (spConfig->cpLfOut || spConfig->cpCrLfOut) {
            if (spConfig->cpLfOut) {
                vUtilConvertLineEnds(&e, cpGrammar, "\n", spConfig->cpLfOut);
                printf("line ends of input converted to LF at: %s\n", spConfig->cpLfOut);
            }
            if (spConfig->cpCrLfOut) {
                vUtilConvertLineEnds(&e, cpGrammar, "\r\n", spConfig->cpCrLfOut);
                printf("line ends of input converted to CRLF at: %s\n", spConfig->cpCrLfOut);
            }
            break;
        }

        // API - validate the grammar - validation phase
        vApiInValidate(vpApi, spConfig->bStrict);

        // API - parse the grammar - syntax phase
        vApiSyntax(vpApi, spConfig->bStrict);

        // API - traverse the AST - semantic phase
        vApiOpcodes(vpApi);

        if (spConfig->bDr) {
            vApiRulesToAscii(vpApi, "index", NULL);
        }
        if (spConfig->bDra) {
            vApiRulesToAscii(vpApi, "alpha", NULL);
        }
        if (spConfig->bDp) {
            pppt_size sSize;
            vApiPpptSize(vpApi, &sSize);
            printf("PPPT SIZES\n");
            printf("  alphabet min char: %"PRIuMAX"\n", sSize.luiAcharMin);
            printf("  alphabet max char: %"PRIuMAX"\n", sSize.luiAcharMax);
            printf("number of PPPT maps: %"PRIuMAX"\n", sSize.luiMaps);
            printf("      PPPT map size: %"PRIuMAX"\n", sSize.luiMapSize);
            if(sSize.luiTableSize == APG_MAX_AINT){
                printf("    PPPT total size: OVERFLOW\n");
            }else{
                printf("    PPPT total size: %"PRIuMAX"\n", sSize.luiTableSize);
            }
            printf("\n");
        }
        if (spConfig->bDo) {
            vApiOpcodesToAscii(vpApi, NULL);
        }

        // API - get the attributes - attribute phase
        if(bApiAttrs(vpApi)){
            if (spConfig->bDa) {
                vApiAttrsToAscii(vpApi, "type", NULL);
            }
        }else{
            printf("ATTRIBUTE ERRORS DETECTED\n");
            vApiAttrsErrorsToAscii(vpApi, "type", NULL);
            if (spConfig->bDa) {
                printf("\n");
                vApiAttrsToAscii(vpApi, "type", NULL);
            }
            break;
        }

        // PPPT
        if(!spConfig->bNoPppt){
            vApiPppt(vpApi, spConfig->cppPRules, spConfig->uiPRules);
        }

        // output the parser files
        if (spConfig->cpOutput) {
            vApiOutput(vpApi, spConfig->cpOutput);
            printf("generated parser output to: %s\n", spConfig->cpOutput);
        }
        break;
    }

    // clean up API exception block
    vApiDtor(vpApi);
    vConfigDtor(vpCfg);
    vMemDtor(vpMem);
    return EXIT_SUCCESS;
}

