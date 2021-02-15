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
#ifndef LIB_VECTOR_H_
#define LIB_VECTOR_H_

/** \file vector.h
 *  \brief Header file for the vector object - a dynamic array.
 */

/** \struct vec_stats
 * \brief Vector usage statistics.
 *
 * If APG_VEC_STATS is defined, the vector object will collect usage statistics
 * to be reported with this structure if requested with \ref vVecStats().
 * If APG_VEC_STATS is not defined, the vector object will not collect usage statistics
 * and \ref vVecStats() will return this structure empty.
 *
 */
typedef struct {
    aint uiElementSize; /**< \brief The number of bytes in one element */
    aint uiOriginalElements; /**< \brief The initial number of elements allocated to the vector. */
    aint uiOriginalBytes; /**< \brief The initial number of bytes allocated to the vector.*/
    aint uiReserved; /**< \brief The current number of elements reserved. */
    aint uiUsed; /**< \brief The current number elements used. */
    aint uiMaxUsed; /**< \brief The maximum number of elements used during the vector's lifetime. */
    aint uiReservedBytes; /**< \brief The current number of bytes reserved. */
    aint uiUsedBytes; /**< \brief The current number of bytes in use. */
    aint uiMaxUsedBytes; /**< \brief The maximum number of bytes used over the lifetime of the vector. */
    aint uiPushed; /**< \brief The total number of elements pushed onto (added to) the vector. */
    aint uiPopped; /**< \brief The total number of elements popped from (removed from) the vector. */
    aint uiGrownCount; /**< \brief The number times the vector was automatically extended. */
    aint uiGrownElements; /**< \brief The number new elements automatically added to the vector. */
    aint uiGrownBytes; /**< \brief The number of bytes automatically added to the vector. */
} vec_stats;

void* vpVecCtor(void* vpMem, aint uiElementSize, aint uiInitialAlloc);
void vVecDtor(void* vpCtx);
abool bVecValidate(void* vpCtx);
void* vpVecPush(void* vpVec, void* vpElement);
void* vpVecPushn(void* vpCtx, void* vpElement, aint uiCount);
void* vpVecPop(void* vpCtx);
void* vpVecPopn(void* vpCtx, aint uiCount);
void* vpVecPopi(void* vpCtx, aint uiIndex);
void* vpVecFirst(void* vpCtx);
void* vpVecLast(void* vpCtx);
void* vpVecAt(void* vpCtx, aint uiIndex);
aint uiVecLen(void* vpCtx);
void* vpVecBuffer(void* vpCtx);
void vVecClear(void* vpCtx);
void vVecStats(void* vpCtx, vec_stats* spStats);

#endif /* LIB_VECTOR_H_ */
