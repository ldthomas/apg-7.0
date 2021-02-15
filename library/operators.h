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
/// \file operators.h
/// \brief Header file for the suite of the parser's operator functions.

#ifndef LIB_OPERATORSP_H_
#define LIB_OPERATORSP_H_
/** @name The Node Operation Functions
 * A function for each of the 15 different types of nodes defined by the SABNF grammar.
 *
 * These functions are for internal, parser use only. They are never to be called directly by the application.
 */
///@{
void vAlt(parser* spCtx, const opcode* spop);
void vCat(parser* spCtx, const opcode* spop);
void vRep(parser* spCtx, const opcode* spop);
void vRnm(parser* spCtx, const opcode* spop);
void vTrg(parser* spCtx, const opcode* spop);
void vTls(parser* spCtx, const opcode* spop);
void vTbs(parser* spCtx, const opcode* spop);
void vUdt(parser* spCtx, const opcode* spop);
void vAnd(parser* spCtx, const opcode* spop);
void vNot(parser* spCtx, const opcode* spop);
void vBkr(parser* spCtx, const opcode* spop);
void vBka(parser* spCtx, const opcode* spop);
void vBkn(parser* spCtx, const opcode* spop);
void vAbg(parser* spCtx, const opcode* spop);
void vAen(parser* spCtx, const opcode* spop);
///@}
#endif /* LIB_OPERATORSP_H_ */
