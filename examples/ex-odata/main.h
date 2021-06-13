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
#ifndef MAIN_H_
#define MAIN_H_

#include <limits.h>
#include <stdio.h>
#include <setjmp.h>

#include "../../library/lib.h"
#include "../../utilities/utilities.h"
#include "../../json/json.h"
#include "../../xml/xml.h"
#include "./odata.h"

typedef struct  {
    aint uiIndex;
    aint uiLength;
} data_id;

typedef struct  {
    data_id sName;
    data_id sContent;
    data_id sRule;
    aint uiRuleId;
    aint uiFailAt;
    abool bFail;
} test;

typedef struct{
    aint uiOffset;
    aint uiCount;
    aint uiRuleIndex;
    const char* cpRuleName;
} rule_constraint;

typedef struct{
    void* vpIt;
    abool bTrace;
} user_data;

typedef struct{
    exception* spException;
    void* vpMem;
    void* vpOdataParser;
    void* vpVec32;
    void* vpVecTests;
    void* vpVecContraintRules;
    void* vpVecConstraints;
    char* cpXmlName;
    char* cpJsonName;
    u32_phrase* spTestCase;
    u32_phrase* spConstraint;
    u32_phrase* spMatch;
    u32_phrase* spInput;
    u32_phrase* spName;
    u32_phrase* spRule;
    u32_phrase* spFailAt;
    test* spCurrentTest;
    rule_constraint* spCurrentConstraint;
    char caBuf[128];
    aint uiRuleCount;
}xml_context;


#endif /* MAIN_H_ */
