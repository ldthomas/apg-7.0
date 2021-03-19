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
/** \file config.c
 * \brief Handles the main function argument list and produces the configuration structure that drives APG.
 *
 * This suite of functions work together to parse the command line parameters into a configuration structure
 * that drives the generator.
 * The parameters can be on the command line directly or in a file (see the \@filename command).
 */
#include <unistd.h>
//#include <limits.h>

#include "../api/api.h"
#include "./config.h"

static const void* s_vpMagicNumber = (void*)"config";
static char* s_cpCommandLineError = "COMMAND LINE ERROR: ";

/** \struct config_ctx
 * \brief The configuration component context.
 */
typedef struct {
    const void* vpValidate; /**< \brief a "magic number" to validate the context */
    exception* spException;
    void* vpMem; /**< \brief memory component context handle */
    void* vpVecArgs; /**< \brief command line or configuration file options as a null-terminated string of strings */
    void* vpVecCwd; /**< \brief the current working directory, the directory "main" is being run in */
    void* vpVecOutput; /**< \brief if non-empty string, the output file name, the name.h and name.c files will be the generated files */
    void* vpVecInput; /**< \brief one or more input file names as a null-terminated string of strings */
    void* vpVecInputAddrs; /**< \brief addresses of the input file name, can be used for "char** argv" type access to file names */
    void* vpVecPRules; /**< \brief one or more protected rule names as a null-terminated string of strings */
    void* vpVecPRulesAddrs; /**< \brief addresses of the rule names */
    void* vpVecGrammar; /**< \brief the input grammar, a concatenation of all input files */
    void* vpVecConfigOut; /**< \brief if non-empty string, generate a default configuration file here */
    void* vpVecConfigIn; /**< \brief if non-empty string, read the configuration from this file name */
    void* vpVecHtmlOut; /**< \brief if non-empty string, put HTML version of annotated input grammar on file name */
    void* vpVecRulesHtmlOut; /**< \brief if non-empty string, put HTML version of rule/UDT dependencies on file name */
    void* vpVecLfOut; /**< \brief if non-empty string, put LF translated line ends on file name */
    void* vpVecCrLfOut; /**< \brief if non-empty string, put CRLF translated line ends on file name */
    aint  uiInputFiles; /**< \brief the number of input files found */
    aint  uiPRules; /**< \brief the number of protected rule names found */
    abool bHelp; /**< \brief the help flag, if set the help screen is printed and processing stops */
    abool bVersion; /**< \brief the version flag, if set the version number is printed and processing stops */
    abool bStrict; /**< \brief if set, the grammar is treated as strict ABNF */
    abool bIgnoreAttrs; /**< \brief if set, skip the attribute calculation */
    abool bNoPppt; /**< \brief if set, skip the PPPT calculation */
    abool bDc; /**< \brief display the complete configuration as found on command line or configuration file */
    abool bDv; /**< \brief verbose display of information during processing - sets uiDg, uiDa, uiDr and uiDc */
    abool bDg; /**< \brief display an annotated version of the input grammar */
    abool bDa; /**< \brief display the rule attributes */
    abool bDo; /**< \brief display human-readable opcodes */
    abool bDp; /**< \brief display PPPT sizes */
    abool bDr; /**< \brief display grammar rule/UDT names in the order they occur in the grammar*/
    abool bDra; /**< \brief display the grammar rule/UDT names alphabetically */
    FILE* spConfigFile; ///< \brief Open file handle for reading a configuration file. NULL if not open.
//    char caErrorMsg[1024]; /**< \brief error messages are stored here for later display */
//    jmp_buf aJmpBuf; /**< \brief long jumps are used to recover from errors deep in the collection of the options */
    config sConfig; /**< \brief the form of the options presented to the user */
} config_ctx;

static void vExtractFileOptions(config_ctx* spCtx);
static void vExtractArgOptions(config_ctx* spCtx, char* cpParams);
static config* spGetConfig(config_ctx* spCtx);
static void vGetArgs(config_ctx* spCtx, int iArgCount, char** cppArgs);

/** \brief Constructs a configuration object to hold all data relating to this instance of the configuration.
 * \param spEx Pointer to a valid exception object. \see XCTOR().
 * \return Pointer to the configuration context.
 * Exception thrown on error.
 */
void* vpConfigCtor(exception* spEx) {
    if(!bExValidate(spEx)){
        vExContext();
    }
    config_ctx* spCtx = NULL;
    aint uiSmall = 256;
    aint uiMedium = 1024;
    aint uiLarge = 4096;
    char caCwd[PATH_MAX];
    aint uiLen;
    void* vpMem = vpMemCtor(spEx);
    spCtx = (config_ctx*) vpMemAlloc(vpMem, (aint) sizeof(config_ctx));
    memset((void*) spCtx, 0, (aint) sizeof(config_ctx));
    spCtx->vpMem = vpMem;
    spCtx->spException = spEx;

    // string of arguments, null-term string of strings
    spCtx->vpVecArgs = vpVecCtor(vpMem, (aint) sizeof(char), uiSmall);

    // the current working directory
    if (getcwd(caCwd, PATH_MAX) == NULL) {
        XTHROW(spEx, "system error - can't get current working directory");
    }
    uiLen = (aint)strlen(caCwd) + 1;
    spCtx->vpVecCwd = vpVecCtor(vpMem, (aint) sizeof(char), (uiLen + 1));

    // the output file name
    spCtx->vpVecOutput = vpVecCtor(vpMem, (aint) sizeof(char), uiSmall);

    // the input file name(s)
    spCtx->vpVecInput = vpVecCtor(vpMem, (aint) sizeof(char), uiMedium);

    // the input file name(s)
    spCtx->vpVecPRules = vpVecCtor(vpMem, (aint) sizeof(char), uiMedium);

    // the array of input file name addresses
    spCtx->vpVecInputAddrs = vpVecCtor(vpMem, (aint) sizeof(char*), uiSmall);

    // the array of input file name addresses
    spCtx->vpVecPRulesAddrs = vpVecCtor(vpMem, (aint) sizeof(char*), uiSmall);

    // the grammar file
    spCtx->vpVecGrammar = vpVecCtor(vpMem, (aint) sizeof(char), uiLarge);

    // the file name of the generated default configuration file
    spCtx->vpVecConfigOut = vpVecCtor(vpMem, (aint) sizeof(char), uiSmall);

    // the file name of the HTML annotated input grammar
    spCtx->vpVecHtmlOut = vpVecCtor(vpMem, (aint) sizeof(char), uiSmall);

    // the file name of the HTML rule/UDT dependencies
    spCtx->vpVecRulesHtmlOut = vpVecCtor(vpMem, (aint) sizeof(char), uiSmall);

    // the file name of the configuration file to read
    spCtx->vpVecConfigIn = vpVecCtor(vpMem, (aint) sizeof(char), uiSmall);

    // the file name of the LF line ends conversion
    spCtx->vpVecLfOut = vpVecCtor(vpMem, (aint) sizeof(char), uiSmall);

    // the file name of the CRLF line ends conversion
    spCtx->vpVecCrLfOut = vpVecCtor(vpMem, (aint) sizeof(char), uiSmall);

    // success
    spCtx->vpValidate = s_vpMagicNumber;
    return (void*) spCtx;
}

/** \brief The configuration destructor.
 *
 * Closes the open config file, if necessary.
 *
 * \param vpCtx Pointer to the configuration context previously returned by a call to the constructor vConfigCtor().
 * NULL is silently ignored. However, if non-NULL it must be a valid config context pointer.
 */
void vConfigDtor(void* vpCtx) {
    config_ctx* spCtx = (config_ctx*) vpCtx;
    if (vpCtx) {
        if (spCtx->vpValidate == s_vpMagicNumber) {
            if(spCtx->spConfigFile){
                fclose(spCtx->spConfigFile);
            }
            void* vpMem = spCtx->vpMem;
            memset(vpCtx, 0, sizeof(*spCtx));
            vMemDtor(vpMem);
        }else{
            vExContext();
        }
    }
}

/** \brief Reads the command line arguments and parses them into a configuration structure.
 * \param vpCtx Pointer to the configuration context previously returned by a call to the constructor vConfigCtor().
 * \param iArgCount the number of arguments on the command line.
 * \param cppArgs an array of pointers to the command line arguments.
 * \return Pointer to a configuration structure with all of the input from the command line
 * in a form easily used by main();
 */
const config* spConfigOptions(void* vpCtx, int iArgCount, char** cppArgs) {
    config_ctx* spCtx = (config_ctx*)vpCtx;;
    vGetArgs(spCtx, iArgCount, cppArgs);
    if(!(spCtx->bHelp || spCtx->bVersion || vpVecFirst(spCtx->vpVecConfigOut))){
        if(vpVecFirst(spCtx->vpVecConfigIn)){
            vExtractFileOptions(spCtx);
        }
    }

    // success
    return spGetConfig(spCtx);
}

/** \brief Prints the help screen when requested or if there is a command line options error.
 *
 */
void vConfigHelp(void){
        printf("usage: apg options\n");
        printf("binary options:\n");
        printf("-i filename           : the file name of the input grammar (see note 5.)\n");
        printf("-o filename           : the file name of the generated C-language header and source files (see note 6.)\n");
        printf("\n");
        printf("unary options:\n");
        printf("--input=filename      : the file name of the input grammar (see note 5.)\n");
        printf("--output=filename     : the file name of the generated C-language header and source files (see note 6.)\n");
        printf("-c                    : generate a default configuration file named \"%s\"\n", CONFIG_FILE);
        printf("--config-file=fname   : generate a default configuration file named \"fname\"\n");
        printf("--p-rules=name[,name] : comma-delimited list of protected rule names (see note 9.)\n");
        printf("--grammar-html=fname  : display input grammar in HTML format on file \"fname\"\n");
        printf("--rules-html=fname    : display rule/UDT names and dependencies in HTML format on file \"fname\"\n");
        printf("--lf=filename         : converts all input line end to LF(\\n) on file \"filename\" (see note 8.)\n");
        printf("--crlf=filename       : converts all input line end to CRLF(\\r\\n) on file \"filename\" (see note 8.)\n");
        printf("@                     : read the options from the configuration file named \"%s\"\n", CONFIG_FILE);
        printf("@filename             : read the options from the configuration file named \"filename\"\n");
        printf("\n");
        printf("flags: true if present, otherwise false\n");
        printf("?                     : display this help screen\n");
        printf("-h                    : display this help screen\n");
        printf("--help                : display this help screen\n");
        printf("-v                    : display version information\n");
        printf("--version             : display version information\n");
        printf("--strict              : only ABNF as strictly defined in RFC 5234 allowed\n");
        printf("--ignore-attributes   : attribute information will not be computed, proceed at your own risk\n");
        printf("--no-pppt             : do not produce Partially-Predictive Parsing Tables (PPPTs)\n");
        printf("\n");
        printf("display flags\n");
        printf("-dv                   : verbose - sets flags -dc, -dg, dr, -dp and -da\n");
        printf("-dc                   : display the complete configuration found on the command line or in the command file\n");
        printf("-dg                   : display an annotated version of the input grammar\n");
        printf("-dr                   : display the grammar rule names, in the order they are found in the grammar\n");
        printf("-dra                  : display the grammar rule names, in alphabetical order\n");
        printf("-da                   : display the rule attributes\n");
        printf("-dp                   : display the Partially-Predictive Parsing Table (PPPT) sizes\n");
        printf("-do                   : display the opcodes in human-readable format (warning: may generate many lines of output)\n");
        printf("\n");
        printf("NOTES:\n");
        printf("1. All options and flags are case sensitive.\n");
        printf("2. Binary options require one or more spaces between the flag and the name.\n");
        printf("3. No spaces are allowed in unary options or flags (no space before or after \"=\").\n");
        printf("4. If any or all of -h, -v or -c (or any of their alternatives) is present all other options are ignored.\n");
        printf("5. File names may be absolute (/home/user/filname) or relative ([./ | ../]mydir/filename).\n");
        printf("   Directories in the path name must exist.\n");
        printf("6. Any file name extension will be stripped and replaced with .h for the header file and .c for the source file.\n");
        printf("7. Absent -h, -v or -c, if a configuration file is indicated (@ or @filename) all other options are ignored.\n");
        printf("8. Both --lf and --crlf may be present. If either is present, all other options except -h, -v and -c are ignored.\n");
        printf("9. Protected rules are protected from being hidden under predictive PPPT-mapped nodes.\n");
        printf("   Rule names are case insensitive. The argument may be a comma-delimited list with no spaces allowed.\n");
        printf("   Multiple instances of the --p-rules flag will accumulate rule names in the list.\n");
        printf("10. No command line arguments generates this help screen.\n");
        printf("\n");
}

/** \brief Display the version number. */
void vConfigVersion(void){
    printf("  version: APG Version %s\n", APG_VERSION);
    printf("copyright: %s\n", APG_COPYRIGHT);
    printf("  license: %s\n", APG_LICENSE);
}

/** \brief Prints a default configuration file.
 *
 * A configuration file may in many cases be a more convenient than a command line for providing the main() program with
 * its options. This program will print a default file with all options available commented out.
 * The user can then conveniently un-comment the options desired and customize it to his/her needs.
 * This is especially useful to avoid the need to re-type long file names onto the command line for repeated applications.
 * \param vpCtx Pointer to the configuration context previously returned by a call to the constructor vConfigCtor().
 * \param cpFileName Name of the file to open and write the default configuration to.
 * If NULL, prints to stdout.
 */
void vConfigDefault(void* vpCtx, char* cpFileName) {
    config_ctx* spCtx = (config_ctx*) vpCtx;
    if(!vpCtx || (spCtx->vpValidate != s_vpMagicNumber)){
        vExContext();
    }
    FILE* spFile = stdout;
    if(cpFileName){
        spFile = fopen(cpFileName, "wb");
        if(!spFile){
            char caBuf[128];
            snprintf(caBuf, 128, "can't open file \"%s\" for writing default configuration file", cpFileName);
            XTHROW(spCtx->spException, caBuf);
        }
    }
    fprintf(spFile, "# APG CONFIGURATION FILE\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# Comments begin with \"#\" and continue to end of line\n");
    fprintf(spFile, "# Blank lines are ignored\n");
    fprintf(spFile, "# Options must begin on first character of a line and must not contain spaces.\n");
    fprintf(spFile, "# File names must not contain spaces.\n");
    fprintf(spFile, "# Trailing white space after an option is stripped.\n");
    fprintf(spFile, "# APG command-line options, -c, --config-file=filename, @, and @filename are not allowed in configuration files.\n");
    fprintf(spFile, "# If present they will generate an error.\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# THE INPUT GRAMMAR\n");
    fprintf(spFile,
            "# The --input option is used to specify the input grammar file.\n");
    fprintf(spFile, "# The file name may be absolute (/home/user/dir) or relative (../backone/dir)\n");
    fprintf(spFile, "# If multiple --input parameters are specified the named files \n");
    fprintf(spFile, "# will be concatenated into a single input grammar file in the order presented\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "#--input=grammar.bnf\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# THE OUTPUT, GENERATED C-LANGUAGE FILES\n");
    fprintf(spFile, "# This option names names the output files, filename.h & filename.c.\n");
    fprintf(spFile, "# The extension, if any, will be stripped and \".c\" added for the source file and \".h\" added for the header file\n");
    fprintf(spFile, "# The file name may be absolute (/home/user/dir) or relative (../backone/dir)\n");
    fprintf(spFile, "# If no --output option exists, no output is generated.\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "#--output=filename\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# ANNOTATED GRAMMAR IN HTML\n");
    fprintf(spFile, "# Output the annotated grammar in HTML format to filename\n");
    fprintf(spFile, "# (file name should have .html file extension)\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "#--grammar-html=filename\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# RULE/UDT NAMES AND DEPENDENCIES IN HTML\n");
    fprintf(spFile, "# Output the rule/UDT names and dependencies in HTML format to filename\n");
    fprintf(spFile, "# (file name should have .html file extension)\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "#--rules-html=filename\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# LINE ENDING CONVERSIONS\n");
    fprintf(spFile, "# Convert the grammar's line endings to LF(linefeed, \n, 0x0A) to filename\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "#--lf=filename\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# Convert the grammar's line endings to CRLF(carriage return + linefeed, \r\n, 0x0D0A) to filename\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "#--crlf=filename\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# THE HELP SCREEN\n");
    fprintf(spFile, "# If present this option will display a usage or help screen and quit.\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "#--help\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# THE VERSION NUMBER AND COPYRIGHT\n");
    fprintf(spFile, "# If present this option will display the version number and copyright and quit.\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "#--version\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# STRICT ABNF\n");
    fprintf(spFile, "# If the strict flag is set, the input grammar must conform strictly to ABNF as\n");
    fprintf(spFile, "# defined in RFCs 5234 & 7405.\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "#--strict\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# IGNORE ATTRIBUTES\n");
    fprintf(spFile, "# If this flag is set, the input grammar attribute calculation will be skipped.\n");
    fprintf(spFile, "# The generator will proceed to output a parser whether there are attribute errors are not.\n");
    fprintf(spFile, "# Proceed at your own risk, or only if you know from previous runs that the attributes are OK.\n");
    fprintf(spFile, "# NOTE: rule/UDT dependencies will not be available if this option is chosen.\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "#--ignore-attributes\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# IGNORE PPPT\n");
    fprintf(spFile, "# If this flag is set, the Partially-Predictive Parsing Tables (PPPTs) calculation will be skipped.\n");
    fprintf(spFile, "# If set, best to compile parsing applications with the macro APG_NO_PPPT defined.\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "#--no-pppt\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# PROTECTED RULES\n");
    fprintf(spFile, "# This option allows for a list of rule names to be protected from being hidden under fully-predictive\n");
    fprintf(spFile, "# PPPT-mapped nodes in the parse tree. The argument may be a comma-delimited list.\n");
    fprintf(spFile, "# Multiple instances of the --p-rules option will accumulate rule names to the list.\n");
    fprintf(spFile, "# Rule names are case insensitive.\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "#--p-rules=rule[,rule[,rule]...]\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# DISPLAY OPTIONS\n");
    fprintf(spFile, "# Display option all begin with \"d\"\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# verbose display, turn on the flags, -dc, -dg, dr, and -da\n");
    fprintf(spFile, "#-dv\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# Display the input grammar with line numbers and explicit control characters, \\t, \\n & \\r.\n");
    fprintf(spFile, "# as ASCII on the stream stdout.\n");
    fprintf(spFile, "#-dg\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# Displays the full contents of the final configuration file.\n");
    fprintf(spFile, "#-dc\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# Display the input grammar with line numbers and explicit control characters, \\t, \\n & \\r.\n");
    fprintf(spFile, "# as HTML on the file filename.\n");
    fprintf(spFile, "#--display-html=filename\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# Display a list of all of the rules, in the order they are found in the grammar.\n");
    fprintf(spFile, "#-dr\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# Display a list of all of the rules, in alphabetical order.\n");
    fprintf(spFile, "#-dra\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# Display the rule attributes.\n");
    fprintf(spFile, "#-da\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# Display the Partially-Predictive Parsing Table (PPPT) sizes.\n");
    fprintf(spFile, "#-dp\n");
    fprintf(spFile, "#\n");
    fprintf(spFile, "# Display the opcodes for each rule in human-readable form (warning: may generate lots of lines).\n");
    fprintf(spFile, "#-do\n");
    fprintf(spFile, "#\n");
    if(spFile != stdout){
        fclose(spFile);
    }
}

/** \brief Displays the full configuration as determined from the command line or command file arguments.
 *
 * After all options have been read from the command line, or command file if present, the final
 * interpretation of them is stored in the configuration structure.
 * This function displays all of the arguments passed to the main function and the configuration that results from them.
 * It is called if the flag -dc is set.
 *
 * \param spConfig Pointer to the configuration structure.
 * \param iArgCount The number of option arguments.
 * \param cppArgs Array of pointers to the arguments
 * \return void
 */
void vConfigDisplay(const config* spConfig, int iArgCount, char** cppArgs){
    int i = 0;
    char* cpTrue = "TRUE";
    char* cpFalse = "FALSE";

    printf(" THE APG CONFIGURATION:\n");
    printf("  command line args(%d):", iArgCount);
    for(; i < iArgCount; i += 1){
        printf(" %s", cppArgs[i]);
    }
    printf("\n");
    printf("                   cwd: %s\n", spConfig->cpCwd);
    if(spConfig->uiInputFiles == 0){
        printf("            input file: \"none\"\n");
    }else if(spConfig->uiInputFiles == 1){
        printf("           input files: %s\n", spConfig->cppInput[0]);
    }else{
        printf("        input files(s):\n");
        for(i = 0; i < (int)(spConfig->uiInputFiles); i += 1){
            printf("                      %d. %s\n", (i+1), spConfig->cppInput[i]);
        }
    }
    if(spConfig->uiPRules == 0){
        printf("  protected rule names: \"none\"\n");
    }else if(spConfig->uiPRules == 1){
        printf("  protected rule names: %s\n", spConfig->cppPRules[0]);
    }else{
        printf("  protected rule names:\n");
        for(i = 0; i < (int)(spConfig->uiPRules); i += 1){
            printf("                      %d. %s\n", (i+1), spConfig->cppPRules[i]);
        }
    }
    if(spConfig->cpOutput){
        printf("       output files(s): %s\n", spConfig->cpOutput);
    }else{
        printf("      output path name: \"none\"\n");
    }
    if(!spConfig->cpLfOut){
        printf("     LF line ends file: \"none\"\n");
    }else{
        printf("     LF line ends file: %s\n", spConfig->cpLfOut);
    }
    if(!spConfig->cpCrLfOut){
        printf("   CRLF line ends file: \"none\"\n");
    }else{
        printf("   CRLF line ends file: %s\n", spConfig->cpCrLfOut);
    }
    if(!spConfig->cpGrammarHtml){
        printf("  grammar to html file: \"none\"\n");
    }else{
        printf("  grammar to html file: %s\n", spConfig->cpGrammarHtml);
    }
    if(!spConfig->cpRulesHtml){
        printf("rules/UDT to html file: \"none\"\n");
    }else{
        printf("rules/UDT to html file: %s\n", spConfig->cpRulesHtml);
    }
    if(spConfig->cpDefaultConfig){
        printf("   create default file: %s\n", spConfig->cpDefaultConfig);
    }else{
        printf("   create default file: no\n");
    }
    if(spConfig->cpUseConfig){
        printf("use configuration file: %s\n", spConfig->cpUseConfig);
    }else{
        printf("use configuration file: no\n");
    }
    printf("                --help: %s\n", (spConfig->bHelp ? cpTrue : cpFalse));
    printf("             --version: %s\n", (spConfig->bVersion ? cpTrue : cpFalse));
    printf("              --strict: %s\n", (spConfig->bStrict ? cpTrue : cpFalse));
    printf("             --no-pppt: %s\n", (spConfig->bNoPppt? cpTrue : cpFalse));
    printf("                   -dv: %s\n", (spConfig->bDv ? cpTrue : cpFalse));
    printf("                   -dc: %s\n", (spConfig->bDc ? cpTrue : cpFalse));
    printf("                   -dg: %s\n", (spConfig->bDg ? cpTrue : cpFalse));
    printf("                   -da: %s\n", (spConfig->bDa ? cpTrue : cpFalse));
    printf("                   -dr: %s\n", (spConfig->bDr ? cpTrue : cpFalse));
    printf("                   -dp: %s\n", (spConfig->bDp ? cpTrue : cpFalse));
    printf("                  -dra: %s\n", (spConfig->bDra ? cpTrue : cpFalse));
    printf("                   -do: %s\n", (spConfig->bDo ? cpTrue : cpFalse));
}

static void vGetArgs(config_ctx* spCtx, int iArgCount, char** cppArgs){
    aint uiStrLen;
    char cZero = 0;
    int iOption = 0;
    for (; iOption < iArgCount; iOption += 1) {
        uiStrLen = (aint) (strlen(cppArgs[iOption]) + 1);
        vpVecPushn(spCtx->vpVecArgs, cppArgs[iOption], uiStrLen);
    }
    vpVecPush(spCtx->vpVecArgs, &cZero);
    vExtractArgOptions(spCtx, (char*)vpVecFirst(spCtx->vpVecArgs));
}
static char* s_cpPos = NULL;
static char* cpGetFirstName(char* cpBegin, aint* uipLength){
    s_cpPos = cpBegin;
    aint uiOffset = 0;
    while(APG_TRUE){
        if(*s_cpPos == ','){
            s_cpPos++;
            break;
        }
        if(*s_cpPos == 0){
            s_cpPos = NULL;
            break;
        }
        s_cpPos++;
        uiOffset++;
    }
    *uipLength = uiOffset;
    return cpBegin;
}
static char* cpGetNextName(aint* uipLength){
    char* cpReturn = s_cpPos;
    aint uiOffset = 0;
    if(s_cpPos){
        while(APG_TRUE){
            if(*s_cpPos == ','){
                s_cpPos++;
                break;
            }
            if(*s_cpPos == 0){
                s_cpPos = NULL;
                break;
            }
            s_cpPos++;
            uiOffset++;
        }
    }
    *uipLength = uiOffset;
    return cpReturn;
}
static void vExtractArgOptions(config_ctx* spCtx, char* cpParams) {
    aint uiStrLen;
    char cZero = 0;
    int iOption = 1;

    // skip over the first argument
    cpParams = (char*)vpVecFirst(spCtx->vpVecArgs);
    uiStrLen = (aint) (strlen(cpParams) + 1);
    cpParams += uiStrLen;
    if (*cpParams == 0) {
        // no parameters, set help flag
        spCtx->bHelp = APG_TRUE;
        return;
    }
    while (*cpParams != 0) {
        uiStrLen = (aint) (strlen(cpParams) + 1);
        if (strcmp(cpParams, "-i") == 0) {
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
            if (*cpParams == 0) {
                XTHROW(spCtx->spException, "options error: -i has no following input file name");
            }
            uiStrLen = (aint) (strlen(cpParams) + 1);
            vpVecPushn(spCtx->vpVecInput, cpParams, uiStrLen);
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
            spCtx->uiInputFiles++;
        } else if (strncmp(cpParams, "--input=", 8) == 0) {
            uiStrLen = (aint) (strlen(&cpParams[8]) + 1);
            vpVecPushn(spCtx->vpVecInput, &cpParams[8], uiStrLen);
            spCtx->uiInputFiles++;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strncmp(cpParams, "--p-rules=", 10) == 0) {
            char cZero = 0;
            char* cpName = cpGetFirstName(&cpParams[10], &uiStrLen);
            vpVecPushn(spCtx->vpVecPRules, cpName, uiStrLen);
            vpVecPush(spCtx->vpVecPRules, &cZero);
            spCtx->uiPRules++;
            while((cpName = cpGetNextName(&uiStrLen))){
                vpVecPushn(spCtx->vpVecPRules, cpName, uiStrLen);
                vpVecPush(spCtx->vpVecPRules, &cZero);
                spCtx->uiPRules++;
            }
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-o") == 0) {
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
            if (*cpParams == 0) {
                XTHROW(spCtx->spException, "options error: -o has no following output file name");
            }
            uiStrLen = (aint) (strlen(cpParams) + 1);
            vpVecPushn(spCtx->vpVecOutput, cpParams, uiStrLen);
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strncmp(cpParams, "--output=", 9) == 0) {
            uiStrLen = (aint) (strlen(&cpParams[9]) + 1);
            vpVecPushn(spCtx->vpVecOutput, &cpParams[9], uiStrLen);
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strncmp(cpParams, "--grammar-html=", 15) == 0) {
            uiStrLen = (aint) (strlen(&cpParams[15]) + 1);
            vpVecPushn(spCtx->vpVecHtmlOut, &cpParams[15], uiStrLen);
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strncmp(cpParams, "--rules-html=", 13) == 0) {
            uiStrLen = (aint) (strlen(&cpParams[13]) + 1);
            vpVecPushn(spCtx->vpVecRulesHtmlOut, &cpParams[13], uiStrLen);
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strncmp(cpParams, "--lf=", 5) == 0) {
            uiStrLen = (aint) (strlen(&cpParams[5]) + 1);
            vpVecPushn(spCtx->vpVecLfOut, &cpParams[5], uiStrLen);
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strncmp(cpParams, "--crlf=", 7) == 0) {
            uiStrLen = (aint) (strlen(&cpParams[7]) + 1);
            vpVecPushn(spCtx->vpVecCrLfOut, &cpParams[7], uiStrLen);
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-c") == 0) {
            uiStrLen = (aint) (strlen(CONFIG_FILE) + 1);
            vpVecPushn(spCtx->vpVecConfigOut, CONFIG_FILE, uiStrLen);
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strncmp(cpParams, "--config-file=", 13) == 0) {
            if (cpParams[13] == '='){
                uiStrLen = (aint) (strlen(&cpParams[14]) + 1);
                vpVecPushn(spCtx->vpVecConfigOut, &cpParams[14], uiStrLen);
            } else {
                vpVecPushn(spCtx->vpVecConfigOut, CONFIG_FILE, uiStrLen);
            }
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (cpParams[0] == '@') {
            if (cpParams[1] == 0) {
                uiStrLen = (aint) (strlen(CONFIG_FILE) + 1);
                vpVecPushn(spCtx->vpVecConfigIn, CONFIG_FILE, uiStrLen);
            } else {
                uiStrLen = (aint) (strlen(&cpParams[1]) + 1);
                vpVecPushn(spCtx->vpVecConfigIn, &cpParams[1], uiStrLen);
            }
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-v") == 0) {
            spCtx->bVersion = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "--version") == 0) {
            spCtx->bVersion = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "?") == 0) {
            spCtx->bHelp = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-h") == 0) {
            spCtx->bHelp = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "--help") == 0) {
            spCtx->bHelp = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-s") == 0) {
            spCtx->bStrict = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "--strict") == 0) {
            spCtx->bStrict = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "--ignore-attributes") == 0) {
            spCtx->bIgnoreAttrs = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "--no-pppt") == 0) {
            spCtx->bNoPppt = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-dra") == 0) {
            spCtx->bDra = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-dr") == 0) {
            spCtx->bDr = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-dg") == 0) {
            spCtx->bDg = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-da") == 0) {
            spCtx->bDa = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-dc") == 0) {
            spCtx->bDc = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-do") == 0) {
            spCtx->bDo = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-dp") == 0) {
            spCtx->bDp = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else if (strcmp(cpParams, "-dv") == 0) {
            spCtx->bDv = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        } else {
            printf("unrecognized option[%d]: %s\n", iOption, cpParams);
            spCtx->bHelp = APG_TRUE;
            uiStrLen = (aint) (strlen(cpParams) + 1);
            cpParams += uiStrLen;
        }
        iOption++;
    }
    // push the final null-term on the string of input file names
    vpVecPush(spCtx->vpVecInput, &cZero);

    // get the current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        vpVecPushn(spCtx->vpVecCwd, cwd, (aint)(strlen(cwd) + 1));
    } else {
        XTHROW(spCtx->spException, "getcwd() failed\n");
    }

    if(spCtx->bDv){
        spCtx->bDg = APG_TRUE;
        spCtx->bDr = APG_TRUE;
        spCtx->bDc = APG_TRUE;
        spCtx->bDp = APG_TRUE;
        spCtx->bDa = APG_TRUE;
    }
}

static void vExtractFileOptions(config_ctx* spCtx){
    void* vpVec = spCtx->vpVecArgs;
    char cZero = 0;
    char c;
    int iChar;
    aint uiNewLine = 1;
    aint uiLineEnd = 2;
    aint uiComment = 3;
    aint uiOption = 4;
    aint uiState = uiNewLine;
    char* cpIn;
    char* cpHelp = "--help";
    char* cpOption;
    aint uiSize;
    aint uiOptionCount;
    char* cpFirst = "args from configuration file";
    char cHyphen = 45;
    char cLF = 10;
    char cCR = 13;
    char cLB = 35;
    char cSP = 32;
    char cTAB = 9;
    char caBuf[128];

    vVecClear(vpVec);
    cpIn = (char*)vpVecFirst(spCtx->vpVecConfigIn);
    if(!cpIn){
        snprintf(caBuf, 128, "%s\nno input configuration file name", s_cpCommandLineError);
        XTHROW(spCtx->spException, caBuf);
    }
    spCtx->spConfigFile = fopen(cpIn, "rb");
    if(!spCtx->spConfigFile){
        snprintf(caBuf, 128, "%s\nunable to open configuration file: %s", s_cpCommandLineError, cpIn);
        XTHROW(spCtx->spException, caBuf);
    }
    uiSize = (aint) (strlen(cpFirst) + 1);
    vpVecPushn(vpVec, (void*)cpFirst, uiSize);
    while((iChar = fgetc(spCtx->spConfigFile)) != EOF){
        c = (char)iChar;
        if(uiState == uiOption){
            if(c == cLB || c == cSP || c == cTAB || c == cLF || c == cCR){
                vpVecPush(vpVec, &cZero);
                if(c == cLF || c == cCR){
                    uiState = uiLineEnd;
                }else{
                    uiState = uiComment;
                }
            }else{
                vpVecPush(vpVec, &c);
            }
        }else if(uiState == uiComment){
            if(c == cLF || c == cCR){
                uiState = uiLineEnd;
            }
        }else if( uiState == uiNewLine){
            if(c == cHyphen){
                uiState = uiOption;
                vpVecPush(vpVec, &c);
            }else  if(c == cLF || c == cCR){
                uiState = uiLineEnd;
            }else{
                uiState = uiComment;
            }
        }else if(uiState == uiLineEnd){
            if(!(c == cLF || c == cCR)){
                uiState = uiNewLine;
                if(c == cHyphen){
                    uiState = uiOption;
                    vpVecPush(vpVec, &c);
                }else{
                    uiState = uiComment;
                }
            }
        }
    }
    fclose(spCtx->spConfigFile);
    spCtx->spConfigFile = NULL;
    vpVecPush(vpVec, &cZero);

    // validate the options
    cpOption = (char*)vpVecFirst(vpVec);
    cpOption += (aint)(strlen(cpOption) + 1);
    uiOptionCount = 0;
    while(cpOption[0] != 0){
        while(APG_TRUE){
            if(strncmp(cpOption, "--output", 8) == 0){
                break;
            }
            if(strncmp(cpOption, "--input", 7) == 0){
                break;
            }
            if(strncmp(cpOption, "--p-rules", 7) == 0){
                break;
            }
            if(strncmp(cpOption, "--grammar-html", 14) == 0){
                break;
            }
            if(strncmp(cpOption, "--rules-html", 12) == 0){
                break;
            }
            if(strncmp(cpOption, "--lf", 4) == 0){
                break;
            }
            if(strncmp(cpOption, "--crlf", 4) == 0){
                break;
            }
            if(strcmp(cpOption, "--help") == 0){
                break;
            }
            if(strcmp(cpOption, "--version") == 0){
                break;
            }
            if(strcmp(cpOption, "--strict") == 0){
                break;
            }
            if(strcmp(cpOption, "--ignore-attributes") == 0){
                break;
            }
            if(strcmp(cpOption, "--no-pppt") == 0){
                break;
            }
            if(strcmp(cpOption, "-dc") == 0){
                break;
            }
            if(strcmp(cpOption, "-dv") == 0){
                break;
            }
            if(strcmp(cpOption, "-do") == 0){
                break;
            }
            if(strcmp(cpOption, "-dp") == 0){
                break;
            }
            if(strcmp(cpOption, "-dr") == 0){
                break;
            }
            if(strcmp(cpOption, "-dra") == 0){
                break;
            }
            if(strcmp(cpOption, "-dg") == 0){
                break;
            }
            if(strcmp(cpOption, "-da") == 0){
                break;
            }
            snprintf(caBuf, 128,
                    "%s\noption unrecognized or not allowed in configuration file: %s", s_cpCommandLineError, cpOption);
            XTHROW(spCtx->spException, caBuf);
        }
        cpOption += (aint)(strlen(cpOption) + 1);
        uiOptionCount++;
    }
    if(!uiOptionCount){
        vpVecPushn(vpVec, cpHelp, (aint)(strlen(cpHelp) + 1));
    }

    // put options in config_ctx
    vVecClear(spCtx->vpVecOutput);
    vVecClear(spCtx->vpVecInput);
    vVecClear(spCtx->vpVecGrammar);
    vVecClear(spCtx->vpVecConfigOut);
    vExtractArgOptions(spCtx, (char*)vpVecFirst(vpVec));
}

static config* spGetConfig(config_ctx* spCtx){
    config* spConfig = &spCtx->sConfig;
    spConfig->uiInputFiles = spCtx->uiInputFiles;
    spConfig->uiPRules = spCtx->uiPRules;
    spConfig->bHelp = spCtx->bHelp;
    spConfig->bVersion = spCtx->bVersion;
    spConfig->bStrict = spCtx->bStrict;
    spConfig->bIgnoreAttrs = spCtx->bIgnoreAttrs;
    spConfig->bNoPppt = spCtx->bNoPppt;
    spConfig->bDc = spCtx->bDc;
    spConfig->bDv = spCtx->bDv;
    spConfig->bDo = spCtx->bDo;
    spConfig->bDp = spCtx->bDp;
    spConfig->bDr = spCtx->bDr;
    spConfig->bDa = spCtx->bDa;
    spConfig->bDra = spCtx->bDra;
    spConfig->bDg = spCtx->bDg;
    spConfig->cpCwd = (char*)vpVecFirst(spCtx->vpVecCwd);
    spConfig->cpDefaultConfig = (char*)vpVecFirst(spCtx->vpVecConfigOut);
    spConfig->cpGrammarHtml= (char*)vpVecFirst(spCtx->vpVecHtmlOut);
    spConfig->cpRulesHtml= (char*)vpVecFirst(spCtx->vpVecRulesHtmlOut);
    spConfig->cpUseConfig = (char*)vpVecFirst(spCtx->vpVecConfigIn);
    spConfig->cpOutput = (char*)vpVecFirst(spCtx->vpVecOutput);
    spConfig->cpLfOut = (char*)vpVecFirst(spCtx->vpVecLfOut);
    spConfig->cpCrLfOut = (char*)vpVecFirst(spCtx->vpVecCrLfOut);
    vVecClear(spCtx->vpVecInputAddrs);
    vVecClear(spCtx->vpVecPRulesAddrs);
    aint ui = 0;
    char* cpName = (char*)vpVecFirst(spCtx->vpVecInput);
    for(; ui < spCtx->uiInputFiles; ui += 1){
        vpVecPush(spCtx->vpVecInputAddrs, (void*)&cpName);
        cpName += strlen(cpName) + 1;
    }
    spConfig->cppInput = (char**)vpVecFirst(spCtx->vpVecInputAddrs);
    cpName = (char*)vpVecFirst(spCtx->vpVecPRules);
    for(ui = 0; ui < spCtx->uiPRules; ui += 1){
        vpVecPush(spCtx->vpVecPRulesAddrs, (void*)&cpName);
        cpName += strlen(cpName) + 1;
    }
    spConfig->cppPRules = (char**)vpVecFirst(spCtx->vpVecPRulesAddrs);
    return spConfig;
}
