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
#ifndef EXCEPTION_H_
#define EXCEPTION_H_

/** \file library/exception.h
 * \brief Structures and macros for exception handling.
 */

/** \def BAD_CONTEXT
 * \brief Application exit code when no exception context is available.
 *
 * Applications will exit with this error code if the user presents an object member function with a bad context pointer.
 */
#define BAD_CONTEXT 99

/** struct exception
 * \brief A structure to describe the type and location of a caught exception.
 */
typedef struct{
    const void* vpValidate; ///< \brief Used by the memory object to validate the exception structure.
    abool try; ///< \brief True for the try block, false for the catch block.
    jmp_buf aJmpBuf; ///< \brief A "long jump" context array. Long jumps to "catch" area on thrown exception.
    unsigned int uiLine; ///< \brief The source code line number where the error occurred. "__LINE__"
    char caMsg[256]; ///< \brief A the caller's error message.
    char caFile[256]; ///< \brief The source code file name where the error occurred. "__FILE__"
    char caFunc[64]; ///< \brief The source code function name where the error occurred. "__func__"
} exception;

/** \def XTHROW(ctx, msg)
 * \brief Exception throw macro.
 *
 * With this single-line macro, the user can specify an error-specific
 * message and the macro will fill in the file, function and line information
 * and call the function to excecute the throw.
 *
 * \param ctx Pointer to a valid memory context previously returned from vpMemCtor().
 * \param msg Pointer to an error message.
 */
#define XTHROW(ctx, msg) vExThrow((ctx), (msg), __LINE__, __FILE__, __func__)

/** \def XCTOR(e)
 * \brief This macro will initialize an exception structure and prepare entry to the "try" block.
 *
 * For an example of usage with an explanation of its operation,
 * see the [example](\ref objects) in Appendix A.
 *
 * \param e Must be an exception structure, not a pointer.
 */
#define XCTOR(e) \
    vExCtor(&e);\
    if(setjmp(e.aJmpBuf) == 0){ \
        e.try = APG_TRUE; \
    }else{ \
        e.try = APG_FALSE; \
    }

void vExCtor(exception* spException);
abool bExValidate(exception* spException);
void vExThrow(exception* spException, const char *cpMsg, unsigned int iLine, const char *cpFile, const char *cpFunc);
void vExRethrow(exception* spMemFrom, exception* spMemTo);
void vExContext();

#endif /* EXCEPTION_H_ */
