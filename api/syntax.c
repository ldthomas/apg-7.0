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
/** \file syntax.c
 * \brief Processes the syntax phase. Parses the grammar and reports any syntax errors.
 */

#include "./api.h"
#include "./apip.h"
#include "../library/parserp.h"
#include "./syntax.h"
#include "./semantics.h"
#include "sabnf-grammar.h"

// Define this macro to turn on tracing during the syntax parsing phase.
//#define SYNTAX_TRACE
#ifdef SYNTAX_TRACE
#endif /* SYNTAX_TRACE */

/** \brief Parse the SABNF grammar to validate that the grammar structure is valid.
 * \param vpCtx - Pointer to an API context previously returned from vpApiCtor().
 * \param bStrict If true, only strict RFC 5234 syntax is allowed.
 * If false, full SABNF syntax is allowed.
 */
void vApiSyntax(void* vpCtx, abool bStrict) {
    if(!bApiValidate(vpCtx)){
        vExContext();
    }
    api* spApi = (api*) vpCtx;
    parser_config sInput;
    parser_state sState;
    syntax_data sData;
    memset((void*) &sInput, 0, sizeof(sInput));
    memset((void*) &sState, 0, sizeof(sState));
    memset((void*) &sData, 0, sizeof(sData));
    // make sure the grammar has been validated
    if (!spApi->bInputValid) {
        XTHROW(spApi->spException, "attempted syntax phase but input grammar not validated");
    }
    if(spApi->bSyntaxValid){
        XTHROW(spApi->spException, "attempted syntax phase but syntax has already been validated)");
    }

    // construct the parser
    spApi->vpParser = vpParserCtor(spApi->spException, vpSabnfGrammarInit);

    // construct the AST
    spApi->vpAst = vpAstCtor(spApi->vpParser);

#ifdef SYNTAX_TRACE
    vpTraceCtor(spApi->vpParser, "a", "a");
#endif /* SYNTAX_TRACE */

    if (!spApi->cpInput || !spApi->uiInputLength) {
        XTHROW(spApi->spException, "expected input not found");
    }
    sInput.acpInput = (const achar*)spApi->cpInput;
    sInput.uiInputLength = spApi->uiInputLength;
    if(sizeof(achar) > sizeof(char)){
        vVecClear(spApi->vpVecGrammar);
        achar* acpInput = (achar*)vpVecPushn(spApi->vpVecGrammar, NULL, sInput.uiInputLength);
        aint ui = 0;
        for(; ui < sInput.uiInputLength; ui++){
            acpInput[ui] = (achar)((uint8_t)spApi->cpInput[ui]);
        }
        sInput.acpInput = (const achar*)acpInput;
    }
    sInput.uiStartRule = SABNF_GRAMMAR_FILE;

    // set up the parser configuration
    sData.spApi = spApi;
    sData.bStrict = bStrict;
    sData.vpAltStack = spApi->vpAltStack;
    sInput.vpUserData = (void*) &sData;

    // set the parser and AST callback functions
    vSabnfGrammarRuleCallbacks(spApi->vpParser);
    vSabnfGrammarAstCallbacks(spApi->vpAst);

    // parser the input grammar
    vParserParse(spApi->vpParser, &sInput, &sState);

    // check the parser's state
    if (!sState.uiSuccess) {
        XTHROW(spApi->spException, "syntax phase - parser failed");
    }

    // syntax success
    spApi->bSyntaxValid = APG_TRUE;
}


