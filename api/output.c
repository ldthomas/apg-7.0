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
/** \file output.c
 * \brief Constructs the source and header files for the generated parser.
 *
 */

#include "../api/api.h"
#include "../api/apip.h"
#include "../api/attributes.h"
#include "../library/parserp.h"

/** \struct init_hdr_out
 * \brief Header for the parser initialization data.
 *
 * This header section of the parser's initialization data defines some of the parser's basic data sizes, types and limits.
 * The remainder defines the offsets (from the beginning of the parser initialization data) to various
 * other data segments.
 *
 * CAVEAT: This structure must match exactly \ref init_hdr in the library file \ref parserp.h.
 *
 */
typedef struct{
    luint luiSizeInInts; /**< The number of integers in the initialization data, including this header */
    luint luiAcharMin; /**< The minimum value of all of the alphabet characters (achar) present in the grammar. */
    luint luiAcharMax; /**< The maximum value of all of the alphabet characters (achar) present in the grammar. */
    luint luiSizeofAchar; /**< The minimum size, in bytes, required to hold all alphabet characters in the grammar. */
    luint luiUintMax; /**< The maximum value of all of the integers present in this initialization data. */
    luint luiSizeofUint; /**< The minimum size, in bytes, required to hold all of the integers in the initialization data. */
    luint luiRuleCount; /**< The number of rules in the grammar. */
    luint luiUdtCount; /**< The number of UDTs in the grammar. */
    luint luiOpcodeCount; /**< The number of opcodes in the grammar. */
    luint luiMapCount; /**< The number rule, UDT, and opcode PPPT maps. */
    luint luiMapSize; /**< The number of bytes in one PPPT map. */
    luint luiVersionOffset; /**< Offset from the beginning of the string table to the null-terminated version number string. */
    luint luiCopyrightOffset; /**< Offset from the beginning of the string table to the null-terminated version number string. */
    luint luiLicenseOffset; /**< Offset from the beginning of the string table to the null-terminated version number string. */
    luint luiChildListOffset; /**<  Offset to the child index list. Each ALT and CAT has a list of children. This list has the opcode indexes of these chilren.*/
    luint luiChildListLength; /**<  The number of indexes in the list. */
    luint luiRulesOffset; /**<  Offset to the list of rule structures. */
    luint luiRulesLength; /**<  The length in integers of the rules list. */
    luint luiUdtsOffset; /**<  Offset to the list of UDT structures. */
    luint luiUdtsLength; /**<  The length in integers of the UDT list. */
    luint luiOpcodesOffset; /**<  Offset to the list of opcode structures. */
    luint luiOpcodesLength; /**<  The length in integers of the opcode list. */
} init_hdr_out;

/** \def OUTPUT_LINE_LENGTH
 * \brief Controls the number of integers per line in the output source file.
 */
#define OUTPUT_LINE_LENGTH 30

/** \def LUINT_MAX(x,y) if((x) < (y))(x) = (y)
 * \brief Replace x with y only if y > x.
 */
#define LUINT_MAX(x,y) if(((x) < (y)) && ((y) != (luint)-1))x = (y)

static const char* s_cpUchar = "uint8_t";
static const char* s_cpUshort = "uint16_t";
static const char* s_cpUint = "uint32_t";
static const char* s_cpUlong = "uint64_t";
static char* s_cpLicenseNotice =
        "/*  *************************************************************************************\n"
        "    Copyright (c) 2021, Lowell D. Thomas\n"
        "    All rights reserved.\n"
        "\n"
        "    This file was generated by and is part of APG Version 7.0.\n"
        "    APG Version 7.0 may be used under the terms of the BSD 2-Clause License.\n"
        "\n"
        "    Redistribution and use in source and binary forms, with or without\n"
        "    modification, are permitted provided that the following conditions are met:\n"
        "\n"
        "    1. Redistributions of source code must retain the above copyright notice, this\n"
        "       list of conditions and the following disclaimer.\n"
        "\n"
        "    2. Redistributions in binary form must reproduce the above copyright notice,\n"
        "       this list of conditions and the following disclaimer in the documentation\n"
        "       and/or other materials provided with the distribution.\n"
        "\n"
        "    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
        "    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
        "    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"
        "    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n"
        "    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
        "    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n"
        "    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n"
        "    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n"
        "    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
        "    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
        "\n"
        "*   *************************************************************************************/\n\n";

static aint uiLastChar(char cCharToFind, const char* cpString);
static abool bGetFileName(const char* cpPathName, char* cpBuffer, aint uiBufferLength);
static abool bSetFileExtension(const char* cpPathName, const char* cpExt, char* cpBuffer, aint uiBufferLength);
static abool bNameToCaps(const char* cpPrefix, const char* cpName, char* cpBuffer, aint uiBufferLength);
static abool bNameToCamelCase(const char* cpPrefix, const char* cpName, char* cpBuffer, aint uiBufferLength);
static void vOutputHeader(api* spApi, const char* cpName, FILE* spOut);
static void vOutputSource(api* spApi, const char* cpName, FILE* spOut);
static void* vpOutputParser(api* spApi);
static int iCompRule(const void* vpL, const void* vpR);
static int iCompUdt(const void* vpL, const void* vpR);
static char cToUpper(char cChar);
static char cToLower(char cChar);
static abool bIsLetter(char cLetter);
static void vPrintChars(FILE* spOut, const uint8_t* ucpChars, aint uiLength);
static void vPrintLongs(FILE* spOut, const luint* luiVals, aint uiLength);
static aint uiGetSize(luint luiValue);
static const char* cpGetType(luint luiVal);
static void* vpMakeParserInit(api* spApi, luint luiUintMax, luint* luipData, aint uiLen);
static void* vpMakeAcharTable(api* spApi, luint luiAcharMax, luint* luipTable, aint uiLen);
static luint* luipMakeInitData(api* spApi);

#ifdef TEST_NAMES
static void vTestNames();
#endif /* TEST_NAMES */

/** \brief Generate a source and header file that can be used to construct a parser for the specified SABNF grammar.
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param cpOutput The root of the file name to use for the generated source and header files.
 * Any extension will be stripped and replaces with .h for the header file and .c for the source file.
 * The name may be relative or absolute. Any directories in the pathname must exist.
 * \param bIgnoreAttributes If true, files will be generated without regard to whether attributes have been generated or not.
 * Allows the caller to side-step or ignore attributes. NOT RECOMMENDED.
 * \return Throws exceptions on errors.
 */
void vApiOutput(void* vpCtx, const char* cpOutput, abool bIgnoreAttributes) {
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spApi = (api*) vpCtx;
    FILE* spHeader = NULL;
    FILE* spSource = NULL;
    char caName[PATH_MAX];
    char caFileName[PATH_MAX];
    size_t uiErrorSize = PATH_MAX + 128;
    char caErrorBuf[uiErrorSize];
    vMsgsClear(spApi->vpLog);
    // validate that we have a valid and fully processed grammar
    if (!spApi->bInputValid) {
        XTHROW(spApi->spException, "attempted output but input grammar not validated");
    }
    if (!spApi->bSyntaxValid) {
        XTHROW(spApi->spException, "attempted output but syntax not validated");
    }
    if (!spApi->bSemanticsValid) {
        XTHROW(spApi->spException,
                "attempted output but opcodes have not been constructed and validated");
    }
    if(!bIgnoreAttributes){
        if (!spApi->bAttributesValid) {
            XTHROW(spApi->spException,
                    "attempted output but attributes have not been constructed and validated");
        }
    }

    // generate the header file
    if(!bGetFileName(cpOutput, caName, PATH_MAX)){
        snprintf(caErrorBuf, uiErrorSize, "unable to extract file name from output parameter: '%s'", cpOutput);
        XTHROW(spApi->spException, caErrorBuf);
    }

    // make sure the file names are good
    if(!bSetFileExtension(cpOutput, "h", caFileName, PATH_MAX)){
        snprintf(caErrorBuf, uiErrorSize, "unable to set file extension on output parameter: '%s'", cpOutput);
        XTHROW(spApi->spException, caErrorBuf);
    }
    spHeader = fopen(caFileName, "wb");
    if (!spHeader) {
        snprintf(caErrorBuf, uiErrorSize, "unable to open header file \"%s\"", caFileName);
        XTHROW(spApi->spException, caErrorBuf);
    }
    vOutputHeader(spApi, caName, spHeader);
    fclose(spHeader);

    if(!bSetFileExtension(cpOutput, "c", caFileName, PATH_MAX)){
        snprintf(caErrorBuf, uiErrorSize, "unable to set file extension on output parameter: '%s'", cpOutput);
        XTHROW(spApi->spException, caErrorBuf);
    }
    spSource = fopen(caFileName, "wb");
    if (!spSource) {
        snprintf(caErrorBuf, uiErrorSize, "unable to open source file \"%s\"", caFileName);
        XTHROW(spApi->spException, caErrorBuf);
    }
    vOutputSource(spApi, caName, spSource);
    fclose(spSource);
}

/** \brief Generate a parser object directly from the specified SABNF grammar.
 *
 * The generated parser has its own memory object and is independent of the parent API object
 * other than sharing the same exception object. That is, throw exceptions will land in the API catch block.
 * But the API and the generated parser must each call their respective destructors to prevent memory leaks.
 *
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param bIgnoreAttributes If true, files will be generated without regard to whether attributes have been generated or not.
 * Allows the caller to side-step or ignore attributes. NOT RECOMMENDED.
 * \return Throws exceptions on errors.
 */
void* vpApiOutputParser(void* vpCtx, abool bIgnoreAttributes) {
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spApi = (api*) vpCtx;
    vMsgsClear(spApi->vpLog);
    // validate that we have a valid and fully processed grammar
    if (!spApi->bInputValid) {
        XTHROW(spApi->spException, "attempted output but input grammar not validated");
    }
    if (!spApi->bSyntaxValid) {
        XTHROW(spApi->spException, "attempted output but syntax not validated");
    }
    if (!spApi->bSemanticsValid) {
        XTHROW(spApi->spException, "attempted output but opcodes have not been constructed and validated");
    }
    if(!bIgnoreAttributes){
        if (!spApi->bAttributesValid) {
            XTHROW(spApi->spException,
                    "attempted output but attributes have not been constructed and validated");
        }
    }

    // generate the parser
    return vpOutputParser(spApi);
}

static void vOutputHeader(api* spApi, const char* cpName, FILE* spOut){
    aint uiBufSize = PATH_MAX;
    char caDefine[PATH_MAX];
    char caWork[PATH_MAX];
    char caRuleCount[PATH_MAX];
    char caUdtCount[PATH_MAX];
    api_rule saRules[spApi->uiRuleCount];
    api_udt saUdts[spApi->uiUdtCount];
    aint ui;
    // version, copyright, etc.
    fprintf(spOut, "//\n");
    fprintf(spOut, "// This C-language parser header was generated by APG Version 7.0.\n");
    fprintf(spOut, "// User modifications invalidate the license agreement and may cause unpredictable results.\n");
    fprintf(spOut, "//\n");
    fprintf(spOut, "%s",s_cpLicenseNotice);

    bNameToCaps("", cpName, caWork, uiBufSize);
    caDefine[0] = '_';
    caDefine[1] = 0;
    strcat(caDefine, caWork);
    strcat(caDefine, "_H_");

    // output define guards
    fprintf(spOut, "#ifndef %s\n", caDefine);
    fprintf(spOut, "#define %s\n", caDefine);

    // alphabetize the rule names
    for(ui = 0; ui < spApi->uiRuleCount; ui++){
        saRules[ui] = spApi->spRules[ui];
    }
    qsort((void*)&saRules[0], spApi->uiRuleCount, sizeof(api_rule), iCompRule);

    // output the rule name ids
    fprintf(spOut, "\n");
    fprintf(spOut, "// rule ids\n");
    for(ui = 0; ui < spApi->uiRuleCount; ui++){
        bNameToCaps(cpName, saRules[ui].cpName, caWork, uiBufSize);
        fprintf(spOut, "#define %s %"PRIuMAX"\n", caWork, (luint)saRules[ui].uiIndex);
    }
    bNameToCaps("RULE_COUNT", cpName, caRuleCount, uiBufSize);
    fprintf(spOut, "#define %s %"PRIuMAX"\n", caRuleCount, (luint)spApi->uiRuleCount);

    if(spApi->uiUdtCount){
        // alphabetize the UDT names
        for(ui = 0; ui < spApi->uiUdtCount; ui++){
            saUdts[ui] = spApi->spUdts[ui];
        }
        qsort((void*)&saUdts[0], spApi->uiUdtCount, sizeof(api_udt), iCompUdt);

        // output the UDT ids
        fprintf(spOut, "\n");
        fprintf(spOut, "// UDT ids\n");
        for(ui = 0; ui < spApi->uiUdtCount; ui++){
            bNameToCaps(cpName, saUdts[ui].cpName, caWork, uiBufSize);
            fprintf(spOut, "#define %s %"PRIuMAX"\n", caWork, (luint)saUdts[ui].uiIndex);
        }
        bNameToCaps("UDT_COUNT", cpName, caUdtCount, uiBufSize);
        fprintf(spOut, "#define %s %"PRIuMAX"\n", caUdtCount, (luint)spApi->uiUdtCount);
    }

    // the init pointer name
    bNameToCamelCase("vp", cpName, caWork, uiBufSize);
    strcat(caWork, "Init");
    fprintf(spOut, "\n");
    fprintf(spOut, "// pointer to parser initialization data\n");
    fprintf(spOut, "extern void* %s;\n", caWork);

    // comment for callback helper functions
    fprintf(spOut, "\n");
    fprintf(spOut, "// Helper function(s) for setting rule/UDT name callbacks.\n");
    fprintf(spOut, "// Un-comment and replace named NULL with pointer to the appropriate callback function.\n");
    fprintf(spOut, "//  NOTE: This can easily be modified for setting AST callback functions:\n");
    fprintf(spOut, "//        Replace parser_callback with ast_callback and\n");
    fprintf(spOut, "//        vParserSetRuleCallback(vpParserCtx) with vAstSetRuleCallback(vpAstCtx) and\n");
    fprintf(spOut, "//        vParserSetUdtCallback(vpParserCtx) with vAstSetUdtCallback(vpAstCtx).\n");
    fprintf(spOut, "/****************************************************************\n");
    bNameToCamelCase("v", cpName, caWork, uiBufSize);
    strcat(caWork, "RuleCallbacks");
    fprintf(spOut, "void %s(void* vpParserCtx){\n", caWork);
    fprintf(spOut, "    aint ui;\n");
    fprintf(spOut, "    parser_callback cb[%s];\n", caRuleCount);
    for(ui = 0; ui < spApi->uiRuleCount; ui++){
        bNameToCaps(cpName, saRules[ui].cpName, caWork, uiBufSize);
        fprintf(spOut, "    cb[%s] = NULL;\n", caWork);
    }
    fprintf(spOut, "    for(ui = 0; ui < (aint)%s; ui++){\n", caRuleCount);
    fprintf(spOut, "        vParserSetRuleCallback(vpParserCtx, ui, cb[ui]);\n");
    fprintf(spOut, "    }\n");
    fprintf(spOut, "}\n");
    if(spApi->uiUdtCount){
        bNameToCamelCase("v", cpName, caWork, uiBufSize);
        strcat(caWork, "UdtCallbacks");
        fprintf(spOut, "void %s(void* vpParserCtx){\n", caWork);
        fprintf(spOut, "    aint ui;\n");
        fprintf(spOut, "    parser_callback cb[%s];\n", caUdtCount);
        for(ui = 0; ui < spApi->uiUdtCount; ui++){
            bNameToCaps(cpName, saUdts[ui].cpName, caWork, uiBufSize);
            fprintf(spOut, "    cb[%s] = NULL;\n", caWork);
        }
        fprintf(spOut, "    for(ui = 0; ui < (aint)%s; ui++){\n", caUdtCount);
        fprintf(spOut, "        vParserSetUdtCallback(vpParserCtx, ui, cb[ui]);\n");
        fprintf(spOut, "    }\n");
        fprintf(spOut, "}\n");
    }
    fprintf(spOut, "****************************************************************/\n");

    // output end of define guards
    fprintf(spOut, "\n");
    fprintf(spOut, "#endif /* %s */\n", caDefine);
}
static void vOutputSource(api* spApi, const char* cpName, FILE* spOut){
    luint* luipInit = NULL;
    char* cpLineBuffer = NULL;
    line* spLine;
    aint ui;
    char caBuf[PATH_MAX];
    init_hdr_out* spHdr;
    aint uiaOpCounts[ID_GEN];
    api_op* spOp = spApi->spOpcodes;

    if(spApi->luipInit){
        vMemFree(spApi->vpMem, spApi->luipInit);
    }
    spApi->luipInit = luipMakeInitData(spApi);
    luipInit = spApi->luipInit;
    spHdr = (init_hdr_out*)luipInit;
    memset((void*)&uiaOpCounts[0], 0, sizeof(uiaOpCounts));
    for(ui = 0; ui < spApi->uiOpcodeCount; ui++, spOp++){
        uiaOpCounts[spOp->uiId]++;
    }

    // version, copyright, etc.
    fprintf(spOut, "//\n");
    fprintf(spOut, "// This C-language parser code was generated by APG Version 7.0.\n");
    fprintf(spOut, "// User modifications invalidate the license agreement and may cause unpredictable results.\n");
    fprintf(spOut, "//\n");
    fprintf(spOut, "%s",s_cpLicenseNotice);
    fprintf(spOut, "#include <stdint.h>\n");
    fprintf(spOut, "\n");

    // the string table
    fprintf(spOut, "static const char caStringTable[%"PRIuMAX"] = {\n", (luint)spApi->uiStringTableLength);
    vPrintChars(spOut, (uint8_t*)spApi->cpStringTable, spApi->uiStringTableLength);
    fprintf(spOut, "};\n");
    fprintf(spOut, "\n");

    // the PPPT maps
    if(spApi->bUsePppt){
        fprintf(spOut, "static const uint8_t ucaPpptTable[%"PRIuMAX"] = {\n", spApi->luiPpptTableLength);
        vPrintChars(spOut,  spApi->ucpPpptTable, spApi->luiPpptTableLength);
        fprintf(spOut, "};\n");
        fprintf(spOut, "\n");
    }

    // the achar table
    if(spApi->uiAcharTableLength){
        fprintf(spOut, "static const %s aAcharTable[%"PRIuMAX"] = {\n",
                cpGetType(spApi->luiAcharMax), (luint)spApi->uiAcharTableLength);
        vPrintLongs(spOut, spApi->luipAcharTable, spApi->uiAcharTableLength);
        fprintf(spOut, "};\n");
        fprintf(spOut, "\n");
    }


    // output the parser initialization data
    fprintf(spOut, "static const %s aParserInit[%"PRIuMAX"] = {\n", cpGetType(spHdr->luiUintMax), spHdr->luiSizeInInts);
    vPrintLongs(spOut, luipInit, (aint)spHdr->luiSizeInInts);
    fprintf(spOut, "};\n");
    fprintf(spOut, "\n");

    // output the parser initialization struct
    fprintf(spOut, "static struct {\n");
    fprintf(spOut, "    uint32_t uiSizeofAchar;\n");
    fprintf(spOut, "    uint32_t uiSizeofUint;\n");
    fprintf(spOut, "    uint32_t uiStringTableLength;\n");
    fprintf(spOut, "    uint32_t uiAcharTableLength;\n");
    fprintf(spOut, "    uint32_t uiPpptTableLength;\n");
    fprintf(spOut, "    uint32_t uiParserInitLength;\n");
    fprintf(spOut, "    const char* cpStringTable;\n");
    fprintf(spOut, "    const uint8_t* ucpPpptTable;\n");
    fprintf(spOut, "    const void* vpAcharTable;\n");
    fprintf(spOut, "    const void* vpParserInit;\n");
    fprintf(spOut, "} s_parser_init = {\n");
    fprintf(spOut, "    %"PRIuMAX",\n", spHdr->luiSizeofAchar);
    fprintf(spOut, "    %"PRIuMAX",\n", spHdr->luiSizeofUint);
    fprintf(spOut, "    %"PRIuMAX",\n", (luint)spApi->uiStringTableLength);
    fprintf(spOut, "    %"PRIuMAX",\n", (luint)spApi->uiAcharTableLength);
    fprintf(spOut, "    %"PRIuMAX",\n", spApi->luiPpptTableLength);
    fprintf(spOut, "    %"PRIuMAX",\n", (luint)spHdr->luiSizeInInts);
    fprintf(spOut, "    caStringTable,\n");
    if(spApi->bUsePppt){
        fprintf(spOut, "    ucaPpptTable,\n");
    }else{
        fprintf(spOut, "    (const uint8_t*)0,\n");
    }
    if(spApi->uiAcharTableLength){
        fprintf(spOut, "    (const void*)aAcharTable,\n");
    }else{
        fprintf(spOut, "    (const void*)0,\n");
    }
    fprintf(spOut, "    (const void*)aParserInit\n");
    fprintf(spOut, "};\n");
    fprintf(spOut, "\n");
    fprintf(spOut, "// void pointer to the parser initialization data\n");
    bNameToCamelCase("vp", cpName, caBuf, PATH_MAX);
    fprintf(spOut, "void* %sInit = (void*)&s_parser_init;\n", caBuf);
    fprintf(spOut, "\n");

    // summary
    fprintf(spOut, "// ALPHABET\n");
    fprintf(spOut, "//  achar min = %"PRIuMAX"\n", spHdr->luiAcharMin);
    fprintf(spOut, "//  achar max = %"PRIuMAX"\n", spHdr->luiAcharMax);
    fprintf(spOut, "//  aint  max = %"PRIuMAX"\n", spHdr->luiUintMax);
    fprintf(spOut, "\n");
    if(spApi->bUsePppt){
        fprintf(spOut, "// PPPT\n");
    }else{
        fprintf(spOut, "// PPPT (not used)\n");
    }
    fprintf(spOut, "//   no. maps = %"PRIuMAX"\n", spApi->luiPpptMapCount);
    fprintf(spOut, "//   map size = %"PRIuMAX" (bytes)\n", spApi->luiPpptMapSize);
    if(spApi->luiPpptTableLength == (luint)APG_MAX_AINT){
        fprintf(spOut, "// table size = %"PRIuMAX" (overflow)\n", spApi->luiPpptTableLength);
    }else{
        fprintf(spOut, "// table size = %"PRIuMAX" (bytes)\n", spApi->luiPpptTableLength);
    }
    fprintf(spOut, "\n");
    fprintf(spOut, "// GRAMMAR\n");
    fprintf(spOut, "//      rules = %"PRIuMAX"\n", (luint)spApi->uiRuleCount);
    fprintf(spOut, "//       UDTs = %"PRIuMAX"\n", (luint)spApi->uiUdtCount);
    fprintf(spOut, "//    opcodes = %"PRIuMAX"\n", (luint)spApi->uiOpcodeCount);
    fprintf(spOut, "//        ---   ABNF original opcodes\n");
    fprintf(spOut, "//        ALT = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_ALT]);
    fprintf(spOut, "//        CAT = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_CAT]);
    fprintf(spOut, "//        REP = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_REP]);
    fprintf(spOut, "//        RNM = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_RNM]);
    fprintf(spOut, "//        TRG = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_TRG]);
    fprintf(spOut, "//        TLS = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_TLS]);
    fprintf(spOut, "//        TBS = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_TBS]);
    fprintf(spOut, "//        ---   SABNF opcodes\n");
    fprintf(spOut, "//        UDT = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_UDT]);
    fprintf(spOut, "//        AND = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_AND]);
    fprintf(spOut, "//        NOT = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_NOT]);
    fprintf(spOut, "//        BKR = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_BKR]);
    fprintf(spOut, "//        BKA = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_BKA]);
    fprintf(spOut, "//        BKN = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_BKN]);
    fprintf(spOut, "//        ABG = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_ABG]);
    fprintf(spOut, "//        AEN = %"PRIuMAX"\n", (luint)uiaOpCounts[ID_AEN]);
    fprintf(spOut, "\n");

    // original grammar
    fprintf(spOut, "// ;original grammar\n");
    luint luiLen = 0;
    spLine = spLinesFirst(spApi->vpLines);
    while(spLine){
        LUINT_MAX(luiLen, spLine->uiTextLength);
        spLine = spLinesNext(spApi->vpLines);
    }
    luiLen += 10;
    if(spApi->cpLineBuffer){
        vMemFree(spApi->vpMem, spApi->cpLineBuffer);
    }
    spApi->cpLineBuffer = (char*)vpMemAlloc(spApi->vpMem, (aint)(sizeof(char) * (size_t)luiLen));
    cpLineBuffer = spApi->cpLineBuffer;
    char* cpGrammar = (char*)vpVecFirst(spApi->vpVecInput);
    if(!cpGrammar){
        fclose(spOut);
        XTHROW(spApi->spException, "input vector should not be empty here");
    }
    cpLineBuffer[0] = '/';
    cpLineBuffer[1] = '/';
    cpLineBuffer[2] = ' ';
    spLine = spLinesFirst(spApi->vpLines);
    while(spLine){
        memcpy((void*)&cpLineBuffer[3], (void*)&cpGrammar[spLine->uiCharIndex], spLine->uiTextLength);
        cpLineBuffer[spLine->uiTextLength + 3] = 0;
        fprintf(spOut, "%s\n", cpLineBuffer);
        spLine = spLinesNext(spApi->vpLines);
    }
    fprintf(spOut, "\n");
}

static void* vpOutputParser(api* spApi){
    luint* luipInit = NULL;
    if(spApi->luipInit){
        vMemFree(spApi->vpMem, spApi->luipInit);
    }
    spApi->luipInit = luipMakeInitData(spApi);
    luipInit = spApi->luipInit;
    init_hdr_out* spHdr = (init_hdr_out*)luipInit;
    parser_init sParserInit = {};
    sParserInit.uiSizeofAchar = (aint)spHdr->luiSizeofAchar;
    sParserInit.uiSizeofUint = (aint)spHdr->luiSizeofUint;

    // the string table
    sParserInit.cpStringTable = spApi->cpStringTable;
    sParserInit.uiStringTableLength = spApi->uiStringTableLength;

    // the PPPT maps
    sParserInit.ucpPpptTable = spApi->ucpPpptTable;
    sParserInit.uiPpptTableLength = spApi->luiPpptTableLength;

    // the achar table
    sParserInit.vpAcharTable = vpMakeAcharTable(spApi, spApi->luiAcharMax,
            spApi->luipAcharTable, spApi->uiAcharTableLength);
    sParserInit.uiAcharTableLength = spApi->uiAcharTableLength;

    sParserInit.uiParserInitLength = (aint)spHdr->luiSizeInInts;
    sParserInit.vpParserInit = vpMakeParserInit(spApi, spHdr->luiUintMax, luipInit, sParserInit.uiParserInitLength);
    return vpParserAllocCtor(spApi->spException, &sParserInit, APG_TRUE);
}

static luint* luipMakeInitData(api* spApi){
    luint* luipUdts = NULL;
    luint* luipRules = NULL;
    luint* luipOpcodes = NULL;
    luint* luipInit = NULL;
    aint ui;
    luint lui, luiLen, luiUdtLen, luiRuleLen, luiOpLen;
    api_udt* spUdt;
    api_rule* spRule;
    api_op* spOp;
    aint uiRuleCount = spApi->uiRuleCount;
    aint uiUdtCount = spApi->uiUdtCount;
    aint uiOpcodeCount = spApi->uiOpcodeCount;
    api_attr_w* spAttrs = ((attrs_ctx*)spApi->vpAttrsCtx)->spAttrs;
    init_hdr_out sHdr = {};
    parser_init sParserInit = {};
    aint uiaOpCounts[ID_GEN];
    memset((void*)&uiaOpCounts[0], 0, sizeof(uiaOpCounts));

    if(uiUdtCount){
        luipUdts = (luint*)vpMemAlloc(spApi->vpMem, (aint)(sizeof(luint) * uiUdtCount * 5));
    }
    luipRules = (luint*)vpMemAlloc(spApi->vpMem, (aint)(sizeof(luint) * uiRuleCount * 7));
    luipOpcodes = (luint*)vpMemAlloc(spApi->vpMem, (aint)(sizeof(luint) * uiOpcodeCount * 5));

    // the string table
    sParserInit.cpStringTable = spApi->cpStringTable;
    sParserInit.uiStringTableLength = spApi->uiStringTableLength;

    // the PPPT maps
    sParserInit.ucpPpptTable = spApi->ucpPpptTable;
    sParserInit.uiPpptTableLength = spApi->luiPpptTableLength;

    sParserInit.uiAcharTableLength = spApi->uiAcharTableLength;
    sParserInit.uiSizeofAchar = uiGetSize(spApi->luiAcharMax);

    luiUdtLen = 0;
    if(spApi->uiUdtCount){
        // convert UDTs
        spUdt = spApi->spUdts;
        for(ui = 0; ui < spApi->uiUdtCount; ui++, spUdt++){
            luipUdts[luiUdtLen++] = (luint)spUdt->uiIndex;
            luipUdts[luiUdtLen++] = (luint)(spUdt->cpName - spApi->cpStringTable);
            luipUdts[luiUdtLen++] = (luint)spUdt->uiEmpty;
        }
    }

    // convert rules
    luiRuleLen = 0;
    spRule = spApi->spRules;
    for(ui = 0; ui < uiRuleCount; ui++, spRule++){
        luipRules[luiRuleLen++] = (luint)spRule->uiIndex;
        luipRules[luiRuleLen++] = (luint)spRule->uiPpptIndex;
        luipRules[luiRuleLen++] = (luint)(spRule->cpName - spApi->cpStringTable);
        luipRules[luiRuleLen++] = (luint)spRule->uiOpOffset;
        luipRules[luiRuleLen++] = (luint)spRule->uiOpCount;
        luipRules[luiRuleLen++] = (luint)spAttrs[ui].bEmpty;
    }

    // convert opcodes
    luiOpLen = 0;
    spOp = spApi->spOpcodes;
    for(ui = 0; ui < uiOpcodeCount; ui++, spOp++){
        luipOpcodes[luiOpLen++] = spOp->uiId;
        uiaOpCounts[spOp->uiId]++;
        switch (spOp->uiId) {
        case ID_ALT:
        case ID_CAT:
            luipOpcodes[luiOpLen++] = (luint)spOp->uiPpptIndex;
            luipOpcodes[luiOpLen++] = (luint)(spOp->uipChildIndex - spApi->uipChildIndexTable);
            luipOpcodes[luiOpLen++] = (luint)spOp->uiChildCount;
            break;
        case ID_REP:
        case ID_TRG:
            luipOpcodes[luiOpLen++] = (luint)spOp->uiPpptIndex;
            luipOpcodes[luiOpLen++] = spOp->luiMin;
            luipOpcodes[luiOpLen++] = spOp->luiMax;
            break;
        case ID_RNM:
            luipOpcodes[luiOpLen++] = (luint)spApi->spRules[spOp->uiIndex].uiPpptIndex;
            luipOpcodes[luiOpLen++] = (luint)spOp->uiIndex;
            break;
        case ID_TLS:
        case ID_TBS:
            luipOpcodes[luiOpLen++] = (luint)spOp->uiPpptIndex;
            luipOpcodes[luiOpLen++] = (luint)(spOp->luipAchar- spApi->luipAcharTable);
            luipOpcodes[luiOpLen++] = (luint)spOp->uiAcharLength;
            break;
        case ID_UDT:
            luipOpcodes[luiOpLen++] = (luint)spOp->uiIndex;
            luipOpcodes[luiOpLen++] = (luint)spOp->uiEmpty;
            break;
        case ID_BKR:
            luipOpcodes[luiOpLen++] = (luint)spOp->uiBkrIndex;
            luipOpcodes[luiOpLen++] = (luint)spOp->uiCase;
            luipOpcodes[luiOpLen++] = (luint)spOp->uiMode;
            break;
        case ID_AND:
        case ID_NOT:
            luipOpcodes[luiOpLen++] = (luint)spOp->uiPpptIndex;
            break;
        case ID_BKA:
        case ID_BKN:
        case ID_ABG:
        case ID_AEN:
            break;
        default:
            XTHROW(spApi->spException, "unrecognized operator ID");
        }
    }

    // fill in the header
    sHdr.luiSizeInInts = (luint)(sizeof(sHdr)/sizeof(luint)) +
            luiRuleLen + luiUdtLen + luiOpLen + (luint)spApi->uiChildIndexTableLength;
    sHdr.luiAcharMin = spApi->luiAcharMin;
    sHdr.luiAcharMax = spApi->luiAcharMax;
    sHdr.luiSizeofAchar = uiGetSize(sHdr.luiAcharMax);
    sHdr.luiRuleCount = uiRuleCount;
    sHdr.luiUdtCount = uiUdtCount;
    sHdr.luiOpcodeCount = uiOpcodeCount;
    sHdr.luiMapCount = spApi->luiPpptMapCount;
    sHdr.luiMapSize = spApi->luiPpptMapSize;
    sHdr.luiVersionOffset = spApi->uiVersionOffset;
    sHdr.luiCopyrightOffset = spApi->uiCopyrightOffset;
    sHdr.luiLicenseOffset = spApi->uiLicenseOffset;
    sHdr.luiChildListOffset = (luint)(sizeof(sHdr)/sizeof(luint));
    sHdr.luiChildListLength = spApi->uiChildIndexTableLength;
    sHdr.luiRulesOffset = sHdr.luiChildListOffset + sHdr.luiChildListLength;
    sHdr.luiRulesLength = (luint)luiRuleLen;
    sHdr.luiUdtsOffset = sHdr.luiRulesOffset + sHdr.luiRulesLength;
    sHdr.luiUdtsLength = luiUdtLen;
    sHdr.luiOpcodesOffset = sHdr.luiUdtsOffset + sHdr.luiUdtsLength;
    sHdr.luiOpcodesLength = luiOpLen;
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiSizeInInts);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiAcharMax);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiSizeofAchar);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiRuleCount);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiUdtCount);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiOpcodeCount);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiVersionOffset);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiCopyrightOffset);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiLicenseOffset);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiChildListOffset);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiChildListLength);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiRulesOffset);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiRulesLength);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiUdtsOffset);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiUdtsLength);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiOpcodesOffset);
    LUINT_MAX(sHdr.luiUintMax, sHdr.luiOpcodesLength);
    if(uiUdtCount){
        for(lui = 0; lui < luiUdtLen; lui++){
            LUINT_MAX(sHdr.luiUintMax, luipUdts[lui]);
        }
    }
    for(lui = 0; lui < luiRuleLen; lui++){
        LUINT_MAX(sHdr.luiUintMax, luipRules[lui]);
    }
    for(lui = 0; lui < luiOpLen; lui++){
        LUINT_MAX(sHdr.luiUintMax, luipOpcodes[lui]);
    }
    LUINT_MAX(sHdr.luiUintMax, (luint)spApi->uiStringTableLength);
    LUINT_MAX(sHdr.luiUintMax, (luint)spApi->uiAcharTableLength);
    LUINT_MAX(sHdr.luiUintMax, spApi->luiPpptTableLength);

    sParserInit.uiSizeofUint= uiGetSize(sHdr.luiUintMax);
    sHdr.luiSizeofUint = (luint)sParserInit.uiSizeofUint;
    luipInit = (luint*)vpMemAlloc(spApi->vpMem, (aint)(sizeof(luint) * sHdr.luiSizeInInts));
    luiLen = 0;
    luipInit[luiLen++] = sHdr.luiSizeInInts;
    luipInit[luiLen++] = sHdr.luiAcharMin;
    luipInit[luiLen++] = sHdr.luiAcharMax;
    luipInit[luiLen++] = sHdr.luiSizeofAchar;
    luipInit[luiLen++] = sHdr.luiUintMax;
    luipInit[luiLen++] = sHdr.luiSizeofUint;
    luipInit[luiLen++] = sHdr.luiRuleCount;
    luipInit[luiLen++] = sHdr.luiUdtCount;
    luipInit[luiLen++] = sHdr.luiOpcodeCount;
    luipInit[luiLen++] = sHdr.luiMapCount;
    luipInit[luiLen++] = sHdr.luiMapSize;
    luipInit[luiLen++] = sHdr.luiVersionOffset;
    luipInit[luiLen++] = sHdr.luiCopyrightOffset;
    luipInit[luiLen++] = sHdr.luiLicenseOffset;
    luipInit[luiLen++] = sHdr.luiChildListOffset;
    luipInit[luiLen++] = sHdr.luiChildListLength;
    luipInit[luiLen++] = sHdr.luiRulesOffset;
    luipInit[luiLen++] = sHdr.luiRulesLength;
    luipInit[luiLen++] = sHdr.luiUdtsOffset;
    luipInit[luiLen++] = sHdr.luiUdtsLength;
    luipInit[luiLen++] = sHdr.luiOpcodesOffset;
    luipInit[luiLen++] = sHdr.luiOpcodesLength;
    for(lui = 0; lui < spApi->uiChildIndexTableLength; lui++){
        luipInit[luiLen++] = (luint)spApi->uipChildIndexTable[lui];
    }
    for(lui = 0; lui < luiRuleLen; lui++){
        luipInit[luiLen++] = luipRules[lui];
    }
    for(lui = 0; lui < luiUdtLen; lui++){
        luipInit[luiLen++] = luipUdts[lui];
    }
    for(lui = 0; lui < luiOpLen; lui++){
        luipInit[luiLen++] = luipOpcodes[lui];
    }
    if(luiLen != sHdr.luiSizeInInts){
        vMemFree(spApi->vpMem, (void*)luipUdts);
        vMemFree(spApi->vpMem, (void*)luipRules);
        vMemFree(spApi->vpMem, (void*)luipOpcodes);
        vMemFree(spApi->vpMem, (void*)luipInit);
        XTHROW(spApi->spException,
                "sanity check - calculated and actual parser initialization lengths not equal");
    }
    vMemFree(spApi->vpMem, (void*)luipRules);
    vMemFree(spApi->vpMem, (void*)luipOpcodes);
    spApi->luipInit = luipInit;
    return luipInit;
}

static aint uiLastChar(char cCharToFind, const char* cpString) {
    aint uiLast = APG_UNDEFINED;
    aint ui = 0;
    for (; ui < (aint) strlen(cpString); ui++) {
        if (cpString[ui] == cCharToFind) {
            uiLast = ui;
        }
    }
    return uiLast;
}
static abool bSetFileExtension(const char* cpPathName, const char* cpExt, char* cpBuffer, aint uiBufferLength) {
    abool bReturn = APG_FAILURE;
    if (cpPathName && cpExt && cpBuffer && uiBufferLength) {
        char cDot = '.';
        while (APG_TRUE) {
            cpBuffer[0] = 0;
            if(cpPathName[0] == 0){
                // path name is empty
                break;
            }
            if((cpPathName[0] == cDot) && (cpPathName[1] == cDot) && (cpPathName[2] == cDot)){
                // three leading dots is an error
                break;
            }
            aint uiDot;
            uiDot = uiLastChar(cDot, cpPathName);
            if ((uiDot == APG_UNDEFINED) || (uiDot == 0) || (uiDot == 1)) {
                uiDot = (aint) strlen(cpPathName);
            }
            if((uiDot + strlen(cpExt) + 2) > uiBufferLength){
                // user's buffer is too small
                break;
            }
            aint ui = 0;
            for (; ui < uiDot; ui++) {
                cpBuffer[ui] = cpPathName[ui];
            }
            if(strlen(cpExt)){
                cpBuffer[ui++] = cDot;
                cpBuffer[ui] = 0;
                strcat(cpBuffer, cpExt);
            }
            bReturn = APG_TRUE;
            break;
        }
    }
    return bReturn;
}
static abool bGetFileName(const char* cpPathName, char* cpBuffer, aint uiBufferLength) {
    abool bReturn = APG_FAILURE;
    if (cpPathName && cpBuffer && uiBufferLength) {
        char cDot = '.';
        char cSlash = '/';
        char cBackSlash = '\\';
        while (APG_TRUE) {
            cpBuffer[0] = 0;
            if(cpPathName[0] == 0){
                // path name is empty
                break;
            }
            if((cpPathName[0] == cDot) && (cpPathName[1] == cDot) && (cpPathName[2] == cDot)){
                // three leading dots is an error
                break;
            }
            aint uiDivider;
            aint uiDot;
            // try linux
            uiDivider = uiLastChar(cSlash, cpPathName);
            if (uiDivider == APG_UNDEFINED) {
                // try windows
                uiDivider = uiLastChar(cBackSlash, cpPathName);
            }
            if (uiDivider == APG_UNDEFINED) {
                uiDivider = 0;
            }else{
                uiDivider++;
            }
            uiDot = uiLastChar(cDot, cpPathName);
            if ((uiDot == APG_UNDEFINED) || (uiDot == 0) || (uiDot == 1)) {
                uiDot = (aint) strlen(cpPathName) + 1;
            }
            aint uiLen = uiDot - uiDivider;
            if (uiLen >= uiBufferLength) {
                // not enough room in caller's buffer for the result
                break;
            }
            aint ui = uiDivider;
            aint uii = 0;
            for (; ui < uiDot; ui++, uii++) {
                cpBuffer[uii] = cpPathName[ui];
            }
            cpBuffer[uii] = 0;
            bReturn = APG_TRUE;
            break;
        }
    }
    return bReturn;
}

static char cToUpper(char cChar){
    if(cChar >= 97 && cChar <= 122){
        return cChar - 32;
    }
    return cChar;
}
static char cToLower(char cChar){
    if(cChar >= 65 && cChar <= 90){
        return cChar + 32;
    }
    return cChar;
}
static abool bIsLetter(char cLetter){
    if(cLetter >= 65 && cLetter <= 90){
        return APG_TRUE;
    }
    if(cLetter >= 97 && cLetter <= 122){
        return APG_TRUE;
    }
    if(cLetter >= 48 && cLetter <= 57){
        return APG_TRUE;
    }
    return APG_FALSE;
}
static abool bNameToCaps(const char* cpPrefix, const char* cpName, char* cpBuffer, aint uiBufferLength){
    char cUnder = '_';
    aint uiPreLen = (aint)strlen(cpPrefix);
    aint uiNameLen = (aint)strlen(cpName);
    aint ui, uii;
    if((uiPreLen + uiNameLen + 1) >= uiBufferLength){
        return APG_FAILURE;
    }
    uii = 0;
    if(uiPreLen){
        for(ui = 0; ui < uiPreLen; ui++){
            if(bIsLetter(cpPrefix[ui])){
                cpBuffer[ui] = cToUpper(cpPrefix[ui]);
            }else{
                cpBuffer[ui] = cUnder;
            }
        }
        cpBuffer[ui] = cUnder;
        uii = ui + 1;
    }
    if(uiNameLen){
        for(ui = 0; ui < uiNameLen; ui++, uii++){
            if(bIsLetter(cpName[ui])){
                cpBuffer[uii] = cToUpper(cpName[ui]);
            }else{
                cpBuffer[uii] = cUnder;
            }
        }
    }
    cpBuffer[uii] = 0;
    return APG_TRUE;
}

static abool bNameToCamelCase(const char* cpPrefix, const char* cpName, char* cpBuffer, aint uiBufferLength){
    aint uiPreLen = (aint)strlen(cpPrefix);
    aint uiNameLen = (aint)strlen(cpName);
    aint ui, uiCount;
    abool bCamel = APG_FALSE;
    if((uiPreLen + uiNameLen) >= uiBufferLength){
        return APG_FAILURE;
    }
    uiCount = 0;
    bCamel = APG_FALSE;
    for(ui = 0; ui < uiPreLen; ui++){
        if(!bIsLetter(cpPrefix[ui])){
            bCamel = APG_TRUE;
        }else{
            if(bCamel){
                cpBuffer[uiCount] = cToUpper(cpPrefix[ui]);
                bCamel = APG_FALSE;
            }else{
                cpBuffer[uiCount] = cToLower(cpPrefix[ui]);
            }
            uiCount++;
        }
    }
    ui = 0;
    bCamel = APG_TRUE;
    for(ui = 0; ui < uiNameLen; ui++){
        if(!bIsLetter(cpName[ui])){
            bCamel = APG_TRUE;
        }else{
            if(bCamel){
                cpBuffer[uiCount] = cToUpper(cpName[ui]);
                bCamel = APG_FALSE;
            }else{
                cpBuffer[uiCount] = cToLower(cpName[ui]);
            }
            uiCount++;
        }
    }
    cpBuffer[uiCount] = 0;
    return APG_TRUE;
}

static int iCompRule(const void* vpL, const void* vpR) {
    api_rule* spL = (api_rule*) vpL;
    api_rule* spR = (api_rule*) vpR;
    aint uiLenL = strlen(spL->cpName);
    aint uiLenR = strlen(spR->cpName);
    char l, r;
    char* cpL = spL->cpName;
    char* cpR = spR->cpName;
    aint uiLesser = uiLenL < uiLenR ? uiLenL : uiLenR;
    while (uiLesser--) {
        l = *cpL;
        if (l >= 65 && l <= 90) {
            l += 32;
        }
        r = *cpR;
        if (r >= 65 && r <= 90) {
            r += 32;
        }
        if (l < r) {
            return -1;
        }
        if (l > r) {
            return 1;
        }
        cpL++;
        cpR++;
    }
    if (uiLenL < uiLenR) {
        return -1;
    }
    if (uiLenL > uiLenR) {
        return 1;
    }
    return 0;
}

static int iCompUdt(const void* vpL, const void* vpR) {
    api_udt* spL = (api_udt*) vpL;
    api_udt* spR = (api_udt*) vpR;
    aint uiLenL = strlen(spL->cpName);
    aint uiLenR = strlen(spR->cpName);
    char l, r;
    char* cpL = spL->cpName;
    char* cpR = spR->cpName;
    aint uiLesser = uiLenL < uiLenR ? uiLenL : uiLenR;
    while (uiLesser--) {
        l = *cpL;
        if (l >= 65 && l <= 90) {
            l += 32;
        }
        r = *cpR;
        if (r >= 65 && r <= 90) {
            r += 32;
        }
        if (l < r) {
            return -1;
        }
        if (l > r) {
            return 1;
        }
        cpL++;
        cpR++;
    }
    if (uiLenL < uiLenR) {
        return -1;
    }
    if (uiLenL > uiLenR) {
        return 1;
    }
    return 0;
}
static void vPrintChars(FILE* spOut, const uint8_t* ucpChars, aint uiLength){
    aint ui = 0;
    aint uiNewLine;
    aint uiEnd = (OUTPUT_LINE_LENGTH) - 1;
    for(; ui < uiLength; ui++){
        if(ui == 0){
            fprintf(spOut, " %d", ucpChars[ui]);
        }else{
            fprintf(spOut, ",%d", ucpChars[ui]);
        }
        uiNewLine = ui % OUTPUT_LINE_LENGTH;
        if(uiNewLine == uiEnd){
            fprintf(spOut, "\n");
        }
    }
}
static void vPrintLongs(FILE* spOut, const luint* luiVals, aint uiLength){
    aint ui = 0;
    aint uiNewLine;
    for(; ui < uiLength; ui++){
        if(ui == 0){
            if(luiVals[ui] == (luint)-1){
                fprintf(spOut, " -1");
            }else{
                fprintf(spOut, " %"PRIuMAX"", luiVals[ui]);
            }
        }else{
            if(luiVals[ui] == (luint)-1){
                fprintf(spOut, ",-1");
            }else{
                fprintf(spOut, ",%"PRIuMAX"", luiVals[ui]);
            }
            uiNewLine = ui % OUTPUT_LINE_LENGTH;
            if(!uiNewLine){
                fprintf(spOut, "\n");
            }
        }
    }
}

static aint uiGetSize(luint luiValue){
    if(luiValue <= 0xFF){
        return 1;
    }
    if(luiValue <= 0xFFFF){
        return 2;
    }
    if(luiValue <= 0xFFFFFFFF){
        return 4;
    }
    return 8;
}

static const char* cpGetType(luint luiValue){
    if(luiValue <= 0xFF){
        return s_cpUchar;
    }
    if(luiValue <= 0xFFFF){
        return s_cpUshort;
    }
    if(luiValue <= 0xFFFFFFFF){
        return s_cpUint;
    }
    return s_cpUlong;
}

static void* vpMakeParserInit(api* spApi, luint luiUintMax, luint* luipData, aint uiLen){
    aint uiSize = uiGetSize(luiUintMax);
    aint ui = 0;
    if(uiSize == 1){
        spApi->vpOutputParserInit = vpMemAlloc(spApi->vpMem, uiLen);
        uint8_t* pChars = (uint8_t*)spApi->vpOutputParserInit;
        for(; ui < uiLen; ui++){
            pChars[ui] = (uint8_t)luipData[ui];
        }
    }else if(uiSize == 2){
        spApi->vpOutputParserInit = vpMemAlloc(spApi->vpMem, (uiLen * 2));
        uint16_t* pChars = (uint16_t*)spApi->vpOutputParserInit;
        for(; ui < uiLen; ui++){
            if(ui == 1750){
                pChars[ui] = (uint16_t)luipData[ui];
            }
            pChars[ui] = (uint16_t)luipData[ui];
        }
    }else if(uiSize == 4){
        spApi->vpOutputParserInit = vpMemAlloc(spApi->vpMem, (uiLen * 4));
        uint32_t* pChars = (uint32_t*)spApi->vpOutputParserInit;
        for(; ui < uiLen; ui++){
            pChars[ui] = (uint32_t)luipData[ui];
        }
    }else if(uiSize == 8){
        spApi->vpOutputParserInit = vpMemAlloc(spApi->vpMem, (uiLen * 8));
        uint64_t* pChars = (uint64_t*)spApi->vpOutputParserInit;
        for(; ui < uiLen; ui++){
            pChars[ui] = (uint64_t)luipData[ui];
        }
    }
    return spApi->vpOutputParserInit;
}
static void* vpMakeAcharTable(api* spApi, luint luiAcharMax, luint* luipTable, aint uiLen){
    aint uiSize = uiGetSize(luiAcharMax);
    aint ui = 0;
    if(uiSize == 1){
        spApi->vpOutputAcharTable = vpMemAlloc(spApi->vpMem, uiLen);
        uint8_t* pChars = (uint8_t*)spApi->vpOutputAcharTable;
        for(; ui < uiLen; ui++){
            pChars[ui] = (uint8_t)luipTable[ui];
        }
    }else if(uiSize == 2){
        spApi->vpOutputAcharTable = vpMemAlloc(spApi->vpMem, (uiLen * 2));
        uint16_t* pChars = (uint16_t*)spApi->vpOutputAcharTable;
        for(; ui < uiLen; ui++){
            pChars[ui] = (uint16_t)luipTable[ui];
        }
    }else if(uiSize == 4){
        spApi->vpOutputAcharTable = vpMemAlloc(spApi->vpMem, (uiLen * 4));
        uint32_t* pChars = (uint32_t*)spApi->vpOutputAcharTable;
        for(; ui < uiLen; ui++){
            pChars[ui] = (uint32_t)luipTable[ui];
        }
    }else if(uiSize == 8){
        spApi->vpOutputAcharTable = vpMemAlloc(spApi->vpMem, (uiLen * 8));
        uint64_t* pChars = (uint64_t*)spApi->vpOutputAcharTable;
        for(; ui < uiLen; ui++){
            pChars[ui] = (uint64_t)luipTable[ui];
        }
    }
    return spApi->vpOutputAcharTable;
}

#ifdef TEST_NAMES
static void vTestNames(){
    // test file names
    char* cpaNames[] = {
            "header.h", "../header", "linuxfolder/linuxname.zip", "D:\\windowsfolder\\windows.c",
            "noext", "./folder/foldernoext", ".hidden", ".hidden/folder/wayup.java", "", ".../error"
    };
    char caBuffer[128];
    aint uiBufferSize = 128;
    aint uiCount = sizeof(cpaNames)/sizeof(char*);
    aint ui;
    printf("TEST bGetFileName():\n");
    printf("path name: file name\n");
    for(ui = 0; ui < uiCount; ui++){
        if(bGetFileName(cpaNames[ui], caBuffer, uiBufferSize)){
            printf("'%s': '%s'\n", cpaNames[ui], caBuffer);
        }else{
            printf("'%s': failed\n", cpaNames[ui]);
        }
    }

    // test extensions
    char* cpaExt[] = {"h", "c", "", "longextension", "zip", "java", "cpp", "hpp", "empty", "exterror"};
    printf("\n");
    printf("TEST bbSetFileExtension():\n");
    printf("file name: extension: added\n");
    for(ui = 0; ui < uiCount; ui++){
        if(bSetFileExtension(cpaNames[ui], cpaExt[ui], caBuffer, uiBufferSize)){
            printf("'%s': '%s': '%s'\n", cpaNames[ui], cpaExt[ui], caBuffer);
        }else{
            printf("'%s': '%s': failed\n", cpaNames[ui], cpaExt[ui]);
        }
    }

    // test upper
    char* cpaUppers[] = {
            "file-name", "text-type", "_type-to-", "_file_name_",
            "UPPER_CASE", "lower_case", ".hidden.h", ".hidden/folder/wayup.java"
    };
    uiCount = sizeof(cpaUppers)/sizeof(char*);
    printf("\n");
    printf("TEST bNameToCaps():\n");
    printf("prefix: name: UPPER\n");
    char* cpPrefix = "my-Pre_Fix";
    for(ui = 0; ui < uiCount; ui++){
        if(bNameToCaps(cpPrefix, cpaUppers[ui], caBuffer, uiBufferSize)){
            printf("'%s': '%s': '%s'\n", cpPrefix, cpaUppers[ui], caBuffer);
        }else{
            printf("'%s': '%s': failed\n", cpPrefix, cpaUppers[ui]);
        }
    }

    //test camel case
    char* cpaCamel[] = {
            "file-name", "text-type", "_type-to-", "_file_name_",
            "UPPER_CASE", "lower_case", ".hidden.h", ".hidden/folder/wayup.java"
    };
    uiCount = sizeof(cpaCamel)/sizeof(char*);
    printf("\n");
    printf("TEST bNameToCamelCase():\n");
    printf("prefix: name: CamelCase\n");
    cpPrefix = "ui";
    for(ui = 0; ui < uiCount; ui++){
        if(bNameToCamelCase(cpPrefix, cpaCamel[ui], caBuffer, uiBufferSize)){
            printf("'%s': '%s': '%s'\n", cpPrefix, cpaCamel[ui], caBuffer);
        }else{
            printf("'%s': '%s': failed\n", cpPrefix, cpaCamel[ui]);
        }
    }
}
#endif /* TEST_NAMES */
