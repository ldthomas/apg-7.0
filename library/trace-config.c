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
/** \file trace-config.c
 * \brief Parse a configuration file and set the trace configuration.
 */


#include "./apg.h"
#ifdef APG_TRACE

#include <time.h>
#include <errno.h>

#include "../utilities/utilities.h"
#include "./parserp.h"
#include "./tracep.h"

#define BUF_SIZE 512

static char* s_cpTrue = "true";
static char* s_cpFalse = "false";

static aint* uipUintValue(char* cpValue, aint* uipResult);
static abool* bpBoolValue(char* cpValue, abool* bpResult);

/** \brief Sets the default trace configuration on construction.
 *
 */
void vSetDefaultConfig(trace* spTrace) {
    aint ui;
    parser* spParser = spTrace->spParserCtx;
    spTrace->sConfig.uiOutputType = TRACE_ASCII;
    spTrace->sConfig.uiHeaderType = TRACE_HEADER_TRACE;
    spTrace->sConfig.bAllRules = APG_TRUE;
    spTrace->sConfig.bAllOps = APG_TRUE;
    spTrace->sConfig.bCountOnly = APG_FALSE;
    spTrace->sConfig.bPppt = (spTrace->spParserCtx->ucpMaps && PPPT_DEFINED) ? APG_TRUE : APG_FALSE;
    spTrace->sConfig.uiFirstRecord = 0;
    spTrace->sConfig.uiMaxRecords = APG_INFINITE;
    for (ui = 0; ui < spParser->uiRuleCount; ui++) {
        // default all rules
        spTrace->sConfig.bpRules[ui] = APG_TRUE;
    }
    if (spParser->uiUdtCount) {
        for (ui = 0; ui < spParser->uiUdtCount; ui++) {
            // default all UDTs
            spTrace->sConfig.bpUdts[ui] = APG_TRUE;
        }
    }
    for (ui = 0; ui < ID_GEN; ui++) {
        // default all opcodes
        spTrace->sConfig.bpOps[ui] = APG_TRUE;
    }
}

/** \brief Set the trace record display type.
 *
 * Choose between ASCII and HTML output mode.
 * \param vpCtx Pointer to a valid trace object context returned from vpTraceCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param uiType One of \ref TRACE_ASCII or \ref TRACE_HTML.
 */
void vTraceOutputType(void* vpCtx, aint uiType){
    trace* spCtx = (trace*) vpCtx;
    if(!bParserValidate(spCtx->spParserCtx)){
        vExContext();
    }
    char caBuf[128];
    char* cpFormat = "trace output type %"PRIuMAX" not recognized\n"
            "must be TRACE_ASCII or TRACE_HTML";
    spCtx->sConfig.uiOutputType = TRACE_ASCII;
    switch(uiType){
    case TRACE_ASCII:
        spCtx->sConfig.uiOutputType = TRACE_ASCII;
        break;
    case TRACE_HTML:
        spCtx->sConfig.uiOutputType = TRACE_HTML;
        break;
    default:
        snprintf(caBuf, 128, cpFormat, (luint)uiType);
        XTHROW(spCtx->spException, caBuf);
        break;
    }
}

/** \brief Called only by apgex. Sets the display type for apgex tracing.
 *
 */
void vTraceApgexType(void* vpCtx, aint uiType){
    trace* spCtx = (trace*) vpCtx;
    if(!bParserValidate(spCtx->spParserCtx)){
        vExContext();
    }
    char caBuf[128];
    char* cpFormat = "trace header type %"PRIuMAX" not recognized\n"
            "must be TRACE_HEADER_TRACE or TRACE_HEADER_APGEX";
    switch(uiType){
    case TRACE_HEADER_TRACE:
        spCtx->sConfig.uiHeaderType = TRACE_HEADER_TRACE;
        break;
    case TRACE_HEADER_APGEX:
        spCtx->sConfig.uiHeaderType = TRACE_HEADER_APGEX;
        break;
    default:
        snprintf(caBuf, 128, cpFormat, (luint)uiType);
        XTHROW(spCtx->spException, caBuf);
        break;
    }
}

/** \brief Read a configuration file and set the trace configuration accordingly.
 * \param vpCtx Pointer to a valid trace object context returned from vpTraceCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpFileName Name of the configuration file. Must refer to an existing file.
 */
void vTraceConfig(void* vpCtx, const char* cpFileName) {
    trace* spCtx = (trace*) vpCtx;
    if(!bParserValidate(spCtx->spParserCtx)){
        vExContext();
    }
    if(!cpFileName){
        XTHROW(spCtx->spException, "configuration file name cannot be NULL");
    }
    if(cpFileName[0] == 0){
        XTHROW(spCtx->spException, "configuration file name cannot be empty");
    }

    // open the configuration file
    FILE* spIn = NULL;
    spCtx->spOpenFile = fopen(cpFileName, "rb");
    if (!spCtx->spOpenFile) {
        XTHROW(spCtx->spException, "can't open the configuration file for reading");
    }
    spIn = spCtx->spOpenFile;
    spCtx->vpLog = vpMsgsCtor(spCtx->spException);

    // start with the defaults
    vSetDefaultConfig(spCtx);
    trace_config* spConfig = &spCtx->sConfig;
    char* cpDelim1 = " =\t\n\r";
    char* cpDelim2 = " \t\n\r";
    char* cpKey, *cpValue;
    abool bValue, bEqual;
    aint ui, uiKeyLen, uiLineLen, uiValue;
    char caLineBuf[BUF_SIZE];
    char caLineSave[BUF_SIZE];
    char caBuf[(2*BUF_SIZE)];
    while(fgets(caLineBuf, BUF_SIZE, spIn)){
        if(caLineBuf[0] == 0x23){
            continue;
        }
        if((caLineBuf[0] == 0x20) || (caLineBuf[0] == 0x09) || (caLineBuf[0] == 0x0A) || (caLineBuf[0] == 0x0D)){
            continue;
        }

        // save a copy of the line for error reporting
        strcpy(caLineSave, caLineBuf);

        // get the key
        uiLineLen = (aint)strlen(caLineBuf);
        cpKey = strtok(caLineBuf, cpDelim1);
        if(!cpKey){
            vMsgsLog(spCtx->vpLog, "invalid key");
            vMsgsLog(spCtx->vpLog, caLineSave);
            continue;
        }

        // skip the =
        uiKeyLen = (aint)strlen(cpKey);
        bEqual = APG_FALSE;
        for(ui = (uiKeyLen + 1); ui < uiLineLen; ui++){
            if(caLineBuf[ui] == 0x3D){
                bEqual = APG_TRUE;
                continue;
            }
            if(((caLineBuf[ui] == 0x20) || (caLineBuf[ui] == 0x09))){
                continue;
            }
            break;
        }
        if(!bEqual){
            vMsgsLog(spCtx->vpLog, "key/value pair not separated with =");
            vMsgsLog(spCtx->vpLog, caLineSave);
            continue;
        }

        cpValue = strtok(&caLineBuf[ui], cpDelim2);
        if(!cpValue){
            vMsgsLog(spCtx->vpLog, "invalid value");
            vMsgsLog(spCtx->vpLog, caLineSave);
            continue;
        }

        // handle the integer values
        if (strcmp("first-record", cpKey) == 0) {
            if(!uipUintValue(cpValue, &uiValue)){
                vMsgsLog(spCtx->vpLog, "invalid unsigned integer value");
                vMsgsLog(spCtx->vpLog, caLineSave);
                continue;
            }
            spConfig->uiFirstRecord = uiValue;
            continue;
        }
        if (strcmp("max-records", cpKey) == 0) {
            if(!uipUintValue(cpValue, &uiValue)){
                vMsgsLog(spCtx->vpLog, "invalid unsigned integer value");
                vMsgsLog(spCtx->vpLog, caLineSave);
                continue;
            }
            if(uiValue){
                spConfig->uiMaxRecords = uiValue;
            }else{
                spConfig->uiMaxRecords = APG_MAX_AINT;
            }
            continue;
        }

        // handle the boolean values
        if(!bpBoolValue(cpValue, &bValue)){
            vMsgsLog(spCtx->vpLog, "invalid true/false value");
            vMsgsLog(spCtx->vpLog, caLineSave);
            continue;
        }
        if (strcmp("all-rules", cpKey) == 0) {
            spConfig->bAllRules = bValue;
            for(ui = 0; ui < spCtx->spParserCtx->uiRuleCount; ui++){
                spConfig->bpRules[ui] = bValue;
            }
            for(ui = 0; ui < spCtx->spParserCtx->uiUdtCount; ui++){
                spConfig->bpUdts[ui] = bValue;
            }
            continue;
        }
        if (strcmp("all-ops", cpKey) == 0) {
            spConfig->bAllOps = bValue;
            for (ui = 0; ui < ID_GEN; ui++) {
                spConfig->bpOps[ui] = bValue;
            }
            continue;
        }
        if (strcmp("count-only", cpKey) == 0) {
            spConfig->bCountOnly = bValue;
            continue;
        }
        if (strcmp("PPPT", cpKey) == 0) {
            if(bValue){
                spConfig->bPppt = (spCtx->spParserCtx->ucpMaps && PPPT_DEFINED) ? APG_TRUE : APG_FALSE;
            }else{
                spConfig->bPppt = APG_FALSE;
            }
            continue;
        }
        if (strcmp("ALT", cpKey) == 0) {
            spConfig->bpOps[ID_ALT] = bValue;
            continue;
        }
        if (strcmp("CAT", cpKey) == 0) {
            spConfig->bpOps[ID_CAT] = bValue;
            continue;
        }
        if (strcmp("REP", cpKey) == 0) {
            spConfig->bpOps[ID_REP] = bValue;
            continue;
        }
        if (strcmp("TRG", cpKey) == 0) {
            spConfig->bpOps[ID_TRG] = bValue;
            continue;
        }
        if (strcmp("TLS", cpKey) == 0) {
            spConfig->bpOps[ID_TLS] = bValue;
            continue;
        }
        if (strcmp("TBS", cpKey) == 0) {
            spConfig->bpOps[ID_TBS] = bValue;
            continue;
        }
        if (strcmp("BKR", cpKey) == 0) {
            spConfig->bpOps[ID_BKR] = bValue;
            continue;
        }
        if (strcmp("AND", cpKey) == 0) {
            spConfig->bpOps[ID_AND] = bValue;
            continue;
        }
        if (strcmp("NOT", cpKey) == 0) {
            spConfig->bpOps[ID_NOT] = bValue;
            continue;
        }
        if (strcmp("BKA", cpKey) == 0) {
            spConfig->bpOps[ID_BKA] = bValue;
            continue;
        }
        if (strcmp("BKN", cpKey) == 0) {
            spConfig->bpOps[ID_BKN] = bValue;
            continue;
        }
        if (strcmp("ABG", cpKey) == 0) {
            spConfig->bpOps[ID_ABG] = bValue;
            continue;
        }
        if (strcmp("AEN", cpKey) == 0) {
            spConfig->bpOps[ID_AEN] = bValue;
            continue;
        }
        if (strncmp("rule:", cpKey, 5) == 0) {
            rule* spRule = spCtx->spParserCtx->spRules;
            for (ui = 0; ui < spCtx->spParserCtx->uiRuleCount; ui++, spRule++) {
                if (iStriCmp(spRule->cpRuleName, &cpKey[5]) == 0) {
                    spConfig->bpRules[spRule->uiRuleIndex] = bValue;
                    goto foundr;
                }
            }
            snprintf(caBuf, (2*BUF_SIZE), "rule name \"%s\" not recognized", cpKey);
            vMsgsLog(spCtx->vpLog, caBuf);
            vMsgsLog(spCtx->vpLog, caLineSave);
            foundr:;
            continue;
        }
        if (strncmp("UDT:", cpKey, 4) == 0) {
            udt* spUdt = spCtx->spParserCtx->spUdts;
            for (ui = 0; ui < spCtx->spParserCtx->uiUdtCount; ui++, spUdt++) {
                if (iStriCmp(spUdt->cpUdtName, &cpKey[4]) == 0) {
                    spConfig->bpUdts[spUdt->uiUdtIndex] = bValue;
                    goto foundu;
                }
            }
            snprintf(caBuf, (2*BUF_SIZE), "UDT name \"%s\" not recognized", cpKey);
            vMsgsLog(spCtx->vpLog, caBuf);
            vMsgsLog(spCtx->vpLog, caLineSave);
            foundu:;
            continue;
        }
        vMsgsLog(spCtx->vpLog, "key/value pair not recognized");
        vMsgsLog(spCtx->vpLog, caLineSave);
    }

    // close the file
    if (spIn) {
        fclose(spIn);
        spCtx->spOpenFile = NULL;
    }

    // display error messages if any
    if(uiMsgsCount(spCtx->vpLog)){
        vUtilPrintMsgs(spCtx->vpLog);
        vMsgsDtor(spCtx->vpLog);
        spCtx->vpLog = NULL;
        XTHROW(spCtx->spException, "errors in trace configuration file");
    }
    vMsgsDtor(spCtx->vpLog);
    spCtx->vpLog = NULL;
}
/** \brief Display the trace object's current configuration.
 *
* \param vpCtx Pointer to a valid trace object context returned from vpTraceCtor().
* If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
* \param cpFileName Name of the file to write the configuration information to.
* If NULL, displays to stdout.
 */
void vTraceConfigDisplay(void* vpCtx, const char* cpFileName) {
    trace* spCtx = (trace*) vpCtx;
    if(!bParserValidate(spCtx->spParserCtx)){
        vExContext();
    }
    FILE* spOut = stdout;
    trace_config* spCfg = &spCtx->sConfig;
    aint ui;
    if (cpFileName) {
        spOut = fopen(cpFileName, "wb");
        if (!spOut) {
            XTHROW(spCtx->spException, "can't open display file for writing");
        }
    }
    fprintf(spOut, "TRACE CONFIGURATION\n");
    fprintf(spOut, "  %-15s: %s\n", "all-rules", (spCfg->bAllRules ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "all-ops", (spCfg->bAllOps ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "count-only", (spCfg->bCountOnly ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "PPPT display", (spCfg->bPppt ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %"PRIuMAX"\n", "first-record", (luint) spCfg->uiFirstRecord);
    fprintf(spOut, "  %-15s: %"PRIuMAX"\n", "max-records", (luint) spCfg->uiMaxRecords);
    fprintf(spOut, "OPCODES\n");
    fprintf(spOut, "  %-15s: %s\n", "ALT", (spCfg->bpOps[ID_ALT] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "CAT", (spCfg->bpOps[ID_CAT] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "REP", (spCfg->bpOps[ID_REP] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "TRG", (spCfg->bpOps[ID_TRG] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "TBS", (spCfg->bpOps[ID_TBS] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "TLS", (spCfg->bpOps[ID_TLS] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "BKR", (spCfg->bpOps[ID_BKR] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "AND", (spCfg->bpOps[ID_AND] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "NOT", (spCfg->bpOps[ID_NOT] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "BKA", (spCfg->bpOps[ID_BKA] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "BKN", (spCfg->bpOps[ID_BKN] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "ABG", (spCfg->bpOps[ID_ABG] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "  %-15s: %s\n", "AEN", (spCfg->bpOps[ID_AEN] ? s_cpTrue : s_cpFalse));
    fprintf(spOut, "RULES\n");
    rule* spRule = spCtx->spParserCtx->spRules;
    for (ui = 0; ui < spCtx->spParserCtx->uiRuleCount; ui++, spRule++) {
        fprintf(spOut, "  %-15s: %s\n", spRule->cpRuleName,
                (spCfg->bpRules[spRule->uiRuleIndex] ? s_cpTrue : s_cpFalse));
    }
    if (spCtx->spParserCtx->uiUdtCount) {
        fprintf(spOut, "UDTS\n");
        udt* spUdt = spCtx->spParserCtx->spUdts;
        for (ui = 0; ui < spCtx->spParserCtx->uiUdtCount; ui++, spUdt++) {
            fprintf(spOut, "  %-15s: %s\n", spUdt->cpUdtName,
                    (spCfg->bpUdts[spUdt->uiUdtIndex] ? s_cpTrue : s_cpFalse));
        }
    }
    if (spOut != stdout) {
        fclose(spOut);
    }
}

/** \brief Generate a configuration file for the current parser.
 *
 * The configuration requires specific knowledge of the rule and UDT names in the SABNF grammar
 * used to define the parser. The generated file will have
 * commented lines for all possible configuration settings.
 * This will serve as a starting point with all options presented for the user's choices.
* \param vpCtx Pointer to a valid trace object context returned from vpTraceCtor().
* If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
* \param cpFileName Name of the file to write the generated configuration to.
* If NULL, displays to stdout.
 */
void vTraceConfigGen(void *vpCtx, const char *cpFileName) {
    trace *spCtx = (trace*) vpCtx;
    if (!bParserValidate(spCtx->spParserCtx)) {
        vExContext();
    }
    FILE *spOut = NULL;
    aint ui;
    spCtx->spOpenFile = stdout;
    while (APG_TRUE) {
        if (cpFileName) {
            spCtx->spOpenFile = fopen(cpFileName, "wb");
            if (!spCtx->spOpenFile) {
                XTHROW(spCtx->spException, "can't open configuration file for writing");
                break;
            }
        }
        spOut = spCtx->spOpenFile;
        time_t tTime = time(NULL);
        fprintf(spOut, "# TRACE CONFIGURATION\n");
        fprintf(spOut, "# Generated by: %s\n", __func__);
        fprintf(spOut, "# %s", asctime(gmtime(&tTime)));
        fprintf(spOut, "#\n");
        fprintf(spOut, "# NOTE 1) All keys and values are case sensitive including the \"rule:\" and \"UDT:\" prefixes.\n");
        fprintf(spOut, "#         However, rule and UDT names are case insensitive\n");
        fprintf(spOut, "# NOTE 2) true may be represented by true or t or 1\n");
        fprintf(spOut, "#         false may be represented by false or f or 0\n");
        fprintf(spOut, "# NOTE 3) Lines beginning with # (0x23) or white space (0x09, 0x0A, 0x0D or 0x20) are ignored.\n");
        fprintf(spOut, "# NOTE 4) Missing keys assume the listed default values.\n");
        fprintf(spOut, "# NOTE 5) Unrecognized keys and values will result in error messages and a thrown exception.\n");
        fprintf(spOut, "#\n");
        fprintf(spOut, "# Sets all rule and UDT names to value. Default = true\n");
        fprintf(spOut, "all-rules = true\n");
        fprintf(spOut, "#\n");
        fprintf(spOut, "# Sets all opcodes to value. Default = true\n");
        fprintf(spOut, "all-ops = true\n");
        fprintf(spOut, "#\n");
        fprintf(spOut, "# If \"PPPT\" is true the Partially-Predictive Parsing Table (PPPT) form of output will be used.\n");
        fprintf(spOut, "# The PPPT form indicates when a predictive table value was used in place of an opcode.\n");
        fprintf(spOut, "# If no PPPT data is available \"PPPT\" is automatically set to false.\n");
        fprintf(spOut, "# \"PPPT\" defaults to true if PPPT data is available, false otherwise.\n");
        fprintf(spOut, "PPPT = true\n");
        fprintf(spOut, "#\n");
        fprintf(spOut, "# If \"count-only\" is true, only a count of the total number of records is displayed.\n");
        fprintf(spOut, "# The printing of individual records is suppressed.\n");
        fprintf(spOut, "# Handy for a first run on large grammars or input strings.\n");
        fprintf(spOut, "# It can help in setting the \"first-record\" and \"max-records\" parameters.\n");
        fprintf(spOut, "# Default = false\n");
        fprintf(spOut, "count-only = false\n");
        fprintf(spOut, "#\n");
        fprintf(spOut, "# \"first-record\" sets the record number of the first record to display.\n");
        fprintf(spOut, "# Records prior are not displayed. Default = 0.\n");
        fprintf(spOut, "first-record = 0\n");
        fprintf(spOut, "#\n");
        fprintf(spOut, "# \"max-records\" sets the maximum number of records to display.\n");
        fprintf(spOut, "# If 0, the maximum number of records is set to APG_MAX_AINT. Default = 0.\n");
        fprintf(spOut, "max-records = 0\n");
        fprintf(spOut, "#\n");
        fprintf(spOut, "# Set the opcodes to be displayed individually.\n");
        fprintf(spOut, "# They all default to the \"all-ops\" value.\n");
        fprintf(spOut, "# Un-comment and set the value if different from \"all-ops\".\n");
        fprintf(spOut, "# Note that depending on the SABNF grammar and input string,\n");
        fprintf(spOut, "# not all of these operators may generate trace records.\n");
        fprintf(spOut, "#ALT = true\n");
        fprintf(spOut, "#CAT = true\n");
        fprintf(spOut, "#REP = true\n");
        fprintf(spOut, "#TRG = true\n");
        fprintf(spOut, "#TBS = true\n");
        fprintf(spOut, "#TLS = true\n");
        fprintf(spOut, "#BKR = true\n");
        fprintf(spOut, "#AND = true\n");
        fprintf(spOut, "#NOT = true\n");
        fprintf(spOut, "#BKA = true\n");
        fprintf(spOut, "#BKN = true\n");
        fprintf(spOut, "#ABG = true\n");
        fprintf(spOut, "#AEN = true\n");
        fprintf(spOut, "#\n");
        fprintf(spOut, "# Set the rule & UDT names to be displayed individually.\n");
        fprintf(spOut, "# They all default to the \"all-rules\" value.\n");
        fprintf(spOut, "# Un-comment and set the value if different from \"all-rules\".\n");
        fprintf(spOut, "# Note that rule names must begin with \"rule:\" with no trailing spaces.\n");
        fprintf(spOut, "# and UDT names must begin with \"UDT:\" with no trailing spaces.\n");
        rule *spRule = spCtx->spParserCtx->spRules;
        for (ui = 0; ui < spCtx->spParserCtx->uiRuleCount; ui++, spRule++) {
            fprintf(spOut, "#rule:%s = true\n", spRule->cpRuleName);
        }
        if (spCtx->spParserCtx->uiUdtCount) {
            fprintf(spOut, "#\n");
            udt *spUdt = spCtx->spParserCtx->spUdts;
            for (ui = 0; ui < spCtx->spParserCtx->uiUdtCount; ui++, spUdt++) {
                fprintf(spOut, "#UDT: %s = true\n", spUdt->cpUdtName);
            }
        }
        break;
    }
    if (spOut != stdout) {
        fclose(spOut);
        spCtx->spOpenFile = NULL;
    }
}

static abool* bpBoolValue(char* cpValue, abool* bpResult){
    abool* bpReturn = NULL;
    if(cpValue[0] == 116 || cpValue[0] == 49){
        *bpResult = APG_TRUE;
        bpReturn = bpResult;
    }else if(cpValue[0] == 102 || cpValue[0] == 48){
        *bpResult = APG_FALSE;
        bpReturn = bpResult;
    }
    return bpReturn;
}
static aint* uipUintValue(char* cpValue, aint* uipResult){
    char* cpTailPtr;
    errno = 0;
    long int iLong = strtol(cpValue, &cpTailPtr, 10);
    if(cpTailPtr == cpValue || errno != 0){
        return NULL;
    }
    if(iLong < 0){
        return NULL;
    }
    if(iLong > APG_MAX_AINT){
        return NULL;
    }
    *uipResult = (aint)iLong;
    return uipResult;
}

#endif /* APG_TRACE */
