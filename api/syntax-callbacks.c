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
/** \file syntax-callbacks.c
 * \brief The callback functions called by the parser. These are the functions that find and report the syntax errors.
 */

#include "./api.h"
#include "./apip.h"
#include "../library/parserp.h"
#include "./syntax.h"
#include "sabnf-grammar.h"

// find the line number
// push an error message on the log stack
// if can't find the line, note error and abort
static void vSynPushError(callback_data* spCallbackData, aint uiCharIndex, char* cpMsg) {
    syntax_data* spData = (syntax_data*) spCallbackData->vpUserData;
    spData->uiErrorsFound++;
    vLineError(spData->spApi, uiCharIndex, "syntax", cpMsg);
    const char* cpLogMsg = cpMsgsFirst(spData->spApi->vpLog);
    const char* cpPrev = cpMsg;
    while(cpLogMsg){
        cpPrev = cpLogMsg;
        cpLogMsg = cpMsgsNext(spData->spApi->vpLog);
    }
    char caBuf[1024];
    snprintf(caBuf, 1024, "error found at character index: %"PRIuMAX"\n%s",
            (luint)uiCharIndex, cpPrev);
    XTHROW(spData->spApi->spException, caBuf);
}

static void vFile(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_ACTIVE) {
        spUserData->uiRulesFound = 0;
        spUserData->uiErrorsFound = 0;
        spUserData->spTopAlt = (alt_data*) vpVecPush(spUserData->vpAltStack, NULL);
        memset((void*) spUserData->spTopAlt, 0, sizeof(*spUserData->spTopAlt));
    } else if (spData->uiParserState == ID_MATCH) {
        if(!spUserData->uiRulesFound){
        vSynPushError(spData, spData->uiParserOffset, "no rules found - grammar must have at least one rule");
        }
    } else {
        vSynPushError(spData, spData->uiParserOffset, "NOMATCH found for file - should never happen");
    }
}
static void vRule(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_ACTIVE) {
        spUserData->uiRuleError = 0;
    } else if (spData->uiParserState == ID_MATCH) {
        spUserData->uiRulesFound++;
    }
}
static void vRuleError(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        spUserData->uiRuleError++;
        vSynPushError(spData, spData->uiParserOffset, "malformed rule found");
    }
}
static void vRuleNameError(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        spUserData->uiRuleError++;
        vSynPushError(spData, spData->uiParserOffset, "malformed rule name");
    }
}
static void vDefinedAsError(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        spUserData->uiRuleError++;
        vSynPushError(spData, spData->uiParserOffset, "malformed \"defined as\", must be \"=\" or \"=/\"");
    }
}
static void vAndOp(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        if (spUserData->bStrict) {
            spUserData->uiRuleError++;
            vSynPushError(spData, spData->uiParserOffset, "AND operator (&) found and strict ABNF specified");
        }
    }
}
static void vNotOp(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        if (spUserData->bStrict) {
            spUserData->uiRuleError++;
            vSynPushError(spData, spData->uiParserOffset, "NOT operator (!) found and strict ABNF specified");
        }
    }
}
static void vBkaOp(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        if (spUserData->bStrict) {
            spUserData->uiRuleError++;
            vSynPushError(spData, spData->uiParserOffset,
                    "positive look behind operator (&&) found and strict ABNF specified");
        }
    }
}
static void vBknOp(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        if (spUserData->bStrict) {
            spUserData->uiRuleError++;
            vSynPushError(spData, spData->uiParserOffset,
                    "negative look behind operator (!!) found and strict ABNF specified");
        }
    }
}
static void vAbgOp(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        if (spUserData->bStrict) {
            spUserData->uiRuleError++;
            vSynPushError(spData, spData->uiParserOffset,
                    "begin of line anchor operator (%^) found and strict ABNF specified");
        }
    }
}
static void vAenOp(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        if (spUserData->bStrict) {
            spUserData->uiRuleError++;
            vSynPushError(spData, spData->uiParserOffset,
                    "end of line anchor operator (%$) found and strict ABNF specified");
        }
    }
}
static void vBkrOp(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        if (spUserData->bStrict) {
            spUserData->uiRuleError++;
            vSynPushError(spData, spData->uiParserOffset,
                    "back reference operator (\\rulename or \\udtname) found and strict ABNF specified");
        }
    }
}
static void vUdtOp(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        if (spUserData->bStrict) {
            spUserData->uiRuleError++;
            vSynPushError(spData, spData->uiParserOffset,
                    "user-defined terminal operator (u_name or e_name) found and strict ABNF specified");
        }
    }
}
static void vTlsOpen(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        spUserData->spTopAlt->uiTlsOpen = spData->uiParserOffset;
    }
}
static void vTlsString(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        if (spUserData->spTopAlt->uiStringTab) {
            spUserData->uiRuleError++;
            vSynPushError(spData, spData->uiParserOffset,
                    "tab (\\t or 0x09) not allowed in case-insensitive literal string (see RFC 5234, char-val)");
            spUserData->spTopAlt->uiStringTab = APG_FALSE;
        }
    }
}
static void vStringTab(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        spUserData->spTopAlt->uiStringTab = APG_TRUE;
    }
}
static void vTlsClose(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        spUserData->spTopAlt->uiTlsOpen = APG_FALSE;
    } else if (spData->uiParserState == ID_NOMATCH) {
        spUserData->uiRuleError++;
        vSynPushError(spData, spData->uiParserOffset,
                "expected open case-insensitive literal string closure not found");
    }
}
static void vClsOpen(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        spUserData->spTopAlt->uiClsOpen = spData->uiParserOffset;
        if (spUserData->bStrict) {
            spUserData->uiRuleError++;
            vSynPushError(spData, spData->uiParserOffset,
                    "case-sensitive literal string ('') found and strict ABNF specified");
        }
    }
}
static void vClsString(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        if (spUserData->spTopAlt->uiStringTab) {
            spUserData->uiRuleError++;
            vSynPushError(spData, spData->uiParserOffset,
                    "tab (\\t or 0x09) not allowed in case-sensitive literal string (see RFC 5234, char-val)");
            spUserData->spTopAlt->uiStringTab = APG_FALSE;
        }
    }
}
static void vClsClose(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        spUserData->spTopAlt->uiClsOpen = APG_FALSE;
    } else if (spData->uiParserState == ID_NOMATCH) {
        spUserData->uiRuleError++;
        vSynPushError(spData, spData->uiParserOffset, "expected open case-sensitive literal string closure not found");
    }
}
static void vProseValOpen(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        spUserData->spTopAlt->uiProseValOpen = spData->uiParserOffset;
        vSynPushError(spData, spData->uiParserOffset,
                "prose-val found. Defined in RFC 5234 but cannot be parsed.)");
    }
}
static void vProseValString(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        if (spUserData->spTopAlt->uiStringTab) {
            spUserData->uiRuleError++;
            vSynPushError(spData, spData->uiParserOffset,
                    "tab (\\t or 0x09) not allowed in prose value (see RFC 5234, prose-val)");
            spUserData->spTopAlt->uiStringTab = APG_FALSE;
        }
    }
}
static void vProseValClose(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        spUserData->spTopAlt->uiProseValOpen = APG_FALSE;
    } else if (spData->uiParserState == ID_NOMATCH) {
        spUserData->uiRuleError++;
        vSynPushError(spData, spData->uiParserOffset, "expected open prose value closure not found");
    }
}
static void vGroupOpen(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        spUserData->spTopAlt = (alt_data*) vpVecPush(spUserData->vpAltStack, NULL);
        memset((void*) spUserData->spTopAlt, 0, sizeof(*spUserData->spTopAlt));
        spUserData->spTopAlt->uiGroupOpen = spData->uiParserOffset;
    }
}
static void vGroupClose(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_NOMATCH) {
        vSynPushError(spData, spData->uiParserOffset, "open group closure expected but not found");
        vpVecPop(spUserData->vpAltStack);
        spUserData->spTopAlt = vpVecFirst(spUserData->vpAltStack);
        spUserData->uiRuleError++;
    } else if (spData->uiParserState == ID_MATCH) {
        vpVecPop(spUserData->vpAltStack);
        spUserData->spTopAlt = vpVecFirst(spUserData->vpAltStack);
    }
}
static void vOptionOpen(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    spUserData->spTopAlt = (alt_data*) vpVecPush(spUserData->vpAltStack, NULL);
    memset((void*) spUserData->spTopAlt, 0, sizeof(*spUserData->spTopAlt));
    spUserData->spTopAlt->uiOptionOpen = spData->uiParserOffset;
}
static void vOptionClose(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_NOMATCH) {
        vSynPushError(spData, spData->uiParserOffset, "open option closure expected but not found");
        vpVecPop(spUserData->vpAltStack);
        spUserData->spTopAlt = vpVecFirst(spUserData->vpAltStack);
        spUserData->uiRuleError++;
    } else if (spData->uiParserState == ID_MATCH) {
        vpVecPop(spUserData->vpAltStack);
        spUserData->spTopAlt = vpVecFirst(spUserData->vpAltStack);
    }
}
static void vBasicElementError(callback_data* spData) {
    syntax_data* spUserData = (syntax_data*) spData->vpUserData;
    if (spData->uiParserState == ID_MATCH) {
        if (!spUserData->uiRuleError++) {
            vSynPushError(spData, spData->uiParserOffset, "malformed element found");
        }
    }
}
static void vLineEndError(callback_data* spData) {
    if (spData->uiParserState == ID_MATCH) {
        vSynPushError(spData, spData->uiParserOffset, "malformed element found");
        vSynPushError(spData, spData->uiParserOffset,
                "line end expected not found, possibly some extraneous after alternation and before line end");
    }
}
/** \brief Set the parser's rule callback functions for the syntax phase.
 * \param vpParserCtx Pointer to the parser's context.
 */
void vSabnfGrammarRuleCallbacks(void* vpParserCtx) {
    aint ui;
    parser_callback cb[RULE_COUNT_SABNF_GRAMMAR];
    cb[SABNF_GRAMMAR_ABGOP] = vAbgOp;
    cb[SABNF_GRAMMAR_AENOP] = vAenOp;
    cb[SABNF_GRAMMAR_ALPHANUM] = NULL;
    cb[SABNF_GRAMMAR_ALTERNATION] = NULL;
    cb[SABNF_GRAMMAR_ALTOP] = NULL;
    cb[SABNF_GRAMMAR_ANDOP] = vAndOp;
    cb[SABNF_GRAMMAR_BASICELEMENT] = NULL;
    cb[SABNF_GRAMMAR_BASICELEMENTERR] = vBasicElementError;
    cb[SABNF_GRAMMAR_BIN] = NULL;
    cb[SABNF_GRAMMAR_BKAOP] = vBkaOp;
    cb[SABNF_GRAMMAR_BKNOP] = vBknOp;
    cb[SABNF_GRAMMAR_BKR_NAME] = NULL;
    cb[SABNF_GRAMMAR_BKRMODIFIER] = NULL;
    cb[SABNF_GRAMMAR_BKROP] = vBkrOp;
    cb[SABNF_GRAMMAR_BLANKLINE] = NULL;
    cb[SABNF_GRAMMAR_BMAX] = NULL;
    cb[SABNF_GRAMMAR_BMIN] = NULL;
    cb[SABNF_GRAMMAR_BNUM] = NULL;
    cb[SABNF_GRAMMAR_BSTRING] = NULL;
    cb[SABNF_GRAMMAR_CATOP] = NULL;
    cb[SABNF_GRAMMAR_CI] = NULL;
    cb[SABNF_GRAMMAR_CLSCLOSE] = vClsClose;
    cb[SABNF_GRAMMAR_CLSOP] = NULL;
    cb[SABNF_GRAMMAR_CLSOPEN] = vClsOpen;
    cb[SABNF_GRAMMAR_CLSSTRING] = vClsString;
    cb[SABNF_GRAMMAR_COMMENT] = NULL;
    cb[SABNF_GRAMMAR_CONCATENATION] = NULL;
    cb[SABNF_GRAMMAR_CS] = NULL;
    cb[SABNF_GRAMMAR_DEC] = NULL;
    cb[SABNF_GRAMMAR_DEFINED] = NULL;
    cb[SABNF_GRAMMAR_DEFINEDAS] = NULL;
    cb[SABNF_GRAMMAR_DEFINEDASERROR] = vDefinedAsError;
    cb[SABNF_GRAMMAR_DEFINEDASTEST] = NULL;
    cb[SABNF_GRAMMAR_DMAX] = NULL;
    cb[SABNF_GRAMMAR_DMIN] = NULL;
    cb[SABNF_GRAMMAR_DNUM] = NULL;
    cb[SABNF_GRAMMAR_DSTRING] = NULL;
    cb[SABNF_GRAMMAR_ENAME] = NULL;
    cb[SABNF_GRAMMAR_FILE] = vFile;
    cb[SABNF_GRAMMAR_GROUP] = NULL;
    cb[SABNF_GRAMMAR_GROUPCLOSE] = vGroupClose;
    cb[SABNF_GRAMMAR_GROUPERROR] = NULL;
    cb[SABNF_GRAMMAR_GROUPOPEN] = vGroupOpen;
    cb[SABNF_GRAMMAR_HEX] = NULL;
    cb[SABNF_GRAMMAR_INCALT] = NULL;
    cb[SABNF_GRAMMAR_LINECONTINUE] = NULL;
    cb[SABNF_GRAMMAR_LINEEND] = NULL;
    cb[SABNF_GRAMMAR_LINEENDERROR] = vLineEndError;
    cb[SABNF_GRAMMAR_MODIFIER] = NULL;
    cb[SABNF_GRAMMAR_NOTOP] = vNotOp;
    cb[SABNF_GRAMMAR_OPTION] = NULL;
    cb[SABNF_GRAMMAR_OPTIONCLOSE] = vOptionClose;
    cb[SABNF_GRAMMAR_OPTIONERROR] = NULL;
    cb[SABNF_GRAMMAR_OPTIONOPEN] = vOptionOpen;
    cb[SABNF_GRAMMAR_OWSP] = NULL;
    cb[SABNF_GRAMMAR_PM] = NULL;
    cb[SABNF_GRAMMAR_PREDICATE] = NULL;
    cb[SABNF_GRAMMAR_PROSVAL] = NULL;
    cb[SABNF_GRAMMAR_PROSVALCLOSE] = vProseValClose;
    cb[SABNF_GRAMMAR_PROSVALOPEN] = vProseValOpen;
    cb[SABNF_GRAMMAR_PROSVALSTRING] = vProseValString;
    cb[SABNF_GRAMMAR_REP_MAX] = NULL;
    cb[SABNF_GRAMMAR_REP_MIN] = NULL;
    cb[SABNF_GRAMMAR_REP_MIN_MAX] = NULL;
    cb[SABNF_GRAMMAR_REP_NUM] = NULL;
    cb[SABNF_GRAMMAR_REPETITION] = NULL;
    cb[SABNF_GRAMMAR_REPOP] = NULL;
    cb[SABNF_GRAMMAR_RNAME] = NULL;
    cb[SABNF_GRAMMAR_RNMOP] = NULL;
    cb[SABNF_GRAMMAR_RULE] = vRule;
    cb[SABNF_GRAMMAR_RULEERROR] = vRuleError;
    cb[SABNF_GRAMMAR_RULELOOKUP] = NULL;
    cb[SABNF_GRAMMAR_RULENAME] = NULL;
    cb[SABNF_GRAMMAR_RULENAMEERROR] = vRuleNameError;
    cb[SABNF_GRAMMAR_RULENAMETEST] = NULL;
    cb[SABNF_GRAMMAR_SPACE] = NULL;
    cb[SABNF_GRAMMAR_STRINGTAB] = vStringTab;
    cb[SABNF_GRAMMAR_TBSOP] = NULL;
    cb[SABNF_GRAMMAR_TLSCASE] = NULL;
    cb[SABNF_GRAMMAR_TLSCLOSE] = vTlsClose;
    cb[SABNF_GRAMMAR_TLSOP] = NULL;
    cb[SABNF_GRAMMAR_TLSOPEN] = vTlsOpen;
    cb[SABNF_GRAMMAR_TLSSTRING] = vTlsString;
    cb[SABNF_GRAMMAR_TRGOP] = NULL;
    cb[SABNF_GRAMMAR_UDT_EMPTY] = NULL;
    cb[SABNF_GRAMMAR_UDT_NON_EMPTY] = NULL;
    cb[SABNF_GRAMMAR_UDTOP] = vUdtOp;
    cb[SABNF_GRAMMAR_UM] = NULL;
    cb[SABNF_GRAMMAR_UNAME] = NULL;
    cb[SABNF_GRAMMAR_WSP] = NULL;
    cb[SABNF_GRAMMAR_XMAX] = NULL;
    cb[SABNF_GRAMMAR_XMIN] = NULL;
    cb[SABNF_GRAMMAR_XNUM] = NULL;
    cb[SABNF_GRAMMAR_XSTRING] = NULL;
    for (ui = 0; ui < (aint) RULE_COUNT_SABNF_GRAMMAR; ui += 1) {
        vParserSetRuleCallback(vpParserCtx, ui, cb[ui]);
    }
}
