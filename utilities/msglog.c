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
/** \file msglog.c
 * \brief A message logging object.
 *
 * This object is a simple, but consistent method for logging multiple messages,
 * whether error messages or warning messages or otherwise,
 * with an iterator for retrieving them.
 *
 * See vUtilPrintMsgs() for a utility that will display all messages in a message log object.
 */

#include "../library/lib.h"

static const void* s_vpMagicNumber = (void*)"msglog";

/** \struct msgs
 * \brief The message log context.
 *
 * For internal object use only.
 */
typedef struct {
    const void *vpValidate; ///< \brief A "magic" number to validate the context.
    exception* spException; ///< \brief Pointer to an exception structure to report fatal errors
                            /// back to the application's catch block.
    void *vpMem; ///< \brief Pointer to a memory object context for all memory allocations.
    aint uiMsgCount; ///< \brief The number of logged messages.
    void *vpVecMsgs; ///< \brief A vector to hold the logged messages.
    void *vpVecIndexes; ///< \brief A vector to hold the indexes of the logged messages in vpVecMsgs.
    aint uiNext; ///< \brief The next message in the message iterator.
} msgs;

/** \brief The Message Log constructor.
 *
 * Allocates memory and initializes the buffers, vectors and iterators.
 * \param spEx Pointer to a valid exception structure initialized with \ref XCTOR().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Pointer to the object context.
 * Exception thrown on memory allocation failure.
 */
void* vpMsgsCtor(exception* spEx) {
    if(bExValidate(spEx)){
        void* vpMem = vpMemCtor(spEx);
        msgs *spCtx = (msgs*) vpMemAlloc(vpMem, sizeof(msgs));
        memset((void*) spCtx, 0, sizeof(msgs));
        spCtx->vpMem = vpMem;
        spCtx->spException = spEx;
        spCtx->vpVecMsgs = vpVecCtor(vpMem, sizeof(char), 1024); // 16 messages of average 64 characters
        spCtx->vpVecIndexes = vpVecCtor(vpMem, sizeof(aint), 16); // 16 messages
        spCtx->vpValidate = s_vpMagicNumber;
        return (void*) spCtx;
    }else{
        vExContext();
    }
    return NULL;
}

/** \brief The object destructor.
 *
 * Frees all memory associated with the object. Clears the memory to prevent accidental use of stale pointers.
 * \param vpCtx A valid context pointer returned from a previous call to \ref vpMsgsCtor().
 * NULL is silently ignored.
 * However, if non-NULL it must be a valid point or the application will silently exit with \ref BAD_CONTEXT exit code.
 */
void vMsgsDtor(void *vpCtx) {
    msgs *spCtx = (msgs*) vpCtx;
    if(vpCtx){
        if(spCtx->vpValidate == s_vpMagicNumber){
            void* vpMem = spCtx->vpMem;
            memset(vpCtx, 0, sizeof(msgs));
            vMemDtor(vpMem);
        }else{
            vExContext();
        }
    }
}

/** \brief Validate a msglog context pointer.
 *
 * \param vpCtx A pointer to a possibly valid msglog context previously return from vpMsgsCtor().
 * \return True if valid. False otherwise.
 */
abool bMsgsValidate(void *vpCtx){
    msgs *spCtx = (msgs*) vpCtx;
    if (spCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        return APG_TRUE;
    }
    return APG_FALSE;
}

/** \brief Clears the object of all messages.
 *
 * Restores it to the same state as a newly created object.
 * \param vpCtx - Pointer to a valid object context, previously returned from \ref vpMsgsCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 */
void vMsgsClear(void *vpCtx) {
    msgs *spCtx = (msgs*) vpCtx;
    if (spCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        vVecClear(spCtx->vpVecMsgs);
        vVecClear(spCtx->vpVecIndexes);
        spCtx->uiNext = 0;
        spCtx->uiMsgCount = 0;
    }else{
        vExContext();
    }
}

/** \brief Logs a message.
 * \param vpCtx - Pointer to a valid object context, previously returned from \ref vpMsgsCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpMsg Pointer to a null-terminated message string.
 */
void vMsgsLog(void *vpCtx, const char *cpMsg) {
    msgs *spCtx = (msgs*) vpCtx;
    if (spCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        if(!cpMsg || !cpMsg[0]){
            XTHROW(spCtx->spException, "NULL or empty messages not allowed");
        }
        // push the message & its index
        aint uiIndex = uiVecLen(spCtx->vpVecMsgs);
        vpVecPushn(spCtx->vpVecMsgs, (void*) cpMsg, (aint) (strlen(cpMsg) + 1));
        vpVecPush(spCtx->vpVecIndexes, (void*) &uiIndex);
        spCtx->uiMsgCount++;
    }else{
        vExContext();
    }
}

/** \brief Get a pointer to the first logged message, if any.
 *
 * Initializes an iterator over the logged messages.
 * \param vpCtx - Pointer to a valid object context, previously returned from \ref vpMsgsCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return A pointer to the first logged message or NULL if none.
 */
const char* cpMsgsFirst(void *vpCtx) {
    char *cpReturn = NULL;
    msgs *spCtx = (msgs*) vpCtx;
    if (spCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        if (spCtx->uiMsgCount) {
            cpReturn = (char*) vpVecFirst(spCtx->vpVecMsgs);
            spCtx->uiNext = 1;
        }
    }else{
        vExContext();
    }
    return cpReturn;
}

/** \brief Get a pointer to the next logged message, if any.
 *
 * Iterates over the logged messages.
 * \param vpCtx - Pointer to a valid object context, previously returned from \ref vpMsgsCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return A pointer to the next logged message. NULL if none.
 */
const char* cpMsgsNext(void *vpCtx) {
    char *cpReturn = NULL;
    msgs *spCtx = (msgs*) vpCtx;
    if (spCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        if (spCtx->uiMsgCount && spCtx->uiNext && (spCtx->uiNext < spCtx->uiMsgCount)) {
            aint *uipIndex = (aint*) vpVecAt(spCtx->vpVecIndexes, spCtx->uiNext);
            if (uipIndex) {
                char *cpMsg = (char*) vpVecAt(spCtx->vpVecMsgs, *uipIndex);
                if (cpMsg) {
                    cpReturn = cpMsg;
                    spCtx->uiNext++;
                } else {
                    // this should never happen
                    XTHROW(spCtx->spException, "unable to retrieve a saved message");
                }
            } // else {end of messages}
        }
    }else{
        vExContext();
    }
    return cpReturn;
}

/** \brief Get the number of logged messages.
 * \param vpCtx - Pointer to a valid object context, previously returned from \ref vpMsgsCtor().
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return The number of logged messages.
 */
aint uiMsgsCount(void *vpCtx) {
    aint uiReturn = 0;
    msgs *spCtx = (msgs*) vpCtx;
    if (spCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        uiReturn = spCtx->uiMsgCount;
    }else{
        vExContext();
    }
    return uiReturn;
}
