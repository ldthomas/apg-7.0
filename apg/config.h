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
/** \file config.h
 * \brief The configuration object header file.
 *
 * This file and \ref config.c define the configuration of the APG command line application.
 * Run ./apg --help to see all options.
 * <pre>
usage: apg options
binary options:
-i filename           : the file name of the input grammar (see note 5.)
-o filename           : the file name of the generated C-language header and source files (see note 6.)

unary options:
--input=filename      : the file name of the input grammar (see note 5.)
--output=filename     : the file name of the generated C-language header and source files (see note 6.)
-c                    : generate a default configuration file named "apg-configuration"
--config-file=fname   : generate a default configuration file named "fname"
--p-rules=name[,name] : comma-delimited list of protected rule names (see note 9.)
--grammar-html=fname  : display input grammar in HTML format on file "fname"
--rules-html=fname    : display rule/UDT names and dependencies in HTML format on file "fname"
--lf=filename         : converts all input line end to LF(\\n) on file "filename" (see note 8.)
--crlf=filename       : converts all input line end to CRLF(\\r\\n) on file "filename" (see note 8.)
@                     : read the options from the configuration file named "apg-configuration"
@filename             : read the options from the configuration file named "filename"

flags: true if present, otherwise false
?                     : display this help screen
-h                    : display this help screen
--help                : display this help screen
-v                    : display version information
--version             : display version information
--strict              : only ABNF as strictly defined in RFC 5234 allowed
--ignore-attributes   : attribute information will not be computed, proceed at your own risk
--no-pppt             : do not produce Partially-Predictive Parsing Tables (PPPTs)

display flags
-dv                   : verbose - sets flags -dc, -dg, dr, -dp and -da
-dc                   : display the complete configuration found on the command line or in the command file
-dg                   : display an annotated version of the input grammar
-dr                   : display the grammar rule names, in the order they are found in the grammar
-dra                  : display the grammar rule names, in alphabetical order
-da                   : display the rule attributes
-dp                   : display the Partially-Predictive Parsing Table (PPPT) sizes
-do                   : display the opcodes in human-readable format (warning: may generate many lines of output)

NOTES:
1. All options and flags are case sensitive.
2. Binary options require one or more spaces between the flag and the name.
3. No spaces are allowed in unary options or flags (no space before or after "=").
4. If any or all of -h, -v or -c (or any of their alternatives) is present all other options are ignored.
5. File names may be absolute (/home/user/filname) or relative ([./ | ../]mydir/filename).
   Directories in the path name must exist.
6. Any file name extension will be stripped and replaced with .h for the header file and .c for the source file.
7. Absent -h, -v or -c, if a configuration file is indicated (@ or @filename) all other options are ignored.
8. Both --lf and --crlf may be present. If either is present, all other options except -h, -v and -c are ignored.
9. Protected rules are protected from being hidden under predictive PPPT-mapped nodes.
   Rule names are case insensitive. The argument may be a comma-delimited list with no spaces allowed.
   Multiple instances of the --p-rules flag will accumulate rule names in the list.
10. No command line arguments generates this help screen.
 * </pre>
 */
#ifndef APG_CONFIG_H_
#define APG_CONFIG_H_

/** \def CONFIG_FILE
 * \brief The default file name for generated configuration files.
 *
 * The command line option "-c" can be used to generate a default configuration file named CONFIG_FILE.
 * To customize the configuration file name use the command line option "--config-file=filename" instead.
 */
#define CONFIG_FILE "apg-configuration"

/** \struct config
 * \brief This data controls the flow of the main program of the APG parser generator.
 *
 * The input command line or file is parsed to fill in this data.
 */
typedef struct {
    char* cpCwd; /**< \brief the current working directory */
    char* cpDefaultConfig; /**< \brief if non-n=NULL, generate a default configuration file using this file name */
    char* cpUseConfig; /**< \brief if non-n=NULL, use this configuration file instead of command line arguments */
    char* cpOutput; /**< \brief the path name for the generated C source & header files */
    char* cpGrammarHtml; /**< \brief if non-null, the file name for the HTML version of the annotated input grammar */
    char* cpRulesHtml; /**< \brief if non-null, the file name for the HTML version of the rule/UDT names and dependencies */
    char* cpLfOut; /**< \brief if non-null, the file name for the converted LF line ends file */
    char* cpCrLfOut; /**< \brief if non-null, the file name for the converted CRLF line ends file */
    char** cppInput; /**< \brief array of uiInputFiles input file names */
    aint uiInputFiles; /**< \brief the number of input files found */
    char** cppPRules; /**< \brief array of protected rule names  */
    aint uiPRules; /**< \brief the number of protected rule names found */
    abool bHelp; /**< \brief the help flag, if set the help screen is printed and processing stops */
    abool bVersion; /**< \brief the version flag, if set the version number is printed and processing stops */
    abool bStrict; /**< \brief if set, the grammar is treated as strict ABNF */
    abool bNoPppt; /**< \brief if set, Partially-Predictive Parsing Tables (PPPTs) will not be produced */
    abool bDv; /**< \brief verobose - sets options -dc, -dg, -dr, and -da  */
    abool bDc; /**< \brief display the complete configuration as found on command line or configuration file */
    abool bDg; /**< \brief display an annotated version of the input grammar */
    abool bDa; /**< \brief display grammar attributes */
    abool bDr; /**< \brief display grammar rule/UDT names in the order they occur in the grammar*/
    abool bDra; /**< \brief display the grammar rule/UDT names alphabetically */
    abool bDo; /**< \brief display the opcodes for each rule in human-readable form */
    abool bDp; ///< \brief display the PPPT size
} config;

void* vpConfigCtor(exception* spEx);
void vConfigDtor(void* vpCtx);
const config* spConfigOptions(void* vpCtx, int iArgCount, char** cppArgs);
void vConfigDefault(void* vpCtx, char* cpFileName);
void vConfigHelp(void);
void vConfigVersion(void);
void vConfigDisplay(const config* spConfig, int iArgCount, char** cppArgs);
#endif /* APG_CONFIG_H_ */
