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
/** \file library/exception.c
 * \brief Exception handling functions.
 *
 * Almost all of APG's objects and other facilities throw exceptions back to the parent
 * scope to report fatal errors. The structure and macros in exception.h and the functions here
 * facilitate all of APG's exception handling features. For a quick usage demonstration
 * see the [example](\ref objects) in Appendix A.
 */

#include "./lib.h"

static const void* s_vpMagicNumber = (const void*)"exception";

static void vStrToBuf(const char *cpStr, char *cpBuf, size_t uiBufSize);

/** \brief Initialize an exception structure.
 *
 * Any attempt to use an exception structure that has not been initialized will cause
 * the application to silently exit with a \ref BAD_CONTEXT exit code.
 * Note that, despite the name, this function does not create the exception
 * it just initializes it. There is no corresponding destructor.

 * For complete preparation for the try and catch blocks, see XCTOR().
 *
 * \param spException pointer to an uninitialized exception structure.
 * \return The structure is cleared and initialized.
 */
void vExCtor(exception* spException){
    if(!spException){
        vExContext();
    }
    memset(spException, 0, sizeof(exception));
    spException->vpValidate = s_vpMagicNumber;
}

/** \brief Test an exception structure for validity.
 * \param spException pointer to an uninitialized exception structure.
 * \return True if the structure is valid (the result of vExCtor()).
 * False otherwise.
 */
abool bExValidate(exception* spException){
    if(spException && (spException->vpValidate == s_vpMagicNumber)){
        return APG_TRUE;
    }
    return APG_FALSE;
}

/** \brief Throws an exception.
 *
 * Uses `longjmp()` to transfer control from the try block to the application's catch block.
 * \param spEx A pointer to an exception structure previously initialized with vExCtor().
 * Silently exits with \ref BAD_CONTEXT exit code if not initialized.
 \param cpMsg The user's description of the error.
 \param uiLine The line number where the error occurred.
 \param cpFile The file name where the error occurred.
 \param cpFunc The function name where the error occurred.
 */
void vExThrow(exception* spEx, const char *cpMsg, unsigned int uiLine, const char *cpFile, const char *cpFunc) {
    if(spEx && (spEx->vpValidate == s_vpMagicNumber)){
        spEx->uiLine = uiLine;
        vStrToBuf(cpMsg, spEx->caMsg, sizeof(spEx->caMsg));
        vStrToBuf(cpFile, spEx->caFile, sizeof(spEx->caFile));
        vStrToBuf(cpFunc, spEx->caFunc, sizeof(spEx->caFunc));
        longjmp(spEx->aJmpBuf, 1);
    }
    vExContext();
}

/** \brief Re-throw an exception from the "from" try/catch-block scope to the "to" try/catch-block scope.
 *
 * \param spExFrom Pointer to the exception to copy from.
 * Silently exits with \ref BAD_CONTEXT exit code if not initialized.
 * \param spExTo Pointer to the exception to copy and throw to.
 * Silently exits with \ref BAD_CONTEXT exit code if not initialized.
 */
void vExRethrow(exception* spExFrom, exception* spExTo){
    if(bExValidate(spExFrom) && bExValidate(spExTo)){
        spExTo->uiLine = spExFrom->uiLine;
        vStrToBuf(spExFrom->caMsg, spExTo->caMsg, sizeof(spExTo->caMsg));
        vStrToBuf(spExFrom->caFile, spExTo->caFile, sizeof(spExTo->caFile));
        vStrToBuf(spExFrom->caFunc, spExTo->caFunc, sizeof(spExTo->caFunc));
        longjmp(spExTo->aJmpBuf, 1);
    }
    vExContext();
    return;
}

/** \brief Handles bad context pointers.
 *
 * When a bad context is passed to a member function, the function obviously has no knowledge of what
 * exception object to use for reporting. Therefore, the application silently exits with a \ref BAD_CONTEXT exit code.
 *
 * Debugging hint: If an application mysteriously exits, check the exit code. If it is \ref BAD_CONTEXT.
 * place a break point on this function and use the debugger's call stack to work backwards to the offending
 * call with the bad context pointer.
 */
void vExContext(){
    exit(BAD_CONTEXT);
}

static void vStrToBuf(const char *cpStr, char *cpBuf, size_t uiBufSize) {
    size_t ui = 0;
    size_t uiLen = strlen(cpStr);
    if (uiLen >= uiBufSize) {
        uiLen = uiBufSize - 1;
    }
    for (; ui < uiLen; ui++) {
        cpBuf[ui] = cpStr[ui];
    }
    cpBuf[ui] = 0;
}

